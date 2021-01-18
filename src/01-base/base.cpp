#include "pch.h"
#include <iomanip>
#include <stdarg.h>
#include <signal.h>

// -----------------------------------------------------------------------------
//
[[noreturn]] void rg::rip() {
    RG_LOG(, F, backtrace().c_str());
    raise(SIGSEGV);
    throw "rip";
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
template<class PRINTF>
static void printToVector(PRINTF p, std::vector<char> & buffer) {
    if (buffer.empty()) buffer.resize(1);
    int n = p();
    if (n < 0) return; // printf error
    if ((size_t)(n + 1) > buffer.size()) {
        buffer.resize((size_t)n + 1);
    }
    n = p();
    RG_ASSERT((size_t)n < buffer.size());
    buffer[n] = 0;
}

// -----------------------------------------------------------------------------
//
const char * rg::formatstr(const char * format, ...) {
    va_list args;
    va_start(args, format);
    thread_local static std::vector<char> buf1;
    printToVector([&]{ return vsnprintf(buf1.data(), std::size(buf1), format, args); }, buf1);
    va_end(args);
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
