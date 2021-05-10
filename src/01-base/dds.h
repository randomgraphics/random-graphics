#pragma once
#include <rg/base.h>
#include <iostream>

///
/// DD pixel format
///
struct DDPixelFormat {
    uint32_t size;   ///< size of this structure
    uint32_t flags;  ///< pixel format flags
    uint32_t fourcc; ///< fourcc
    uint32_t bits;   ///< bits of the format
    uint32_t rMask;  ///< R, Y
    uint32_t gMask;  ///< G, U
    uint32_t bMask;  ///< B, V
    uint32_t aMask;  ///< A, A
};

///
/// DDS file header
///
struct DDSFileHeader {
    /// \cond NEVER
    uint32_t      size;
    uint32_t      flags;
    uint32_t      height;
    uint32_t      width;
    uint32_t      pitchOrLinearSize; // The number of bytes per scan line in an uncompressed texture; the total number of bytes in the top level texture for a compressed texture. The pitch must be DWORD aligned.
    uint32_t      depth;
    uint32_t      mipCount;
    uint32_t      reserved[11];
    DDPixelFormat ddpf;
    uint32_t      caps;
    uint32_t      caps2;
    uint32_t      caps3;
    uint32_t      caps4;
    uint32_t      reserved2;
    /// \endcond
};
static_assert(sizeof(DDSFileHeader) == 124);

///
/// dds image reader
///
class DDSReader {
    std::istream & _file;
    DDSFileHeader  _header;
    rg::ImageDesc  _imgDesc;

    enum FormatConversion {
        FC_NONE,
        FC_BGRA8888_TO_RGBA8888,
    };

    rg::ColorFormat   _originalFormat;
    FormatConversion  _formatConversion;

    static FormatConversion sCheckFormatConversion(rg::ColorFormat);
    static void sConvertFormat(FormatConversion fc, void * data, size_t size);

public:

    ///
    /// Constructor
    ///
    DDSReader(std::istream & f) : _file(f), _formatConversion(FC_NONE) {}

    ///
    /// Destructor
    ///
    ~DDSReader() = default;

    ///
    /// Check file format. Return true if the file is DDS file
    ///
    bool checkFormat();

    ///
    /// Read DDS header
    ///
    const rg::ImageDesc & readHeader();

    ///
    /// Read DDS image
    ///
    bool readPixels(void * buf, size_t size) const;
};
