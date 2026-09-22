# Crash telemetry sink

Receives the crash reports `sgp::processCrashTelemetry` uploads and stores them in
D1, one row per report. `schema.sql` is the data model and says why it is shaped
the way it is; `parse.js` is the report grammar; the Worker is the little that is
left over.

The reports arrive symbolized. `sgp/crash_telemetry.cpp` resolves them against the
PDB shipped beside the exe before uploading, and will not send a fault it could not
resolve -- so a frame here carries a function, a source line and whatever the
optimizer inlined into it, not a bare address. Nothing in this directory needs the
PDB, and nothing here can recover a name the client did not send.

## Deploy

```sh
npm i -g wrangler                                            # once
wrangler login                                               # once
wrangler d1 create ja2-crashes                               # paste the id into wrangler.toml
wrangler d1 execute ja2-crashes --remote --file=schema.sql
wrangler deploy
```

Then put the printed `https://ja2-crash-telemetry.<subdomain>.workers.dev` into
`gamedir/Ja2.ini`:

```ini
[Ja2 Settings]
CRASH_TELEMETRY_URL = https://ja2-crash-telemetry.<subdomain>.workers.dev
```

Empty or missing key = telemetry off, reports just accumulate locally.

Re-running the schema is safe — every statement in it is `IF NOT EXISTS`. Changing
a view means dropping it first: `DROP VIEW report_frames;` then re-run the file.
Nothing derived is stored, so a view rewritten today re-derives every report ever
received rather than only the ones that arrive afterwards.

There is no view yet that says which bug a report is an instance of. That is a
policy, and the input to it is a season of real reports rather than a guess made
before the table has any — which is exactly the kind of thing to add as a view,
once there is something to look at.

## Mining it

```sh
d1() { wrangler d1 execute ja2-crashes --remote --command "$1"; }

# what is actually breaking, grouped by where the client said the code was
d1 "SELECT f.source, count(*) n, count(DISTINCT r.build) builds
      FROM reports r JOIN report_frames f ON f.id = r.id AND f.idx = 0
     WHERE f.source IS NOT NULL GROUP BY 1 ORDER BY n DESC LIMIT 20"

# assertions say their own file and line, no symbols needed
d1 "SELECT assert_file, assert_line, count(*) n, max(assert_msg) msg
      FROM reports WHERE assert_line IS NOT NULL GROUP BY 1,2 ORDER BY n DESC LIMIT 20"

# reports per day
d1 "SELECT date(received_at, 'unixepoch') d, count(*) n FROM reports GROUP BY d ORDER BY d DESC LIMIT 30"

# what else was loaded when it crashed — cnc-ddraw, an overlay, Wine
d1 "SELECT m.name, count(*) n FROM report_modules m GROUP BY m.name ORDER BY n DESC"

# the backtrace of one report, as the client resolved it
d1 "SELECT idx, func, source, inlined FROM report_frames WHERE id = '<id>' ORDER BY idx"

# reports from builds nobody can symbolize any more: imported from the Discord
# channel this replaced, or sent before the PDB shipped. No image stamp, no names.
d1 "SELECT count(*) FROM reports WHERE image IS NULL"

# reports the parser did not fully understand — should be empty
d1 "SELECT id, received_at, build FROM reports WHERE raw IS NOT NULL"
```

To symbolize, turn a row back into the file `symbolize_crash` expects:

```sh
wrangler d1 execute ja2-crashes --remote --json \
  --command "SELECT text FROM report_text WHERE id = '<id>'" \
  | jq -r '.[0].results[0].text' > report.txt
symbolize_crash report.txt JA2.exe
```

## Test without the game

```sh
node test.mjs      # grammar, views, status-code contract. No network, no wrangler.
```

`test.mjs` runs `schema.sql` in node's built-in SQLite and hands the Worker a
binding backed by it, so the views and the insert are the real ones.

Against a local instance:

```sh
wrangler d1 execute ja2-crashes --local --file=schema.sql   # the local one, once
wrangler dev       # local, http://localhost:8787, its own local D1
curl -X POST --data-binary @../gamedir/crash_report_001.txt localhost:8787
wrangler d1 execute ja2-crashes --local --command "SELECT * FROM crashes"
```

Any real `crash_report_*.txt` out of a gamedir works as a body; expect `204` for
one, and `400` for junk (`curl -X POST -d hello localhost:8787`). `--local` and
`--remote` are different databases, and leaving the flag off is not the same as
either.

## Status codes are a contract

The client (`reportIsSettled()` in `sgp/crash_telemetry.cpp`) deletes its copy of a
report on 2xx and on 400/413/415, and keeps it on everything else. So:

| Status | Meaning | Client does |
| --- | --- | --- |
| 2xx | stored | deletes its copy |
| 400 | not a crash report, or too big | deletes its copy |
| 429 | throttled | keeps it, retries next launch |
| 5xx | our fault (D1 unreachable, binding missing) | keeps it, retries next launch |

**Never answer a settling 4xx for a failure on our side** — that silently destroys
the report. 429 is the one 4xx that is safe here, precisely because the client does
not settle it.

Uploading the same report twice is not an error and does not make two rows: the
primary key is the hash of the report text, and a request that times out after we
committed is one the client will send again next launch.

## Importing the old Discord channel

The reports posted to Discord before this existed are moved over by
`import-discord.mjs`. It needs a bot with View Channel and Read Message History on
that channel, and the channel's id:

```sh
DISCORD_BOT_TOKEN=... node import-discord.mjs <channel-id>
for f in import-*.sql; do wrangler d1 execute ja2-crashes --remote --file="$f"; done
```

It writes SQL rather than talking to D1, so it needs no API token, and it is
re-runnable: rows are keyed by the hash of the report text and inserted with
`INSERT OR IGNORE`. The count it prints at the end should match
`SELECT count(*) FROM reports`.

`received_at` for those rows comes from the Discord message timestamp, which is the
only record of when we received them — so import before the channel goes away, and
check the count before deleting it.

## Rate limiting

A per-IP cap, via the `UPLOAD_LIMITER` binding in `wrangler.toml`, checked before
the body is read. The ceiling (50/minute) has to clear `kMaxUploadsPerRun` (20) in
the client, or a player draining a backlog throttles themselves.

This has to be a binding with a `.limit()` call in `worker.js`, not a dashboard
rule: WAF rate limiting rules need a zone, and a `workers.dev` subdomain is not
one. The counter is per-colo and best-effort, so treat the number as a rough
ceiling.

Adding the binding from the Cloudflare dashboard instead would leave `wrangler.toml`
out of sync, and the next `wrangler deploy` would drop it. Edit the file.

## Free tier

100k Worker requests/day, and for D1 5 GB of storage, 5M rows read/day and 100k
rows written/day — where "rows written" counts index entries too, which is why the
schema has no secondary indexes. One report is one row. A report is 2-8 KB, so
5 GB is several hundred thousand of them.

Nothing deletes anything. When that stops being true:

```sh
wrangler d1 execute ja2-crashes --remote \
  --command "DELETE FROM reports WHERE received_at < unixepoch('now', '-1 year')"
```

## Not done

No authentication. The endpoint is public and its URL ships in every player's
`Ja2.ini`, so assume it will eventually be found; the size and `*** CRASH` checks
only keep out drive-by scanners. Every field in a row is therefore attacker-chosen
— they are bound, never interpolated, and nothing reads one back as anything but
text. Blast radius of abuse is rows we delete.

The per-IP limiter does nothing against a distributed flood — that would cost the
D1 write budget and the 100k/day request budget, not money.

Nothing pushes. A crash that has never been seen before does not announce itself
anywhere; you go and look.
