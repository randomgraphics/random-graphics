#pragma once
#include "base.h"
#include <memory>
#include <type_traits>
#include <array>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <variant>
#include <algorithm>
#include <sstream>
#include <cstring>

#if !RG_BUILD_STATIC
# define GLAD_GLAPI_EXPORT
#endif
#if RG_MSWIN
# include <glad/glad_wgl.h>
#elif RG_UNIX_LIKE
# define RG_HAS_EGL 1
# include <glad/glad.h>
#endif

// Check OpenGL error. Call "actionOnFailure" when GL error is detected.
#define RG_GLCHK(func, actionOnFailure)                                     \
    if( true ) {                                                            \
        func;                                                               \
        if (!glGetError) {                                                  \
            RG_LOGE("gl not initialized properly...");                      \
            actionOnFailure;                                                \
        } else {                                                            \
            auto err = glGetError();                                        \
            if (GL_NO_ERROR != err) {                                       \
                RG_LOGE("function %s failed. (error=0x%x)", #func, err );   \
                actionOnFailure;                                            \
            }                                                               \
        }                                                                   \
    } else void(0)

namespace rg {
namespace gl {

// -----------------------------------------------------------------------------
/// initialize all GL/EGL extentions for utlities in header.
bool initExtensions();

// -----------------------------------------------------------------------------
/// enable OpenGL debug runtime
void enableDebugRuntime();

// -----------------------------------------------------------------------------
//
std::string printGLInfo(bool printExtensionList);

// -----------------------------------------------------------------------------
/// The 'optionalFilename' parameter is optional and is only used when printing
/// shader compilation error.
GLuint loadShaderFromString(const char* source, size_t length, GLenum shaderType, const char* optionalFilename = nullptr);

// -----------------------------------------------------------------------------
/// the program name parameter is optional and is only used to print link error.
GLuint linkProgram(const std::vector<GLuint>& shaders, const char* optionalProgramName = nullptr);

// -----------------------------------------------------------------------------
//
inline void clearScreen(
    float r = .0f, float g = .0f, float b = .0f, float a = 1.0f,
    float d = 1.0f,
    uint32_t s = 0,
    GLbitfield flags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)
{
    if (flags | GL_COLOR_BUFFER_BIT) glClearColor(r, g, b, a);
    if (flags | GL_DEPTH_BUFFER_BIT) glClearDepth(d);
    if (flags | GL_STENCIL_BUFFER_BIT) glClearStencil(s);
    glClear(flags);
}

// -----------------------------------------------------------------------------
//
inline GLint getInt(GLenum name)
{
    GLint value;
    glGetIntegerv(name, &value);
    return value;
}

inline GLint getInt(GLenum name, GLint i)
{
    GLint value;
    glGetIntegeri_v(name, i, &value);
    return value;
}

// -----------------------------------------------------------------------------
//
#if RG_HAS_EGL
const char * eglError2String(khronos_int32_t err);
#endif

// -----------------------------------------------------------------------------
//
struct InternalFormatDesc {
    GLenum      internalFormat;
    ColorFormat colorFormat;
};
static inline constexpr InternalFormatDesc INTERNAL_FORMATS[] = {
    { GL_R8              , ColorFormat::R_8_UNORM(), },
    { GL_R8_SNORM        , ColorFormat::R_8_SNORM(), },
    { GL_R16             , ColorFormat::R_16_UNORM(), },
    { GL_R16_SNORM       , ColorFormat::R_16_SNORM(), },
    { GL_RG8             , ColorFormat::RG_8_8_UNORM(), },
    { GL_RG8_SNORM       , ColorFormat::RG_8_8_SNORM(), },
    { GL_RG16            , ColorFormat::RG_16_16_UNORM(), },
    { GL_RG16_SNORM      , ColorFormat::RG_16_16_SNORM(), },
    { GL_R3_G3_B2        , ColorFormat::RGB_3_3_2_UNORM(), },
    { GL_RGB4            , ColorFormat::make(ColorFormat::LAYOUT_4_4_4_4, ColorFormat::SIGN_UNORM, ColorFormat::SWIZZLE_RGB1), },
    { GL_RGB5            , ColorFormat::make(ColorFormat::LAYOUT_5_5_5_1, ColorFormat::SIGN_UNORM, ColorFormat::SWIZZLE_RGB1), },
    { GL_RGB8            , ColorFormat::RGB_8_8_8_UNORM(), },
    { GL_RGB8_SNORM      , ColorFormat::RGB_8_8_8_SNORM(), },
    { GL_RGB10           , ColorFormat::make(ColorFormat::LAYOUT_10_10_10_2, ColorFormat::SIGN_UNORM, ColorFormat::SWIZZLE_RGB1), },
    //{ GL_RGB12           , ColorFormat::(), },
    { GL_RGB16_SNORM     , ColorFormat::make(ColorFormat::LAYOUT_16_16_16_16, ColorFormat::SIGN_SNORM, ColorFormat::SWIZZLE_RGB1), },
    //{ GL_RGBA2           , ColorFormat::(), },
    //{ GL_RGBA4           , ColorFormat::(), },
    //{ GL_RGB5_A1         , ColorFormat::(), },
    { GL_RGBA8           , ColorFormat::RGBA_8_8_8_8_UNORM(), },
    { GL_RGBA8_SNORM     , ColorFormat::RGBA_8_8_8_8_SNORM(), },
    // { GL_RGB10_A2        , ColorFormat::(), },
    // { GL_RGB10_A2UI      , ColorFormat::(), },
    // { GL_RGBA12          , ColorFormat::(), },
    // { GL_RGBA16          , ColorFormat::(), },
    // { GL_SRGB8           , ColorFormat::(), },
    // { GL_SRGB8_ALPHA8    , ColorFormat::(), },
    // { GL_R16F            , ColorFormat::(), },
    // { GL_RG16F           , ColorFormat::(), },
    // { GL_RGB16F          , ColorFormat::(), },
    // { GL_RGBA16F         , ColorFormat::(), },
    // { GL_R32F            , ColorFormat::(), },
    // { GL_RG32F           , ColorFormat::(), },
    // { GL_RGB32F          , ColorFormat::(), },
    // { GL_RGBA32F         , ColorFormat::(), },
    // { GL_R11F_G11F_B10F  , ColorFormat::(), },
    // { GL_RGB9_E5         , ColorFormat::(), },
    // { GL_R8I             , ColorFormat::(), },
    // { GL_R8UI            , ColorFormat::(), },
    // { GL_R16I            , ColorFormat::(), },
    // { GL_R16UI           , ColorFormat::(), },
    // { GL_R32I            , ColorFormat::(), },
    // { GL_R32UI           , ColorFormat::(), },
    // { GL_RG8I            , ColorFormat::(), },
    // { GL_RG8UI           , ColorFormat::(), },
    // { GL_RG16I           , ColorFormat::(), },
    // { GL_RG16UI          , ColorFormat::(), },
    // { GL_RG32I           , ColorFormat::(), },
    // { GL_RG32UI          , ColorFormat::(), },
    // { GL_RGB8I           , ColorFormat::(), },
    // { GL_RGB8UI          , ColorFormat::(), },
    // { GL_RGB16I          , ColorFormat::(), },
    // { GL_RGB16UI         , ColorFormat::(), },
    // { GL_RGB32I          , ColorFormat::(), },
    // { GL_RGB32UI         , ColorFormat::(), },
    // { GL_RGBA8I          , ColorFormat::(), },
    // { GL_RGBA8UI         , ColorFormat::(), },
    // { GL_RGBA16I         , ColorFormat::(), },
    // { GL_RGBA16UI        , ColorFormat::(), },
    // { GL_RGBA32I         , ColorFormat::(), },
    // { GL_RGBA32UI        , ColorFormat::(), },
    { GL_NONE            , ColorFormat::UNKNOWN() },
};
inline constexpr const InternalFormatDesc & getInternalFormatDesc(GLenum f) {
    for(size_t i = 0; i < std::size(INTERNAL_FORMATS); ++i) {
        if (f == INTERNAL_FORMATS[i].internalFormat) {
            return INTERNAL_FORMATS[i]; // found it.
        }
    }
    return INTERNAL_FORMATS[std::size(INTERNAL_FORMATS) - 1];
}

// -----------------------------------------------------------------------------
//
template<GLenum TARGET>
struct QueryObject {
    enum Status_ {
        EMPTY,   // the query object is not created yet.
        IDLE,    // the query object is idle and ready to use.
        RUNNING, // in between begin() and end()
        PENDING, // query is issued. but result is yet to returned.
    };

    GLuint qo = 0;
    Status_ status = EMPTY;

    QueryObject() = default;
    ~QueryObject() { cleanup(); }

    RG_NO_COPY(QueryObject);

    // can move
    QueryObject(QueryObject&& that) { qo = that.qo; status = that.status; that.qo = 0; that.status = EMPTY; }
    QueryObject & operator=(QueryObject&& that) {
        if (this != &that) {
            qo = that.qo;
            status = that.status;
            that.qo = 0;
            that.status = EMPTY;
        }
    }

    bool empty() const { return EMPTY == status; }
    bool idle() const { return IDLE == status; }
    bool running() const { return RUNNING == status; }
    bool pending() const { return PENDING == status; }

    void cleanup() {
#ifdef __ANDROID__
        if (qo) glDeleteQueriesEXT(1, &qo), qo = 0;
#else
        if (qo) glDeleteQueries(1, &qo), qo = 0;
#endif
        status = IDLE;
    }

    void allocate() {
        cleanup();
#ifdef __ANDROID__
        glGenQueriesEXT(1, &qo);
#else
        glGenQueries(1, &qo);
#endif
        status = IDLE;
    }

    void begin() {
        if (IDLE == status) {
#ifdef __ANDROID__
            glBeginQueryEXT(TARGET, qo);
#else
            glBeginQuery(TARGET, qo);
#endif
            status = RUNNING;
        }
    }

    void end() {
        if (RUNNING == status) {
#ifdef __ANDROID__
            glEndQueryEXT(TARGET);
#else
            glEndQuery(TARGET);
#endif
            status = PENDING;
        }
    }

    void mark() {
        if (IDLE == status) {
#ifdef __ANDROID__
            glQueryCounterEXT(qo, TARGET);
#else
            glQueryCounter(qo, TARGET);
#endif
            status = PENDING;
        }
    }

    bool getResult(uint64_t & result) {
        if (PENDING != status) return false;
        GLint available;
#ifdef __ANDROID__
        glGetQueryObjectivEXT(qo, GL_QUERY_RESULT_AVAILABLE, &available);
#else
        glGetQueryObjectiv(qo, GL_QUERY_RESULT_AVAILABLE, &available);
#endif
        if (!available) return false;

#ifdef __ANDROID__
            glGetQueryObjectui64vEXT(qo, GL_QUERY_RESULT, &result);
#else
        glGetQueryObjectui64v(qo, GL_QUERY_RESULT, &result);
#endif
        status = IDLE;
        return true;
    }

    // returns 0, if the query is still pending.
    template<uint64_t DEFAULT_VALUE = 0>
    uint64_t getResult() const
    {
        uint64_t ret = DEFAULT_VALUE;
        return getResult(ret) ? ret : DEFAULT_VALUE;
    }
};

// -----------------------------------------------------------------------------
/// Helper class to manage GL buffer object.
template<GLenum TARGET, size_t MIN_GPU_BUFFER_LENGH = 0>
struct BufferObject
{
    GLuint bo = 0;
    size_t length = 0; // buffer length in bytes.
    GLenum mapped_target = 0;

    RG_NO_COPY(BufferObject);
    RG_NO_MOVE(BufferObject);

    BufferObject() {}

    ~BufferObject() { cleanup(); }

    static GLenum GetTarget() { return TARGET; }

    template<typename T, GLenum T2 = TARGET>
    void allocate(size_t count, const T * ptr, GLenum usage = GL_STATIC_DRAW)
    {
        cleanup();
        glGenBuffers(1, &bo);
        // Note: ARM Mali GPU doesn't work well with zero sized buffers. So
        // we create buffer that is large enough to hold at least one element.
        length = std::max(count, MIN_GPU_BUFFER_LENGH) * sizeof(T);
        glBindBuffer(T2, bo);
        glBufferData(T2, length, ptr, usage);
        glBindBuffer(T2, 0); // unbind
    }

    void cleanup()
    {
        if (bo) glDeleteBuffers(1, &bo), bo = 0;
        length = 0;
    }

    bool empty() const { return 0 == bo; }

    template<typename T, GLenum T2 = TARGET>
    void update(const T * ptr, size_t offset = 0, size_t count = 1)
    {
        glBindBuffer(T2, bo);
        glBufferSubData(T2, offset * sizeof(T), count * sizeof(T), ptr);
    }

    template<GLenum T2 = TARGET>
    void bind() const {
        glBindBuffer(T2, bo);
    }

    template<GLenum T2 = TARGET>
    static void unbind() {
        glBindBuffer(T2, 0);
    }

    template<GLenum T2 = TARGET>
    void bindBase(GLuint base) const
    {
        glBindBufferBase(T2, base, bo);
    }

    template<typename T, GLenum T2 = TARGET>
    void getData(T * ptr, size_t offset, size_t count)
    {
        glBindBuffer(T2, bo);
        void* mapped = nullptr;
        mapped = glMapBufferRange(T2, offset * sizeof(T), count * sizeof(T), GL_MAP_READ_BIT);
        if (mapped) {
            memcpy(ptr, mapped, count * sizeof(T));
            glUnmapBuffer(T2);
        }
    }

    template<GLenum T2 = TARGET>
    void* map(size_t offset, size_t count) {
        bind();
        void* ptr = nullptr;
        ptr = glMapBufferRange(T2, offset, count, GL_MAP_READ_BIT);
        RG_ASSERT(ptr);
        mapped_target = TARGET;
        return ptr;
    }

    template<GLenum T2 = TARGET>
    void* map() {
        return map<T2>(0, length);
    }

    void unmap() {
        if (mapped_target) {
            bind();
            glUnmapBuffer(mapped_target);
            mapped_target = 0;
        }
    }

    operator GLuint() const { return bo; }
};

// -----------------------------------------------------------------------------
//
template<typename T, GLenum TARGET, size_t MIN_GPU_BUFFER_LENGTH = 0>
struct TypedBufferObject
{
    std::vector<T>                                  c; // CPU data
    gl::BufferObject<TARGET, MIN_GPU_BUFFER_LENGTH> g; // GPU data

    void allocateGpuBuffer()
    {
        g.allocate(c.size(), c.data());
    }

    void syncGpuBuffer()
    {
        g.update(c.data(), 0, c.size());
    }

    // Synchornosly copy buffer content from GPU to CPU.
    // Note that this call is EXTREMELY expensive, since it stalls both CPU and GPU.
    void syncToCpu()
    {
        glFinish();
        g.getData(c.data(), 0, c.size());
    }

    void cleanup()
    {
        c.clear();
        g.cleanup();
    }
};

// -----------------------------------------------------------------------------
//
template<typename T, GLenum TARGET1, GLenum TARGET2, size_t MIN_GPU_BUFFER_LENGTH = 0>
struct TypedBufferObject2
{
    std::vector<T>                                   c; // CPU data
    gl::BufferObject<TARGET1, MIN_GPU_BUFFER_LENGTH> g1; // GPU data
    gl::BufferObject<TARGET2, MIN_GPU_BUFFER_LENGTH> g2; // GPU data

    void allocateGpuBuffer()
    {
        g1.allocate(c.size(), c.data());
        g2.allocate(c.size(), c.data());
    }

    void syncGpuBuffer()
    {
        g1.update(c.data(), 0, c.size());
        g2.update(c.data(), 0, c.size());
    }

    void cleanup()
    {
        c.clear();
        g1.cleanup();
        g2.cleanup();
    }

    template<GLenum TT>
    void bind() const
    {
        if constexpr (TT == TARGET1) {
            g1.bind();
        }
        else {
            static_assert(TT == TARGET2);
            g2.bind();
        }
    }

    template<GLenum TT>
    void bindBase(GLuint base) const
    {
        if constexpr (TT == TARGET1) {
            g1.bindBase(base);
        }
        else {
            static_assert(TT == TARGET2);
            g2.bindBase(base);
        }
    }
};

// -----------------------------------------------------------------------------
//
class VertexArrayObject
{
    GLuint _va = 0;
public:
    ~VertexArrayObject() { cleanup(); }

    void allocate()
    {
        cleanup();
        glGenVertexArrays(1, &_va);
    }

    void cleanup()
    {
        if (_va) glDeleteVertexArrays(1, &_va), _va = 0;
    }

    void bind() const
    {
        glBindVertexArray(_va);
    }

    void unbind() const
    {
        glBindVertexArray(0);
    }

    operator GLuint () const { return _va; }
};

// -----------------------------------------------------------------------------
//
struct AutoShader
{
    GLuint shader;

    AutoShader(GLuint s = 0) : shader(s) {}
    ~AutoShader() { cleanup(); }

    void cleanup() { if (shader) glDeleteShader(shader), shader = 0; }

    RG_NO_COPY(AutoShader);

    // can move
    AutoShader(AutoShader&& rhs) : shader(rhs.shader) { rhs.shader = 0; }
    AutoShader & operator=(AutoShader&& rhs) { if (this != &rhs) { cleanup(); shader = rhs.shader; rhs.shader = 0; } return *this; }

    operator GLuint() const { return shader; }
};

class SamplerObject {
    GLuint _id = 0;
public:

    SamplerObject() {}
    ~SamplerObject() { cleanup(); }

    RG_NO_COPY(SamplerObject);

    // can move
    SamplerObject(SamplerObject && that) { _id = that._id; that._id = 0; }
    SamplerObject & operator=(SamplerObject && that) {
        if (this != &that) {
            cleanup();
            _id = that._id; that._id = 0;
        }
        return *this;
    }

    operator GLuint() const { return _id; }

    void allocate() { cleanup(); glGenSamplers(1, &_id); }
    void cleanup() { if (_id) glDeleteSamplers(1, &_id), _id = 0; }
    void bind(size_t unit) const { RG_ASSERT(glIsSampler(_id)); glBindSampler((GLuint)unit, _id); }
};

inline void bindTexture(GLenum target, uint32_t stage, GLuint texture)
{
    glActiveTexture(GL_TEXTURE0 + stage);
    glBindTexture(target, texture);
}

class TextureObject
{
public:

    // no copy
    TextureObject(const TextureObject&) = delete;
    TextureObject& operator=(const TextureObject&) = delete;

    // can move
    TextureObject(TextureObject&& rhs) noexcept
            : _desc(rhs._desc)
    {
        rhs._desc.id = 0;
    }
    TextureObject& operator=(TextureObject&& rhs) noexcept
    {
        if (this != &rhs) {
            cleanup();
            _desc = rhs._desc;
            rhs._desc.id = 0;
        }
        return *this;
    }

    // default constructor
    TextureObject() { cleanup(); }

    ~TextureObject() { cleanup(); }

    struct TextureDesc
    {
        GLuint id = 0;   // all other fields are undefined, if id is 0.
        GLenum target;
        GLenum format; // GL internal format.
        uint32_t width;
        uint32_t height;
        uint32_t depth; // this is number of layers for 2D array texture and is always 6 for cube texture.
        uint32_t mips;
    };

    const TextureDesc& getDesc() const { return _desc; }

    GLenum target() const { return _desc.target; }
    GLenum id() const { return _desc.id; }

    bool empty() const { return 0 == _desc.id; }

    bool is2D() const { return GL_TEXTURE_2D == _desc.target; }

    bool isArray() const { return GL_TEXTURE_2D_ARRAY == _desc.target; }

    void attach(GLenum target, GLuint id);

    void attach(const TextureObject & that) { attach(that._desc.target, that._desc.id); }

    void allocate2D(GLenum f, size_t w, size_t h, size_t m = 1);

    void allocate2DArray(GLenum f, size_t w, size_t h, size_t l, size_t m = 1);

    void allocateCube(GLenum f, size_t w, size_t m = 1);

    void setPixels(
            size_t level,
            size_t x, size_t y, size_t w, size_t h,
            size_t rowPitchInBytes, // set to 0, if pixels are tightly packed.
            const void * pixels) const;

    // Set to rowPitchInBytes 0, if pixels are tightly packed.
    void setPixels(size_t layer, size_t level, size_t x, size_t y, size_t w, size_t h, size_t rowPitchInBytes, const void* pixels) const;

    RawImage getBaseLevelPixels() const;

    void cleanup()
    {
        if (_owned && _desc.id) {
            glDeleteTextures(1, &_desc.id);
        }
        _desc.id = 0;
        _desc.target = GL_NONE;
        _desc.format = GL_NONE;
        _desc.width = 0;
        _desc.height = 0;
        _desc.depth = 0;
        _desc.mips = 0;
    }

    void bind(size_t stage) const
    {
        glActiveTexture(GL_TEXTURE0 + (int)stage);
        glBindTexture(_desc.target, _desc.id);
    }

    void unbind() const { glBindTexture(_desc.target, 0); }

    operator GLuint() const { return _desc.id; }

private:
    TextureDesc _desc;
    bool _owned = false;
    void applyDefaultParameters();
};

// SSBO for in-shader debug output. It is currently working on Windows only. Running it on Android crashes the driver.
struct DebugSSBO
{
    std::vector<float> buffer;
    mutable std::vector<float> printed;
    int* counter = nullptr;
    gl::BufferObject<GL_SHADER_STORAGE_BUFFER> g;

    ~DebugSSBO()
    {
        cleanup();
    }

    void allocate(size_t n)
    {
        cleanup();
        buffer.resize(n + 1);
        printed.resize(buffer.size());
        counter = (int*)& buffer[0];
        g.allocate(buffer.size(), buffer.data(), GL_STATIC_READ);
    }

    void bind(int slot = 15) const
    {
        if (g) glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, g);
    }

    void cleanup()
    {
        buffer.clear();
        printed.clear();
        counter = nullptr;
        g.cleanup();
    }

    void clearCounter()
    {
        if (!counter) return;
        *counter = 0;
        g.update(counter, 0, 1);
    }

    void pullDataFromGPU()
    {
        if (buffer.empty()) return;
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        g.getData(buffer.data(), 0, buffer.size());
    }

    std::string printLastResult() const;
};

struct FullScreenQuad
{
    // vertex array
    GLuint                            va = 0;
    gl::BufferObject<GL_ARRAY_BUFFER> vb;

    FullScreenQuad() {}

    ~FullScreenQuad()
    {
        cleanup();
    }

    void allocate();

    void cleanup();

    void draw() const
    {
        RG_ASSERT(va);
        glBindVertexArray(va);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
};

class SimpleGlslProgram
{
    GLuint _program = 0;

    //struct Uniform
    //{
    //    GLint location;
    //};
    //std::vector<Uniform> _uniforms;

public:

    // optional program name (for debug log)
    std::string name;

#ifdef _DEBUG
    std::string vsSource, psSource, csSource;
#endif

    RG_NO_COPY(SimpleGlslProgram);
    RG_NO_MOVE(SimpleGlslProgram);

    SimpleGlslProgram(const char * optionalProgramName = nullptr)
    {
        if (optionalProgramName) name = optionalProgramName;
    }

    ~SimpleGlslProgram()
    {
        cleanup();
    }

    bool loadVsPs(const char* vscode, const char* pscode)
    {
#ifdef _DEBUG
        if (vscode) vsSource = vscode;
        if (pscode) psSource = pscode;
#endif
        cleanup();
        AutoShader vs = loadShaderFromString(vscode, 0, GL_VERTEX_SHADER, name.c_str());
        AutoShader ps = loadShaderFromString(pscode, 0, GL_FRAGMENT_SHADER, name.c_str());
        if ((vscode && !vs) || (pscode && !ps)) return false;
        _program = linkProgram({ vs, ps }, name.c_str());
        return _program != 0;
    }

    bool loadCs(const char * code)
    {
#ifdef _DEBUG
        if (code) csSource = code;
#endif
        cleanup();
        AutoShader cs = loadShaderFromString(code, 0, GL_COMPUTE_SHADER, name.c_str());
        if (!cs) return false;
        _program = linkProgram({ cs }, name.c_str());

        return _program != 0;
    }

    //void InitUniform(const char* name)
    //{
    //    auto loc = glGetUniformLocation(_program, name);
    //    if (-1 != loc) {
    //        _uniforms.push_back({ loc });
    //    }
    //}

    //template<typename T>
    //void UpdateUniform(size_t index, const T& value) const
    //{
    //    gl::UpdateUniformValue(_uniforms[index].location, value);
    //}

    void use() const
    {
        glUseProgram(_program);
    }

    void cleanup()
    {
        //_uniforms.clear();
        if (_program) glDeleteProgram(_program), _program = 0;
    }

    GLint getUniformLocation(const char* name_) const
    {
        return glGetUniformLocation(_program, name_);
    }

    GLint getUniformBinding(const char* name_) const
    {
        auto loc = glGetUniformLocation(_program, name_);
        if (-1 == loc) return -1;
        GLint binding;
        glGetUniformiv(_program, loc, &binding);
        return binding;
    }

    operator GLuint() const
    {
        return _program;
    }
};

class SimpleUniform {
public:

    using Value = std::variant<
        int,
        uint32_t,
        float,
        std::array<float,2>,
        std::array<float,3>,
        std::array<float,4>,
        std::array<int,2>,
        std::array<int,3>,
        std::array<int,4>,
        std::array<uint32_t,2>,
        std::array<uint32_t,3>,
        std::array<uint32_t,4>,
        // glm::mat3x3,
        // glm::mat4x4,
        std::vector<float>
        >;

    Value value;

    SimpleUniform(std::string name) : _name(name) {}

    template<typename T>
    SimpleUniform(std::string name, const T & v) : value(v), _name(name) {}

    bool init(GLuint program)
    {
        if (program > 0) {
            _location = glGetUniformLocation(program, _name.c_str());
        }
        else {
            _location = -1;
        }
        return _location > -1;
    }

    void apply() const {
        if (_location < 0) return;
        std::visit([&](auto && v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>)                           glUniform1i (_location, v);
            else if constexpr (std::is_same_v<T, uint32_t>)                 glUniform1ui(_location, v);
            else if constexpr (std::is_same_v<T, float>)                    glUniform1f (_location, v);
            else if constexpr (std::is_same_v<T, std::array<float,2>>)      glUniform2fv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<float,3>>)      glUniform3fv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<float,4>>)      glUniform4fv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<int,2>>)        glUniform2iv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<int,3>>)        glUniform3iv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<int,4>>)        glUniform4iv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<uint32_t,2>>)   glUniform2uiv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<uint32_t,3>>)   glUniform3uiv(_location, 1, v.data());
            else if constexpr (std::is_same_v<T, std::array<uint32_t,4>>)   glUniform4uiv(_location, 1, v.data());
            // else if constexpr (std::is_same_v<T, glm::mat3x3>)  glUniformMatrix3fv(_location, 1, false, glm::value_ptr(v));
            // else if constexpr (std::is_same_v<T, glm::mat4x4>)  glUniformMatrix4fv(_location, 1, false, glm::value_ptr(v));
            else if constexpr (std::is_same_v<T, const std::vector<float>>) glUniform1fv(_location, (GLsizei)v.size(), v.data());
        }, value);
    }

private:
    const std::string _name;
    GLint _location = -1;
};

class SimpleTextureCopy {
    struct CopyProgram {
        SimpleGlslProgram program;
        GLint tex0Binding = -1;
    };
    std::unordered_map<GLuint, CopyProgram> _programs; // key is texture target.
    FullScreenQuad _quad;
    GLuint _sampler = 0;
    GLuint _fbo = 0;
public:
    RG_NO_COPY(SimpleTextureCopy);
    RG_NO_MOVE(SimpleTextureCopy);
    SimpleTextureCopy() {}
    ~SimpleTextureCopy() { cleanup(); }
    bool init();
    void cleanup();
    struct TextureSubResource {
        GLenum   target;
        GLuint   id;
        uint32_t level;
        uint32_t z; // layer index for 2d array texture.
        //TODO: uint32_t x, y, w, h;
    };
    void copy(const TextureSubResource & src, const TextureSubResource & dst);
    void copy(const TextureObject & src, uint32_t srcLevel, uint32_t srcZ,
                const TextureObject & dst, uint32_t dstLevel, uint32_t dstZ) {
        auto & s = src.getDesc();
        auto & d = dst.getDesc();
        copy(
                {s.target, s.id, srcLevel, srcZ},
                {d.target, d.id, dstLevel, dstZ}
        );
    }
};

// -----------------------------------------------------------------------------
// For asynchronous timer (not time stamp) queries
struct GpuTimeElapsedQuery {

    std::string name;

    explicit GpuTimeElapsedQuery(std::string n = std::string("")) : name(n) { _q.allocate(); }

    ~GpuTimeElapsedQuery() {}

    // returns duration in nanoseconds
    GLuint64 duration() const { return _result; }

    void start() { _q.begin(); }

    void stop();

    // Print stats to string
    std::string print() const;

    friend inline std::ostream & operator<<(std::ostream & s, const GpuTimeElapsedQuery & t) {
        s << t.print();
        return s;
    }

private:
    QueryObject<GL_TIME_ELAPSED> _q;
    uint64_t _result = 0;
};

// -----------------------------------------------------------------------------
// GPU time stamp query
class GpuTimestamps {
public:
    explicit GpuTimestamps(std::string name = std::string("")) : _name(name) {}

    void start() {
        RG_ASSERT(!_started);
        if (!_started) {
            _started = true;
            _count = 0;
            mark("start time");
        }
    }

    void stop() {
        RG_ASSERT(_started);
        if (_started) {
            mark("end time");
            _started = false;
        }
    }

    void mark(std::string name) {
        RG_ASSERT(_started);
        if (!_started) return;

        if (_count == _marks.size()) {
            _marks.emplace_back();
            _marks.back().name = name;
        }

        RG_ASSERT(_count < _marks.size());
        _marks[_count++].mark();
    }

    // Print stats of timestamps to string
    std::string print(const char * ident = nullptr) const;

private:
    struct Query {
        std::string name;
        QueryObject<GL_TIMESTAMP> q;
        uint64_t result = 0;
        Query() { q.allocate(); }
        RG_NO_COPY(Query);
        RG_DEFAULT_MOVE(Query);
        void mark() {
            if (q.idle()) {
                q.mark();
            } else {
                q.getResult(result);
            }
        }
    };

    std::string _name;
    std::vector<Query> _marks;
    size_t _count = 0;
    bool _started = false;
};

// -----------------------------------------------------------------------------
/// Helper class to initialize OpenGL offscreen pbuffer render context
class PBufferRenderContext {
    class Impl;
    Impl * _impl;
public:
    struct CreationParameters {
        uint32_t width = 1, height = 1;
        
        // create new contex that share resouce with current context on the calling thread, if any.
        bool shared = true;

        // set to true to create a debug runtime.
        bool debug = RG_BUILD_DEBUG;
    };

    PBufferRenderContext(const CreationParameters &);

    ~PBufferRenderContext();

    RG_NO_COPY(PBufferRenderContext);

    // can move
    PBufferRenderContext(PBufferRenderContext && that) : _impl(that._impl) { that._impl = nullptr; }
    PBufferRenderContext & operator=(PBufferRenderContext && that);

    bool good() const; // check if the context is in good/working condition.
    void makeCurrent();
    void swapBuffers();
};

// Store and restore OpenGL context
class RenderContextStack {
    class Impl;
    Impl * _impl;
public:

    RG_NO_COPY(RenderContextStack);
    RG_NO_MOVE(RenderContextStack);

    RenderContextStack();

    // the destructor will automatically pop out any previously pushed context.
    ~RenderContextStack();

    // push current context to the top of the stack
    void push();

    // make the top context current, but not popping it out of the stack.
    void apply();

    // apply then the top context current, then pop it out of the stack.
    void pop();
};

} // namespace gl
} // namespace rg