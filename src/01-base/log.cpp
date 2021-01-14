#include "pch.h"
#include <stdarg.h>

using namespace rg;
using namespace rg::log;

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
}

rg::log::Controller::Globals rg::log::Controller::g;

// ---------------------------------------------------------------------------------------------------------------------
//
Controller * rg::log::Controller::getInstance(const char *) {
    // TODO: create controller for non-empty tag
    return g.root;
}

// ---------------------------------------------------------------------------------------------------------------------
//
const char * rg::log::Helper::formatlog(const char * format, ...) {

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

    return "";
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::log::Helper::post(const char *) {
}
