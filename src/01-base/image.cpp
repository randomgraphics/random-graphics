#include "pch.h"
//#include "dds.h"
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
    if (!isPowerOf2(rowAlignment)) {
        RG_LOGE("alignment is not power of 2.");
        return false;
    }
    if (offset % rowAlignment) {
        RG_LOGE("offset is not aligned.");
    }
    if (pitch % rowAlignment) {
        RG_LOGE("pitch is not aligned.");
        return false;
    }
    if (slice % rowAlignment) {
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

    auto aw = nextMultiple<uint32_t>(p.width, fd.blockWidth);
    auto ah = nextMultiple<uint32_t>(p.height, fd.blockHeight); 

    p.pitch  = std::max(aw * p.step / 8u, (uint32_t)pitch);
    p.pitch  = (p.pitch + (uint32_t)alignment - 1) & ~((uint32_t)alignment - 1); // make sure pitch meets alignment requirement.
    p.slice  = std::max(p.pitch * ah, (uint32_t)slice);
    p.size   = p.slice * p.depth;
    p.rowAlignment = (uint32_t)alignment;

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
    for(;;) {
        for(size_t i = 0; i < layers_; ++i) {
            planes.push_back(mip);
            mip.offset += mip.size;
        }
        ++levels;

        // check for loop end
        if (levels >= levels_) break;
        if (1 == mip.width && 1 == mip.height && 1 == mip.depth ) break;

        // next level
        if (mip.width > 1) mip.width >>= 1;
        if (mip.height > 1) mip.height >>= 1;
        if (mip.depth > 1) mip.depth >>= 1;
        mip = ImagePlaneDesc::make(mip.format, mip.width, mip.height, mip.depth, mip.step, 0, 0, mip.rowAlignment);
    }

    layers = layers_;
    size = mip.offset;

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

    // allocate pixel buffer
    size_t imageSize = size();
    mPixels.reset((uint8_t*)aalloc(mDesc.plane(0, 0).rowAlignment, imageSize)); // TODO: LSM of all planes' alignment?
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

    // Load from common image file via stb_image library
    // TODO: hdr/grayscale support
    int x,y,n;
    auto data = stbi_load_from_callbacks(&io, &fp, &x, &y, &n, 4);
    if (data) {
        auto image = RawImage(ImageDesc(ImagePlaneDesc::make(ColorFormat::RGBA_8_8_8_8_UNORM(), (uint32_t)x, (uint32_t)y)), data);
        RG_ASSERT(image.desc().valid());
        stbi_image_free(data);
        return image;
    }

    // TODO: KTX, DDS support

    // // try read as DDS
    // fp.seek(begin, GN::FileSeek::SET);
    // DDSReader dds(fp);
    // if (dds.checkFormat()) {
    //     auto image = RawImage(dds.readHeader());
    //     if (image.empty()) return {};
    //     if (!dds.readPixels(image.data(), image.size())) return {};
    //     return image;
    // }

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

// -----------------------------------------------------------------------------
//
rg::RawImage rg::RawImage::loadCube(
    const ConstRange<uint8_t>& nx,
    const ConstRange<uint8_t>& px,
    const ConstRange<uint8_t>& nz,
    const ConstRange<uint8_t>& pz,
    const ConstRange<uint8_t>& py,
    const ConstRange<uint8_t>& ny) {
    int x, xtemp, y, ytemp, n, ntemp;

    auto nxd = stbi_load_from_memory(nx.data(), static_cast<int>(nx.size()), &x, &y, &n, 4);
    if (!nxd) {
        RG_LOGE("Failed to load negative X face from memory: unrecognized image format.");
        return {};
    }
    auto pxd = stbi_load_from_memory(px.data(), static_cast<int>(px.size()), &xtemp, &ytemp, &ntemp, 4);
    if (!pxd) {
        RG_LOGE("Failed to load positive X face from memory: unrecognized image format.");
        return {};
    }
    if (x != xtemp || y != ytemp || n != ntemp) {
        RG_LOGE("Cubemap face dimensions do not match.");
        return {};
    }
    auto nzd = stbi_load_from_memory(nz.data(), static_cast<int>(nz.size()), &xtemp, &ytemp, &ntemp, 4);
    if (!nzd) {
        RG_LOGE("Failed to load negative Z face from memory: unrecognized image format.");
        return {};
    }
    if (x != xtemp || y != ytemp || n != ntemp) {
        RG_LOGE("Cubemap face dimensions do not match.");
        return {};
    }
    auto pzd = stbi_load_from_memory(pz.data(), static_cast<int>(pz.size()), &xtemp, &ytemp, &ntemp, 4);
    if (!pzd) {
        RG_LOGE("Failed to load positive Z face from memory: unrecognized image format.");
        return {};
    }
    if (x != xtemp || y != ytemp || n != ntemp) {
        RG_LOGE("Cubemap face dimensions do not match.");
        return {};
    }
    auto pyd = stbi_load_from_memory(py.data(), static_cast<int>(py.size()), &xtemp, &ytemp, &ntemp, 4);
    if (!pyd) {
        RG_LOGE("Failed to load positive Y face from memory: unrecognized image format.");
        return {};
    }
    if (x != xtemp || y != ytemp || n != ntemp) {
        RG_LOGE("Cubemap face dimensions do not match.");
        return {};
    }
    auto nyd = stbi_load_from_memory(ny.data(), static_cast<int>(ny.size()), &xtemp, &ytemp, &ntemp, 4);
    if (!nyd) {
        RG_LOGE("Failed to load negative Y face from memory: unrecognized image format.");
        return {};
    }
    if (x != xtemp || y != ytemp || n != ntemp) {
        RG_LOGE("Cubemap face dimensions do not match.");
        return {};
    }

    auto image = RawImage(ImageDesc(ImagePlaneDesc::make(ColorFormat::RGBA_8_8_8_8_UNORM(), (uint32_t)x, (uint32_t)y), 6, 1));
    ImageDesc desc = image.desc();
    if (desc.pitch(0) != static_cast<uint32_t>(4 * x)) {
        RG_LOGE("Unsupported byte format.");
        return{};
    }
    if (desc.plane(0).size < static_cast<uint32_t>(4 * x * y)) {
        RG_LOGE("Unsupported byte format.");
        return {};
    }
    memcpy(image.pixel(0, 0), nxd, 4 * x * y);
    memcpy(image.pixel(1, 0), pxd, 4 * x * y);
    memcpy(image.pixel(2, 0), nzd, 4 * x * y);
    memcpy(image.pixel(3, 0), pzd, 4 * x * y);
    memcpy(image.pixel(4, 0), pyd, 4 * x * y);
    memcpy(image.pixel(5, 0), nyd, 4 * x * y);
    RG_ASSERT(image.desc().valid());
    stbi_image_free(nxd);
    stbi_image_free(pxd);
    stbi_image_free(nzd);
    stbi_image_free(pzd);
    stbi_image_free(pyd);
    stbi_image_free(nyd);

    return image;
}