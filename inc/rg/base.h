#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <mutex>

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

// Disable some known "harmless" warnings. So we can use /W4 throughout our code base.
#ifdef _MSC_VER
#pragma warning(disable : 4201) // nameless struct/union
#pragma warning(disable : 4458) // declaration hides class member
#endif

/// Log macros
//@{
#define RG_LOG(tag, severity, ...) do { \
    using namespace rg::log::macros; \
    if (rg::log::Controller::getInstance(tag)->enabled(severity)) { \
        rg::log::Helper((int)severity, __FILE__, __LINE__, __FUNCTION__)(__VA_ARGS__); \
    } } while(0)
#define RG_LOGF(...) RG_LOG(, F, __VA_ARGS__)
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
#define RG_DLOGF(...) RG_DLOG(, F, __VA_ARGS__)
#define RG_DLOGE(...) RG_DLOG(, E, __VA_ARGS__)
#define RG_DLOGW(...) RG_DLOG(, W, __VA_ARGS__)
#define RG_DLOGI(...) RG_DLOG(, I, __VA_ARGS__)
#define RG_DLOGV(...) RG_DLOG(, V, __VA_ARGS__)
#define RG_DLOGB(...) RG_DLOG(, B, __VA_ARGS__)
//@}

/// call this when encountering unrecoverable error.
#define RG_RIP(...)                              \
    do {                                         \
        RG_LOGF(__VA_ARGS__);                    \
        rg::rip(__FILE__, __LINE__, "RIP", "");  \
    } while (0)

/// Check for required conditions. call RIP when the condition is not met.
#define RG_CHK(x)                                                \
    if (!(x)) {                                                  \
        rg::rip(__FILE__, __LINE__, "condition #x didn't met");  \
    } else                                                       \
        void(0)

/// Runtime assertion
//@{
#if RG_BUILD_DEBUG
#define RG_ASSERT(x, message)                      \
    if (!(x)) {                                    \
        rg::rip(__FILE__, __LINE__, #x, message);  \
    } else                                         \
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

/// enable default move semantics
#define RG_DEFAULT_MOVE(X)            \
    X(X&&)     = default;             \
    X& operator=(X&&) = default

/// export/import type decl
//@{
#ifdef _WIN32
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

namespace log {

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

    Controller(const char * name) : _tag(name) {}
    
    ~Controller() = default;

public:

    static inline Controller * getInstance() { return g.root; }
    static inline Controller * getInstance(Controller * c) { return c; }
    static Controller * getInstance(const char * tag);

    bool enabled(int severity) const {
        return _enabled && severity <= g.severity;
    }
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

inline std::stringstream s(const char * str) {
    std::stringstream ss;
    ss << str;
    return ss;
}

}; // namespace macro

class Helper {

    struct Info {
        int          severity;
        const char * file;
        int          line;
        const char * func;
    };

    Info _info;

    const char * formatlog(const char *, ...);

    void post(const char *);

public:

    template<class... Args>
    Helper(Args&&... args) : _info{args...} {
    }

    template<class... Args>
    void operator()(const char * format, Args&&... args) {
        post(formatlog(format, std::forward<Args>(args)...));
    }

    void operator()(const std::basic_ostream<char> & s) {
        post(formatlog(reinterpret_cast<const std::stringstream&>(s).str().c_str()));
    }

    template<class... Args>
    void operator()(Controller * c, const char * format, Args&&... args) {
        if (Controller::getInstance(c)->enabled(_info.severity)) {
            operator()(format, std::forward<Args>(args)...);
        }
    }

    void operator()(Controller * c, const std::basic_ostream<char> & s) {
        if (Controller::getInstance(c)->enabled(_info.severity)) {
            operator()(s);
        }
    }
};

} // namespace log

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
            l,
            si012,
            si3,
            sw0,
            sw1,
            sw2,
            sw3,
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
    static constexpr ColorFormat L_8_UNORM()                   { return make(LAYOUT_8, SIGN_UNORM, SWIZZLE_RRR1); }
    static constexpr ColorFormat A_8_UNORM()                   { return make(LAYOUT_8, SIGN_UNORM, SWIZZLE_111R); }

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
    static constexpr ColorFormat BGR_8_8_8_UNORM()             { return make(LAYOUT_8_8_8, SIGN_UNORM, SWIZZLE_BGR1); }
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

} // namespace rg