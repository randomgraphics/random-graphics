#include "pch.h"
#include "dds.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_ASSERT RG_ASSERT
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
#include "stb_image.h"
#include "stb_image_write.h"
#include <algorithm>
#include <numeric>
#include <filesystem>

using namespace rg;

// *********************************************************************************************************************
// ImagePlaneDesc
// *********************************************************************************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
bool rg::ImagePlaneDesc::valid() const {
    // check format
    if (!format.valid()) {
        RG_LOGE("invalid format");
        return false;
    }

    // check dimension
    if (0 == width || 0 == height || 0 == depth) {
        RG_LOGE("dimension can't zero!");
        return false;
    }

    // check pitches
    auto & cld = format.layoutDesc();
    auto w = nextMultiple<uint32_t>( width, cld.blockWidth );
    auto h = nextMultiple<uint32_t>( height, cld.blockHeight );
    if (step < cld.pixelBits) {
        RG_LOGE("step is too small!");
        return false;
    }
    if (pitch < w * cld.pixelBits / 8) {
        RG_LOGE("pitch is too small!");
        return false;
    }
    if (slice < pitch * h) {
        RG_LOGE("slice is too small!");
        return false;
    }
    if (size < slice * depth) {
        RG_LOGE("size is too small!");
        return false;
    }

    // check alignment
    if ((pitch * cld.blockHeight) % cld.blockBytes) {
        RG_LOGE("Pitch is not aligned to pixel boundary.");
        return false;
    }
    if (offset % cld.blockBytes) {
        RG_LOGE("offset is not aligned to pixel boundary.");
    }
    if (slice % cld.blockBytes) {
        RG_LOGE("slice is not aligned to pixel boundary.");
        return false;
    }

    // done
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::ImagePlaneDesc rg::ImagePlaneDesc::make(ColorFormat format, size_t width, size_t height, size_t depth,
                                            size_t step, size_t pitch, size_t slice) {

    if (!format.valid()) {
        RG_LOGE("invalid color format: 0x%X", format.u32);
        return {};
    }

    const auto & fd = format.layoutDesc();

    // calculate alignments
    // if (0 != alignment && !isPowerOf2(alignment)) {
    //     RG_LOGE("image alignment (%d) must be 0 or power of 2.", alignment);
    //     return {};
    // }
    RG_ASSERT(0 == (fd.blockBytes % fd.blockHeight));
    uint32_t rowAlignment = fd.blockBytes / fd.blockHeight;
    // alignment = ceilPowerOf2(std::max((uint32_t)alignment, (uint32_t)fd.blockBytes));

    ImagePlaneDesc p;
    p.format    = format;
    p.width     = (uint32_t)(width ? width : 1);
    p.height    = (uint32_t)(height ? height : 1);
    p.depth     = (uint32_t)(depth ? depth : 1);
    p.step      = std::max((uint32_t)step, (uint32_t)fd.pixelBits);
    // p.alignment = (uint32_t)alignment;

    // align image size to pixel block size
    auto aw  = nextMultiple<uint32_t>(p.width, fd.blockWidth);
    auto ah  = nextMultiple<uint32_t>(p.height, fd.blockHeight);

    // calculate row pitch, aligned to rowAlignment.
    // Note that we can't just use nextMultiple here, since rowAlignment might not be power of 2.
    p.pitch = std::max(aw * p.step / 8u, (uint32_t)pitch);
    p.pitch = (p.pitch + rowAlignment - 1) / rowAlignment * rowAlignment;

    // calculate slice and plane size of the image.
    p.slice  = std::max(p.pitch * ah, (uint32_t)slice);
    p.size   = p.slice * p.depth;

    // done
    RG_ASSERT(p.valid());
    return p;
}

// ---------------------------------------------------------------------------------------------------------------------

struct RGBA8 {
    uint8_t x, y, z, w;
};

struct float4 {
    float x, y, z, w;
};

struct uint128_t {
    uint64_t lo;
    uint64_t hi;
    uint32_t segment(uint32_t offset, uint32_t count) const {
        if (offset + count <= 64) {
            uint64_t mask = (((uint64_t)1) << count) - 1;
            return (uint32_t)((lo >> offset) & mask);
        } else if (offset >= 64) {
            uint64_t mask = (((uint64_t)1) << count) - 1;
            return (uint32_t)(hi >> (offset - 64) & mask);
        } else {
            // This means the segment is crossing the low and hi
            RG_THROW("unsupported yet.");
        }
    }
};

// ---------------------------------------------------------------------------------------------------------------------
/// Convert one color channel to float, based on the channel format/sign
/// \param value The channel value
/// \param width The number of valid bits in that value
/// \param sign  The channel's data format
static inline float tofloat(uint32_t value, uint32_t width, ColorFormat::Sign sign) {
    auto castToFloat = [](uint32_t u32) {
        union { ;
            float f;
            uint32_t u;
        };
        u = u32;
        return f;
    };
    uint32_t mask = (1u << width) - 1;
    value &= mask;
    switch(sign) {
        case ColorFormat::SIGN_UNORM:
            return (float)value / (float)mask;

        case ColorFormat::SIGN_FLOAT:
            // 10 bits    =>                         EE EEEFFFFF
            // 11 bits    =>                        EEE EEFFFFFF
            // 16 bits    =>                   SEEEEEFF FFFFFFFF
            // Float bits => SEEEEEEE EFFFFFFF FFFFFFFF FFFFFFFF
            // 0x0000001F => 00000000 00000000 00000000 00011111
            // 0x0000003F => 00000000 00000000 00000000 00111111
            // 0x000003E0 => 00000000 00000000 00000011 11100000
            // 0x000007C0 => 00000000 00000000 00000111 11000000
            // 0x00007C00 => 00000000 00000000 01111100 00000000
            // 0x000003FF => 00000000 00000000 00000011 11111111
            // 0x38000000 => 00111000 00000000 00000000 00000000
            // 0x7f800000 => 01111111 10000000 00000000 00000000
            // 0x00008000 => 00000000 00000000 10000000 00000000
            if (width == 32) {
                return castToFloat(value);
            } else if (width == 16) {
                uint32_t u32 = ((value & 0x8000) << 16) | ((( value & 0x7c00) + 0x1C000) << 13) | // exponential
                               ((value & 0x03FF) << 13); // mantissa
                return castToFloat(u32);
            } else if (width == 11) {
                uint32_t u32 = ((((value & 0x07c0) << 17) + 0x38000000) & 0x7f800000) | // exponential
                               ((value & 0x003f) << 17); // Mantissa
                return castToFloat(u32);
            } else if (width == 10) {
                uint32_t u32 = ((((value & 0x03E0) << 18) + 0x38000000) & 0x7f800000) | // exponential
                               ((value & 0x001f) << 18); // Mantissa
                return castToFloat(u32);
            } else {
                RG_THROW("unsupported yet.");
            }

        case ColorFormat::SIGN_UINT:
            return (float)value;

        case ColorFormat::SIGN_SNORM:
        case ColorFormat::SIGN_GNORM:
        case ColorFormat::SIGN_BNORM:
        case ColorFormat::SIGN_SINT:
        case ColorFormat::SIGN_GINT:
        case ColorFormat::SIGN_BINT:
        default:
            // not supported yet.
            RG_THROW("unsupported yet.");
    }
}

// ---------------------------------------------------------------------------------------------------------------------
/// Convert pixel of arbitrary format to float4. Do not support compressed format.
static inline float4 convertToFloat4(const ColorFormat::LayoutDesc & ld, const ColorFormat & format,
                                     const void * pixel) {
    RG_ASSERT(1 == ld.blockWidth && 1 == ld.blockHeight); // do not support compressed format.

    const uint128_t * src = (const uint128_t*)pixel;

    // labmda to convert one channel
    auto convertChannel = [&](uint32_t swizzle) {
        if (ColorFormat::SWIZZLE_0 == swizzle) return 0.f;
        if (ColorFormat::SWIZZLE_1 == swizzle) return 1.f;
        const auto & ch = ld.channels[swizzle];
        auto sign = (ColorFormat::Sign)((swizzle < 3) ? format.sign012 : format.sign3);
        return tofloat(src->segment(ch.shift, ch.bits), ch.bits, sign);
    };

    return {
        convertChannel(format.swizzle0),
        convertChannel(format.swizzle1),
        convertChannel(format.swizzle2),
        convertChannel(format.swizzle3),
    };
}

// ---------------------------------------------------------------------------------------------------------------------
//
static RGBA8 convertToRGBA8(const ColorFormat::LayoutDesc & ld, const ColorFormat & format,
                            const void * src) {
    if (ColorFormat::RGBA8() == format) {
        // shortcut for RGBA8 format.
        return *(const RGBA8*)src;
    } else {
        // this is the general case that could in theory handle any format.
        auto f4 = convertToFloat4(ld, format, src);
        RGBA8 result;
        result.x = (uint8_t)std::clamp<uint32_t>((uint32_t)(f4.x * 255.0f), 0, 255);
        result.y = (uint8_t)std::clamp<uint32_t>((uint32_t)(f4.y * 255.0f), 0, 255);
        result.z = (uint8_t)std::clamp<uint32_t>((uint32_t)(f4.z * 255.0f), 0, 255);
        result.w = (uint8_t)std::clamp<uint32_t>((uint32_t)(f4.w * 255.0f), 0, 255);
        return result;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
//
static std::vector<RGBA8> convertToRGBA8(const ImagePlaneDesc & plane, const void * pixels, uint32_t z) {
    if (plane.empty()) {
        RG_LOGE("Can't save empty image plane.");
        return {};
    }
    const uint8_t * p = (const uint8_t *)pixels;
    auto ld = plane.format.layoutDesc();
    std::vector<RGBA8> colors;
    colors.reserve(plane.width * plane.height);
    for(uint32_t y = 0; y < plane.height; ++y) {
        for(uint32_t x = 0; x < plane.width; ++x) {
            colors.push_back(convertToRGBA8(ld, plane.format, p + plane.pixel(x, y, z)));
        }
    }
    return colors;
}

// ---------------------------------------------------------------------------------------------------------------------
//
static std::vector<float4> convertToFloat4(const ImagePlaneDesc & plane, const void * pixels, uint32_t z) {
    if (plane.empty()) {
        RG_LOGE("Can't save empty image plane.");
        return {};
    }
    const uint8_t * p = (const uint8_t *)pixels;
    auto ld = plane.format.layoutDesc();
    std::vector<float4> colors;
    colors.reserve(plane.width * plane.height);
    for(uint32_t y = 0; y < plane.height; ++y) {
        for(uint32_t x = 0; x < plane.width; ++x) {
            colors.push_back(convertToFloat4(ld, plane.format, p + plane.pixel(x, y, z)));
        }
    }
    return colors;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::ImagePlaneDesc::saveToPNG(const std::string & filename, const void * pixels, uint32_t z) {
    auto colors = convertToRGBA8(*this, pixels, z);
    if (colors.empty()) return;
    stbi_write_png(filename.c_str(), (int)width, (int)height, 4, colors.data(), 4);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::ImagePlaneDesc::saveToJPG(const std::string & filename, const void * pixels, uint32_t z, int quality) {
    auto colors = convertToRGBA8(*this, pixels, z);
    if (colors.empty()) return;
    stbi_write_jpg(filename.c_str(), (int)width, (int)height, 4, colors.data(), quality);
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::ImagePlaneDesc::saveToHDR(const std::string & filename, const void * pixels, uint32_t z) {
    auto colors = convertToFloat4(*this, pixels, z);
    if (colors.empty()) return;
    stbi_write_hdr(filename.c_str(), (int)width, (int)height, 4, (const float*)colors.data());
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::ImagePlaneDesc::save(const std::string & filename, const void * pixels, uint32_t z) {
    auto ext = std::filesystem::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](char c) { return (char)tolower(c); });
    if (".jpg" == ext || ".jpeg" == ext) {
        saveToJPG(filename, pixels, z);
    } else if (".png" == ext) {
        saveToJPG(filename, pixels, z);
    } else if (".hdr" == ext) {
        saveToHDR(filename, pixels, z);
    } else {
        RG_LOGE("Unsupported file extension: %s", ext.c_str());
    }
}

// *********************************************************************************************************************
// ImageDesc
// *********************************************************************************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
bool rg::ImageDesc::valid() const {

    if (planes.size() == 0) {
        // supposed to be an empty descriptor
        if (0 != levels || 0 != layers || 0 != size) {
            RG_LOGE("empty descriptor should have zero on all members variables.");
            return false;
        } else {
            return true;
        }
    }

    if (layers * levels != planes.size()) {
        RG_LOGE("image plane array size must be equal to (layers * levels)");
        return false;
    }

    // check mipmaps
    for (uint32_t f = 0; f < layers; ++f)
    for (uint32_t l = 0; l < levels; ++l) {
        auto & m = plane(f, l);
        if (!m.valid()) {
            RG_LOGE("image plane [%d] is invalid", index(f, l));
            return false;
        }
        if ((m.offset + m.size) > size) {
            RG_LOGE("image plane [%d]'s (offset + size) is out of range.", index(f, l));
            return false;
        }
    }

    // success
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::ImageDesc::reset(const ImagePlaneDesc & basemap, uint32_t layers_, uint32_t levels_, ConsructionOrder order) {

    // clear old data
    planes.clear();
    layers = 0;
    levels = 0;
    size = 0;
    RG_ASSERT(valid());

    if (!basemap.valid()) {
        return;
    }

    if (0 == layers_) layers_ = 1;

    // calculate number of mipmap levels.
    {
        uint32_t maxLevels = 1;
        uint32_t w = basemap.width;
        uint32_t h = basemap.height;
        uint32_t d = basemap.depth;
        while(w > 1 || h > 1|| d > 1) {
            if (w > 1) w >>= 1;
            if (h > 1) h >>= 1;
            if (d > 1) d >>= 1;
            ++maxLevels;
        }
        if (0 == levels_ || levels_ > maxLevels) levels_ = maxLevels;
    }

    // build full mipmap chain
    layers = layers_;
    levels = levels_;
    planes.resize(layers_ * levels_);
    uint32_t offset = 0;
    if (MIP_MAJOR == order) {
        // In this mode, the outer loop is mipmap level, the inner loop is layers/faces
        ImagePlaneDesc mip = basemap;
        for(uint32_t m = 0; m < levels_; ++m) {
            for(size_t i = 0; i < layers_; ++i) {
                mip.offset = offset;
                planes[index(i, m)] = mip;
                offset += mip.size;
            }
            // next level
            if (mip.width > 1) mip.width >>= 1;
            if (mip.height > 1) mip.height >>= 1;
            if (mip.depth > 1) mip.depth >>= 1;
            mip = ImagePlaneDesc::make(mip.format, mip.width, mip.height, mip.depth, mip.step, 0, 0);
        }
    } else {
        // In this mode, the outer loop is faces/layers, the inner loop is mipmap levels
        for(size_t f = 0; f < layers_; ++f) {
            ImagePlaneDesc mip = basemap;
            for(size_t m = 0; m < levels_; ++m) {
                mip.offset = offset;
                planes[index(f, m)] = mip;
                offset += mip.size;
                // next level
                if (mip.width > 1) mip.width >>= 1;
                if (mip.height > 1) mip.height >>= 1;
                if (mip.depth > 1) mip.depth >>= 1;
                mip = ImagePlaneDesc::make(mip.format, mip.width, mip.height, mip.depth, mip.step, 0, 0);
            }
        }
    }

    size = offset;

    // done
    RG_ASSERT(valid());
}

// *********************************************************************************************************************
// RawImage
// *********************************************************************************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
rg::RawImage::RawImage(ImageDesc&& desc, const void * initialContent, size_t initialContentSizeInbytes) {
    _proxy.desc = std::move(desc);
    construct(initialContent, initialContentSizeInbytes);
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::RawImage::RawImage(const ImageDesc & desc, const void * initialContent, size_t initialContentSizeInbytes) {
    _proxy.desc = std::move(desc);
    construct(initialContent, initialContentSizeInbytes);
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::RawImage::~RawImage() {
    afree(_proxy.data);
    _proxy.data = nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------
//
void rg::RawImage::construct(const void * initialContent, size_t initialContentSizeInbytes) {
    // clear old image data.
    afree(_proxy.data);
    _proxy.data = nullptr;

    // deal with empty image
    if (_proxy.desc.empty()) {
        RG_LOGE("image is empty.");
        return;
    }

    // (re)allocate pixel buffer
    size_t imageSize = size();
    _proxy.data = (uint8_t*)aalloc(4, imageSize); // TODO: larger alignment for larger pixel?
    if (!_proxy.data) {
        RG_LOGE("failed to construct image: out of memory.");
        return;
    }

    // store the initial content
    if (initialContent) {
        if (0 == initialContentSizeInbytes) {
            initialContentSizeInbytes = imageSize;
        } else if (initialContentSizeInbytes != imageSize) {
            RG_LOGW("incoming pixel buffer size does not equal to calculated image size.");
        }
        memcpy(_proxy.data, initialContent, std::min(imageSize, initialContentSizeInbytes));
    }
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::RawImage rg::RawImage::load(std::istream & fp) {
    // store current stream position
    auto begin = fp.tellg();

    // setup stbi io callback
    stbi_io_callbacks io = {};
    io.read = [](void* user, char* data, int size) -> int {
        auto fp = (std::istream *)user;
        fp->read(data, size);
        return (int)fp->gcount();
    };
    io.skip = [](void* user, int n) {
        auto fp = (std::istream*)user;
        fp->seekg(n, std::ios::cur);
    };
    io.eof = [](void* user) -> int {
        auto fp = (std::istream*)user;
        return fp->eof();
    };

    // try read as DDS first
    DDSReader dds(fp);
    if (dds.checkFormat()) {
        auto image = RawImage(dds.readHeader());
        if (image.empty()) return {};
        if (!dds.readPixels(image.data(), image.size())) return {};
        return image;
    }

    // Load from common image file via stb_image library
    // TODO: hdr/grayscale support
    int x,y,n;
    fp.seekg(begin, std::ios::beg);
    auto data = stbi_load_from_callbacks(&io, &fp, &x, &y, &n, 4);
    if (data) {
        auto image = RawImage(ImageDesc(ImagePlaneDesc::make(ColorFormat::RGBA_8_8_8_8_UNORM(), (uint32_t)x, (uint32_t)y)), data);
        RG_ASSERT(image.desc().valid());
        stbi_image_free(data);
        return image;
    }

    // TODO: KTX, DDS support

    RG_LOGE("Failed to load image from stream: unrecognized image format.");
    return {};
}

// ---------------------------------------------------------------------------------------------------------------------
//
rg::RawImage rg::RawImage::load(const ConstRange<uint8_t> & data) {
    auto str = std::string((const char*)data.data(), data.size());
    auto iss = std::istringstream(str);
    return load(iss);
}
