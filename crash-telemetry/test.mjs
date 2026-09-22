// node test.mjs -- the report grammar, the views that read it, and the status-code
// contract the client depends on. No network, no wrangler, no D1: the fake binding
// below is node:sqlite running the real schema.sql, so an insert that would fail
// against D1 fails here too.
//
// The property that matters most is the round trip: a row has to render back into
// the exact bytes that were uploaded, because tools/symbolize_crash takes a file
// and the upload is the only copy once the report is deleted from the player's
// disk. id is the sha-256 of those bytes, so hashing report_text proves it.
import assert from "node:assert";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { DatabaseSync } from "node:sqlite";
import { COLUMNS, INSERT, parseReport, reportId, toRow } from "./parse.js";
import worker from "./worker.js";

// As the handler writes it, and as crash_telemetry.cpp uploads it: symbolized,
// with an image stamp and inlined levels under the frame they were folded into.
const SYMBOLIZED =
	"\r\n*** CRASH  code=C0000005  eip=00C640A4  esp=202BF99C  ebp=202BFA18 ***\r\n" +
	"  time 2026-09-20 09:46:30 UTC\r\n" +
	"  build ddb691318\r\n" +
	"  image 6AB220FF\r\n" +
	"  handle o'marco\r\n" +
	"  access violation: write to  2FD49008\r\n" +
	"  modules (base size name):\r\n" +
	"    004F0000 22E3C000 ja2.exe\r\n" +
	"    77BA0000 001BF000 ntdll.dll\r\n" +
	"  [0] 00C640A4 AutoResolveScreenHandle+0x2C  Auto Resolve.cpp:680\r\n" +
	"      inlined RenderAutoResolve  Auto Resolve.cpp:1204\r\n" +
	"      inlined ClipBlitToRect  vsurface.cpp:88\r\n" +
	"  [1] 00C64100 GameLoop+0x1F1  gameloop.cpp:385\r\n" +
	"  [2] 77BFE181\r\n";

// What the Discord channel holds: no image line, no names. Still a report.
// eip repeats frame 0, which is why the column does not exist; true of all 491.
const BARE =
	"\r\n*** CRASH  code=E1A55E27  eip=004031D5  esp=00000000  ebp=00000000 ***\r\n" +
	"  time 2026-08-01 00:00:00 UTC\r\n" +
	"  build c9100aace\r\n" +
	"  assertion failed at line 412 of Soldier Control.cpp\r\n" +
	"  message: it's bad\r\n" +
	"  modules (base size name):\r\n" +
	"    00400000 00A00000 ja2.exe\r\n" +
	"  [0] 004031D5\r\n";

const p = parseReport(SYMBOLIZED);
assert.deepEqual(p.unparsed, []);
assert.equal(p.image, 0x6AB220FF);
assert.deepEqual(p.frames[0], {
	a: 0x00C640A4,
	s: "AutoResolveScreenHandle+0x2C  Auto Resolve.cpp:680",
	i: ["RenderAutoResolve  Auto Resolve.cpp:1204", "ClipBlitToRect  vsurface.cpp:88"],
});
assert.deepEqual(p.frames[2], { a: 0x77BFE181 });   // outside ja2.exe, never named
assert.equal(parseReport(BARE).image, null);

// An inlined line with no frame above it is a line out of place, not a silent drop.
assert.deepEqual(parseReport("      inlined Nowhere  x.cpp:1\r\n").unparsed,
	["      inlined Nowhere  x.cpp:1"]);

const db = new DatabaseSync(":memory:");
db.exec(readFileSync("schema.sql", "utf8"));
const one = (sql, ...v) => db.prepare(sql).get(...v);

// The Worker's view of D1: prepare/bind/run over the real schema, so a statement
// that would be rejected there is rejected here.
let throttled = false;
const ENV = {
	DB: { prepare: (sql) => ({ bind: (...v) => ({ run: async () => db.prepare(sql).run(...v) }) }) },
	UPLOAD_LIMITER: { limit: async () => ({ success: !throttled }) },
};
const post = (body, env = ENV) =>
	worker.fetch(new Request("https://x/", { method: "POST", body }), env);

for (const text of [SYMBOLIZED, BARE]) assert.equal((await post(text)).status, 204);

// The frame splits into function and source where the client put two spaces, and
// `inlined` is the innermost level -- where the code actually was.
const frames = db.prepare("SELECT idx, func, source, inlined, name FROM report_frames" +
	" WHERE id = ? ORDER BY idx").all(await reportId(SYMBOLIZED));
assert.deepEqual(frames, [
	{ idx: 0, func: "AutoResolveScreenHandle+0x2C", source: "Auto Resolve.cpp:680",
	  inlined: "ClipBlitToRect  vsurface.cpp:88", name: "ja2.exe" },
	{ idx: 1, func: "GameLoop+0x1F1", source: "gameloop.cpp:385", inlined: null, name: "ja2.exe" },
	{ idx: 2, func: null, source: null, inlined: null, name: "ntdll.dll" },
]);


// The whole point: byte for byte, both shapes.
for (const original of [SYMBOLIZED, BARE]) {
	const id = await reportId(original);
	const text = one("SELECT text FROM report_text WHERE id = ?", id).text;
	assert.equal(text, original);
	assert.equal(createHash("sha256").update(text, "utf8").digest("hex"), id);
}

// Same report twice is one row: postReport() reports a timeout as unsettled even
// when we committed, so the client keeps the file and sends it again next launch.
assert.equal((await post(SYMBOLIZED)).status, 204);
assert.equal(one("SELECT count(*) c FROM reports").c, 2);
assert.equal(COLUMNS.length, 17);

// ---- the status-code contract ---------------------------------------------

// Junk: the client deletes these, so they must be 400.
assert.equal((await post("hello")).status, 400);
assert.equal((await post("x".repeat(32 * 1024 + 1))).status, 400);
assert.equal((await worker.fetch(new Request("https://x/"), ENV)).status, 405);

// Throttled: 429, and nothing is stored. reportIsSettled() leaves 429 unsettled,
// so the client keeps the report for next launch.
throttled = true;
assert.equal((await post(BARE)).status, 429);
throttled = false;

// Our failures must be 5xx, or the client throws away a report we never stored.
const broken = { ...ENV, DB: { prepare: () => { throw new Error("D1_ERROR"); } } };
assert.equal((await post(SYMBOLIZED, broken)).status, 503);
assert.equal((await post(SYMBOLIZED, { ...ENV, DB: null })).status, 503);

console.log("ok");
