// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.



#if defined(_MSC_VER)

#include "debug_util.h"

#include <windows.h>


#include <dbghelp.h>

#include <winhttp.h>

#include <vfs/Tools/vfs_log.h>



// The arraysize(arr) macro returns the # of elements in an array arr.
// The expression is a compile-time constant, and therefore can be
// used in defining new arrays, for example.  If you use arraysize on
// a pointer by mistake, you will get a compile-time error.
//
// One caveat is that arraysize() doesn't accept any array of an
// anonymous type or a type defined inside a function.  In these rare
// cases, you have to use the unsafe ARRAYSIZE_UNSAFE() macro below.  This is
// due to a limitation in C++'s template system.  The limitation might
// eventually be removed, but it hasn't happened yet.

// This template function declaration is used in defining arraysize.
// Note that the function doesn't need an implementation, as we only
// use its type.
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];

// That gcc wants both of these prototypes seems mysterious. VC, for
// its part, can't decide which to use (another mystery). Matching of
// template overloads: the final frontier.
#ifndef _MSC_VER
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];
#endif

#define arraysize(array) (sizeof(ArraySizeHelper(array)))


namespace {
	// SymbolContext is a threadsafe singleton that wraps the DbgHelp Sym* family
	// of functions. The Sym* family of functions may only be invoked by one
	// thread at a time. SymbolContext code may access a symbol server over the
	// network while holding the lock for this singleton. In the case of high
	// latency, this code will adversly affect performance.
	//
	// There is also a known issue where this backtrace code can interact
	// badly with breakpad if breakpad is invoked in a separate thread while
	// we are using the Sym* functions. This is because breakpad does now
	// share a lock with this function. See this related bug:
	//
	// http://code.google.com/p/google-breakpad/issues/detail?id=311
	//
	// This is a very unlikely edge case, and the current solution is to
	// just ignore it.
	class SymbolContext {
	public:
		static SymbolContext* Get() {
			static SymbolContext* s_singleton = NULL;
			if(!s_singleton) s_singleton = new SymbolContext();
			return s_singleton;
			// We use a leaky singleton because code may call this during process
			// termination.

			//return Singleton<SymbolContext, LeakySingletonTraits<SymbolContext> >::get();
		}

		// Returns the error code of a failed initialization.
		DWORD init_error() const {
			return init_error_;
		}

		// For the given trace, attempts to resolve the symbols, and output a trace
		// to the ostream os. The format for each line of the backtrace is:
		//
		// <tab>SymbolName[0xAddress+Offset] (FileName:LineNo)
		//
		// This function should only be called if Init() has been called. We do not
		// LOG(FATAL) here because this code is called might be triggered by a
		// LOG(FATAL) itself.
		void OutputTraceToStream(const std::vector<void*>& trace, sgp::Logger::LogInstance* os) {
			//AutoLock lock(lock_);

			for (size_t i = 0; (i < trace.size()); ++i) {
				const int kMaxNameLength = 256;
				DWORD_PTR frame = reinterpret_cast<DWORD_PTR>(trace[i]);

				// Code adapted from MSDN example:
				// http://msdn.microsoft.com/en-us/library/ms680578(VS.85).aspx
				ULONG64 buffer[
					(sizeof(SYMBOL_INFO) +
						kMaxNameLength * sizeof(wchar_t) +
						sizeof(ULONG64) - 1) /
						sizeof(ULONG64)];

				// Initialize symbol information retrieval structures.
				DWORD64 sym_displacement = 0;
				PSYMBOL_INFO symbol = reinterpret_cast<PSYMBOL_INFO>(&buffer[0]);
				symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
				symbol->MaxNameLen = kMaxNameLength;
				BOOL has_symbol = SymFromAddr(GetCurrentProcess(), frame,
					&sym_displacement, symbol);

				// Attempt to retrieve line number information.
				DWORD line_displacement = 0;
				IMAGEHLP_LINE64 line = {};
				line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				BOOL has_line = SymGetLineFromAddr64(GetCurrentProcess(), frame,
					&line_displacement, &line);

				// Output the backtrace line.
				(*os) << "    ";
				if (has_symbol)
				{
					(*os) << " [0x" << trace[i] << "+" << sym_displacement << "]\t";
					(*os) << vfs::String(symbol->Name);
				}
				else
				{
					(*os) << "  [0x" << trace[i] << "]\t";
					// If there is no symbol information, add a spacer.
					(*os) << " (No symbol)";
				}
				if (has_line)
				{
					(*os) << " - (" << line.FileName << ":" << line.LineNumber << ")";
				}
				(*os) << sgp::endl;
			}
			(*os) << sgp::endl;
		}

	private:
		SymbolContext() : init_error_(ERROR_SUCCESS) {
			// Initializes the symbols for the process.
			// Defer symbol load until they're needed, use undecorated names, and
			// get line numbers.
			SymSetOptions(SYMOPT_DEFERRED_LOADS |
				SYMOPT_UNDNAME |
				SYMOPT_LOAD_LINES);
			if (SymInitialize(GetCurrentProcess(), NULL, TRUE)) {
				init_error_ = ERROR_SUCCESS;
			} else {
				__debugbreak();
				init_error_ = GetLastError();
			}
		}

		DWORD init_error_;
		//Lock lock_;
		DISALLOW_COPY_AND_ASSIGN(SymbolContext);
	};

} // namespace

#define ENABLE_STACK_TRACE 0

StackTrace::StackTrace() {
	// From http://msdn.microsoft.com/en-us/library/bb204633(VS.85).aspx,
	// the sum of FramesToSkip and FramesToCapture must be less than 63,
	// so set it to 62.
	const int kMaxCallers = 62;	
	// TODO(ajwong): Migrate this to StackWalk64.
	
#if ENABLE_STACK_TRACE

	// WANNE: This only works with Visual Studio version >= 2008
	#if _MSC_VER >= 1500

	void* callers[kMaxCallers];
	int count = CaptureStackBackTrace(0, kMaxCallers, callers, NULL);
	
	// Not used, because we use CaptureStackBackTrace()
	//int count = RtlCaptureStackBackTrace(0, kMaxCallers, callers, NULL);
	
	if (count > 0) {
		trace_.resize(count);
		memcpy(&trace_[0], callers, sizeof(callers[0]) * count);
	} 
	else 
	{
		trace_.resize(0);
	}

	#endif

#endif
}

static struct StackTraceLog {
	sgp::Logger_ID id;
	StackTraceLog() {
		id = sgp::Logger::instance().createLogger();
		sgp::Logger::instance().connectFile(id, L"stack_trace.log", false, sgp::Logger::FLUSH_ON_ENDL);
	};
} s_log;

void StackTrace::PrintBacktrace(const char* msg) {
	sgp::Logger::LogInstance log = SGP_LOG(s_log.id);
	OutputToStream(msg, &log);
}

// Called first-chance from the vectored crash handler, so it runs *inside* the
// faulting thread with a possibly-wrecked heap: the very crash we want to report
// is often heap corruption (a trashed std::map, a wild pointer). Anything that
// allocates — the game Logger, std::vector, DbgHelp's Sym* — would fault again
// and take the report down with it. So this path is deliberately heap-free: only
// stack buffers and raw Win32. It emits raw module VAs; symbolize them offline
// against the shipped PDB:  llvm-symbolizer --obj=JA2.exe <addr>
namespace sgp {

// Stamped into every crash report so a report maps to the exact build's PDB.
// Copied into a fixed buffer (no heap) — safe to read from a crash handler.
static char s_buildId[64] = { 0 };
void setCrashBuildId(const char* id) {
	if (id) lstrcpynA(s_buildId, id, sizeof(s_buildId));
}

void writeExceptionBacktrace(_EXCEPTION_POINTERS* ep) {
	if (ep == NULL || ep->ContextRecord == NULL || ep->ExceptionRecord == NULL)
		return;

	// The same fault storms (Wine re-dispatches the crashing window message) and
	// walking a wrecked stack can fault us in turn. Dump each distinct faulting
	// address once, never re-enter. ponytail: single globals, fine in a crash.
	static void* s_lastFault = NULL;
	static bool  s_inDump    = false;
	void* fault = ep->ExceptionRecord->ExceptionAddress;
	if (s_inDump || fault == s_lastFault)
		return;
	s_lastFault = fault;
	s_inDump = true;

	// One fresh, numbered file per crash. CREATE_NEW claims the first free number,
	// so reports never overwrite each other and pile up while a player is offline;
	// the telemetry uploader drains and deletes them when it can reach the server.
	char name[32];
	HANDLE h = INVALID_HANDLE_VALUE;
	for (int n = 1; n <= 999; ++n) {
		wsprintfA(name, "crash_report_%03d.txt", n);
		h = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if (h != INVALID_HANDLE_VALUE) break;
		if (GetLastError() != ERROR_FILE_EXISTS) break; // real error, stop trying
	}
	if (h == INVALID_HANDLE_VALUE) { s_inDump = false; return; }

	char line[192];
	auto emit = [&](int n) { DWORD w; WriteFile(h, line, n, &w, NULL); };

	const CONTEXT&          c = *ep->ContextRecord;
	const EXCEPTION_RECORD& r = *ep->ExceptionRecord;

	emit(wsprintfA(line,
		"\r\n*** CRASH  code=%08lX  eip=%08lX  esp=%08lX  ebp=%08lX ***\r\n",
		r.ExceptionCode, c.Eip, c.Esp, c.Ebp));
	if (s_buildId[0])
		emit(wsprintfA(line, "  build %s\r\n", s_buildId));
	if (r.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r.NumberParameters >= 2)
		emit(wsprintfA(line, "  access violation: %s %08lX\r\n",
			r.ExceptionInformation[0] == 1 ? "write to " :
			r.ExceptionInformation[0] == 8 ? "execute at" : "read from",
			(DWORD)r.ExceptionInformation[1]));

	// Code range of the main module, to keep the walk to our own return addresses.
	HMODULE mod = GetModuleHandleA(NULL);
	DWORD modLo = (DWORD)(DWORD_PTR)mod;
	IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)mod;
	IMAGE_NT_HEADERS* nt  = (IMAGE_NT_HEADERS*)(modLo + dos->e_lfanew);
	DWORD modHi = modLo + nt->OptionalHeader.SizeOfImage;

	// Committed stack bounds from the TEB, so a bad ebp can't fault our reads.
	NT_TIB* tib = (NT_TIB*)NtCurrentTeb();
	DWORD stkLo = (DWORD)(DWORD_PTR)tib->StackLimit;
	DWORD stkHi = (DWORD)(DWORD_PTR)tib->StackBase;

	emit(wsprintfA(line, "  [0] %08lX\r\n", c.Eip)); // exact crash site
	DWORD ebp = c.Ebp;
	for (int i = 1; i < 64; ++i) {
		if (ebp < stkLo || ebp + 8 > stkHi || (ebp & 3)) break;
		DWORD ret  = *(DWORD*)(ebp + 4);
		DWORD next = *(DWORD*)ebp;
		if (ret >= modLo && ret < modHi)
			emit(wsprintfA(line, "  [%d] %08lX\r\n", i, ret));
		if (next <= ebp) break; // frames must climb the stack
		ebp = next;
	}
	FlushFileBuffers(h);
	CloseHandle(h);
	s_inDump = false;
}
} // namespace sgp

// --- Crash telemetry: upload pending crash_report_*.txt on the next launch. ---
// Runs at startup (heap healthy), never from a crash handler. Ordinary code here.
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

// POST one report file to url. Returns true only on a 2xx (safe to delete it).
bool postReport(const wchar_t* url, const char* path) {
	HANDLE fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fh == INVALID_HANDLE_VALUE) return false;
	DWORD size = GetFileSize(fh, NULL);
	std::vector<char> body(size ? size : 1);
	DWORD got = 0;
	BOOL read_ok = ReadFile(fh, body.data(), size, &got, NULL);
	CloseHandle(fh);
	if (!read_ok || got != size) return false;

	URL_COMPONENTS uc = {}; uc.dwStructSize = sizeof(uc);
	wchar_t host[256] = {}, urlpath[1024] = {};
	uc.lpszHostName = host;    uc.dwHostNameLength = _countof(host);
	uc.lpszUrlPath = urlpath;  uc.dwUrlPathLength  = _countof(urlpath);
	if (!WinHttpCrackUrl(url, 0, 0, &uc)) return false;

	HINTERNET hSession = WinHttpOpen(L"JA2-1.13-crash-telemetry",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return false;

	bool ok = false;
	if (HINTERNET hConnect = WinHttpConnect(hSession, host, uc.nPort, 0)) {
		DWORD flags = (uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
		if (HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", urlpath, NULL,
				WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags)) {
			if (WinHttpSendRequest(hReq, L"Content-Type: text/plain\r\n", (DWORD)-1,
					body.data(), size, size, 0) &&
				WinHttpReceiveResponse(hReq, NULL)) {
				DWORD status = 0, len = sizeof(status);
				WinHttpQueryHeaders(hReq,
					WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
					WINHTTP_HEADER_NAME_BY_INDEX, &status, &len, WINHTTP_NO_HEADER_INDEX);
				ok = (status >= 200 && status < 300);
			}
			WinHttpCloseHandle(hReq);
		}
		WinHttpCloseHandle(hConnect);
	}
	WinHttpCloseHandle(hSession);
	return ok;
}

} // anonymous namespace

namespace sgp {
void processCrashTelemetry(const wchar_t* url) {
	if (url == NULL || url[0] == L'\0') return; // no endpoint configured: feature off

	int consent = readConsent();
	if (consent < 0) { // first run: ask once, remember the answer
		int r = MessageBoxW(NULL,
			L"This build can send crash reports to the developers to help fix bugs.\n"
			L"A report contains only technical crash data \x2014 no personal information.\n\n"
			L"Send crash reports automatically?",
			L"Jagged Alliance 2 v1.13 \x2014 Crash Reporting",
			MB_YESNO | MB_ICONQUESTION);
		writeConsent(r == IDYES);
		consent = (r == IDYES) ? 1 : 0;
	}
	if (consent != 1) return; // declined: leave reports on disk, accumulating

	// Upload each pending report; delete only the ones that post cleanly, so a
	// failed upload (no connection) simply retries next launch.
	WIN32_FIND_DATAA fd;
	HANDLE hFind = FindFirstFileA("crash_report_*.txt", &fd);
	if (hFind == INVALID_HANDLE_VALUE) return;
	do {
		if (postReport(url, fd.cFileName))
			DeleteFileA(fd.cFileName);
	} while (FindNextFileA(hFind, &fd));
	FindClose(hFind);
}
} // namespace sgp

void StackTrace::OutputToStream(const char* msg, sgp::Logger::LogInstance* os) {
	SymbolContext* context = SymbolContext::Get();
	DWORD error = context->init_error();
	if (error != ERROR_SUCCESS)
	{
		(*os)	<< "Error initializing symbols (" << error 
				<< "). Dumping unresolved backtrace:"
				<< sgp::endl;
		for (size_t i = 0; (i < trace_.size()); ++i)
		{
			(*os) << "\t" << trace_[i] << sgp::endl;
		}
	}
	else
	{
		(*os) << L"Backtrace: " << (msg != NULL ? msg : "") << sgp::endl;
		context->OutputTraceToStream(trace_, os);
	}
}

#endif // _MSC_VER
