// node test.mjs — exercises the status-code contract the client depends on.
import assert from "node:assert";
import worker from "./worker.js";

const REPORT = `
*** CRASH  code=C0000005  eip=0071D5A0  esp=202BF99C  ebp=202BFA18 ***
  time 2026-07-26 10:41:02 UTC
  build 6a941c06
  handle @marco*evil
  access violation: read from 00000002 [x](https://evil.test) \`
  [0] 0071D5A0
  [1] 006BE7DE
`;

let sent = null;               // what we handed Discord on the last call
let upstream = () => new Response(null, { status: 204 });
globalThis.fetch = async (url, init) => { sent = init; return upstream(); };

let throttled = false;
const ENV = {
  DISCORD_WEBHOOK: "https://discord.test/hook",
  UPLOAD_LIMITER: { limit: async () => ({ success: !throttled }) },
};

const post = (body, env = ENV) =>
  worker.fetch(new Request("https://x/", { method: "POST", body }), env);

// happy path
let r = await post(REPORT);
assert.equal(r.status, 204);
const content = JSON.parse(sent.body.get("payload_json")).content;
assert.match(content, /C0000005/);
assert.match(content, /read from 00000002/);
assert.match(content, /build `6a941c06`/);
assert.match(content, /marcoevil/);            // markdown and @ stripped from the handle
// every field is attacker-chosen: no field may carry markup or a link into the
// channel, and none may close the backticks or bold the summary wraps it in.
assert.ok(!content.includes("]("), content);   // no link syntax out of the report
assert.ok(!content.includes("://"), content);  // and no bare URL either
assert.equal(content.match(/`/g).length, 4);   // only the two pairs summarize() opens
assert.deepEqual(JSON.parse(sent.body.get("payload_json")).allowed_mentions, { parse: [] });
assert.equal(await sent.body.get("files[0]").text(), REPORT);

// an assertion report: same "*** CRASH" envelope, but the code is always the same
// one, so the summary has to quote the assert's own line, file and message instead.
const ASSERT_REPORT = `
*** CRASH  code=E1A55E27  eip=7B00FA12  esp=0032F900  ebp=0032F950 ***
  time 2026-07-31 16:54:06 UTC
  build 6a941c06
  assertion failed at line 412 of Soldier Control.cpp
  message: bad **pMerc** [x](https://evil.test)
  [0] 7B00FA12
`;
r = await post(ASSERT_REPORT);
assert.equal(r.status, 204);
const assertContent = JSON.parse(sent.body.get("payload_json")).content;
assert.match(assertContent, /\*\*assert\*\* line 412 of Soldier Control.cpp/);
assert.match(assertContent, /bad pMerc/);      // markdown stripped from the message
assert.doesNotMatch(assertContent, /E1A55E27/); // the code itself tells nobody anything
assert.ok(!assertContent.includes("]("), assertContent);
assert.ok(!assertContent.includes("://"), assertContent);
assert.equal(assertContent.match(/`/g).length, 2); // only the pair around the build

// junk: client should delete these, so they must be 400
assert.equal((await post("hello")).status, 400);
assert.equal((await post("x".repeat(32 * 1024 + 1))).status, 400);

// throttled: 429, and nothing reaches Discord. reportIsSettled() leaves 429
// unsettled, so the client keeps the report for next launch.
throttled = true;
sent = null;
assert.equal((await post(REPORT)).status, 429);
assert.equal(sent, null);
throttled = false;

// our failures: client must keep the report, so these must be 5xx
upstream = () => new Response(null, { status: 429 });          // Discord rate limit
assert.equal((await post(REPORT)).status, 503);
upstream = () => { throw new Error("network"); };
assert.equal((await post(REPORT)).status, 503);
upstream = () => new Response(null, { status: 204 });
assert.equal((await post(REPORT, {})).status, 503);            // secret not set

// wrong method
assert.equal((await worker.fetch(new Request("https://x/"), {})).status, 405);

console.log("ok");
