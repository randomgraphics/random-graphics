#include "rg/base.h"
#include <filesystem>

#define CATCH_CONFIG_MAIN // Let Catch provide main():
#include "catch.hpp"

using namespace rg;
using namespace std::string_literals;

// ---------------------------------------------------------------------------------------------------------------------
//
TEST_CASE("log", "[base]") {
    struct LogData {
        std::string tag;
        std::string log;
        static void func(void * context, const LogDesc & desc, const char * text) {
            auto d = (LogData*)context;
            d->tag = desc.tag;
            d->log = text;
        }
    };
    LogData data;
    setLogCallback({LogData::func, &data});
    auto end = ScopeExit([]{ setLogCallback({}); }); // restore default log callback at the end.

    RG_LOGI("printf style log %d", 1);
    CHECK(data.log == "printf style log 1"); data.log.clear();
    
    RG_LOGI(s("c++ style log ") << 1 << 2);
    CHECK(data.log == "c++ style log 12"); data.log.clear();
    
    RG_LOG("tag1", I, "log with immediate controller name");
    CHECK(data.tag == "tag1");
    
    auto ctrl = rg::log::Controller::getInstance("tag2");
    RG_LOG(ctrl, I, "log with external controller instance");
    CHECK(data.tag == "tag2");

    // Log and return one-liner.
    auto foo = [](bool b){
        if (b)
            return RG_LOGI("true"), true;
        else
            return RG_LOGI("false"), false;
    };
    CHECK(foo(true));
    CHECK(data.log == "true");
    CHECK(!foo(false));
    CHECK(data.log == "false");
}

// ---------------------------------------------------------------------------------------------------------------------
//
TEST_CASE("formatstr", "[base]") {
    CHECK("abcd 10"s == rg::formatstr("abcd %d", 10));
}

// ---------------------------------------------------------------------------------------------------------------------
// quick test of image loading from file.
TEST_CASE("image", "[base]") {
    SECTION("dxt1") {
        auto desc = ImageDesc(rg::ImagePlaneDesc::make(ColorFormat::DXT1_UNORM(), 256, 256, 1), 6, 0);
        REQUIRE(desc.plane(0, 0).alignment == 8);
        REQUIRE(desc.slice(0, 0) == 32768);
        REQUIRE(desc.slice(0, 1) == 8192);
        REQUIRE(desc.slice(0, 2) == 2048);
        REQUIRE(desc.slice(0, 3) == 512);
        REQUIRE(desc.slice(0, 4) == 128);
        REQUIRE(desc.slice(0, 5) == 32);
        REQUIRE(desc.slice(0, 6) == 8);
        REQUIRE(desc.slice(0, 7) == 8);
        REQUIRE(desc.slice(0, 8) == 8);
        REQUIRE(desc.size == 43704 * 6);
    }
    SECTION("jpg") {
        auto path = std::filesystem::path(TEST_FOLDER) / "alien-planet.jpg";
        RG_LOGI("load image from file: %s", path.string().c_str());
        RawImage ri = RawImage::load(path.string());
        REQUIRE(!ri.empty());
        CHECK(ri.width() == 600);
        CHECK(ri.height() == 486);
    }
}

// ---------------------------------------------------------------------------------------------------------------------
//
#if HAS_OPENGL
#include "rg/opengl.h"
using namespace rg::opengl;
TEST_CASE("context", "[opengl]") {

    SECTION("default pbuffer context") {
        PBufferRenderContext rc({});
        CHECK(rc.good());
    }
}
#endif