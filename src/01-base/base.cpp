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

#if RG_ANDROID
#include <unwind.h>
#include <cxxabi.h>
#include <dlfcn.h>
#endif

// -----------------------------------------------------------------------------
//
void rg::breakIntoDebugger() {
    auto bt = backtrace();
    if (!bt.empty()) {
        RG_LOG(, F, bt.c_str());
    }
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
    ss << file << ":" << line << " - " << message << std::endl;
    auto bt = backtrace();
    if (!bt.empty()) {
        RG_LOGE("%s", bt.c_str());
        ss << backtrace() << std::endl;
    }
    throw std::runtime_error(ss.str());
}

// -----------------------------------------------------------------------------
//
std::string rg::backtrace(int indent) {
#if RG_ANDROID
    struct android_backtrace_state
    {
        void **current;
        void **end;

        static _Unwind_Reason_Code android_unwind_callback(struct _Unwind_Context* context, void* arg)
        {
            android_backtrace_state* state = (android_backtrace_state *)arg;
            uintptr_t pc = _Unwind_GetIP(context);
            if (pc) {
                if (state->current == state->end)
                    return _URC_END_OF_STACK;
                else
                    *state->current++ = reinterpret_cast<void*>(pc);
            }
            return _URC_NO_REASON;
        }
    };

    std::string prefix;
    for(int i = 0; i < indent; ++i) prefix += ' ';

    std::stringstream ss;
    ss << prefix << "android stack dump\n";

    const int max = 100;
    void* buffer[max];

    android_backtrace_state state;
    state.current = buffer;
    state.end = buffer + max;

    _Unwind_Backtrace(android_backtrace_state::android_unwind_callback, &state);

    int count = (int)(state.current - buffer);

    for (int idx = 0; idx < count; idx++)
    {
        const void* addr = buffer[idx];
        const char* symbol = "<no symbol>";

        Dl_info info;
        if (dladdr(addr, &info) && info.dli_sname)
        {
            symbol = info.dli_sname;
        }
        int status = 0;
        char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);
        ss << prefix << formatstr(
            "%03d: 0x%p %s\n",
            idx,
            addr,
            (NULL != demangled && 0 == status) ?
            demangled : symbol);
        if (NULL != demangled)
            free(demangled);
    }

    ss << prefix << "android stack dump done\n";

    return ss.str();
#else
    (void)indent;
    return {};
#endif
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
