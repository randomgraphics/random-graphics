#include "pch.h"
#include "dds.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT RG_ASSERT
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
// #define STBI_MALLOC rg::HeapMemory::alloc
// #define STBI_REALLOC rg::HeapMemory::realloc
// #define STBI_FREE rg::HeapMemory::dealloc
#include "stb_image.h"
#include <algorithm>

using namespace rg;

// *****************************************************************************
// ImagePlaneDesc
// *****************************************************************************

// -----------------------------------------------------------------------------
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
    static_assert(!isPowerOf2(0));
    if (!isPowerOf2(alignment)) {
        RG_LOGE("alignment is not power of 2.");
        return false;
    }
    if (offset % alignment) {
        RG_LOGE("offset is not aligned.");
    }
    if ((pitch * cld.blockHeight) % alignment) {
        RG_LOGE("pitch is not aligned.");
        return false;
    }
    if (slice % alignment) {
        RG_LOGE("slice is not aligned.");
        return false;
    }

    // done
    return true;
}

// -----------------------------------------------------------------------------
//
rg::ImagePlaneDesc rg::ImagePlaneDesc::make(ColorFormat format, size_t width, size_t height, size_t depth, size_t step, size_t pitch, size_t slice, size_t alignment) {

    if (!format.valid()) {
        RG_LOGE("invalid color format: 0x%X", format.u32);
        return {};
    }

    if (0 == alignment) {
        alignment = 4; // aligned to 32bits by default.
    } else if (!isPowerOf2(alignment)) {
        RG_LOGW("image alignment must be power of 2.");
        alignment = ceilPowerOf2(alignment); // make sure alignment is power of 2.
    }

    const auto & fd = format.layoutDesc();

    ImagePlaneDesc p;
    p.format = format;
    p.width  = (uint32_t)(width ? width : 1);
    p.height = (uint32_t)(height ? height : 1);
    p.depth  = (uint32_t)(depth ? depth : 1);
    p.step   = std::max((uint32_t)step, (uint32_t)fd.pixelBits);

    // calculate alignment
    RG_ASSERT(isPowerOf2(fd.blockBytes));
    p.alignment = std::max((uint32_t)alignment, (uint32_t)fd.blockBytes);
    uint32_t rowAlignment = p.alignment / fd.blockHeight;

    // align image size to pixel block size
    auto aw  = nextMultiple<uint32_t>(p.width, fd.blockWidth);
    auto ah  = nextMultiple<uint32_t>(p.height, fd.blockHeight);

    p.pitch  = nextMultiple(std::max(aw * p.step / 8u, (uint32_t)pitch), rowAlignment);
    p.slice  = std::max(p.pitch * ah, (uint32_t)slice);
    p.size   = p.slice * p.depth;

    // done
    RG_ASSERT(p.valid());
    return p;
}

// *****************************************************************************
// ImageDesc
// *****************************************************************************

// -----------------------------------------------------------------------------
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
        RG_LOGE("plane arary size must be eaul to (layers * levels)");
        return false;
    }

    // check mipmaps
    for (uint32_t f = 0; f < layers; ++f)
    for (uint32_t l = 0; l < levels; ++l) {
        auto & m = plane(f, l);

        if (!m.valid()) {
            RG_LOGE("plane descritor [%d] is invalid", index(f, l));
            return false;
        }
    }

    // success
    return true;
}

// -----------------------------------------------------------------------------
//
void rg::ImageDesc::reset(const ImagePlaneDesc & basemap, uint32_t layers_, uint32_t levels_) {

    if (!basemap.valid()) {
        planes.clear();
        layers = 0;
        levels = 0;
        size = 0;
        RG_ASSERT(valid());
        return;
    }

    if (0 == layers_) layers_ = 1;

    if (0 == levels_) levels_ = UINT_MAX;

    // build full mipmap chain
    ImagePlaneDesc mip = basemap;
    levels = 0;
    uint32_t offset = 0;
    for(;;) {
        for(size_t i = 0; i < layers_; ++i) {
            mip.offset = offset;
            planes.push_back(mip);
            offset += mip.size;
        }
        ++levels;

        // check for loop end
        if (levels >= levels_) break;
        if (1 == mip.width && 1 == mip.height && 1 == mip.depth ) break;

        // next level
        if (mip.width > 1) mip.width >>= 1;
        if (mip.height > 1) mip.height >>= 1;
        if (mip.depth > 1) mip.depth >>= 1;
        mip = ImagePlaneDesc::make(mip.format, mip.width, mip.height, mip.depth, mip.step, 0, 0, mip.alignment);
    }

    layers = layers_;
    size = offset;

    // done
    RG_ASSERT(valid());
}

// *****************************************************************************
// RawImage
// *****************************************************************************

// -----------------------------------------------------------------------------
//
rg::RawImage::RawImage(ImageDesc&& desc, const void * initialContent, size_t initialContentSizeInbytes)
: mDesc(std::move(desc)) {
    construct(initialContent, initialContentSizeInbytes);
}

// -----------------------------------------------------------------------------
//
rg::RawImage::RawImage(const ImageDesc & desc, const void * initialContent, size_t initialContentSizeInbytes)
: mDesc(desc) {
    construct(initialContent, initialContentSizeInbytes);
}

// -----------------------------------------------------------------------------
//
void rg::RawImage::construct(const void * initialContent, size_t initialContentSizeInbytes) {
    // allocate pixel buffer
    size_t imageSize = size();
    mPixels.reset((uint8_t*)aalloc(mDesc.plane(0, 0).alignment, imageSize)); // TODO: LSM of all planes' alignment?
    if (!mPixels) {
        return;
    }

    // store the initial content
    if (initialContent) {
        if (0 == initialContentSizeInbytes) {
            initialContentSizeInbytes = imageSize;
        } else if (initialContentSizeInbytes != imageSize) {
            RG_LOGW("incoming pixel buffer size does not equal to calculated image size.");
        }
        memcpy(mPixels.get(), initialContent, std::min(imageSize, initialContentSizeInbytes));
    }
}

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
//
rg::RawImage rg::RawImage::load(const ConstRange<uint8_t> & data) {
    auto str = std::string((const char*)data.data(), data.size());
    auto iss = std::istringstream(str);
    return load(iss);
}
