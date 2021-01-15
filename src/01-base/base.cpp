#include "pch.h"
#include <iomanip>
#include <stdarg.h>

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
