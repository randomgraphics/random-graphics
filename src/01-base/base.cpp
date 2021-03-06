#include "pch.h"
#include "internal-helpers.h"
#include <iomanip>
#include <stdarg.h>
#include <sstream>
#if RG_MSWIN
#include <windows.h>
#else
#include <signal.h>
#endif

// -----------------------------------------------------------------------------
//
[[noreturn]] void rg::rip() {
    RG_LOG(, F, backtrace().c_str());
    breakIntoDebugger();
    exit(-1);
}

// -----------------------------------------------------------------------------
//
void rg::breakIntoDebugger() {
#if RG_MSWIN
    ::DebugBreak();
#else
    raise(SIGTRAP);
#endif
}

// -----------------------------------------------------------------------------
//
[[noreturn]] void rg::throwRuntimeErrorException(const char * file, int line, const char * message) {
    std::stringstream ss;
    ss << file << ":" << line << " - " << message << std::endl
       << backtrace() << std::endl;
    throw std::runtime_error(ss.str());
}

// -----------------------------------------------------------------------------
//
std::string rg::backtrace() {
    return {};
}

// -----------------------------------------------------------------------------
//
void * rg::aalloc(size_t a, size_t s) {
#if RG_MSWIN
    return _aligned_malloc(s, a);
#else
    return aligned_alloc(a, s);
#endif
}

// -----------------------------------------------------------------------------
//
void rg::afree(void * p) {
#if RG_MSWIN
    _aligned_free(p);
#else
    ::free(p);
#endif
}

// -----------------------------------------------------------------------------
//
const char * rg::formatstr(const char * format, ...) {
    thread_local static std::vector<char> buf1;
    RG_PRINT_TO_VECTOR(buf1, format);
    return buf1.data();
}

// -----------------------------------------------------------------------------
//
std::string rg::ns2str(uint64_t ns) {
    auto us  = ns / 1000ul;
    auto ms  = us / 1000ul;
    auto sec = ms / 1000ul;
    std::stringstream s;
    s << std::fixed << std::setw(5) << std::setprecision(1);
    if (sec > 0) {
        s << (float) (ms) / 1000.0f << "s ";
    } else if (ms > 0) {
        s << (float) (us) / 1000.0f << "ms";
    } else if (us > 0) {
        s << (float) (ns) / 1000.0f << "us";
    } else {
        s << ns << "ns";
    }
    return s.str();
}
