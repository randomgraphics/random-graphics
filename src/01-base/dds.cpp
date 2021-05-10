#include "pch.h"
#include "dds.h"

#ifndef MAKE_FOURCC
#define MAKE_FOURCC(ch0, ch1, ch2, ch3)     \
            ((uint32_t)(uint8_t)(ch0) |         \
            ((uint32_t)(uint8_t)(ch1) << 8) |   \
            ((uint32_t)(uint8_t)(ch2) << 16) |  \
            ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKE_FOURCC) */

///
/// DDS format flags
///
enum DdsFlag {
    DDS_DDPF_SIZE               = sizeof(DDPixelFormat)          ,
    DDS_DDPF_ALPHAPIXELS        = 0x00000001                     ,
    DDS_DDPF_ALPHA              = 0x00000002                     ,
    DDS_DDPF_FOURCC             = 0x00000004                     ,
    DDS_DDPF_PALETTEINDEXED8    = 0x00000020                     ,
    DDS_DDPF_RGB                = 0x00000040                     ,
    DDS_DDPF_ZBUFFER            = 0x00000400                     ,
    DDS_DDPF_STENCILBUFFER      = 0x00004000                     ,
    DDS_DDPF_LUMINANCE          = 0x00020000                     ,
    DDS_DDPF_BUMPLUMINANCE      = 0x00040000                     ,
    DDS_DDPF_BUMPDUDV           = 0x00080000                     ,
    DDS_DDSD_CAPS               = 0x00000001                     ,
    DDS_DDSD_HEIGHT             = 0x00000002                     ,
    DDS_DDSD_WIDTH              = 0x00000004                     ,
    DDS_DDSD_PIXELFORMAT        = 0x00001000                     ,
    DDS_DDSD_MIPMAPCOUNT        = 0x00020000                     ,
    DDS_DDSD_DEPTH              = 0x00800000                     ,
    DDS_CAPS_ALPHA              = 0x00000002                     ,
    DDS_CAPS_COMPLEX            = 0x00000008                     ,
    DDS_CAPS_PALETTE            = 0x00000100                     ,
    DDS_CAPS_TEXTURE            = 0x00001000                     ,
    DDS_CAPS_MIPMAP             = 0x00400000                     ,
    DDS_CAPS2_CUBEMAP           = 0x00000200                     ,
    DDS_CAPS2_CUBEMAP_ALLFACES  = 0x0000fc00                     ,
    DDS_CAPS2_VOLUME            = 0x00200000                     ,
    DDS_FOURCC_UYVY             = MAKE_FOURCC('U', 'Y', 'V', 'Y') ,
    DDS_FOURCC_R8G8_B8G8        = MAKE_FOURCC('R', 'G', 'B', 'G') ,
    DDS_FOURCC_YUY2             = MAKE_FOURCC('Y', 'U', 'Y', '2') ,
    DDS_FOURCC_G8R8_G8B8        = MAKE_FOURCC('G', 'R', 'G', 'B') ,
    DDS_FOURCC_DXT1             = MAKE_FOURCC('D', 'X', 'T', '1') ,
    DDS_FOURCC_DXT2             = MAKE_FOURCC('D', 'X', 'T', '2') ,
    DDS_FOURCC_DXT3             = MAKE_FOURCC('D', 'X', 'T', '3') ,
    DDS_FOURCC_DXT4             = MAKE_FOURCC('D', 'X', 'T', '4') ,
    DDS_FOURCC_DXT5             = MAKE_FOURCC('D', 'X', 'T', '5') ,
    DDS_FOURCC_A16B16G16R16     = 36                             ,
    DDS_FOURCC_Q16W16V16U16     = 110                            ,
    DDS_FOURCC_R16F             = 111                            ,
    DDS_FOURCC_G16R16F          = 112                            ,
    DDS_FOURCC_A16B16G16R16F    = 113                            ,
    DDS_FOURCC_R32F             = 114                            ,
    DDS_FOURCC_G32R32F          = 115                            ,
    DDS_FOURCC_A32B32G32R32F    = 116                            ,
    DDS_FOURCC_CxV8U8           = 117                            ,
};

///
/// \note this struct should be synchronized with color format definition
///
static struct DdpfDesc {
    rg::ColorFormat clrfmt;
    DDPixelFormat   ddpf;
} const s_ddpfDescTable[] = {
    { rg::ColorFormat::BGR_8_8_8_UNORM(),               { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 24,   0xff0000,   0x00ff00,   0x0000ff,          0 } },
    { rg::ColorFormat::BGRA_8_8_8_8_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 } },
    { rg::ColorFormat::BGRX_8_8_8_8_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff,          0 } },
    { rg::ColorFormat::BGR_5_6_5_UNORM(),               { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 16,     0xf800,     0x07e0,     0x001f,          0 } },
    { rg::ColorFormat::BGRX_5_5_5_1_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 16,     0x7c00,     0x03e0,     0x001f,          0 } },
    { rg::ColorFormat::BGRA_5_5_5_1_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 16,     0x7c00,     0x03e0,     0x001f,     0x8000 } },
    { rg::ColorFormat::BGRA_4_4_4_4_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 16,     0x0f00,     0x00f0,     0x000f,     0xf000 } },
  //{ rg::ColorFormat::BGR_2_3_3(),                     { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0,  8,       0xe0,       0x1c,       0x03,          0 } },
    { rg::ColorFormat::A_8_UNORM(),                     { DDS_DDPF_SIZE, DDS_DDPF_ALPHA,                                   0,  8,          0,          0,          0,       0xff } },
  //{ rg::ColorFormat::BGRA_2_3_3_8(),                  { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 16,     0x00e0,     0x001c,     0x0003,     0xff00 } },
    { rg::ColorFormat::BGRX_4_4_4_4_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 16,     0x0f00,     0x00f0,     0x000f,          0 } },
  //{ rg::ColorFormat::BGRA_10_10_10_2_UNORM(),         { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 } },
    { rg::ColorFormat::RGBA_8_8_8_8_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 } },
    { rg::ColorFormat::RGBX_8_8_8_8_UNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000,          0 } },
    { rg::ColorFormat::RG_16_16_UNORM(),                { DDS_DDPF_SIZE, DDS_DDPF_RGB,                                     0, 32, 0x0000ffff, 0xffff0000, 0x00000000,          0 } },
    { rg::ColorFormat::RGBA_10_10_10_2_UNORM(),         { DDS_DDPF_SIZE, DDS_DDPF_RGB | DDS_DDPF_ALPHAPIXELS,              0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000 } },
  //{ rg::ColorFormat::A8P8_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_PALETTEINDEXED8 | DDS_DDPF_ALPHAPIXELS,  0, 16,          0,          0,          0,     0xff00 } },
  //{ rg::ColorFormat::P8_UNORM(),                      { DDS_DDPF_SIZE, DDS_DDPF_PALETTEINDEXED8,                         0,  8,          0,          0,          0,          0 } },
    { rg::ColorFormat::L_8_UNORM(),                     { DDS_DDPF_SIZE, DDS_DDPF_LUMINANCE,                               0,  8,       0xff,          0,          0,          0 } },
    { rg::ColorFormat::LA_8_8_UNORM(),                  { DDS_DDPF_SIZE, DDS_DDPF_LUMINANCE | DDS_DDPF_ALPHAPIXELS,        0, 16,     0x00ff,          0,          0,     0xff00 } },
  //{ rg::ColorFormat::LA_4_4_UNORM(),                  { DDS_DDPF_SIZE, DDS_DDPF_LUMINANCE | DDS_DDPF_ALPHAPIXELS,        0,  8,       0x0f,          0,          0,       0xf0 } },
  //{ rg::ColorFormat::L_16_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_LUMINANCE,                               0, 16,     0xffff,          0,          0,          0 } },
    { rg::ColorFormat::RG_8_8_SNORM(),                  { DDS_DDPF_SIZE, DDS_DDPF_BUMPDUDV,                                0, 16,     0x00ff,     0xff00,     0x0000,     0x0000 } },
  //{ rg::ColorFormat::UVL_5_5_6(),                     { DDS_DDPF_SIZE, DDS_DDPF_BUMPLUMINANCE,                           0, 16,     0x001f,     0x03e0,     0xfc00,          0 } },
  //{ rg::ColorFormat::UVLX_8_8_8_8(),                  { DDS_DDPF_SIZE, DDS_DDPF_BUMPLUMINANCE,                           0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000,          0 } },
    { rg::ColorFormat::RGBA_8_8_8_8_SNORM(),            { DDS_DDPF_SIZE, DDS_DDPF_BUMPDUDV,                                0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 } },
    { rg::ColorFormat::RG_16_16_UNORM(),                { DDS_DDPF_SIZE, DDS_DDPF_BUMPDUDV,                                0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 } },
  //{ rg::ColorFormat::UVWA_10_10_10_2(),               { DDS_DDPF_SIZE, DDS_DDPF_BUMPDUDV | DDS_DDPF_ALPHAPIXELS,         0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000 } },
    { rg::ColorFormat::R_16_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_ZBUFFER,                                 0, 16,          0,     0xffff,          0,          0 } },
  //{ rg::ColorFormat::UYVY(),                          { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_UYVY,  0,          0,          0,          0,          0 } },
  //{ rg::ColorFormat::R8G8_B8G8(),                     { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,               DDS_FOURCC_R8G8_B8G8,  0,          0,          0,          0,          0 } },
  //{ rg::ColorFormat::YUY2(),                          { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_YUY2,  0,          0,          0,          0,          0 } },
  //{ rg::ColorFormat::G8R8_G8B8(),                     { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,               DDS_FOURCC_G8R8_G8B8,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::DXT1_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_DXT1,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::DXT3_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_DXT2,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::DXT3_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_DXT3,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::DXT5_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_DXT4,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::DXT5_UNORM(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_DXT5,  0,          0,          0,          0,          0 } },
  //{ rg::ColorFormat::D_32_FLOAT(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,               D3DFMT_D32F_LOCKABLE,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RGBA_16_16_16_16_UNORM(),        { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,            DDS_FOURCC_A16B16G16R16,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RGBA_16_16_16_16_SNORM(),        { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,            DDS_FOURCC_Q16W16V16U16,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::R_16_FLOAT(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_R16F,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RG_16_16_FLOAT(),                { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                 DDS_FOURCC_G16R16F,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RGBA_16_16_16_16_FLOAT(),        { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,           DDS_FOURCC_A16B16G16R16F,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::R_32_FLOAT(),                    { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                    DDS_FOURCC_R32F,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RG_32_32_FLOAT(),                { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                 DDS_FOURCC_G32R32F,  0,          0,          0,          0,          0 } },
    { rg::ColorFormat::RGBA_32_32_32_32_FLOAT(),        { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,           DDS_FOURCC_A32B32G32R32F,  0,          0,          0,          0,          0 } },
  //{ rg::ColorFormat::CxV8U8(),                        { DDS_DDPF_SIZE, DDS_DDPF_FOURCC,                  DDS_FOURCC_CxV8U8,  0,          0,          0,          0,          0 } },
};

/// DXGI_FORMAT enum copied from MSDN (https://docs.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format)
enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN,
    DXGI_FORMAT_R32G32B32A32_TYPELESS,
    DXGI_FORMAT_R32G32B32A32_FLOAT,
    DXGI_FORMAT_R32G32B32A32_UINT,
    DXGI_FORMAT_R32G32B32A32_SINT,
    DXGI_FORMAT_R32G32B32_TYPELESS,
    DXGI_FORMAT_R32G32B32_FLOAT,
    DXGI_FORMAT_R32G32B32_UINT,
    DXGI_FORMAT_R32G32B32_SINT,
    DXGI_FORMAT_R16G16B16A16_TYPELESS,
    DXGI_FORMAT_R16G16B16A16_FLOAT,
    DXGI_FORMAT_R16G16B16A16_UNORM,
    DXGI_FORMAT_R16G16B16A16_UINT,
    DXGI_FORMAT_R16G16B16A16_SNORM,
    DXGI_FORMAT_R16G16B16A16_SINT,
    DXGI_FORMAT_R32G32_TYPELESS,
    DXGI_FORMAT_R32G32_FLOAT,
    DXGI_FORMAT_R32G32_UINT,
    DXGI_FORMAT_R32G32_SINT,
    DXGI_FORMAT_R32G8X24_TYPELESS,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,
    DXGI_FORMAT_R10G10B10A2_TYPELESS,
    DXGI_FORMAT_R10G10B10A2_UNORM,
    DXGI_FORMAT_R10G10B10A2_UINT,
    DXGI_FORMAT_R11G11B10_FLOAT,
    DXGI_FORMAT_R8G8B8A8_TYPELESS,
    DXGI_FORMAT_R8G8B8A8_UNORM,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_R8G8B8A8_UINT,
    DXGI_FORMAT_R8G8B8A8_SNORM,
    DXGI_FORMAT_R8G8B8A8_SINT,
    DXGI_FORMAT_R16G16_TYPELESS,
    DXGI_FORMAT_R16G16_FLOAT,
    DXGI_FORMAT_R16G16_UNORM,
    DXGI_FORMAT_R16G16_UINT,
    DXGI_FORMAT_R16G16_SNORM,
    DXGI_FORMAT_R16G16_SINT,
    DXGI_FORMAT_R32_TYPELESS,
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_R32_FLOAT,
    DXGI_FORMAT_R32_UINT,
    DXGI_FORMAT_R32_SINT,
    DXGI_FORMAT_R24G8_TYPELESS,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT,
    DXGI_FORMAT_R8G8_TYPELESS,
    DXGI_FORMAT_R8G8_UNORM,
    DXGI_FORMAT_R8G8_UINT,
    DXGI_FORMAT_R8G8_SNORM,
    DXGI_FORMAT_R8G8_SINT,
    DXGI_FORMAT_R16_TYPELESS,
    DXGI_FORMAT_R16_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_R16_UNORM,
    DXGI_FORMAT_R16_UINT,
    DXGI_FORMAT_R16_SNORM,
    DXGI_FORMAT_R16_SINT,
    DXGI_FORMAT_R8_TYPELESS,
    DXGI_FORMAT_R8_UNORM,
    DXGI_FORMAT_R8_UINT,
    DXGI_FORMAT_R8_SNORM,
    DXGI_FORMAT_R8_SINT,
    DXGI_FORMAT_A8_UNORM,
    DXGI_FORMAT_R1_UNORM,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
    DXGI_FORMAT_R8G8_B8G8_UNORM,
    DXGI_FORMAT_G8R8_G8B8_UNORM,
    DXGI_FORMAT_BC1_TYPELESS,
    DXGI_FORMAT_BC1_UNORM,
    DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,
    DXGI_FORMAT_BC2_UNORM,
    DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,
    DXGI_FORMAT_BC3_UNORM,
    DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,
    DXGI_FORMAT_BC4_UNORM,
    DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,
    DXGI_FORMAT_BC5_UNORM,
    DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_B5G6R5_UNORM,
    DXGI_FORMAT_B5G5R5A1_UNORM,
    DXGI_FORMAT_B8G8R8A8_UNORM,
    DXGI_FORMAT_B8G8R8X8_UNORM,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
    DXGI_FORMAT_B8G8R8A8_TYPELESS,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8X8_TYPELESS,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB,
    DXGI_FORMAT_BC6H_TYPELESS,
    DXGI_FORMAT_BC6H_UF16,
    DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,
    DXGI_FORMAT_BC7_UNORM,
    DXGI_FORMAT_BC7_UNORM_SRGB,
    DXGI_FORMAT_AYUV,
    DXGI_FORMAT_Y410,
    DXGI_FORMAT_Y416,
    DXGI_FORMAT_NV12,
    DXGI_FORMAT_P010,
    DXGI_FORMAT_P016,
    DXGI_FORMAT_420_OPAQUE,
    DXGI_FORMAT_YUY2,
    DXGI_FORMAT_Y210,
    DXGI_FORMAT_Y216,
    DXGI_FORMAT_NV11,
    DXGI_FORMAT_AI44,
    DXGI_FORMAT_IA44,
    DXGI_FORMAT_P8,
    DXGI_FORMAT_A8P8,
    DXGI_FORMAT_B4G4R4A4_UNORM,
    DXGI_FORMAT_P208,
    DXGI_FORMAT_V208,
    DXGI_FORMAT_V408,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE,
    DXGI_FORMAT_FORCE_UINT
};

struct DXGIFormatInfo {
    rg::ColorFormat format;
    DXGI_FORMAT     dxgi;
    const char *    dxgiName;
};
static const DXGIFormatInfo dxgiFormats[] = {
#define D3D_FORMAT(format, d3d9, dxgi) { rg::ColorFormat::format(), DXGI_FORMAT_##dxgi, "DXGI_FORMAT_"#dxgi },
#include "d3d-format-meta.h"
#undef D3D_FORMAT
};

struct DX10Info {
    DXGI_FORMAT format;    // DXGI_FORMAT
    int32_t     dim;       // D3D10_RESOURCE_DIMENSION
    uint32_t    miscFlag;  // D3D10_RESOURCE_MISC_FLAG
    uint32_t    arraySize;
    uint32_t    reserved;
};

// ---------------------------------------------------------------------------------------------------------------------
/// return image face count
static uint32_t sGetImageFaceCount(const DDSFileHeader & header) {
    if ((DDS_DDSD_DEPTH   & header.flags) &&
        (DDS_CAPS_COMPLEX & header.caps)  &&
        (DDS_CAPS2_VOLUME & header.caps2)) {
        return 1; // volume texture
    } else if (DDS_CAPS_COMPLEX  & header.caps &&
               DDS_CAPS2_CUBEMAP & header.caps2 &&
               DDS_CAPS2_CUBEMAP_ALLFACES ==
               (header.caps2 & DDS_CAPS2_CUBEMAP_ALLFACES)) {
        return 6; // cubemap
    } else if (0 == (DDS_CAPS2_CUBEMAP & header.caps2) &&
               0 == (DDS_CAPS2_VOLUME & header.caps2)) {
        return 1; // 2D texture
    } else {
        RG_LOGE("Fail to detect image face count!");
        return 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
/// return image depth
static uint32_t sGetImageDepth( const DDSFileHeader & header ) {
    return DDS_DDSD_DEPTH & header.flags ? header.depth : 1;
}

// ---------------------------------------------------------------------------------------------------------------------
/// \brief return FMT_INVAID if falied
static rg::ColorFormat getImageFormat( const DDPixelFormat & ddpf ) {
    uint32_t flags = ddpf.flags;
    if( flags & DDS_DDPF_FOURCC ) flags = DDS_DDPF_FOURCC;

    bool fourcc = !!( flags & DDS_DDPF_FOURCC );
    bool bits   = !!( flags & ( DDS_DDPF_ALPHA
                          | DDS_DDPF_PALETTEINDEXED8
                          | DDS_DDPF_RGB
                          | DDS_DDPF_ZBUFFER
                          | DDS_DDPF_STENCILBUFFER
                          | DDS_DDPF_BUMPLUMINANCE
                          | DDS_DDPF_BUMPDUDV ) );
    bool r      = !!( flags & ( DDS_DDPF_RGB
                          | DDS_DDPF_STENCILBUFFER
                          | DDS_DDPF_LUMINANCE
                          | DDS_DDPF_BUMPLUMINANCE
                          | DDS_DDPF_BUMPDUDV) );
    bool g      = !!( flags & ( DDS_DDPF_RGB
                          | DDS_DDPF_ZBUFFER
                          | DDS_DDPF_STENCILBUFFER
                          | DDS_DDPF_BUMPLUMINANCE
                          | DDS_DDPF_BUMPDUDV) );
    bool b      = !!( flags & ( DDS_DDPF_RGB
                          | DDS_DDPF_STENCILBUFFER
                          | DDS_DDPF_BUMPLUMINANCE
                          | DDS_DDPF_BUMPDUDV ) );
    bool a      = !!( flags & ( DDS_DDPF_ALPHAPIXELS
                          | DDS_DDPF_ALPHA
                          | DDS_DDPF_BUMPDUDV) );

    for( uint32_t i = 0;
         i < sizeof(s_ddpfDescTable)/sizeof(s_ddpfDescTable[0]);
         ++i ) {
        const DdpfDesc & desc = s_ddpfDescTable[i];
        if( DDS_DDPF_SIZE != ddpf.size ) continue;
        if( flags != desc.ddpf.flags ) continue;
        if( fourcc && ddpf.fourcc != desc.ddpf.fourcc ) continue;
        if( bits && ddpf.bits != desc.ddpf.bits ) continue;
        if( r && ddpf.rMask != desc.ddpf.rMask ) continue;
        if( g && ddpf.gMask != desc.ddpf.gMask ) continue;
        if( b && ddpf.bMask != desc.ddpf.bMask ) continue;
        if( a && ddpf.aMask != desc.ddpf.aMask ) continue;

        // found!
        return desc.clrfmt;
    }

    // failed
    RG_LOGE( "unknown DDS format!" );
    return rg::ColorFormat::UNKNOWN();
}

// ---------------------------------------------------------------------------------------------------------------------
//
static rg::ColorFormat DXGIFormat2ColorFormat(DXGI_FORMAT dxgi) {
    for(const auto & i : dxgiFormats) {
        if (i.dxgi == dxgi) {
            if (i.format) {
                return i.format;
            } else {
                RG_LOGE( "unsupported DXGI format: %s", i.dxgiName);
                return rg::ColorFormat::UNKNOWN();
            }
        }
    }
    RG_LOGE( "invalid DXGI format: %d", dxgi);
    return rg::ColorFormat::UNKNOWN();
}

// *********************************************************************************************************************
// DDSReader public functions
// *********************************************************************************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
bool DDSReader::checkFormat() {
    uint32_t u32;
    if (!_file.read((char*)&u32, 4) || 4 != _file.gcount()) return false;
    return u32 == MAKE_FOURCC('D', 'D', 'S', ' ');
}

// ---------------------------------------------------------------------------------------------------------------------
//
const rg::ImageDesc & DDSReader::readHeader() {

    _imgDesc = {};

    // read header
    if (!_file.read((char*)&_header, sizeof(_header)) || _file.gcount() != sizeof(_header)) {
        RG_LOGE( "fail to read DDS file header!" );
        return _imgDesc;
    }

    // validate header flags
    uint32_t required_flags = DDS_DDSD_WIDTH | DDS_DDSD_HEIGHT;
    if( required_flags != (required_flags & _header.flags) )
    {
        RG_LOGE( "damage DDS header!" );
        return _imgDesc;
    }

    if( DDS_DDPF_PALETTEINDEXED8 & _header.ddpf.flags )
    {
        RG_LOGE( "do not support palette format!" );
        return _imgDesc;
    }

    // get image format
    if( MAKE_FOURCC('D','X','1','0') == _header.ddpf.fourcc )
    {
        // read DX10 info
        DX10Info dx10;
        if (!_file.read((char*)&dx10, sizeof(dx10)) || _file.gcount() != sizeof(dx10)) {
            RG_LOGE( "fail to read DX10 info header!" );
            return _imgDesc;
        }

        _originalFormat = DXGIFormat2ColorFormat(dx10.format);
        if(rg::ColorFormat::UNKNOWN() == _originalFormat) return _imgDesc;
    }
    else
    {
        _originalFormat = getImageFormat( _header.ddpf );
        if( rg::ColorFormat::UNKNOWN() == _originalFormat ) return _imgDesc;
    }

    // BGR format is not compatible with D3D10/D3D11 hardware. So we need to convert it to RGB format.
    // sUpdateSwizzle() will update format swizzle from BGR to RGB. And we'll do data convertion later
    // in readImage() function.
    _formatConversion = sCheckFormatConversion(_originalFormat);

    // grok image dimension
    uint32_t faces  = sGetImageFaceCount( _header );
    if (0 == faces) return _imgDesc;
    uint32_t width  = _header.width;
    uint32_t height = _header.height;
    uint32_t depth  = sGetImageDepth( _header );

    // grok miplevel information
    bool hasMipmap = ( DDS_DDSD_MIPMAPCOUNT & _header.flags )
                  && ( DDS_CAPS_MIPMAP & _header.caps )
                  && ( DDS_CAPS_COMPLEX & _header.caps );
    uint32_t levels = hasMipmap ? _header.mipCount : 1;
    if( 0 == levels ) levels = 1;

    // Create image descriptor. Here we assume that the offset of each mipmap layer calculated by the image descriptor
    // completely matches the actual data offset in DDS file.
    _imgDesc = rg::ImageDesc(rg::ImagePlaneDesc::make(_originalFormat, width, height, depth), faces, levels);
    RG_ASSERT( _imgDesc.valid() );

    // success
    return _imgDesc;
}

// ---------------------------------------------------------------------------------------------------------------------
//
bool DDSReader::readPixels(void * o_data, size_t o_size) const {
    if (!o_data) {
        RG_LOGE("null output buffer.");
        return false;
    }
    if (o_size < _imgDesc.size) {
        RG_LOGE("output buffer size is not large enough");
        return false;
    }

    if (!_file.read((char*)o_data, (int)_imgDesc.size) || _file.gcount() != (int)_imgDesc.size) {
        RG_LOGE("failed to read DDS pixels.");
        return false;
    }

    // Do format conversion, if needed.
    if( FC_NONE != _formatConversion )
    {
        // There might be gaps between each mipmap level, or even between each scan line.
        // But we assume that the data in the gaps are not important and could be converted
        // as well without any side effects. We also assume that each scan line always starts
        // from pixel size aligned address (4 bytes aligned for 8888 format) regardless of gaps.
        sConvertFormat( _formatConversion, o_data, _imgDesc.size );
    }

    // success
    return true;
}

// *********************************************************************************************************************
// DDSReader private functions
// *********************************************************************************************************************

// ---------------------------------------------------------------------------------------------------------------------
//
DDSReader::FormatConversion
DDSReader::sCheckFormatConversion(rg::ColorFormat format) {
    if (rg::ColorFormat::LAYOUT_8_8_8_8 == format.layout &&
        rg::ColorFormat::SWIZZLE_B == format.swizzle0 &&
        rg::ColorFormat::SWIZZLE_G == format.swizzle1 &&
        rg::ColorFormat::SWIZZLE_R == format.swizzle2) {
        format.swizzle0 = rg::ColorFormat::SWIZZLE_R;
        format.swizzle1 = rg::ColorFormat::SWIZZLE_G;
        format.swizzle2 = rg::ColorFormat::SWIZZLE_B;
        format.swizzle3 = rg::ColorFormat::SWIZZLE_A;
        return FC_BGRA8888_TO_RGBA8888;
    } else {
        return FC_NONE;
    }
}

// ---------------------------------------------------------------------------------------------------------------------
//
void DDSReader::sConvertFormat(FormatConversion fc, void * data, size_t size ) {
    if (FC_BGRA8888_TO_RGBA8888 == fc) {
        size_t   numPixels = size / 4;
        uint32_t * pixels  = (uint32_t*)data;
        uint32_t * end     = pixels + numPixels;
        for( ; pixels < end; ++pixels ) {
            uint32_t a = (*pixels) & 0xFF000000;
            uint32_t r = (*pixels) & 0xFF0000;
            uint32_t g = (*pixels) & 0xFF00;
            uint32_t b = (*pixels) & 0xFF;
            *pixels = a | (r>>16) | g | (b<<16);
        }
    }
}
