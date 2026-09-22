// Crash-telemetry sink for JA2 1.13. Takes the POST that sgp::processCrashTelemetry
// makes and stores the report as a row in D1. See schema.sql for what a row is and
// parse.js for how the report's own text becomes one.
//
// The status codes matter -- the client acts on them (reportIsSettled()):
//   2xx  stored; the client deletes its copy
//   400  junk, never going to be accepted; the client deletes its copy
//   429  throttled; the client keeps the file and retries next launch
//   5xx  our problem; the client keeps the file and retries next launch
// So never answer a settling 4xx for a failure on our side: that throws the report
// away. 429 is the one 4xx that is safe, because the client does not settle it.

import { INSERT, reportId, toRow } from "./parse.js";

// Reports run 2-8 KB, and the fixed bounds in writeExceptionBacktrace (128
// modules, 64 frames) cap them near 10 KB. Kept equal to kMaxReportBytes in the
// client: anything the client is willing to send must not meet a settling 400
// here, or the upload is what destroys the report.
const MAX_BYTES = 32 * 1024;

export default {
	async fetch(request, env) {
		if (request.method !== "POST") return new Response("POST only\n", { status: 405 });
		if (!env.DB) {
			// The d1_databases binding in wrangler.toml, missing or misnamed.
			console.error("DB unset (see [[d1_databases]] in wrangler.toml)");
			return new Response("not configured\n", { status: 503 });
		}

		// Per-IP cap, checked before the body is read so a flood costs nothing. What
		// it protects is the D1 write budget, which is billed per row. 429 is
		// deliberate: reportIsSettled() does not settle it, so a throttled player
		// keeps the report and it goes out next launch. Answering 400 deletes it.
		const ip = request.headers.get("cf-connecting-ip") || "unknown";
		const { success } = await env.UPLOAD_LIMITER.limit({ key: ip });
		if (!success) return new Response("slow down\n", { status: 429 });

		// Trust the declared length only as a cheap early out; the real cap is on the
		// bytes actually read, since Content-Length can lie or be absent.
		const declared = Number(request.headers.get("content-length") || 0);
		if (declared > MAX_BYTES) return new Response("too large\n", { status: 400 });

		const text = await request.text();
		if (text.length > MAX_BYTES) return new Response("too large\n", { status: 400 });
		// The endpoint is public and unauthenticated -- the URL ships in every
		// player's Ja2.ini. This is not security, just a filter that keeps drive-by
		// POSTs and scanners out of the table. Anything determined gets through;
		// that is fine, the blast radius is a row we delete.
		if (!text.includes("*** CRASH")) return new Response("not a crash report\n", { status: 400 });

		// Every value below is lifted out of the uploaded file and is therefore
		// attacker-chosen. It is bound, never interpolated, and nothing reads it
		// back as anything but text.
		try {
			const row = toRow(text, await reportId(text), Math.floor(Date.now() / 1000));
			await env.DB.prepare(INSERT).bind(...row).run();
		} catch (e) {
			// 503, not 400: a report we failed to store is one the client must keep.
			console.error("insert failed:", e.message);
			return new Response("storage failed\n", { status: 503 });
		}
		return new Response(null, { status: 204 });
	},
};
