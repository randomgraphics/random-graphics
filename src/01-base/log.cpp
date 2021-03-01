#include "pch.h"
#include "internal-helpers.h"
#include <stdarg.h>
#include <atomic>
#include <sstream>
#include <iomanip>
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
static void writeToSystemLog(const char * tag, int severity, const std::string & messageWithNewLine) {
#if RG_ANDROID
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

    // Android logcat has about 4K limit of the single log size. We sometimes generate very long log, like
    // shader compile errors. To avoid hitting the limit, we split the log into lines, with the assumption
    // that a single-line log message will rarely hit Android's limit.
    std::istringstream text(messageWithNewLine);
    static std::mutex m;
     // hold a log to prevent log from different threads being interleaved.
    std::lock_guard<std::mutex> guard(m);
    std::string line;
    while (std::getline(text, line)) {
        __android_log_print(priority, tag, "%s", line.c_str());
        // f << prefix << tag << l << std::endl;
    }
#else
    // GDB/LLDB ?
    (void)tag;
    (void)severity;
    (void)messageWithNewLine;
#endif
}

// ---------------------------------------------------------------------------------------------------------------------
//
static void defaultLogCallback(void *, const LogDesc & desc, const char * text) {
    if (!text || !*text) return;

    // determine log prefix based on severity
    std::stringstream prefixStream;
    prefixStream << "[" << sev2str(desc.severity) << "] ";
    auto prefix = prefixStream.str();

    // create a space indentation in the same length as the prefix
    std::string indent;
    indent.insert(0, prefix.size(), ' ');

    // now indent the message text.
    std::stringstream ss;
    std::istringstream iss(text);
    std::string line;
    int lineCount = 0;
    while (std::getline(iss, line)) {
        if (0 == lineCount) {
            // this is the 1st line of the log
            ss << prefix;
            if (desc.severity < rg::log::macros::I) {
                ss << desc.file << ":" << desc.line << " - ";
            }
            ss << line << std::endl;
        } else {
            ss << indent << line << std::endl;
        }
        // f << prefix << tag << l << std::endl;
        ++lineCount;
    }

    // determine the output file descriptor
    FILE * fp;
    if (desc.severity >= rg::log::macros::I) {
        fp = stdout;
    } else {
        fp = stderr;
    }

    // write to file
    auto str = ss.str();
    fprintf(fp, "%s", str.c_str());

    // write to system log
    writeToSystemLog(desc.tag, desc.severity, str);
}

// ---------------------------------------------------------------------------------------------------------------------
//
std::atomic<LogCallback> globalLogCallback = { { defaultLogCallback, nullptr } };
LogCallback rg::setLogCallback(LogCallback lc) {
    if (!lc.func) lc.func = defaultLogCallback;
    return globalLogCallback.exchange(lc);
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::log::Controller::Globals::Globals() {
    root = new Controller("RandomG");
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
