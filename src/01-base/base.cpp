#include "pch.h"
#include <iomanip>
#include <stdarg.h>
#include <signal.h>

// -----------------------------------------------------------------------------
//
[[noreturn]] void rg::rip() {
    RG_LOG(, F, backtrace().c_str());
    raise(SIGSEGV);
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
    thread_local static char buf1[16384];
    va_list args;
    va_start(args, format);
#ifdef _MSC_VER
    _vsnprintf_s(buf1, std::size(buf1), _TRUNCATE, format, args);
#else
    vsnprintf(buf1, std::size(buf1), format, args);
#endif
    va_end(args);
    buf1[std::size(buf1)-1] = 0;
    return buf1;
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
