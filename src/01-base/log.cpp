#include "pch.h"
#include "internal-helpers.h"
#include <stdarg.h>
#include <atomic>

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
static void defaultLogCallback(void *, const LogDesc & desc, const char * text) {
    if (!text || !*text) return;
    if (desc.severity >= rg::log::macros::I) {
        fprintf(stdout, "[%s] %s\n", sev2str(desc.severity).c_str(),  text);
    } else {
        fprintf(stderr, "[%s] %s(%d): %s\n", sev2str(desc.severity).c_str(), desc.file, desc.line, text);
    }
}
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
