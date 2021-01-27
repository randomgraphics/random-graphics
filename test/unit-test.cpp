#include "rg/base.h"

#define CATCH_CONFIG_MAIN // Let Catch provide main():
#include "catch.hpp"

using namespace rg;
using namespace std::string_literals;

struct LogData {
    std::string tag;
    std::string log;
    static void func(void * context, const LogDesc & desc, const char * text) {
        auto d = (LogData*)context;
        d->tag = desc.tag;
        d->log = text;
    }
};

TEST_CASE("log", "[base]") {
    LogData data;
    setLogCallback({LogData::func, &data});

    RG_LOGI("printf style log %d", 1);
    CHECK(data.log == "printf style log 1"); data.log.clear();
    
    RG_LOGI(s("c++ style log ") << 1 << 2);
    CHECK(data.log == "c++ style log 12"); data.log.clear();
    
    RG_LOG("tag1", I, "log with immediate controller name");
    CHECK(data.tag == "tag1");
    
    auto ctrl = rg::log::Controller::getInstance("tag2");
    RG_LOG(ctrl, I, "log with external controller instance");
    CHECK(data.tag == "tag2");
    
    setLogCallback({});
}

TEST_CASE("formatstr", "[base]") {
    CHECK("abcd 10"s == rg::formatstr("abcd %d", 10));
}

#if HAS_OPENGL

#include "rg/opengl.h"

using namespace rg::gl;

TEST_CASE("context", "[opengl]") {

    SECTION("default pbuffer context") {
        PBufferRenderContext rc({});
        CHECK(rc.good());
    }
}

#endif