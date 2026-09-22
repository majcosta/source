// The crash report grammar, as writeExceptionBacktrace (sgp/crash_report.cpp)
// emits it, turned into the columns of the `reports` table.
//
// Nothing here decides anything. Every field is a line the report wrote, kept
// as it was written: which module a frame belongs to, and what bug a report is
// an instance of, are views in schema.sql, so redefining either reprocesses all
// of history instead of only what arrives afterwards.
//
// The one exception is the report that does not parse. Any non-blank line the
// grammar below does not consume means the writer emitted something this file
// has not been taught -- so the whole text is kept in `raw` for that row, and
// only for those rows. That is the case where the upload is the only copy.

const HEADER = /^\*\*\* CRASH  code=([0-9A-F]{8})  eip=[0-9A-F]{8}  esp=([0-9A-F]{8})  ebp=([0-9A-F]{8}) \*\*\*$/;
const TIME = /^ {2}time (\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) UTC$/;
const BUILD = /^ {2}build (.*)$/;
const HANDLE = /^ {2}handle (.*)$/;
const ASSERT = /^ {2}assertion failed at line (\d+) of (.*)$/;
const MESSAGE = /^ {2}message: (.*)$/;
const AV = /^ {2}access violation: (write to|execute at|read from) +([0-9A-F]{8})$/;
const MODULES = /^ {2}modules \(base size name\):$/;
const MODULE = /^ {4}([0-9A-F]{8}) ([0-9A-F]{8}) (.*)$/;
// Everything after the address is what crash_telemetry.cpp resolved before
// uploading: "Name+0xOFF" and, when the PDB had one, "  file.cpp:123". Optional,
// because a report from before that existed, or one whose PDB went missing, is
// still a report.
const FRAME = /^ {2}\[(\d+)\] ([0-9A-F]{8})(?: (.*))?$/;
// A level the optimizer folded into the frame above, innermost last.
const INLINE = /^ {6}inlined (.*)$/;
// The main image's TimeDateStamp: which binary this is, as opposed to which
// commit. See sgp/crash_report.cpp.
const IMAGE = /^ {2}image ([0-9A-F]{8})$/;

// The report spells the direction of a faulting access three ways, and they are
// the three string literals in crash_report.cpp. Folding them to one word each
// is the same kind of step as dropping the " UTC" off the timestamp: a constant
// rewritten, no information in it to lose.
const AV_KIND = { "write to": "write", "execute at": "execute", "read from": "read" };

export function parseReport(text) {
	const out = {
		reported_at: null, build: null, handle: null, image: null,
		code: null, esp: null, ebp: null,
		av_kind: null, av_addr: null,
		assert_file: null, assert_line: null, assert_msg: null,
		modules: [], frames: [], unparsed: [],
	};
	for (const line of text.split(/\r?\n/)) {
		if (line.trim() === "") continue;
		let m;
		if ((m = HEADER.exec(line))) {
			// eip is not a column: the writer emits it again as frame [0].
			out.code = m[1];
			out.esp = parseInt(m[2], 16);
			out.ebp = parseInt(m[3], 16);
		} else if ((m = TIME.exec(line))) {
			// Without the suffix, so SQLite's date functions accept it. It is the
			// player's clock and can say anything; received_at is the honest one.
			out.reported_at = m[1];
		} else if ((m = BUILD.exec(line))) {
			out.build = m[1];
		} else if ((m = IMAGE.exec(line))) {
			out.image = parseInt(m[1], 16);
		} else if ((m = HANDLE.exec(line))) {
			out.handle = m[1];
		} else if ((m = ASSERT.exec(line))) {
			out.assert_line = Number(m[1]);
			out.assert_file = m[2];
		} else if ((m = MESSAGE.exec(line))) {
			out.assert_msg = m[1];
		} else if ((m = AV.exec(line))) {
			out.av_kind = AV_KIND[m[1]];
			out.av_addr = parseInt(m[2], 16);
		} else if (MODULES.test(line)) {
			// The table header. Consumed so it is not mistaken for a stray line.
		} else if ((m = MODULE.exec(line))) {
			out.modules.push({ n: m[3], b: parseInt(m[1], 16), s: parseInt(m[2], 16) });
		} else if ((m = FRAME.exec(line))) {
			// Frames are stored positionally, so a gap in the numbering would
			// silently shift every frame after it. The writer never leaves one.
			if (Number(m[1]) !== out.frames.length) out.unparsed.push(line);
			// s is the symbol text exactly as it arrived, not split into name and
			// file and line: what a name means is a question for a view, and the
			// row has to render back into the bytes that were uploaded.
			else out.frames.push(m[3] === undefined ? { a: parseInt(m[2], 16) }
				: { a: parseInt(m[2], 16), s: m[3] });
		} else if ((m = INLINE.exec(line))) {
			// Belongs to the frame above it; on its own it is a line out of place.
			const frame = out.frames[out.frames.length - 1];
			if (frame === undefined) out.unparsed.push(line);
			else (frame.i = frame.i || []).push(m[1]);
		} else {
			out.unparsed.push(line);
		}
	}
	return out;
}

// sha-256 of the uploaded text, and the row's primary key. Two uploads of one
// report are one row: postReport() reports a timeout as "not settled" even when
// we committed, so the client keeps the file and sends it again next launch.
export async function reportId(text) {
	const digest = await crypto.subtle.digest("SHA-256", new TextEncoder().encode(text));
	return [...new Uint8Array(digest)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

// Written once here so the Worker and the Discord import cannot disagree about
// what a row is; each renders the values its own way.
export const COLUMNS = [
	"id", "received_at", "reported_at", "build", "image", "code", "esp", "ebp",
	"av_kind", "av_addr", "assert_file", "assert_line", "assert_msg", "handle",
	"modules", "frames", "raw",
];

export function toRow(text, id, receivedAt) {
	const p = parseReport(text);
	return [
		id, receivedAt, p.reported_at, p.build, p.image, p.code, p.esp, p.ebp,
		p.av_kind, p.av_addr, p.assert_file, p.assert_line, p.assert_msg, p.handle,
		JSON.stringify(p.modules), JSON.stringify(p.frames),
		p.unparsed.length ? text : null,
	];
}

const PREFIX = `INSERT OR IGNORE INTO reports (${COLUMNS.join(", ")}) VALUES `;

export const INSERT = PREFIX + `(${COLUMNS.map(() => "?").join(", ")})`;

// The same statement with the values written into it, for the Discord import:
// it has no D1 binding to bind against and feeds `wrangler d1 execute --file`.
// SQLite string literals have no escape character, so doubling the quote is the
// entire rule, and that is what makes generating SQL around report text safe. A
// NUL would end the statement early; the client cannot emit one, and this drops
// it rather than trusting that.
export function insertStatement(row) {
	const lit = (v) =>
		v === null || v === undefined ? "NULL" :
		typeof v === "number" ? String(v) :
		"'" + String(v).replace(/\0/g, "").replace(/'/g, "''") + "'";
	return PREFIX + `(${row.map(lit).join(", ")});`;
}
