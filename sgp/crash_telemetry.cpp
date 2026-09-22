// Crash telemetry: symbolize the crash_report_*.txt files the crash handler left
// behind and upload them, on the next launch.
//
// Nothing the receiving end cannot read is sent. A fault is a list of addresses
// and means nothing without the PDB that explains them, which is not something
// the other end can be assumed to still have, so a fault goes out only once its
// frames have names. An assertion already says what it is and goes out either way.
//
// Deliberately a separate translation unit from crash_report.cpp. Everything in
// there runs inside a faulting thread and may not allocate; everything here runs at
// startup with a healthy heap and is ordinary code. Keeping the two apart keeps the
// no-heap rule easy to see and easy to hold.

#if defined(_MSC_VER)

#include "crash_report.h"

#include <windows.h>
#include <winhttp.h>
#include <dbghelp.h>
#include <process.h> // _beginthreadex for the detached upload thread

#include <cstdio>  // snprintf, sscanf_s
#include <cstring> // strstr
#include <string>
#include <vector>

namespace {

// Persisted consent: 1 = yes, 0 = no, -1 = not asked yet.
int readConsent() {
	HANDLE h = CreateFileA("telemetry.consent", GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return -1;
	char c = 0; DWORD got = 0;
	ReadFile(h, &c, 1, &got, NULL);
	CloseHandle(h);
	return (got == 1 && c == '1') ? 1 : 0;
}

void writeConsent(bool yes) {
	HANDLE h = CreateFileA("telemetry.consent", GENERIC_WRITE, 0, NULL,
		CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return;
	DWORD w; WriteFile(h, yes ? "1" : "0", 1, &w, NULL);
	CloseHandle(h);
}

// A report bigger than this is not one of ours; never put it on the wire. Kept
// equal to MAX_BYTES in the sink, which answers a settling 400 above it: sending
// what the sink will not take is how an upload destroys the report it carries.
const DWORD kMaxReportBytes = 32 * 1024;

// --- symbolization ---------------------------------------------------------
//
// Names go on here rather than in the handler. SymInitialize parses the whole PDB:
// it allocates megabytes and takes the loader lock, and crash_report.cpp runs
// inside a faulting thread where neither is allowed. By the time this file runs
// the process is healthy and this is an ordinary background thread, so the report
// can have its names filled in on the way out instead of never.
//
// Addresses in a report are runtime VAs from a /DYNAMICBASE image, so they mean
// nothing without the base the report recorded. Rather than reload the PDB at each
// report's base, the image is loaded once at its preferred base and every address
// is turned into an RVA first: the same arithmetic, one PDB parse per launch.

// Names are already undecorated; a template one can still run to thousands of
// characters, and the sink answers a settling 400 above 32 KB.
const int kMaxSymbolChars = 120;

// A pathological inline chain should not push a report over the sink's cap on its
// own; past this depth the innermost frames are the ones worth having anyway.
const DWORD kMaxInlineFrames = 8;

DWORD64 s_symBase = 0; // preferred image base, once symInit() has succeeded

// What the running image stamps into its own reports; see crash_report.cpp.
// __ImageBase is linker-provided, so this asks the loader nothing.
extern "C" IMAGE_DOS_HEADER __ImageBase;
DWORD ourImageStamp() {
	const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
		reinterpret_cast<const char*>(&__ImageBase) + __ImageBase.e_lfanew);
	return nt->FileHeader.TimeDateStamp;
}

// Loads the PDB beside our own exe, once. Everything here is best-effort: no
// symbols simply means reports go out exactly as they do today.
bool symInit() {
	static int state = 0; // 0 = untried, 1 = ready, -1 = nothing to load
	if (state != 0) return state > 0;
	state = -1;

	char exePath[MAX_PATH] = { 0 };
	if (!GetModuleFileNameA(NULL, exePath, ARRAYSIZE(exePath))) return false;

	char* slash = strrchr(exePath, '\\');
	char* dot = strrchr(slash != NULL ? slash : exePath, '.');
	if (dot == NULL) return false;

	// Without a PDB beside the exe DbgHelp still "succeeds" and answers with
	// export names, which are worse than raw addresses because they look
	// authoritative. Check for the file rather than pay the load to find out.
	char pdbPath[MAX_PATH] = { 0 };
	lstrcpynA(pdbPath, exePath, ARRAYSIZE(pdbPath));
	lstrcpynA(pdbPath + (dot - exePath), ".pdb", 5);
	if (GetFileAttributesA(pdbPath) == INVALID_FILE_ATTRIBUTES) return false;

	char dir[MAX_PATH] = { 0 };
	lstrcpynA(dir, exePath, ARRAYSIZE(dir));
	if (slash != NULL) dir[slash - exePath] = '\0';

	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_NO_PROMPTS);
	// FALSE: do not enumerate this process's own modules. Exactly one image is
	// registered, at its preferred base, and nothing else is ever asked about.
	if (!SymInitialize(GetCurrentProcess(), dir, FALSE)) return false;
	// A base of zero tells DbgHelp to use the image's own preferred base and hand
	// it back, which is the base every RVA below is added to.
	s_symBase = SymLoadModuleEx(GetCurrentProcess(), NULL, exePath, NULL, 0, 0, NULL, 0);
	if (s_symBase == 0) {
		SymCleanup(GetCurrentProcess());
		return false;
	}
	state = 1;
	return true;
}

// One location as the report writes it: "name+0x1A  file.cpp:123".
std::string formatLocation(const char* name, DWORD64 offset, const IMAGEHLP_LINE64* line) {
	char text[320] = { 0 };
	int n = snprintf(text, sizeof(text), "%.*s+0x%lX",
		kMaxSymbolChars, name, static_cast<unsigned long>(offset));
	if (n < 0) return std::string();
	std::string out(text, n);
	if (line != NULL && line->FileName != NULL) {
		// File name only, like the module table and the assertion line already do.
		// The directory is the build machine's, which is noise once it leaves here.
		const char* file = line->FileName;
		for (const char* p = line->FileName; *p != '\0'; ++p)
			if (*p == '\\' || *p == '/') file = p + 1;
		n = snprintf(text, sizeof(text), "  %.80s:%lu", file, line->LineNumber);
		if (n > 0) out.append(text, n);
	}
	return out;
}

// Where an address really is: the function that owns it, then everything the
// optimizer inlined into it, innermost last. Release builds inline freely, so
// without this a frame reports whichever function swallowed the code -- and the
// grouping key downstream would be built from that same wrong name. This is the
// walk symbolize_crash.cpp does; the reasoning there applies unchanged.
//
// Empty when nothing resolves: a frame-pointer walk picks up values that are not
// return addresses, and those have no name to find.
std::vector<std::string> describeRva(DWORD64 rva) {
	std::vector<std::string> located;
	HANDLE process = GetCurrentProcess();
	const DWORD64 address = s_symBase + rva;

	// DbgHelp writes the name into the tail of the structure, hence the buffer.
	char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = { 0 };
	SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
	const auto reset = [&] {
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol->MaxNameLen = MAX_SYM_NAME;
	};

	DWORD64 offset = 0;
	DWORD displacement = 0;
	IMAGEHLP_LINE64 line = { sizeof(IMAGEHLP_LINE64) };

	reset();
	if (!SymFromAddr(process, address, &offset, symbol)) return located;
	const bool haveLine = SymGetLineFromAddr64(process, address, &displacement, &line) != FALSE;
	located.push_back(formatLocation(symbol->Name, offset, haveLine ? &line : NULL));

	// Inline contexts run innermost first, so walk them backwards to keep the
	// caller-to-callee order the rest of the report reads in.
	const DWORD inlined = SymAddrIncludeInlineTrace(process, address);
	DWORD context = 0, frameIndex = 0;
	if (inlined == 0 || inlined > kMaxInlineFrames ||
			!SymQueryInlineTrace(process, address, 0, address, address, &context, &frameIndex))
		return located;

	for (DWORD i = inlined; i-- > 0;) {
		reset();
		if (!SymFromInlineContext(process, address, context + i, &offset, symbol)) continue;
		line = { sizeof(IMAGEHLP_LINE64) };
		const bool ok = SymGetLineFromInlineContext(process, address, context + i, 0,
			&displacement, &line) != FALSE;
		located.push_back(formatLocation(symbol->Name, offset, ok ? &line : NULL));
	}
	return located;
}

// Appends a name to every frame line that resolves, in place. Leaves body alone
// and returns false when there is nothing to add -- no PDB, a report from another
// build, or a stack the walk could not make sense of.
bool symbolizeReport(std::vector<char>& body) {
	const std::string text(body.begin(), body.end());

	// The PDB beside us answers for one binary only. Resolving another one's
	// addresses against it yields names that are wrong and look right, which is
	// worse than the addresses they replaced. The build id catches a report from
	// another commit; the image stamp catches another build of the same commit,
	// which is a different executable and the case that is easy to miss.
	const char* buildId = sgp::crashBuildId();
	if (buildId[0] == '\0') return false;
	char wanted[96] = { 0 };
	int n = snprintf(wanted, sizeof(wanted), "  build %s\r\n", buildId);
	if (n < 0 || text.find(wanted) == std::string::npos) return false;
	n = snprintf(wanted, sizeof(wanted), "  image %08lX\r\n", ourImageStamp());
	if (n < 0 || text.find(wanted) == std::string::npos) return false;
	if (!symInit()) return false;

	DWORD64 imageBase = 0, imageSize = 0;
	std::string out;
	out.reserve(text.size() + 2048);
	int named = 0;

	for (size_t at = 0; at < text.size();) {
		size_t stop = text.find('\n', at);
		const size_t next = (stop == std::string::npos) ? text.size() : stop + 1;
		if (stop == std::string::npos) stop = text.size();
		// The line without its terminator, so anything appended lands before it.
		while (stop > at && (text[stop - 1] == '\r' || text[stop - 1] == '\n')) --stop;
		const std::string line = text.substr(at, stop - at);

		out.append(line);
		unsigned base = 0, size = 0, index = 0, addr = 0;
		if (imageBase == 0 && line.compare(0, 4, "    ") == 0 &&
				sscanf_s(line.c_str(), "    %8x %8x", &base, &size) == 2) {
			// The loader lists the main image first, and it is the only one this
			// PDB can speak for; the rest are named by the module table already.
			imageBase = base;
			imageSize = size;
		} else if (imageBase != 0 && sscanf_s(line.c_str(), "  [%u] %8x", &index, &addr) == 2 &&
				addr >= imageBase && addr < imageBase + imageSize) {
			const std::vector<std::string> located = describeRva(addr - imageBase);
			if (!located.empty()) {
				out.append(" ").append(located[0]);
				// Inlined code gets its own lines under the frame, at six spaces:
				// two is a frame, four is a module, and one line per location keeps
				// the frame numbering positional the way the reader expects.
				const std::string eol = (next > stop) ? text.substr(stop, next - stop) : "\r\n";
				for (size_t k = 1; k < located.size(); ++k)
					out.append(eol).append("      inlined ").append(located[k]);
				++named;
			}
		}
		out.append(text, stop, next - stop);
		at = next;
	}

	// Over the cap the sink answers a settling 400 and the client deletes the
	// file: an upload that grew too large destroys the report it was carrying.
	if (named == 0 || out.size() > kMaxReportBytes) return false;
	body.assign(out.begin(), out.end());
	return true;
}

// The two kinds of report the handler writes are readable for different reasons.
// A fault is nothing but addresses: strip the names and there is no content left,
// which is how 281 rows of `code|module+offset` ended up in the database against
// builds whose PDBs nobody kept. An assertion states its own file, line and
// message, so it is readable on its own terms -- the frames under it are the path
// that reached it, worth having and not what makes it worth sending.
bool reportIsAssertion(const std::string& text) {
	return text.find("\r\n  assertion failed at line ") != std::string::npos;
}

// Names on what can carry them, and a verdict on whether to send at all.
bool prepareUpload(std::vector<char>& body) {
	const std::string text(body.begin(), body.end());
	const bool named = symbolizeReport(body);
	return named || reportIsAssertion(text);
}

// POST one report file to url. Returns the HTTP status, or 0 if the request never
// completed (no connection, DNS failure, timeout) — see reportIsSettled().
DWORD postReport(const wchar_t* url, const char* path) {
	HANDLE fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) return 0;
	DWORD size = GetFileSize(fh, NULL);
	if (size > kMaxReportBytes) { CloseHandle(fh); return 413; }
	std::vector<char> body(size ? size : 1);
	DWORD got = 0;
	BOOL read_ok = ReadFile(fh, body.data(), size, &got, NULL);
	CloseHandle(fh);
	if (!read_ok || got != size) return 0;

	// Nothing unreadable goes on the wire. Zero is "never completed", so
	// reportIsSettled() keeps the file and the next launch tries again -- which is
	// what we want if the PDB turns up beside the exe later. A report that stays
	// unreadable is taken unsent by the existing age reaper.
	//
	// The file on disk is left as the handler wrote it either way: it is the
	// record, and symbolize_crash has to keep reading it.
	if (!prepareUpload(body)) return 0;
	size = static_cast<DWORD>(body.size());

	URL_COMPONENTS uc = {}; uc.dwStructSize = sizeof(uc);
	wchar_t host[256] = {}, urlpath[1024] = {};
	uc.lpszHostName = host;    uc.dwHostNameLength = _countof(host);
	uc.lpszUrlPath = urlpath;  uc.dwUrlPathLength  = _countof(urlpath);
	if (!WinHttpCrackUrl(url, 0, 0, &uc)) return 400;

	HINTERNET hSession = WinHttpOpen(L"JA2-1.13-crash-telemetry",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return 0;
	// Bound every phase. Even off the main thread these must not hang forever: the
	// thread holds the report files open-ish and we want the queue drained or given
	// up on within seconds, not to leave a socket parked for the whole session.
	WinHttpSetTimeouts(hSession, 5000, 5000, 10000, 15000);

	DWORD status = 0;
	if (HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0)) {
		DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		if (HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", urlpath, NULL,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags)) {
			if (WinHttpSendRequest(hReq, L"Content-Type: text/plain\r\n", (DWORD)-1,
					body.data(), size, size, 0) &&
				WinHttpReceiveResponse(hReq, NULL)) {
				DWORD len = sizeof(status);
				WinHttpQueryHeaders(hReq,
					WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);
			}
			WinHttpCloseHandle(hReq);
		}
		WinHttpCloseHandle(hConnect);
	}
	WinHttpCloseHandle(hSession);
	return status;
}

// Whether a report is done with, i.e. safe to delete. Uploaded (2xx), or rejected
// as content the server will never take (a malformed or oversized body). Anything
// else — no connection, 5xx, and notably 404/403 from a mistyped or misconfigured
// CRASH_TELEMETRY_URL — keeps the file, so a bad setting loses nobody's report.
bool reportIsSettled(DWORD status) {
	return (status >= 200 && status < 300) ||
		status == 400 || status == 413 || status == 415;
}

// A report stamped "build local" comes from a developer build with no released
// PDB: nobody at the receiving end can symbolize it, so it never goes on the
// wire — and never gets reaped either, it is the developer's to delete.
bool isFromLocalBuild(const char* path) {
	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return false;
	// The build line is within the first few lines of the header.
	char head[160] = {};
	DWORD got = 0;
	ReadFile(h, head, sizeof(head) - 1, &got, NULL);
	CloseHandle(h);
	return strstr(head, "  build local") != NULL;
}

// Reports older than this are stale: the crash they describe is long since shipped
// past, and a player who was offline for a season should not upload a season of them.
const DWORD kMaxReportAgeDays = 30;

bool olderThan(const FILETIME& ft, DWORD days) {
	FILETIME now;
	GetSystemTimeAsFileTime(&now);
	ULARGE_INTEGER t = { ft.dwLowDateTime, ft.dwHighDateTime };
	ULARGE_INTEGER n = { now.dwLowDateTime, now.dwHighDateTime };
	if (n.QuadPart <= t.QuadPart) return false; // clock skew: treat as fresh
	return (n.QuadPart - t.QuadPart) > days * 24ULL * 60 * 60 * 10000000ULL;
}

// One launch drains at most this many, so a crash-looping build cannot turn startup
// into a long upload session. The rest wait for the next launch.
const int kMaxUploadsPerRun = 20;

wchar_t s_telemetryUrl[512];

// Drains the pending reports. Runs detached: if the player quits first the process
// exits from under it, which costs nothing — an interrupted upload leaves the file
// on disk and it goes out next launch.
unsigned __stdcall telemetryThread(void*) {
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA("crash_report_*.txt", &fd);
	if (hFind == INVALID_HANDLE_VALUE) return 0;
	int sent = 0;
	do {
		if (isFromLocalBuild(fd.cFileName)) continue;
		if (olderThan(fd.ftLastWriteTime, kMaxReportAgeDays)) {
			DeleteFileA(fd.cFileName);
			continue;
		}
		if (sent++ >= kMaxUploadsPerRun) break;
		if (reportIsSettled(postReport(s_telemetryUrl, fd.cFileName)))
			DeleteFileA(fd.cFileName);
	} while (FindNextFileA(hFind, &fd));
	FindClose(hFind);
	return 0;
}

} // anonymous namespace

namespace sgp {
void processCrashTelemetry(const wchar_t* url) {
	if (url == NULL || url[0] == L'\0') return; // no endpoint configured: feature off

	int consent = readConsent();
	if (consent < 0) { // first run: ask once, remember the answer
		int r = MessageBoxW(NULL,
			L"This build can send crash reports to the developers to help fix bugs.\n"
			L"A report contains where the game crashed, the names of the loaded\n"
			L"modules, and the HANDLE from your Ja2.ini if you set one. No file\n"
			L"paths, no save games, nothing else about your machine.\n\n"
			L"Send crash reports automatically?",
			L"Jagged Alliance 2 v1.13 \x2014 Crash Reporting",
			MB_YESNO | MB_ICONQUESTION);
		writeConsent(r == IDYES);
		consent = (r == IDYES) ? 1 : 0;
	}
	if (consent != 1) return; // declined: leave reports on disk, accumulating

	// Hand the draining to a detached thread. The uploads are synchronous WinHttp
	// calls with seconds-long timeouts, and this runs on the startup path: on the
	// main thread a slow or unreachable endpoint is a stall the player sees before
	// the splash screen. Nothing waits on the result, so it can take as long as it
	// takes. The consent prompt above stays here, on purpose — that one is a
	// question, and a question has to be asked before anything is sent.
	lstrcpynW(s_telemetryUrl, url, ARRAYSIZE(s_telemetryUrl));
	// _beginthreadex, not CreateThread: the upload path uses the CRT (std::vector),
	// which wants its per-thread state set up and torn down.
	uintptr_t t = _beginthreadex(NULL, 0, telemetryThread, NULL, 0, NULL);
	if (t) CloseHandle((HANDLE)t);
}
} // namespace sgp

#endif // _MSC_VER
