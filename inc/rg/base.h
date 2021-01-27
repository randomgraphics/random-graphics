#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <mutex>
#include <memory>

/// Set RG_BUILD_DEBUG to 0 to disable debug features.
#ifndef RG_BUILD_DEBUG
#ifdef _DEBUG
#define RG_BUILD_DEBUG 1
#else
#define RG_BUILD_DEBUG 0
#endif
#endif

/// Set RG_BUILD_STATIC to 0 to built as shared library.
#ifndef RG_BUILD_STATIC
#define RG_BUILD_STATIC 1
#endif

/// determine operating system
//@{
#if defined(_WIN32)
#define RG_MSWIN 1
#elif defined(__APPLE__)
#define RG_UNIX_LIKE 1
#define RG_DARWIN 1
#elif defined(__linux__)
#define RG_UNIX_LIKE 1
#define RG_LINUX 1
#ifdef __ANDROID__
#define RG_ANDROID 1
#endif
#endif
//@}

// Disable some known "harmless" warnings. So we can use /W4 throughout our code base.
#ifdef _MSC_VER
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4458) // declaration hides class member
#endif

/// Log macros
//@{
#define RG_LOG(tag__, severity, ...) do { \
    using namespace rg::log::macros; \
    auto ctrl__ = rg::log::Controller::getInstance(tag__); \
    if (ctrl__->enabled(severity)) { \
        rg::log::Helper(ctrl__->tag().c_str(), __FILE__, __LINE__, __FUNCTION__, (int)severity)(__VA_ARGS__); \
    } } while(0)
#define RG_LOGE(...) RG_LOG(, E, __VA_ARGS__)
#define RG_LOGW(...) RG_LOG(, W, __VA_ARGS__)
#define RG_LOGI(...) RG_LOG(, I, __VA_ARGS__)
#define RG_LOGV(...) RG_LOG(, V, __VA_ARGS__)
#define RG_LOGB(...) RG_LOG(, B, __VA_ARGS__)
//@}

/// Log macros enabled only in debug build
//@{
#if RG_BUILD_DEBUG
#define RG_DLOG RG_LOG
#else
#define RG_DLOG(...) void(0)
#endif
#define RG_DLOGE(...) RG_DLOG(, E, __VA_ARGS__)
#define RG_DLOGW(...) RG_DLOG(, W, __VA_ARGS__)
#define RG_DLOGI(...) RG_DLOG(, I, __VA_ARGS__)
#define RG_DLOGV(...) RG_DLOG(, V, __VA_ARGS__)
#define RG_DLOGB(...) RG_DLOG(, B, __VA_ARGS__)
//@}

/// call this when encountering unrecoverable error.
#define RG_RIP(...)                         \
    do {                                    \
        RG_LOG(, F, "[RIP] " __VA_ARGS__);  \
        rg::rip();                          \
    } while (0)

/// Check for required condition, call the failure clause if the condition is not met.
#define RG_CHK(x, action_on_false)                      \
    if (!(x)) {                                         \
        RG_LOG(, F, "Condition ("#x") didn't met.");    \
        action_on_false;                                \
    } else                                              \
        void(0)

/// Check for required conditions. call RIP when the condition is not met.
#define RG_RIP_IF_NOT(x)  RG_CHK(x, rg::rip())

/// Runtime assertion
//@{
#if RG_BUILD_DEBUG
#define RG_ASSERT(x, ...)              \
    if (!(x)) {                        \
        RG_RIP("ASSERT failure: "#x);  \
    } else                             \
        void(0)
#else
#define RG_ASSERT(...) (void) 0
#endif
//@}

/// disable copy semantic of a class.
#define RG_NO_COPY(X)                 \
    X(const X&) = delete;             \
    X& operator=(const X&) = delete

/// disable move semantic of a class.
#define RG_NO_MOVE(X)                 \
    X(X&&)     = delete;              \
    X& operator=(X&&) = delete

/// enable default copy semantics
#define RG_DEFAULT_COPY(X)            \
    X(const X&) = default;            \
    X& operator=(const X&) = default

/// enable default move semantics
#define RG_DEFAULT_MOVE(X)            \
    X(X&&)     = default;             \
    X& operator=(X&&) = default

/// define move semantics for class implemented using pimpl pattern
#define RG_PIMPL_MOVE(X, pimpl_member, method_to_delete_pimpl) \
    X(X && w) : pimpl_member(w.pimpl_member) { w.pimpl_member = nullptr; } \
    X & operator=(X && w) { \
        if (&w != this) { \
            this->method_to_delete_pimpl(); \
            this->pimpl_member = w.pimpl_member; \
            w.pimpl_member = nullptr; \
        } \
    }

/// export/import type decl
//@{
#if RG_MSWIN
#define RG_IMPORT __declspec(dllimport)
#define RG_EXPORT __declspec(dllexport)
#else
#define RG_IMPORT
#define RG_EXPORT
#endif
#if RG_BUILD_STATIC
#define RG_API
#elif defined(RG_INTERNAL)
#define RG_API RG_EXPORT
#else
#define RG_API RG_IMPORT
#endif
//@}

/// endianess
//@{
#if defined(__LITTLE_ENDIAN__) || defined(__BIG_ENDIAN__)
#define RG_LITTLE_ENDIAN defined(__LITTLE_ENDIAN__) ///< true on little endian machine
#define RG_BIT_ENDIAN    defined(__BIG_ENDIAN__)    ///< true on big endian machine
#elif defined(_PPC_)
#define RG_LITTLE_ENDIAN  0
#define RG_BIG_ENDIAN     1
#else
#define RG_LITTLE_ENDIAN  1
#define RG_BIG_ENDIAN     0
#endif
#if !( RG_LITTLE_ENDIAN ^ RG_BIG_ENDIAN )
#error Must be either little engine or big endian.
#endif
//@}

namespace rg {

struct LogDesc {
    const char * tag;
    const char * file;
    int          line;
    const char * func;
    int          severity;
};

struct LogCallback {
    void (*func)(void * context, const LogDesc & desc, const char * text) = nullptr;
    void * context = nullptr;
    void operator()(const LogDesc & desc, const char * text) const { func(context, desc, text); }
};

/// Set log callback.
/// \param lc set to null to restore to default callback.
/// \return returns the current callback function pointer.
RG_API LogCallback setLogCallback(LogCallback lc);

namespace log { // namespace for log implementation details

class Controller {

    static struct Globals {
        std::mutex m;
        Controller * root;
        int severity;
        std::map<std::string, Controller*> instances;
        Globals();
        ~Globals();
    } g;

    std::string  _tag;
    bool         _enabled = true;

    Controller(const char * tag) : _tag(tag) {}
    
    ~Controller() = default;

public:

    static inline Controller * getInstance() { return g.root; }
    static inline Controller * getInstance(Controller * c) { return c; }
    static Controller * getInstance(const char * tag);

    bool enabled(int severity) const {
        return _enabled && severity <= g.severity;
    }

    const std::string & tag() const { return _tag; }
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

struct LogStream {
    std::stringstream ss;
    template<typename T>
    LogStream & operator<<(T && t) {
        ss << std::forward<T>(t);
        return *this;
    }
};

inline LogStream s(const char * str) {
    LogStream ss;
    ss.ss << str;
    return ss;
}

}; // namespace macro

class Helper {

    LogDesc _desc;

    const char * formatlog(const char *, ...);

    void post(const char *);

public:

    template<class... Args>
    Helper(Args&&... args) : _desc{args...} {
    }

    template<class... Args>
    void operator()(const char * format, Args&&... args) {
        post(formatlog(format, std::forward<Args>(args)...));
    }

    template<class... Args>
    void operator()(const std::string & format, Args&&... args) {
        post(formatlog(format.c_str(), std::forward<Args>(args)...));
    }

    template<class... Args>
    void operator()(const std::stringstream & format, Args&&... args) {
        post(formatlog(format.str().c_str(), std::forward<Args>(args)...));
    }

    void operator()(const macros::LogStream & s) {
        post(formatlog(s.ss.str().c_str()));
    }

    template<class... Args>
    void operator()(Controller * c, const char * format, Args&&... args) {
        if (Controller::getInstance(c)->enabled(_desc.severity)) {
            _desc.tag = c->tag().c_str();
            operator()(format, std::forward<Args>(args)...);
        }
    }

    template<class... Args>
    void operator()(Controller * c, const std::string & format, Args&&... args) {
        operator()(c, format.c_str(), std::forward<Args>(args)...);
    }

    template<class... Args>
    void operator()(Controller * c, const std::stringstream & format, Args&&... args) {
        operator()(c, format.str().c_str(), std::forward<Args>(args)...);
    }

    void operator()(Controller * c, const macros::LogStream & s) {
        if (Controller::getInstance(c)->enabled(_desc.severity)) {
            _desc.tag = c->tag().c_str();
            operator()(s);
        }
    }
};

} // namespace log

/// Force termination of the current process.
[[noreturn]] void rip();

/// dump current callstck to string
std::string backtrace();

/// allocated aligned memory. The returned pointer must be freed by
void * aalloc(size_t alignment, size_t bytes);

/// free memory allocated by aalloc()
void afree(void *);

/// Return's pointer to the internal storage. The content will be overwritten
/// by the next call on the same thread.
const char * formatstr(const char * format, ...);

/// convert duration in nanoseconds to string
std::string ns2str(uint64_t ns);

/// call exit function automatically at scope exit
template<typename PROC>
class ScopeExit {
    PROC & _proc;
    bool _active = true;
public:

    ScopeExit(PROC && proc) : _proc(proc) {}

    ~ScopeExit() { exit(); }

    RG_NO_COPY(ScopeExit);
    RG_NO_MOVE(ScopeExit);

    /// manually call the exit function.
    void exit() {
        if (_active) {
            _active = false;
            _proc();
        }
    }

    // dismiss the scope exit action w/o calling it.
    void dismiss() { _active = false; }
};

/// Commonly used math constants
//@{
inline static constexpr float PI = 3.1415926535897932385f;
inline static constexpr float HALF_PI = (PI / 2.0f);
inline static constexpr float TWO_PI = (PI * 2.0f);
//@}

// Basic math utlities
//@{

// -----------------------------------------------------------------------------
/// degree -> radian
template<typename T>
inline T deg2rad( T a ) { return a * (T)0.01745329252f; }

// -----------------------------------------------------------------------------
/// radian -> degree
template<typename T>
inline T rad2deg( T a ) { return a * (T)57.29577951f; }

// -----------------------------------------------------------------------------
/// check if n is power of 2
template<typename T>
inline constexpr bool isPowerOf2(T n) {
    return ( 0 == (n & (n - 1)) ) && ( 0 != n );
}

// -----------------------------------------------------------------------------
/// the smallest power of 2 larger or equal to n
inline constexpr uint32_t ceilPowerOf2(uint32_t n) {
    n -= 1;
    n |= n >> 16;
    n |= n >> 8;
    n |= n >> 4;
    n |= n >> 2;
    n |= n >> 1;
    return n + 1;
}

// -----------------------------------------------------------------------------
/// the smallest power of 2 larger or equal to n
inline constexpr uint64_t ceilPowerOf2(uint64_t n) {
    n -= 1;
    n |= n >> 32;
    n |= n >> 16;
    n |= n >> 8;
    n |= n >> 4;
    n |= n >> 2;
    n |= n >> 1;
    return n + 1;
}

// -----------------------------------------------------------------------------
/// the lastest power of 2 smaller or equal to n
inline constexpr uint32_t floorPowerOf2(uint32_t n) {
    n |= n >> 16;
    n |= n >> 8;
    n |= n >> 4;
    n |= n >> 2;
    n |= n >> 1;
    return (n + 1) >> 1;
}

// -----------------------------------------------------------------------------
/// the lastest power of 2 smaller or equal to n
inline constexpr uint64_t floorPowerOf2(uint64_t n) {
    n |= n >> 32;
    n |= n >> 16;
    n |= n >> 8;
    n |= n >> 4;
    n |= n >> 2;
    n |= n >> 1;
    return (n + 1) >> 1;
}

// -----------------------------------------------------------------------------
//
template < typename T >
inline constexpr T clamp(T value, const T & vmin, const T & vmax) {
    return vmin > value ? vmin : vmax < value ? vmax : value;
}

// -----------------------------------------------------------------------------
/// align numeric value up to the next multiple of the alignment.
/// Note that the alignment must be 2^N.
template< typename T >
inline constexpr T nextMultiple(const T & value, const T & alignment) {
    RG_ASSERT(isPowerOf2(alignment));
    return ( value + (alignment-1) ) & ~(alignment-1);
}

//@}

///
/// color format structure
///
union ColorFormat {
    ///
    /// color layout
    ///
    enum Layout {
        LAYOUT_UNKNOWN = 0,
        LAYOUT_1,
        LAYOUT_2_2_2_2,
        LAYOUT_3_3_2,
        LAYOUT_4_4,
        LAYOUT_4_4_4_4,
        LAYOUT_5_5_5_1,
        LAYOUT_5_6_5,
        LAYOUT_8,
        LAYOUT_8_8,
        LAYOUT_8_8_8,
        LAYOUT_8_8_8_8,
        LAYOUT_10_11_11,
        LAYOUT_11_11_10,
        LAYOUT_10_10_10_2,
        LAYOUT_16,
        LAYOUT_16_16,
        LAYOUT_16_16_16_16,
        LAYOUT_32,
        LAYOUT_32_32,
        LAYOUT_32_32_32,
        LAYOUT_32_32_32_32,
        LAYOUT_24,
        LAYOUT_8_24,
        LAYOUT_24_8,
        LAYOUT_4_4_24,
        LAYOUT_32_8_24,
        LAYOUT_DXT1,
        LAYOUT_DXT3,
        LAYOUT_DXT3A,
        LAYOUT_DXT5,
        LAYOUT_DXT5A,
        LAYOUT_DXN,
        LAYOUT_CTX1,
        LAYOUT_DXT3A_AS_1_1_1_1,
        LAYOUT_GRGB,
        LAYOUT_RGBG,
        NUM_COLOR_LAYOUTS,
    };
    static_assert(NUM_COLOR_LAYOUTS <= 64);

    ///
    /// color channel desc
    ///
    struct ChannelDesc {
        uint8_t shift; ///< bit offset in the pixel
        uint8_t bits;  ///< number of bits of the channel.
    };

    ///
    /// color layout descriptor
    ///
    struct LayoutDesc {
        uint8_t     blockWidth  : 4;    ///< width of color block
        uint8_t     blockHeight : 4;    ///< heiht of color block
        uint8_t     blockBytes;         ///< bytes of one color block
        uint8_t     pixelBits;          ///< bits per pixel
        uint8_t     numChannels;        ///< number of channels
        ChannelDesc channels[4];        ///< channel descriptors
    };

    ///
    /// Layout descriptors. Indexed by the layout enum value.
    ///
    inline static constexpr LayoutDesc LAYOUTS[] = {
        //BW  BH  BB   BPP   CH      CH0        CH1          CH2          CH3
        { 0 , 0 , 0  , 0   , 0 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_UNKNOWN,
        { 8 , 1 , 1  , 1   , 1 , { { 0 , 1  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_1,
        { 1 , 1 , 1  , 8   , 4 , { { 0 , 2  }, { 2  , 2  }, { 4  , 2  }, { 6  , 2  } } }, //LAYOUT_2_2_2_2,
        { 1 , 1 , 1  , 8   , 3 , { { 0 , 3  }, { 3  , 3  }, { 8  , 2  }, { 0  , 0  } } }, //LAYOUT_3_3_2,
        { 1 , 1 , 1  , 8   , 2 , { { 0 , 4  }, { 4  , 4  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_4_4,
        { 1 , 1 , 2  , 16  , 4 , { { 0 , 4  }, { 4  , 4  }, { 8  , 4  }, { 12 , 4  } } }, //LAYOUT_4_4_4_4,
        { 1 , 1 , 2  , 16  , 4 , { { 0 , 5  }, { 10 , 5  }, { 15 , 5  }, { 15 , 1  } } }, //LAYOUT_5_5_5_1,
        { 1 , 1 , 2  , 16  , 3 , { { 0 , 5  }, { 5  , 6  }, { 11 , 5  }, { 0  , 0  } } }, //LAYOUT_5_6_5,
        { 1 , 1 , 1  , 8   , 1 , { { 0 , 8  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_8,
        { 1 , 1 , 2  , 16  , 2 , { { 0 , 8  }, { 8  , 8  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_8_8,
        { 1 , 1 , 3  , 24  , 3 , { { 0 , 8  }, { 8  , 8  }, { 16 , 8  }, { 0  , 0  } } }, //LAYOUT_8_8_8,
        { 1 , 1 , 4  , 32  , 4 , { { 0 , 8  }, { 8  , 8  }, { 16 , 8  }, { 24 , 8  } } }, //LAYOUT_8_8_8_8,
        { 1 , 1 , 4  , 32  , 3 , { { 0 , 10 }, { 10 , 11 }, { 21 , 11 }, { 0  , 0  } } }, //LAYOUT_10_11_11,
        { 1 , 1 , 4  , 32  , 3 , { { 0 , 11 }, { 11 , 11 }, { 22 , 10 }, { 0  , 0  } } }, //LAYOUT_11_11_10,
        { 1 , 1 , 4  , 32  , 4 , { { 0 , 10 }, { 10 , 10 }, { 20 , 10 }, { 30 , 2  } } }, //LAYOUT_10_10_10_2,
        { 1 , 1 , 2  , 16  , 1 , { { 0 , 16 }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_16,
        { 1 , 1 , 4  , 32  , 2 , { { 0 , 16 }, { 16 , 16 }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_16_16,
        { 1 , 1 , 8  , 64  , 4 , { { 0 , 16 }, { 16 , 16 }, { 32 , 16 }, { 48 , 1  } } }, //LAYOUT_16_16_16_16,
        { 1 , 1 , 4  , 32  , 1 , { { 0 , 32 }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_32,
        { 1 , 1 , 8  , 64  , 2 , { { 0 , 32 }, { 32 , 32 }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_32_32,
        { 1 , 1 , 12 , 96  , 3 , { { 0 , 32 }, { 32 , 32 }, { 64 , 32 }, { 0  , 0  } } }, //LAYOUT_32_32_32,
        { 1 , 1 , 16 , 128 , 4 , { { 0 , 32 }, { 32 , 32 }, { 64 , 32 }, { 96 , 32 } } }, //LAYOUT_32_32_32_32,
        { 1 , 1 , 3  , 24  , 1 , { { 0 , 24 }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_24,
        { 1 , 1 , 4  , 32  , 2 , { { 0 , 8  }, { 8  , 24 }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_8_24,
        { 1 , 1 , 4  , 32  , 2 , { { 0 , 24 }, { 24 , 8  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_24_8,
        { 1 , 1 , 4  , 32  , 4 , { { 0 , 4  }, { 4  , 4  }, { 8  , 24 }, { 0  , 0  } } }, //LAYOUT_4_4_24,
        { 1 , 1 , 8  , 64  , 3 , { { 0 , 32 }, { 32 , 8  }, { 40 , 24 }, { 0  , 0  } } }, //LAYOUT_32_8_24,
        { 4 , 4 , 8  , 4   , 4 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXT1,
        { 4 , 4 , 16 , 8   , 4 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXT3,
        { 4 , 4 , 8  , 4   , 1 , { { 0 , 4  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXT3A,
        { 4 , 4 , 16 , 8   , 4 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXT5,
        { 4 , 4 , 8  , 4   , 1 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXT5A,
        { 4 , 4 , 16 , 8   , 2 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_DXN,
        { 4 , 4 , 8  , 4   , 2 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_CTX1,
        { 4 , 4 , 8  , 4   , 4 , { { 0 , 1  }, { 1  , 1  }, { 2  , 1  }, { 3  , 1  } } }, //LAYOUT_DXT3A_AS_1_1_1_1,
        { 2 , 1 , 4  , 16  , 4 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_GRGB,
        { 2 , 1 , 4  , 16  , 4 , { { 0 , 0  }, { 0  , 0  }, { 0  , 0  }, { 0  , 0  } } }, //LAYOUT_RGBG,
    };
    static_assert(std::size(LAYOUTS) == NUM_COLOR_LAYOUTS);
    static_assert(LAYOUTS[LAYOUT_UNKNOWN].blockWidth == 0);

    ///
    /// color sign
    ///
    enum Sign {
        SIGN_UNORM, ///< normalized unsigned integer
        SIGN_SNORM, ///< normalized signed integer
        SIGN_GNORM, ///< normalized gamma integer
        SIGN_BNORM, ///< normalized bias integer
        SIGN_UINT,  ///< unsigned integer
        SIGN_SINT,  ///< signed integer
        SIGN_GINT,  ///< gamma integer
        SIGN_BINT,  ///< bias integer
        SIGN_FLOAT, ///< float
    };

    ///
    /// color swizzle
    ///
    enum Swizzle {
        SWIZZLE_X = 0,
        SWIZZLE_Y = 1,
        SWIZZLE_Z = 2,
        SWIZZLE_W = 3,
        SWIZZLE_0 = 4,
        SWIZZLE_1 = 5,
        SWIZZLE_R = 0,
        SWIZZLE_G = 1,
        SWIZZLE_B = 2,
        SWIZZLE_A = 3,
    };

    ///
    /// color swizzle for 4 channels
    ///
    enum Swizzle4 {
        SWIZZLE_RGBA = (0<<0) | (1<<3) | (2<<6) | (3<<9),
        SWIZZLE_BGRA = (2<<0) | (1<<3) | (0<<6) | (3<<9),
        SWIZZLE_RGB1 = (0<<0) | (1<<3) | (2<<6) | (5<<9),
        SWIZZLE_BGR1 = (2<<0) | (1<<3) | (0<<6) | (5<<9),
        SWIZZLE_RRRG = (0<<0) | (0<<3) | (0<<6) | (1<<9),
        SWIZZLE_RG00 = (0<<0) | (1<<3) | (4<<6) | (4<<9),
        SWIZZLE_RG01 = (0<<0) | (1<<3) | (4<<6) | (5<<9),
        SWIZZLE_R000 = (0<<0) | (4<<3) | (4<<6) | (4<<9),
        SWIZZLE_R001 = (0<<0) | (4<<3) | (4<<6) | (5<<9),
        SWIZZLE_RRR1 = (0<<0) | (0<<3) | (0<<6) | (5<<9),
        SWIZZLE_111R = (5<<0) | (5<<3) | (5<<6) | (0<<9),
    };

    /// color format data members
    //@{
    struct {
#if RG_LITTLE_ENDIAN
        unsigned int layout   : 6;
        unsigned int sign012  : 4; ///< sign for R/G/B channels
        unsigned int sign3    : 4; ///< sign for alpha channel
        unsigned int swizzle0 : 3;
        unsigned int swizzle1 : 3;
        unsigned int swizzle2 : 3;
        unsigned int swizzle3 : 3;
        unsigned int reserved : 6; ///< reserved, must be zero
#else
        unsigned int reserved : 6; ///< reserved, must be zero
        unsigned int swizzle3 : 3;
        unsigned int swizzle2 : 3;
        unsigned int swizzle1 : 3;
        unsigned int swizzle0 : 3;
        unsigned int sign3    : 4; ///< sign for alpha channel
        unsigned int sign012  : 4; ///< sign for R/G/B channels
        unsigned int layout   : 6;
#endif
    };
    uint32_t         u32;   ///< color format as unsigned integer
    //@}

    ///
    /// construct from individual properties
    ///
    static constexpr ColorFormat make(Layout l, Sign si012, Sign si3, Swizzle sw0, Swizzle sw1, Swizzle sw2, Swizzle sw3) {
        return {
            (unsigned int)l,
            (unsigned int)si012,
            (unsigned int)si3,
            (unsigned int)sw0,
            (unsigned int)sw1,
            (unsigned int)sw2,
            (unsigned int)sw3,
            0,
        };
    }

    ///
    /// construct from individual properties
    ///
    static constexpr ColorFormat make(Layout l, Sign si012, Sign si3, Swizzle4 sw0123) {
        return make(
            l,
            si012,
            si3,
            (Swizzle)(((int)sw0123>>0)&3),
            (Swizzle)(((int)sw0123>>3)&3),
            (Swizzle)(((int)sw0123>>6)&3),
            (Swizzle)(((int)sw0123>>9)&3));
    }

    ///
    /// construct from individual properties
    ///
    static constexpr ColorFormat make(Layout l, Sign si0123, Swizzle4 sw0123) {
        return make(l, si0123, si0123, sw0123);
    }

    ///
    /// check for empty format
    ///
    constexpr bool empty() const { return 0 == layout; }

    ///
    /// self validity check
    ///
    constexpr bool valid() const {
        return
            0 < layout && layout < NUM_COLOR_LAYOUTS &&
            sign012  <= SIGN_FLOAT &&
            sign3    <= SIGN_FLOAT &&
            swizzle0 <= SWIZZLE_1 &&
            swizzle1 <= SWIZZLE_1 &&
            swizzle2 <= SWIZZLE_1 &&
            swizzle3 <= SWIZZLE_1 &&
            0 == reserved;
    }

    ///
    /// get layout descriptor
    ///
    constexpr const LayoutDesc & layoutDesc() const { return LAYOUTS[layout]; }

    ///
    /// Get bits-per-pixel
    ///
    constexpr uint8_t bitsPerPixel() const { return LAYOUTS[layout].pixelBits; }

    ///
    /// Get bytes-per-pixel-block
    ///
    constexpr uint8_t bytesPerBlock() const { return LAYOUTS[layout].blockBytes; }

    ///
    /// equality check
    ///
    constexpr bool operator==( const ColorFormat & c ) const { return u32 == c.u32; }

    ///
    /// equality check
    ///
    constexpr bool operator!=( const ColorFormat & c ) const { return u32 != c.u32; }

    ///
    /// less operator
    ///
    constexpr bool operator<( const ColorFormat & c ) const { return u32 < c.u32; }

    /// alias of commonly used color formats
    ///
    static constexpr ColorFormat UNKNOWN() { return {}; }

    // 8 bits
    static constexpr ColorFormat R_8_UNORM()                   { return make(LAYOUT_8, SIGN_UNORM, SWIZZLE_R001); }
    static constexpr ColorFormat R_8_SNORM()                   { return make(LAYOUT_8, SIGN_SNORM, SWIZZLE_R001); }
    static constexpr ColorFormat L_8_UNORM()                   { return make(LAYOUT_8, SIGN_UNORM, SWIZZLE_RRR1); }
    static constexpr ColorFormat A_8_UNORM()                   { return make(LAYOUT_8, SIGN_UNORM, SWIZZLE_111R); }
    static constexpr ColorFormat RGB_3_3_2_UNORM()             { return make(LAYOUT_3_3_2, SIGN_UNORM, SWIZZLE_RGB1); }

    // 16 bits
    static constexpr ColorFormat BGRA_4_4_4_4_UNORM()          { return make(LAYOUT_4_4_4_4, SIGN_UNORM, SWIZZLE_BGRA); }
    static constexpr ColorFormat BGRX_4_4_4_4_UNORM()          { return make(LAYOUT_4_4_4_4, SIGN_UNORM, SWIZZLE_BGR1); }
    static constexpr ColorFormat BGR_5_6_5_UNORM()             { return make(LAYOUT_5_6_5, SIGN_UNORM, SWIZZLE_BGR1); }
    static constexpr ColorFormat BGRA_5_5_5_1_UNORM()          { return make(LAYOUT_5_5_5_1, SIGN_UNORM, SWIZZLE_BGRA); }
    static constexpr ColorFormat BGRX_5_5_5_1_UNORM()          { return make(LAYOUT_5_5_5_1, SIGN_UNORM, SWIZZLE_BGR1); }

    static constexpr ColorFormat RG_8_8_UNORM()                { return make(LAYOUT_8_8, SIGN_UNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_8_8_SNORM()                { return make(LAYOUT_8_8, SIGN_SNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat LA_8_8_UNORM()                { return make(LAYOUT_8_8, SIGN_UNORM, SWIZZLE_RRRG); }

    static constexpr ColorFormat R_16_UNORM()                  { return make(LAYOUT_16, SIGN_UNORM, SWIZZLE_R001); }
    static constexpr ColorFormat R_16_SNORM()                  { return make(LAYOUT_16, SIGN_SNORM, SWIZZLE_R001); }
    static constexpr ColorFormat R_16_UINT()                   { return make(LAYOUT_16, SIGN_UINT , SWIZZLE_R001); }
    static constexpr ColorFormat R_16_SINT()                   { return make(LAYOUT_16, SIGN_SINT , SWIZZLE_R001); }
    static constexpr ColorFormat R_16_FLOAT()                  { return make(LAYOUT_16, SIGN_FLOAT, SWIZZLE_R001); }

    static constexpr ColorFormat L_16_UNORM()                  { return make(LAYOUT_16, SIGN_UNORM, SWIZZLE_RRR1); }

    // 24 bits

    static constexpr ColorFormat RGB_8_8_8_UNORM()             { return make(LAYOUT_8_8_8, SIGN_UNORM, SWIZZLE_RGB1); }
    static constexpr ColorFormat RGB_8_8_8_SNORM()             { return make(LAYOUT_8_8_8, SIGN_SNORM, SWIZZLE_RGB1); }
    static constexpr ColorFormat BGR_8_8_8_UNORM()             { return make(LAYOUT_8_8_8, SIGN_UNORM, SWIZZLE_BGR1); }
    static constexpr ColorFormat BGR_8_8_8_SNORM()             { return make(LAYOUT_8_8_8, SIGN_SNORM, SWIZZLE_BGR1); }
    static constexpr ColorFormat R_24_FLOAT()                  { return make(LAYOUT_24, SIGN_FLOAT, SWIZZLE_R001); }

    // 32 bits
    static constexpr ColorFormat RGBA_8_8_8_8_UNORM()          { return make(LAYOUT_8_8_8_8, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_8_8_8_8_UNORM_SRGB()     { return make(LAYOUT_8_8_8_8, SIGN_UNORM, SIGN_GNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_8_8_8_8_SNORM()          { return make(LAYOUT_8_8_8_8, SIGN_SNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA8()                       { return RGBA_8_8_8_8_UNORM(); }
    static constexpr ColorFormat UBYTE4N()                     { return RGBA_8_8_8_8_UNORM(); }

    static constexpr ColorFormat RGBX_8_8_8_8_UNORM()          { return make(LAYOUT_8_8_8_8, SIGN_UNORM, SWIZZLE_RGB1); }

    static constexpr ColorFormat BGRA_8_8_8_8_UNORM()          { return make(LAYOUT_8_8_8_8, SIGN_UNORM, SWIZZLE_BGRA); }
    static constexpr ColorFormat BGRA8()                       { return BGRA_8_8_8_8_UNORM(); }

    static constexpr ColorFormat BGRX_8_8_8_8_UNORM()          { return make(LAYOUT_8_8_8_8, SIGN_UNORM, SWIZZLE_BGR1); }

    static constexpr ColorFormat RGBA_10_10_10_2_UNORM()       { return make(LAYOUT_10_10_10_2, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_10_10_10_2_UINT()        { return make(LAYOUT_10_10_10_2, SIGN_UINT , SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_10_10_10_SNORM_2_UNORM() { return make(LAYOUT_10_10_10_2, SIGN_SNORM, SIGN_UNORM, SWIZZLE_RGBA); }

    static constexpr ColorFormat RG_16_16_UNORM()              { return make(LAYOUT_16_16, SIGN_UNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_16_16_SNORM()              { return make(LAYOUT_16_16, SIGN_SNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_16_16_UINT()               { return make(LAYOUT_16_16, SIGN_UINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_16_16_SINT()               { return make(LAYOUT_16_16, SIGN_SINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_16_16_FLOAT()              { return make(LAYOUT_16_16, SIGN_FLOAT, SWIZZLE_RG01); }
    static constexpr ColorFormat USHORT2N()                    { return RG_16_16_UNORM(); }
    static constexpr ColorFormat SHORT2N()                     { return RG_16_16_SNORM(); }
    static constexpr ColorFormat USHORT2()                     { return RG_16_16_UINT(); }
    static constexpr ColorFormat SHORT2()                      { return RG_16_16_SINT(); }
    static constexpr ColorFormat HALF2()                       { return RG_16_16_FLOAT(); }

    static constexpr ColorFormat LA_16_16_UNORM()              { return make(LAYOUT_16_16, SIGN_UNORM, SWIZZLE_RRRG); }

    static constexpr ColorFormat R_32_UNORM()                  { return make(LAYOUT_32, SIGN_UNORM, SWIZZLE_R001); }
    static constexpr ColorFormat R_32_SNORM()                  { return make(LAYOUT_32, SIGN_SNORM, SWIZZLE_R001); }
    static constexpr ColorFormat R_32_UINT()                   { return make(LAYOUT_32, SIGN_UINT, SWIZZLE_R001); }
    static constexpr ColorFormat R_32_SINT()                   { return make(LAYOUT_32, SIGN_SINT, SWIZZLE_R001); }
    static constexpr ColorFormat R_32_FLOAT()                  { return make(LAYOUT_32, SIGN_FLOAT, SWIZZLE_R001); }
    static constexpr ColorFormat UINT1N()                      { return R_32_UNORM(); }
    static constexpr ColorFormat INT1N()                       { return R_32_SNORM(); }
    static constexpr ColorFormat UINT1()                       { return R_32_UINT(); }
    static constexpr ColorFormat INT1()                        { return R_32_SINT(); }
    static constexpr ColorFormat FLOAT1()                      { return R_32_FLOAT(); }

    static constexpr ColorFormat GR_8_UINT_24_UNORM()          { return make(LAYOUT_8_24, SIGN_UINT, SIGN_UNORM, SWIZZLE_G, SWIZZLE_R, SWIZZLE_0, SWIZZLE_1); }
    static constexpr ColorFormat GX_8_24_UNORM()               { return make(LAYOUT_8_24, SIGN_UINT, SIGN_UNORM, SWIZZLE_G, SWIZZLE_0, SWIZZLE_0, SWIZZLE_1); }

    static constexpr ColorFormat RG_24_UNORM_8_UINT()          { return make(LAYOUT_24_8, SIGN_UNORM, SIGN_UINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RX_24_8_UNORM()               { return make(LAYOUT_24_8, SIGN_UNORM, SIGN_UINT, SWIZZLE_RG01); }
    static constexpr ColorFormat XG_24_8_UINT()                { return make(LAYOUT_24_8, SIGN_UNORM, SIGN_UINT, SWIZZLE_0, SWIZZLE_G, SWIZZLE_0, SWIZZLE_1); }

    static constexpr ColorFormat RG_24_FLOAT_8_UINT()          { return make(LAYOUT_24_8, SIGN_FLOAT, SIGN_UINT, SWIZZLE_RG01); }

    static constexpr ColorFormat GRGB_UNORM()                  { return make(LAYOUT_GRGB, SIGN_UNORM, SWIZZLE_RGB1); }
    static constexpr ColorFormat RGBG_UNORM()                  { return make(LAYOUT_RGBG, SIGN_UNORM, SWIZZLE_RGB1); }

    // 64 bits
    static constexpr ColorFormat RGBA_16_16_16_16_UNORM()      { return make(LAYOUT_16_16_16_16, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_16_16_16_16_SNORM()      { return make(LAYOUT_16_16_16_16, SIGN_SNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_16_16_16_16_UINT()       { return make(LAYOUT_16_16_16_16, SIGN_UINT , SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_16_16_16_16_SINT()       { return make(LAYOUT_16_16_16_16, SIGN_SINT , SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_16_16_16_16_FLOAT()      { return make(LAYOUT_16_16_16_16, SIGN_FLOAT, SWIZZLE_RGBA); }
    static constexpr ColorFormat USHORT4N()                    { return RGBA_16_16_16_16_UNORM(); }
    static constexpr ColorFormat SHORT4N()                     { return RGBA_16_16_16_16_SNORM(); }
    static constexpr ColorFormat USHORT4()                     { return RGBA_16_16_16_16_UINT(); }
    static constexpr ColorFormat SHORT4()                      { return RGBA_16_16_16_16_SINT(); }
    static constexpr ColorFormat HALF4()                       { return RGBA_16_16_16_16_FLOAT(); }

    static constexpr ColorFormat RGBX_16_16_16_16_UNORM()      { return make(LAYOUT_16_16_16_16, SIGN_UNORM, SWIZZLE_RGB1); }

    static constexpr ColorFormat RG_32_32_UNORM()              { return make(LAYOUT_32_32, SIGN_UNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_32_32_SNORM()              { return make(LAYOUT_32_32, SIGN_SNORM, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_32_32_UINT()               { return make(LAYOUT_32_32, SIGN_UINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_32_32_SINT()               { return make(LAYOUT_32_32, SIGN_SINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RG_32_32_FLOAT()              { return make(LAYOUT_32_32, SIGN_FLOAT, SWIZZLE_RG01); }
    static constexpr ColorFormat FLOAT2()                      { return RG_32_32_FLOAT(); }

    static constexpr ColorFormat RGX_32_FLOAT_8_UINT_24()      { return make(LAYOUT_32_8_24, SIGN_FLOAT, SIGN_UINT, SWIZZLE_RG01); }
    static constexpr ColorFormat RXX_32_8_24_FLOAT()           { return make(LAYOUT_32_8_24, SIGN_FLOAT, SIGN_UINT, SWIZZLE_R001); }
    static constexpr ColorFormat XGX_32_8_24_UINT()            { return make(LAYOUT_32_8_24, SIGN_UINT, SIGN_UINT, SWIZZLE_0, SWIZZLE_G, SWIZZLE_0, SWIZZLE_1); }

    // 96 bits
    static constexpr ColorFormat RGB_32_32_32_UNORM()          { return make(LAYOUT_32_32_32, SIGN_UNORM, SWIZZLE_RGB1); }
    static constexpr ColorFormat RGB_32_32_32_SNORM()          { return make(LAYOUT_32_32_32, SIGN_SNORM, SWIZZLE_RGB1); }
    static constexpr ColorFormat RGB_32_32_32_UINT()           { return make(LAYOUT_32_32_32, SIGN_UINT , SWIZZLE_RGB1); }
    static constexpr ColorFormat RGB_32_32_32_SINT()           { return make(LAYOUT_32_32_32, SIGN_SINT , SWIZZLE_RGB1); }
    static constexpr ColorFormat RGB_32_32_32_FLOAT()          { return make(LAYOUT_32_32_32, SIGN_FLOAT, SWIZZLE_RGB1); }
    static constexpr ColorFormat FLOAT3()                      { return RGB_32_32_32_FLOAT(); }

    // 128 bits
    static constexpr ColorFormat RGBA_32_32_32_32_UNORM()      { return make(LAYOUT_32_32_32_32, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_32_32_32_32_SNORM()      { return make(LAYOUT_32_32_32_32, SIGN_SNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_32_32_32_32_UINT()       { return make(LAYOUT_32_32_32_32, SIGN_UINT , SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_32_32_32_32_SINT()       { return make(LAYOUT_32_32_32_32, SIGN_SINT , SWIZZLE_RGBA); }
    static constexpr ColorFormat RGBA_32_32_32_32_FLOAT()      { return make(LAYOUT_32_32_32_32, SIGN_FLOAT, SWIZZLE_RGBA); }
    static constexpr ColorFormat UINT4N()                      { return RGBA_32_32_32_32_UNORM(); }
    static constexpr ColorFormat SINT4N()                      { return RGBA_32_32_32_32_SNORM(); }
    static constexpr ColorFormat UINT4()                       { return RGBA_32_32_32_32_UINT(); }
    static constexpr ColorFormat SINT4()                       { return RGBA_32_32_32_32_SINT(); }
    static constexpr ColorFormat FLOAT4()                      { return RGBA_32_32_32_32_FLOAT(); }

    // compressed
    static constexpr ColorFormat DXT1_UNORM()                  { return make(LAYOUT_DXT1, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT1_UNORM_SRGB()             { return make(LAYOUT_DXT1, SIGN_GNORM, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT3_UNORM()                  { return make(LAYOUT_DXT3, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT3_UNORM_SRGB()             { return make(LAYOUT_DXT3, SIGN_GNORM, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT5_UNORM()                  { return make(LAYOUT_DXT5, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT5_UNORM_SRGB()             { return make(LAYOUT_DXT5, SIGN_GNORM, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT5A_UNORM()                 { return make(LAYOUT_DXT5A, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXT5A_SNORM()                 { return make(LAYOUT_DXT5A, SIGN_SNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXN_UNORM()                   { return make(LAYOUT_DXN, SIGN_UNORM, SWIZZLE_RGBA); }
    static constexpr ColorFormat DXN_SNORM()                   { return make(LAYOUT_DXN, SIGN_SNORM, SWIZZLE_RGBA); }
};
static_assert(4 == sizeof(ColorFormat));
static_assert(ColorFormat::UNKNOWN().layoutDesc().blockWidth == 0);
static_assert(!ColorFormat::UNKNOWN().valid());
static_assert(ColorFormat::UNKNOWN().empty());
static_assert(ColorFormat::RGBA8().valid());
static_assert(!ColorFormat::RGBA8().empty());

///
/// compose RGBA8 color constant
///
template<typename T>
inline constexpr uint32_t makeRGBA8(T r, T g, T b, T a) {
    return (
        ( ((uint32_t)(r)&0xFF) <<  0 ) |
        ( ((uint32_t)(g)&0xFF) <<  8 ) |
        ( ((uint32_t)(b)&0xFF) << 16 ) |
        ( ((uint32_t)(a)&0xFF) << 24 ) );
}

///
/// compose BGRA8 color constant
///
template<typename T>
inline constexpr uint32_t makeBGRA8(T r, T g, T b, T a) {
    return (
        ( ((uint32_t)(b)&0xFF) <<  0 ) |
        ( ((uint32_t)(g)&0xFF) <<  8 ) |
        ( ((uint32_t)(r)&0xFF) << 16 ) |
        ( ((uint32_t)(a)&0xFF) << 24 ) );
}

/// This represents a single 1D/2D/3D image in an more complex image structure.
// Note: avoid using size_t in this structure. So the size of the structure will never change, regardless of compile platform.
struct ImagePlaneDesc {
    
    /// pixel format
    ColorFormat format = ColorFormat::UNKNOWN();

    /// Plane width in pixels
    uint32_t width = 0;
    
    /// Plane height in pixels
    uint32_t height = 0;
    
    /// Plane depth in pixels
    uint32_t depth = 0;

    /// bits (not BYTES) from one pixel to next. Minimal valid value is pixel size.
    uint32_t step = 0;

    /// Bytes from one row to next. Minimal valid value is (width * step) and aligned to alignment.
    uint32_t pitch = 0;

    /// Bytes from one slice to next. Minimal valid value is (pitch * height)
    uint32_t slice = 0;

    /// Bytes of the whole plane. Minimal valid value is (slice * depth)
    uint32_t size = 0;

    /// Bytes between first pixel of the plane to the first pixel of the whole image.
    uint32_t offset = 0;

    /// row alignment
    uint32_t rowAlignment = 0;

    /// returns offset of particular pixel within the plane
    size_t pixel(size_t x, size_t y, size_t z = 0) const {
        RG_ASSERT(x < width && y < height && z < depth);
        size_t r = z * slice + y * pitch + x * step / 8;
        RG_ASSERT(r < size);
        return r + offset;
    }

    /// check if this is a valid image plane descriptor. Note that valid descriptor is never empty.
    bool valid() const;

    /// check if this is an empty descriptor. Note that empty descriptor is never valid.
    bool empty() const { return ColorFormat::UNKNOWN() == format; }

    /// Create a new image plane descriptor
    static ImagePlaneDesc make(ColorFormat format, size_t width, size_t height = 1, size_t depth = 1, size_t step = 0, size_t pitch = 0, size_t slice = 0, size_t alignment = 4);
};

///
/// Represent a complex image with optional mipmap chain
///
struct ImageDesc {

    // ****************************
    /// \name member data
    // ****************************

    //@{

    std::vector<ImagePlaneDesc> planes; ///< length of array = layers * mips;
    uint32_t layers = 0; ///< number of layers
    uint32_t levels = 0; ///< number of mipmap levels
    uint32_t size = 0;   ///< total size in bytes;

    //@}

    // ****************************
    /// \name ctor/dtor/copy/move
    // ****************************

    //@{

    ImageDesc() = default;

    ///
    /// Construct image descriptor from basemap and layer/level count. If anything goes wrong, constructs an empty image descriptor.
    ///
    /// \param basemap the base image
    /// \param layers number of layers. must be positive integer
    /// \param levels number of mipmap levels. set to 0 to automatically build full mipmap chain.
    /// 
    ImageDesc(const ImagePlaneDesc & basemap, size_t layers = 1, size_t levels = 1) { reset(basemap, (uint32_t)layers, (uint32_t)levels); }

    // can copy. can move.
    RG_DEFAULT_COPY(ImageDesc);
    RG_DEFAULT_MOVE(ImageDesc);

    //@}

    // ****************************
    /// \name public methods
    // ****************************

    //@{

    ///
    /// check if the image is empty or not
    ///
    bool empty() const { return planes.empty(); }

    ///
    /// make sure this is a meaningfull image descriptor
    ///
    bool valid() const;

    /// methods to return properties of the specific plane.
    //@{
    const ImagePlaneDesc & plane (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)]; }
    ImagePlaneDesc       & plane (size_t layer = 0, size_t level = 0)       { return planes[index(layer, level)]; }
    ColorFormat            format(size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].format; }
    uint32_t               width (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].width; }
    uint32_t               height(size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].height; }
    uint32_t               depth (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].depth; }
    uint32_t               step  (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].step; }
    uint32_t               pitch (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].pitch; }
    uint32_t               slice (size_t layer = 0, size_t level = 0) const { return planes[index(layer, level)].slice; }
    //@}

    ///
    /// returns offset of particular pixel
    ///
    size_t pixel(size_t layer, size_t level, size_t x = 0, size_t y = 0, size_t z = 0) const {
        const auto & d = planes[index(layer, level)];
        auto r = d.pixel(x, y, z);
        RG_ASSERT(r < size);
        return r;
    }

    // void vertFlipInpace(void * pixels, size_t sizeInBytes);

private:

    /// return plane index
    size_t index(size_t layer, size_t level) const {
        RG_ASSERT(layer < layers);
        RG_ASSERT(level < levels);
        return (level * layers) + layer;
    }

    /// reset the descriptor
    void reset(const ImagePlaneDesc & basemap, uint32_t layers, uint32_t levels);
};

///
/// A basic image class
///
class RawImage {

public:

    /// \name ctor/dtor/copy/move
    //@{
    RawImage() = default;
    RawImage(ImageDesc&& desc, const void * initialContent = nullptr, size_t initialContentSizeInbytes = 0);
    RG_NO_COPY(RawImage);
    RG_DEFAULT_MOVE(RawImage);
    //@}

    /// \name basic property query
    //@{

    /// return descriptor of the whole image
    const ImageDesc& desc() const { return mDesc; }

    /// return descriptor of a image plane
    const ImagePlaneDesc & desc(size_t layer, size_t level) const { return mDesc.plane(layer, level); }

    /// return pointer to pixel buffer.
    const uint8_t* data() const { return mPixels.get(); }

    /// return pointer to pixel buffer.
    uint8_t* data() { return mPixels.get(); }

    /// return size of the whole image in bytes.
    uint32_t size() const { return mDesc.size; }

    /// check if the image is empty or not.
    bool empty() const { return mDesc.empty(); }

    //@}

    /// \name query properties of the specific plane.
    //@{
    ColorFormat format(size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).format; }
    uint32_t    width (size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).width; }
    uint32_t    height(size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).height; }
    uint32_t    depth (size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).depth; }
    uint32_t    step  (size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).step; }
    uint32_t    pitch (size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).pitch; }
    uint32_t    slice (size_t layer = 0, size_t level = 0) const { return mDesc.plane(layer, level).slice; }
    //@}

    /// \name methods to return pointer to particular pixel
    //@{
    const uint8_t* pixel(size_t layer, size_t level, size_t x = 0, size_t y = 0, size_t z = 0) const { return mPixels.get() + mDesc.pixel(layer, level, x, y, z); }
    uint8_t*       pixel(size_t layer, size_t level, size_t x = 0, size_t y = 0, size_t z = 0)       { return mPixels.get() + mDesc.pixel(layer, level, x, y, z); }
    //@}

    /// \name load & save
    //@{
    static RawImage load(std::istream &);
    static RawImage load(const char * filename) {
        std::ifstream f(filename, std::ios::binary);
        if (!f.good()) return {};
        return load(f);
    }
    // TODO: save to std::ostream
    //@}

private:

    struct FreePixelArray {
        void operator()(uint8_t * p) {
            afree(p);
        }
    };
    std::unique_ptr<uint8_t, FreePixelArray> mPixels;
    ImageDesc mDesc;
};

} // namespace rg