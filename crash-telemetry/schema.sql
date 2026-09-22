-- D1 schema for the crash reports sgp::processCrashTelemetry uploads.
--
--   npx wrangler d1 create ja2-crashes
--   npx wrangler d1 execute ja2-crashes --remote --file=schema.sql
--
-- One row per report, holding what the report said and nothing else. Which
-- module a frame lands in, and which bug a report is an instance of, are the
-- views below: both are things we will change our minds about, and a view
-- redefined re-derives all of history, where a stored column would need a
-- backfill and would be wrong until it ran.

CREATE TABLE IF NOT EXISTS reports (
  id          TEXT PRIMARY KEY,  -- sha-256 of the uploaded text; INSERT OR IGNORE
  received_at INTEGER NOT NULL,  -- our clock, epoch seconds
  reported_at TEXT,              -- the report's own "time" line, UTC, as a datetime
  build       TEXT,              -- game version string, names the commit
  image       INTEGER,           -- the main image's TimeDateStamp: names the binary
  code        TEXT,              -- C0000005, E1A55E27 (an assertion), ...
  esp         INTEGER,
  ebp         INTEGER,           -- eip is not here: it is frame 0
  av_kind     TEXT,              -- read | write | execute, NULL unless it faulted on one
  av_addr     INTEGER,
  assert_file TEXT,              -- basename only, the writer strips the directory
  assert_line INTEGER,
  assert_msg  TEXT,
  handle      TEXT,              -- the player's optional self-chosen HANDLE
  modules     TEXT NOT NULL,     -- JSON [{n,b,s}] in load order; b is per-run, ASLR
  frames      TEXT NOT NULL,     -- JSON [{a,s?,i?}], index = frame number; a is the
                                 -- address, s the symbol text the client resolved,
                                 -- i the levels inlined into it, innermost last
  raw         TEXT               -- NULL except when a line did not parse; see parse.js
);

-- No secondary indexes on purpose. D1 bills writes per row and counts index
-- entries among them, and SQLite scans a table this size without noticing. Add
-- one when a query is actually slow, not before.

-- Module bases are ASLR'd, so an address only means something next to the table
-- the report carried. This is the join symbolize_crash.cpp does at line 230.
-- name is lowercased: the loader reports whatever case the path was written in,
-- so one binary arrives as ja2.exe, JA2.exe and JA2.EXE and splits its crashes
-- three ways. Folded here rather than on the way in, because `modules` holds the
-- bytes the report was uploaded as and report_text has to render them back
-- exactly -- which is why report_text reads that column and not this view.
CREATE VIEW IF NOT EXISTS report_modules AS
SELECT r.id,
       m.key                                 AS idx,
       lower(json_extract(m.value, '$.n'))   AS name,
       json_extract(m.value, '$.b') AS base,
       json_extract(m.value, '$.s') AS size
  FROM reports r, json_each(r.modules) m;

-- Every return address the stack walk printed, resolved where it lands in a
-- loaded image. name and rva are NULL for the ones that do not: the writer
-- prints each frame whether or not it points at code, and on a wrecked stack
-- many of them are garbage. That is a fact about the crash, not a parse failure.
CREATE VIEW IF NOT EXISTS report_frames AS
SELECT r.id,
       f.key                              AS idx,
       json_extract(f.value, '$.a')       AS addr,
       -- What the client resolved, split where it put the two spaces: everything
       -- before them is the function, everything after is file:line. NULL for a
       -- report that predates client-side symbolizing, or whose PDB was missing.
       CASE WHEN json_extract(f.value, '$.s') IS NULL THEN NULL
            WHEN instr(json_extract(f.value, '$.s'), '  ') = 0
            THEN json_extract(f.value, '$.s')
            ELSE substr(json_extract(f.value, '$.s'), 1,
                        instr(json_extract(f.value, '$.s'), '  ') - 1) END AS func,
       CASE WHEN instr(COALESCE(json_extract(f.value, '$.s'), ''), '  ') = 0 THEN NULL
            ELSE substr(json_extract(f.value, '$.s'),
                        instr(json_extract(f.value, '$.s'), '  ') + 2) END AS source,
       -- The innermost level the optimizer folded in, which is where the code
       -- actually was. NULL when nothing was inlined here.
       json_extract(f.value, '$.i[#-1]')  AS inlined,
       m.name                             AS name,
       json_extract(f.value, '$.a') - m.base AS rva
  FROM reports r, json_each(r.frames) f
  LEFT JOIN report_modules m
    ON m.id = r.id AND json_extract(f.value, '$.a') >= m.base
   AND json_extract(f.value, '$.a') < m.base + m.size;

-- A report back in the form it arrived in, because tools/symbolize_crash takes a
-- file and there is no file any more:
--   wrangler d1 execute ja2-crashes --remote --json \
--     --command "SELECT text FROM report_text WHERE id = '<id>'" \
--     | jq -r '.[0].results[0].text' > report.txt
-- Byte-for-byte what writeExceptionBacktrace wrote, which is how test.mjs checks
-- it. The lists come out in json_each's scan order, i.e. load order for modules
-- and frame order for frames; symbolize_crash resolves modules by range and
-- reads each frame's own index, so order is a matter of how the file reads.
CREATE VIEW IF NOT EXISTS report_text AS
WITH crlf(nl) AS (VALUES (char(13) || char(10)))
SELECT r.id,
       nl
       || printf('*** CRASH  code=%s  eip=%08X  esp=%08X  ebp=%08X ***',
                 r.code, json_extract(r.frames, '$[0].a'), r.esp, r.ebp) || nl
       || CASE WHEN r.reported_at IS NULL THEN ''
               ELSE printf('  time %s UTC', r.reported_at) || nl END
       || CASE WHEN r.build IS NULL THEN ''
               ELSE printf('  build %s', r.build) || nl END
       || CASE WHEN r.image IS NULL THEN ''
               ELSE printf('  image %08X', r.image) || nl END
       || CASE WHEN r.handle IS NULL THEN ''
               ELSE printf('  handle %s', r.handle) || nl END
       || CASE WHEN r.assert_line IS NULL THEN ''
               ELSE printf('  assertion failed at line %d of %s',
                           r.assert_line, r.assert_file) || nl END
       || CASE WHEN r.assert_msg IS NULL THEN ''
               ELSE printf('  message: %s', r.assert_msg) || nl END
       -- The three spellings are the string literals in crash_report.cpp; the
       -- trailing space on "write to " is the writer's own column alignment.
       || CASE r.av_kind WHEN 'read'    THEN printf('  access violation: read from %08X', r.av_addr) || nl
                         WHEN 'write'   THEN printf('  access violation: write to  %08X', r.av_addr) || nl
                         WHEN 'execute' THEN printf('  access violation: execute at %08X', r.av_addr) || nl
                         ELSE '' END
       || '  modules (base size name):' || nl
       || COALESCE((SELECT group_concat(printf('    %08X %08X %s',
                                        json_extract(m.value, '$.b'),
                                        json_extract(m.value, '$.s'),
                                        json_extract(m.value, '$.n')) || nl, '')
                      FROM json_each(r.modules) m), '')
       -- Each frame, then whatever the optimizer folded into it, one line each.
       -- json_each gives the inline levels in the order they were stored, which is
       -- the order they were written: outermost first.
       || COALESCE((SELECT group_concat(
                      printf('  [%d] %08X', f.key, json_extract(f.value, '$.a'))
                      || CASE WHEN json_extract(f.value, '$.s') IS NULL THEN ''
                              ELSE ' ' || json_extract(f.value, '$.s') END
                      || nl
                      || COALESCE((SELECT group_concat('      inlined ' || i.value || nl, '')
                                     FROM json_each(f.value, '$.i') i), ''), '')
                      FROM json_each(r.frames) f), '')
       AS text
  FROM reports r, crlf;
