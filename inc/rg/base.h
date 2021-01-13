#pragma once

#include <vector>
#include <string>
#include <sstream>

#define RG_LOG(severity, ...) do {\
    using namespace rg::log::macros; \
    if (rg::log::Controller::getInstance()->enabled(severity)) { \
        rg::log::Helper((int)severity, __FILE__, __LINE__, __FUNCTION__)(__VA_ARGS__); \
    } } while(0)

namespace rg {


namespace str {

const char * format(const char *, ...);

} // namespace str

namespace log {

class Controller {

    bool _enabled;

    explicit Controller(const char * name);

public:

    static Controller * getInstance(const char * name = nullptr);
    static inline Controller * getInstance(Controller * c) { return c; }

    ~Controller();

    bool enabled(int) const { return _enabled; }

    void setEnabled(bool);
};

namespace macros {
    inline constexpr int F = 0;  // fatal
    inline constexpr int E = 10; // error
    inline constexpr int W = 20; // warning
    inline constexpr int I = 30; // informational
    inline constexpr int V = 40; // verbose
    inline constexpr int B = 50; // babble
    
    inline Controller * c(const char * str = nullptr) {
        return Controller::getInstance(str);
    }

    inline std::stringstream ss(const char * str) {
        std::stringstream ss;
        ss << str;
        return ss;
    }
}

class Helper {

    struct Info {
        int          severity;
        const char * file;
        int          line;
        const char * function;
    };

    Info _info;

    void writeLine(const char * text);

public:

    template<class... Args>
    Helper(Args&&... args) : _info{args...} {
    }

    template<class... Args>
    void operator()(const char * format, Args&&... args) {
        writeLine(str::format(format, std::forward<Args>(args)...));
    }

    void operator()(const std::basic_ostream<char> & s) {
        writeLine(reinterpret_cast<const std::stringstream&>(s).str().c_str());
    }

    template<class... Args>
    void operator()(Controller * c, const char * format, Args&&... args) {
        if (c && c->enabled(_info.severity)) {
            writeLine(str::format(format, std::forward<Args>(args)...));
        }
    }

    void operator()(Controller * c, const std::basic_ostream<char> & s) {
        if (c && c->enabled(_info.severity)) {
            operator()(s);
        }
    }
};

} // namespace log

} // namespace rg