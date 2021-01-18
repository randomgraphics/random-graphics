#ifndef RG_INTERNAL
#define RG_INTERNAL
#endif
#include "rg/opengl.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <atomic>
#include <stack>
#include <iomanip>
#include <cmath>

#ifdef __ANDROID__
#include <dlfcn.h>
#endif

using namespace rg;
using namespace rg::gl;

// -----------------------------------------------------------------------------
//
void rg::gl::enableDebugRuntime()
{
    struct OGLDebugOutput
    {
        static const char* source2String(GLenum source)
        {
            switch (source)
            {
                case GL_DEBUG_SOURCE_API_ARB: return "GL API";
                case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: return "Window System";
                case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: return "Shader Compiler";
                case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: return "Third Party";
                case GL_DEBUG_SOURCE_APPLICATION_ARB: return "Application";
                case GL_DEBUG_SOURCE_OTHER_ARB: return "Other";
                default: return "INVALID_SOURCE";
            }
        }

        static const char* type2String(GLenum type)
        {
            switch (type)
            {
                case GL_DEBUG_TYPE_ERROR_ARB: return "Error";
                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: return "Deprecation";
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: return "Undefined Behavior";
                case GL_DEBUG_TYPE_PORTABILITY_ARB: return "Portability";
                case GL_DEBUG_TYPE_PERFORMANCE_ARB: return "Performance";
                case GL_DEBUG_TYPE_OTHER_ARB: return "Other";
                default: return "INVALID_TYPE";
            }
        }

        static const char* severity2String(GLenum severity)
        {
            switch (severity)
            {
                case GL_DEBUG_SEVERITY_HIGH_ARB: return "High";
                case GL_DEBUG_SEVERITY_MEDIUM_ARB: return "Medium";
                case GL_DEBUG_SEVERITY_LOW_ARB: return "Low";
                default: return "INVALID_SEVERITY";
            }
        }

        static void GLAPIENTRY messageCallback(GLenum source,
                                               GLenum type,
                                               GLuint id,
                                               GLenum severity,
                                               GLsizei, // length,
                                               const GLchar* message,
                                               const void*) {
            // Determine log level
            bool error_ = false;
            bool warning = false;
            bool info = false;
            switch (type)
            {
                case GL_DEBUG_TYPE_ERROR_ARB:
                case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: error_ = true; break;

                case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
                case GL_DEBUG_TYPE_PORTABILITY:
                    switch (severity)
                    {
                        case GL_DEBUG_SEVERITY_HIGH_ARB:
                        case GL_DEBUG_SEVERITY_MEDIUM_ARB:  warning = true; break;
                        case GL_DEBUG_SEVERITY_LOW_ARB: break;
                        default: error_ = true; break;
                    }
                    break;

                case GL_DEBUG_TYPE_PERFORMANCE_ARB:
                    switch (severity)
                    {
                        case GL_DEBUG_SEVERITY_HIGH_ARB: warning = true; break;
                        case GL_DEBUG_SEVERITY_MEDIUM_ARB: // shader recompiliation, buffer data read back.
                        case GL_DEBUG_SEVERITY_LOW_ARB: break; // verbose: performance warnings from redundant state changes
                        default: error_ = true; break;
                    }
                    break;


                case GL_DEBUG_TYPE_OTHER_ARB:
                    switch (severity)
                    {
                        case GL_DEBUG_SEVERITY_HIGH_ARB: error_ = true; break;
                        case GL_DEBUG_SEVERITY_MEDIUM_ARB: warning = true; break;
                        case GL_DEBUG_SEVERITY_LOW_ARB:
                        case GL_DEBUG_SEVERITY_NOTIFICATION: break; // verbose
                        default: error_ = true; break;
                    }
                    break;

                default: error_ = true; break;
            }

            std::string s = formatstr(
                    "(id=[%d] source=[%s] type=[%s] severity=[%s]): %s\n%s",
                    id, source2String(source), type2String(type), severity2String(severity), message, backtrace().c_str());
            if (error_)
                RG_LOGE("[GL ERROR] %s", s.c_str());
            else if (warning)
                RG_LOGW("[GL WARNING] %s", s.c_str());
            else if (info)
                RG_LOGI("[GL INFO] %s", s.c_str());
        }
    };

    if (GLAD_GL_ARB_debug_output) {
        RG_GLCHK(glDebugMessageCallbackARB(&OGLDebugOutput::messageCallback, nullptr));
        RG_GLCHK(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB));
        RG_GLCHK(glDebugMessageControlARB(
            GL_DONT_CARE, // source
            GL_DONT_CARE, // type
            GL_DONT_CARE, // severity
            0, // count
            nullptr, // ids
            GL_TRUE));
    }
}

// -----------------------------------------------------------------------------
//
std::string rg::gl::printGLInfo(bool printExtensionList)
{
    std::stringstream info;

    // vendor and version info.
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* version = (const char*)glGetString(GL_VERSION);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* glsl = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLint maxsls = -1, maxslsFast = -1;
    if (GLAD_GL_EXT_shader_pixel_local_storage) {
        glGetIntegerv(GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_SIZE_EXT, &maxsls);
        glGetIntegerv(GL_MAX_SHADER_PIXEL_LOCAL_STORAGE_FAST_SIZE_EXT, &maxslsFast);
    }
    info <<
        "\n\n"
        "===================================================\n"
        "        OpenGL Implementation Information\n"
        "---------------------------------------------------\n"
        "               OpenGL vendor : " << vendor << "\n"
        "              OpenGL version : " << version << "\n"
        "             OpenGL renderer : " << renderer << "\n"
        "                GLSL version : " << glsl << "\n"
        "       Max FS uniform blocks : " << getInt(GL_MAX_FRAGMENT_UNIFORM_BLOCKS) << "\n"
        "      Max uniform block size : " << getInt(GL_MAX_UNIFORM_BLOCK_SIZE) * 4 << " bytes\n"
        "           Max texture units : " << getInt(GL_MAX_TEXTURE_IMAGE_UNITS) << "\n"
        "    Max array texture layers : " << getInt(GL_MAX_ARRAY_TEXTURE_LAYERS) << "\n"
        "       Max color attachments : " << getInt(GL_MAX_COLOR_ATTACHMENTS) << "\n"
        "           Max SSBO binding  : " << getInt(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS) << "\n"
        "         Max SSBO FS blocks  : " << getInt(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS) << "\n"
        "        Max SSBO block size  : " << getInt(GL_MAX_SHADER_STORAGE_BLOCK_SIZE) * 4 << " bytes\n"
        "       Max CS WorkGroup size : " << getInt(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0) << ","
        << getInt(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1) << ","
        << getInt(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2) << "\n"
        "      Max CS WorkGroup count : " << getInt(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0) << ","
        << getInt(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1) << ","
        << getInt(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2) << "\n"
        "    Max shader local storage : total=" << maxsls << ", fast=" << maxslsFast << "\n"
        ;

    if (printExtensionList) {
        info << "---------------------------------------------------\n";
        std::vector<std::string> extensions;
        GLint n = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &n);
        for (int i = 0; i < n; ++i) {
            extensions.push_back((const char*)glGetStringi(GL_EXTENSIONS, i));
        }
        std::sort(extensions.begin(), extensions.end());
        for (int i = 0; i < n; ++i) {
            info << "    " << extensions[i] << "\n";
        }
    }

    info <<
         "===================================================\n";

    // done
    return info.str();
}

// -----------------------------------------------------------------------------
//
#if RG_LINUX
const char * gl::eglError2String(EGLint err) {
    switch(err) {
        case EGL_SUCCESS:
            return "The last function succeeded without error.";
        case EGL_NOT_INITIALIZED:
            return "EGL is not initialized, or could not be initialized, for the specified EGL display connection.";
        case EGL_BAD_ACCESS:
            return "EGL cannot access a requested resource (for example a context is bound in another thread).";
        case EGL_BAD_ALLOC:
            return "EGL failed to allocate resources for the requested operation.";
        case EGL_BAD_ATTRIBUTE:
            return "An unrecognized attribute or attribute value was passed in the attribute list.";
        case EGL_BAD_CONTEXT:
            return "An EGLContext argument does not name a valid EGL rendering context.";
        case EGL_BAD_CONFIG:
            return "An EGLConfig argument does not name a valid EGL frame buffer configuration.";
        case EGL_BAD_CURRENT_SURFACE:
            return "The current surface of the calling thread is a window, pixel buffer or pixmap that is no longer valid.";
        case EGL_BAD_DISPLAY:
            return "An EGLDisplay argument does not name a valid EGL display connection.";
        case EGL_BAD_SURFACE:
            return "An EGLSurface argument does not name a valid surface (window, pixel buffer or pixmap) configured for GL rendering.";
        case EGL_BAD_MATCH:
            return "Arguments are inconsistent (for example, a valid context requires buffers not supplied by a valid surface).";
        case EGL_BAD_PARAMETER:
            return "One or more argument values are invalid.";
        case EGL_BAD_NATIVE_PIXMAP:
            return "A NativePixmapType argument does not refer to a valid native pixmap.";
        case EGL_BAD_NATIVE_WINDOW:
            return "A NativeWindowType argument does not refer to a valid native window.";
        case EGL_CONTEXT_LOST:
            return "A power management event has occurred. The application must destroy all contexts and reinitialise OpenGL ES state and objects to continue rendering.";
        default:
            return "unknown error";
    }
}
#endif

// -----------------------------------------------------------------------------
//
void rg::gl::TextureObject::attach(GLenum target, GLuint id) {
    cleanup();
    _owned = false;
    _desc.target = target;
    _desc.id = id;
    bind(0);

    glGetTexLevelParameteriv(_desc.target, 0, GL_TEXTURE_WIDTH, (GLint*)&_desc.width);
    RG_ASSERT(_desc.width);

    glGetTexLevelParameteriv(target, 0, GL_TEXTURE_HEIGHT, (GLint*)&_desc.height);
    RG_ASSERT(_desc.height);

    // determine depth
    switch(target) {
        case GL_TEXTURE_2D_ARRAY:
        case GL_TEXTURE_3D:
            glGetTexLevelParameteriv(target, 0, GL_TEXTURE_DEPTH, (GLint*)&_desc.depth);
            RG_ASSERT(_desc.depth);
            break;
        case GL_TEXTURE_CUBE_MAP:
            _desc.depth = 6;
            break;
        default:
            _desc.depth = 1;
            break;
    }

    GLint maxLevel;
    glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &maxLevel);
    _desc.mips = (uint32_t)maxLevel + 1;

    glGetTexLevelParameteriv(target, 0, GL_TEXTURE_INTERNAL_FORMAT, (GLint*)&_desc.format);

    unbind();
}

// -----------------------------------------------------------------------------
//
void rg::gl::TextureObject::allocate2D(GLenum f, size_t w, size_t h, size_t m)
{
    cleanup();
    _desc.target = GL_TEXTURE_2D;
    _desc.format = f;
    _desc.width = (uint32_t)w;
    _desc.height = (uint32_t)h;
    _desc.depth = (uint32_t)1;
    _desc.mips = (uint32_t)m;
    _owned = true;
    RG_GLCHK(glGenTextures(1, &_desc.id));
    RG_GLCHK(glBindTexture(_desc.target, _desc.id));
    applyDefaultParameters();
    RG_GLCHK(glTexStorage2D(_desc.target, (GLsizei)_desc.mips, f, (GLsizei)_desc.width, (GLsizei)_desc.height));
    RG_GLCHK(glBindTexture(_desc.target, 0));
}

// -----------------------------------------------------------------------------
//
void rg::gl::TextureObject::allocate2DArray(GLenum f, size_t w, size_t h, size_t l, size_t m)
{
    cleanup();
    _desc.target = GL_TEXTURE_2D_ARRAY;
    _desc.format = f;
    _desc.width = (uint32_t)w;
    _desc.height = (uint32_t)h;
    _desc.depth = (uint32_t)l;
    _desc.mips = (uint32_t)m;
    _owned = true;
    RG_GLCHK(glGenTextures(1, &_desc.id));
    RG_GLCHK(glBindTexture(_desc.target, _desc.id));
    applyDefaultParameters();
    RG_GLCHK(glTexStorage3D(_desc.target, (GLsizei)_desc.mips, f, (GLsizei)_desc.width, (GLsizei)_desc.height, (GLsizei)_desc.depth));
    RG_GLCHK(glBindTexture(_desc.target, 0));
}

// -----------------------------------------------------------------------------
//
void rg::gl::TextureObject::allocateCube(GLenum f, size_t w, size_t m)
{
    cleanup();
    _desc.target = GL_TEXTURE_CUBE_MAP;
    _desc.format = f;
    _desc.width = (uint32_t)w;
    _desc.height = (uint32_t)w;
    _desc.depth = 6;
    _desc.mips = (uint32_t)m;
    _owned = true;
    RG_GLCHK(glGenTextures(1, &_desc.id));
    RG_GLCHK(glBindTexture(GL_TEXTURE_CUBE_MAP, _desc.id));
    applyDefaultParameters();
    RG_GLCHK(glTexStorage2D(GL_TEXTURE_CUBE_MAP, (GLsizei)_desc.mips, f, (GLsizei)_desc.width, (GLsizei)_desc.width));
    RG_GLCHK(glBindTexture(_desc.target, 0));
}

void rg::gl::TextureObject::applyDefaultParameters() {
    RG_ASSERT(_desc.width > 0);
    RG_ASSERT(_desc.height > 0);
    RG_ASSERT(_desc.depth > 0);
    RG_ASSERT(_desc.mips > 0);
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_BASE_LEVEL, 0));
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_MAX_LEVEL, _desc.mips - 1));
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_MIN_FILTER, _desc.mips > 1 ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST));
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    RG_GLCHK(glTexParameteri(_desc.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
}

// // -----------------------------------------------------------------------------
// //
// void rg::gl::TextureObject::setPixels(
//         size_t level,
//         size_t x, size_t y,
//         size_t w, size_t h,
//         size_t rowPitchInBytes,
//         const void * pixels) const {
//     if (empty()) return;
//     RG_GLCHKDBG(glBindTexture(_desc.target, _desc.id));
//     auto & cf = getGLenumDesc(_desc.format);
//     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//     RG_ASSERT(0 == (rowPitchInBytes * 8 % cf.bits));
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, rowPitchInBytes * 8 / cf.bits));
//     RG_GLCHKDBG(glTexSubImage2D(
//             _desc.target,
//             (GLint)level, (GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h,
//             cf.glFormat, cf.glType, pixels));
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//     RG_GLCHK(;);
// }

// void rg::gl::TextureObject::setPixels(size_t layer, size_t level, size_t x, size_t y, size_t w, size_t h, size_t rowPitchInBytes, const void* pixels) const
// {
//     if (empty()) return;

//     RG_GLCHKDBG(glBindTexture(_desc.target, _desc.id));
//     auto& cf = getGLenumDesc(_desc.format);
//     RG_ASSERT(0 == (rowPitchInBytes * 8 % cf.bits));
//     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, (int)(rowPitchInBytes * 8 / cf.bits));

//     RG_GLCHKDBG(glTexSubImage3D(_desc.target, (GLint)level, (GLint)x, (GLint)y, (GLint)layer, (GLsizei)w, (GLsizei)h, 1, cf.glFormat, cf.glType, pixels));

//     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//     RG_GLCHK(;);
// }

// // -----------------------------------------------------------------------------
// //
// ManagedRawImage rg::gl::TextureObject::getBaseLevelPixels() const {
//     if (empty()) return {};
//     ManagedRawImage image(ImageDesc(_desc.format, _desc.width, _desc.height, _desc.depth));
// #ifdef __ANDROID__
//     if (_desc.target == GL_TEXTURE_2D) {
//         GLuint _frameBuffer = 0;
//         glGenFramebuffers(1, &_frameBuffer);
//         glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer);
//         glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _desc.target, _desc.id, 0);
//         glReadBuffer(GL_COLOR_ATTACHMENT0);
//         glReadPixels(0, 0, _desc.width, _desc.height, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
//         RG_GLCHK(;);
//     } else {
//         RG_LOGE("read texture 2D array pixels is not implemented on android yet.");
//     }
// #else
//     auto& cf = getGLenumDesc(_desc.format);
//     glPixelStorei(GL_UNPACK_ALIGNMENT, image.alignment());
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, (int)image.pitch() * 8 / (int)cf.bits);
//     glBindTexture(_desc.target, _desc.id);
//     glGetTexImage(_desc.target, 0, cf.glFormat, cf.glType, image.data());
//     glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//     RG_GLCHK(;);
// #endif
//     return image;
// }

// -----------------------------------------------------------------------------
//
std::string rg::gl::DebugSSBO::printLastResult() const
{
    if (!counter) return {};
    auto count = std::min<size_t>((*counter), buffer.size() - 1);
    auto dataSize = sizeof(float) * (count + 1);
    if (0 != memcmp(buffer.data(), printed.data(), dataSize)) {
        memcpy(printed.data(), buffer.data(), dataSize);
        std::stringstream ss;
        ss << "count = " << *counter << " [";
        for (size_t i = 0; i < count; ++i) {
            auto value = printed[i + 1];
            if (std::isnan(value)) ss << std::endl;
            else ss << value << ", ";
        }
        ss << "]";
        return ss.str();
    }
    return {};
}

// -----------------------------------------------------------------------------
//
void rg::gl::FullScreenQuad::allocate()
{
    const float vertices[] = {
        -1.f, -1.f, 0.f, 1.f,
         3.f, -1.f, 0.f, 1.f,
        -1.f,  3.f, 0.f, 1.f,
    };

    //Cleanup previous array if any.
    cleanup();

    //Create new array.
    RG_GLCHK(glGenVertexArrays(1, &va));
    RG_GLCHK(glBindVertexArray(va));
    vb.allocate(sizeof(vertices), vertices);
    RG_GLCHK(vb.bind());
    RG_GLCHK(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (const void*)0));
    RG_GLCHK(glEnableVertexAttribArray(0));
    RG_GLCHK(glBindVertexArray(0)); // unbind
}

// -----------------------------------------------------------------------------
//
void rg::gl::FullScreenQuad::cleanup()
{
    vb.cleanup();

    //If we actually have a vertex array to cleanup.
    if (va) {
        //Delete the vertex array.
        glDeleteVertexArrays(1, &va);

        //Reset this to mark it as cleaned up.
        va = 0;
    }
    //RG_GLCHK(;);
}

// -----------------------------------------------------------------------------
//
static const char* shaderType2String(GLenum shaderType) {
    switch (shaderType) {
        case GL_VERTEX_SHADER: return "vertex";
        case GL_FRAGMENT_SHADER: return "fragment";
        case GL_COMPUTE_SHADER: return "compute";
        default: return "";
    }
}

// -----------------------------------------------------------------------------
static std::string addLineCount(const std::string & in) {
    std::stringstream ss;
    ss << "(  1) : ";
    int line = 1;
    for (auto ch : in) {
        if ('\n' == ch) {
            ss << "\n(" << std::setw(3) << line << ") : ";
            ++line;
        }
        else ss << ch;
    }
    return ss.str();
}

// -----------------------------------------------------------------------------
//
GLuint rg::gl::loadShaderFromString(const char* source, size_t length, GLenum shaderType, const char* optionalFilename)
{
    if (!source) return 0;
    const char* sources[] = { source };
    if (0 == length) length = strlen(source);
    GLint sizes[] = { (GLint)length };
    auto shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, sources, sizes);
    glCompileShader(shader);

    // check for shader compile errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        glDeleteShader(shader);
        RG_LOGE(
                "\n================== Failed to compile %s shader '%s' ====================\n"
                "%s\n"
                "\n============================= GLSL shader source ===============================\n"
                "%s\n"
                "\n================================================================================\n",
                shaderType2String(shaderType), optionalFilename ? optionalFilename : "<no-name>",
                infoLog,
                addLineCount(source).c_str());
        return 0;
    }

    // done
    RG_ASSERT(shader);
    return shader;
}

// -----------------------------------------------------------------------------
//
GLuint rg::gl::linkProgram(const std::vector<GLuint> & shaders, const char* optionalProgramName)
{
    auto program = glCreateProgram();
    for (auto s : shaders) if (s) glAttachShader(program, s);
    glLinkProgram(program);
    for (auto s : shaders) if (s) glDetachShader(program, s);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        glDeleteProgram(program);
        RG_LOGE("Failed to link program %s:\n%s", optionalProgramName ? optionalProgramName : "", infoLog);
        return 0;
    }

    // Enable the following code to dump GL program binary to disk.
#if 0
    if (optionalProgramName) {
        std::string outfilename = std::string(optionalProgramName) + ".bin";
        std::ofstream fs;
        fs.open(outfilename);
        if (fs.good()) {
            std::vector<uint8_t> buffer(1024 * 1024 * 1024); // allocate 1MB buffer.
            GLsizei len;
            GLenum dummyFormat;
            RG_GLCHK(glGetProgramBinary(program, (GLsizei)buffer.size(), &len, &dummyFormat, buffer.data()));
            fs.write((const char*)buffer.data(), len);
        }
    }
#endif

    // done
    RG_ASSERT(program);
    return program;
}

// -----------------------------------------------------------------------------
//
bool rg::gl::SimpleTextureCopy::init() {
    const char * vscode = R"(#version 320 es
        out vec2 v_uv;
        void main()
        {
            const vec4 v[] = vec4[](
                vec4(-1., -1.,  0., 0.),
                vec4( 3., -1.,  2., 0.),
                vec4(-1.,  3.,  0., 2.));
            gl_Position = vec4(v[gl_VertexID].xy, 0., 1.);
            v_uv = v[gl_VertexID].zw;
        }
    )";
    const char * pscode = R"(
        #version 320 es
        precision mediump float;
        layout(binding = 0) uniform %s u_tex0;
        in vec2 v_uv;
        out vec4 o_color;
        void main()
        {
            o_color = texture(u_tex0, %s).xyzw;
        }
    )";

    // tex2d program
    {
        auto & prog2d = _programs[GL_TEXTURE_2D];
        auto ps2d = formatstr(pscode, "sampler2D", "u_uv");
        if (!prog2d.program.loadVsPs(vscode, ps2d)) return false;
        prog2d.tex0Binding = prog2d.program.getUniformBinding("u_tex0");
    }

    // tex2d array program
    {
        auto & prog2darray = _programs[GL_TEXTURE_2D_ARRAY];
        auto ps2darray = formatstr(pscode, "sampler2DArray", "vec3(u_uv, 0.)");
        if (!prog2darray.program.loadVsPs(vscode, ps2darray)) return false;
        prog2darray.tex0Binding = prog2darray.program.getUniformBinding("u_tex0");
    }

    // create sampler object
    glGenSamplers(1, &_sampler);
    glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glSamplerParameterf(_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameterf(_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glSamplerParameterf(_sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    _quad.allocate();
    glGenFramebuffers(1, &_fbo);

    RG_GLCHK(;); // make sure we have no errors.
    return true;
}

// -----------------------------------------------------------------------------
//
void rg::gl::SimpleTextureCopy::cleanup() {
    _programs.clear();
    _quad.cleanup();
    if (_fbo) glDeleteFramebuffers(1, &_fbo), _fbo = 0;
    if (_sampler) glDeleteSamplers(1, &_sampler), _sampler = 0;
}

// -----------------------------------------------------------------------------
//
void rg::gl::SimpleTextureCopy::copy(const TextureSubResource & src, const TextureSubResource & dst) {
    // get destination texture size
    uint32_t dstw = 0, dsth = 0;
    glBindTexture(dst.target, dst.id);
    glGetTexLevelParameteriv(dst.target, dst.level, GL_TEXTURE_WIDTH, (GLint*)&dstw);
    glGetTexLevelParameteriv(dst.target, dst.level, GL_TEXTURE_HEIGHT, (GLint*)&dsth);

    // attach FBO to the destination texture
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    switch(dst.target) {
        case GL_TEXTURE_2D:
            glFramebufferTexture1D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dst.id, dst.level);
            break;

        case GL_TEXTURE_2D_ARRAY:
            glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, dst.id, dst.level, dst.z);
            break;

        default:
            // 3D or cube texture
            RG_LOGE("unsupported destination texture target.");
            return;
    }
    constexpr GLenum drawbuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawbuffer);
    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
        RG_LOGE("the frame buffer is not complete.");
        return;
    }

    // get the porgram based on source target
    auto & prog = _programs[src.target];
    if (0 == prog.program) {
        RG_LOGE("unsupported source texture target.");
        return;
    }

    // do the copy
    prog.program.use();
    glActiveTexture(GL_TEXTURE0 + prog.tex0Binding);
    glBindTexture(src.target, src.id);
    glBindSampler(prog.tex0Binding, _sampler);
    glViewport(0, 0, dstw, dsth);
    _quad.draw();
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    // done. make sure we are error clean.
    RG_GLCHKDBG(;);
}

// -----------------------------------------------------------------------------
//
void rg::gl::GpuTimeElapsedQuery::stop() {
    if (_q.running()) {
        _q.end();
    }
    else {
        _q.getResult(_result);
    }
}

// -----------------------------------------------------------------------------
//
std::string rg::gl::GpuTimeElapsedQuery::print() const {
    return name + " : " + ns2str(duration());
}

// -----------------------------------------------------------------------------
//
std::string rg::gl::GpuTimestamps::print(const char * ident) const {
    if (_marks.size() < 2) return {};
    std::stringstream ss;
    GLuint64 startTime = _marks[0].result;
    GLuint64 prevTime = startTime;
    if (!ident) ident = "";
    if (0 == startTime) {
        ss << ident << "all timestamp queries are pending...\n";
    } else {

        auto getDuration = [](uint64_t a, uint64_t b) {
            return b >= a ? ns2str(b - a) : "  <n/a>";
        };

        size_t maxlen = 0;
        for (size_t i = 1; i < _marks.size(); ++i) {
            maxlen = std::max(_marks[i].name.size(), maxlen);
        }
        for (size_t i = 1; i < _marks.size(); ++i) {
            auto current = _marks[i].result;
            if (0 == current) {
                ss << ident << "pending...\n";
                break;
            }
            //auto fromStart = current > startTime ? (current - startTime) : 0;
            auto delta = getDuration(prevTime, current);
            ss << ident << std::setw(maxlen) << std::left << _marks[i].name << std::setw(0) << " : "  << delta << std::endl;
            prevTime = current;
        }
        ss << ident << "total = " << getDuration(_marks.front().result, _marks.back().result) << std::endl;
    }
    return ss.str();
}
