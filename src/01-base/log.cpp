#include "pch.h"
#include "internal-helpers.h"
#include <stdarg.h>
#include <atomic>
#include <sstream>
#if RG_MSWIN
#include <windows.h>
#elif RG_ANDROID
#include <android/log.h>
#endif

using namespace rg;
using namespace rg::log;
using namespace std::string_literals;

// -----------------------------------------------------------------------------
//
static inline std::string sev2str(int sev) {
    switch(sev) {
        case rg::log::macros::F : return "FATAL  "s;
        case rg::log::macros::E : return "ERROR  "s;
        case rg::log::macros::W : return "WARN   "s;
        case rg::log::macros::I : return "INFO   "s;
        case rg::log::macros::V : return "VERBOSE"s;
        case rg::log::macros::B : return "BABBLE "s;
        default                 : return std::to_string(sev);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
//
static void writeToSystemLog(int severity, const std::string & messageWithNewLine) {
#if RG_MSWIN
    (void)severity;
    ::OutputDebugStringA(messageWithNewLine.c_str());
#elif RG_ANDROID
    int priority;
    if (severity <= rg::log::macros::F) {
        priority = ANDROID_LOG_FATAL;
    } else if (severity <= rg::log::macros::E) {
        priority = ANDROID_LOG_ERROR;
    } else if (severity <= rg::log::macros::W) {
        priority = ANDROID_LOG_WARN;
    } else if (severity <= rg::log::macros::I) {
        priority = ANDROID_LOG_INFO;
    } else {
        priority = ANDROID_LOG_VERBOSE;
    }
    {
        std::istringstream text(messageWithNewLine);
        static std::mutex m;
        std::lock_guard<std::mutex> guard(m); // this is to prevent log from different threads to get interleaved with each other.
        // static std::string logFileName = getLogFileName();
        // std::ofstream f(logFileName, std::ios_base::app);
        // auto prefix = getLogPrefix();
        std::string l;
        while (std::getline(text, l)) {
            __android_log_print(priority, "RandomG", "%s", l.c_str());
            // f << prefix << tag << l << std::endl;
        }
    }
#else
    // GDB/LLDB ?
    (void)severity;
    (void)messageWithNewLine;
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
//
static void defaultLogCallback(void *, const LogDesc & desc, const char * text) {
    if (!text || !*text) return;
    
    std::stringstream ss;
    FILE * fp;
    if (desc.severity >= rg::log::macros::I) {
        ss << "[" << sev2str(desc.severity) << "] " << text << std::endl;
        fp = stdout;
        fprintf(stdout, "[%s] %s\n", sev2str(desc.severity).c_str(),  text);
    } else {
        ss << "[" << sev2str(desc.severity) << "] " << desc.file << ":" << desc.line << " - " << text << std::endl;
        fp = stderr;
    }

    auto str = ss.str();
    fprintf(fp, "%s", str.c_str());
    
    writeToSystemLog(desc.severity, str);
}

// ---------------------------------------------------------------------------------------------------------------------
//
RG_API std::atomic<LogCallback> globalLogCallback = { { defaultLogCallback, nullptr } };
RG_API LogCallback rg::setLogCallback(LogCallback lc) {
    if (!lc.func) lc.func = defaultLogCallback;
    return globalLogCallback.exchange(lc);
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::log::Controller::Globals::Globals() {
    root = new Controller("");
    severity = rg::log::macros::I;
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::log::Controller::Globals::~Globals() {
    for(auto iter : instances) {
        delete iter.second;
    }
    instances.clear();
    delete root;
}

rg::log::Controller::Globals rg::log::Controller::g;

// ---------------------------------------------------------------------------------------------------------------------
//
Controller * rg::log::Controller::getInstance(const char * tag) {
    if (!tag || !*tag) return g.root;
    auto & c = g.instances[tag];
    if (!c) c = new Controller(tag);
    return c;
}

// ---------------------------------------------------------------------------------------------------------------------
//
const char * rg::log::Helper::formatlog(const char * format_, ...) {
    thread_local static std::vector<char> buf1_;
    RG_PRINT_TO_VECTOR(buf1_, format_);
    return buf1_.data();
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::log::Helper::post(const char * s) {
    globalLogCallback.load()(_desc, s);
}
