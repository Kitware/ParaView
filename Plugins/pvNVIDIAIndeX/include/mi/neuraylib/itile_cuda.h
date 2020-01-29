/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for CUDA tiles

#ifndef MI_NEURAYLIB_ITILE_CUDA_H
#define MI_NEURAYLIB_ITILE_CUDA_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

/** \if IRAY_API \addtogroup mi_neuray_rendering
    \elseif DICE_API \addtogroup mi_neuray_rtmp
    \else \addtogroup mi_neuray_misc
    \endif
@{
*/

/// Abstract interface for a CUDA based tile.
///
/// A tile is a rectangular array of pixels of a certain pixel type.
///
/// This interface is similar to #mi::neuraylib::ITile, however the tile resides
/// in CUDA memory and get_pixel and set_pixel operations are not supported.
class ITile_cuda : public
    mi::base::Interface_declare<0x475f7cb9,0xaac8,0x4aff,0x80,0x23,0x9d,0x89,0x76,0x08,0xa5,0x0e>
{
public:
    /// Returns the pixel type used by the CUDA tile.
    ///
    /// \see \ref mi_neuray_types for a list of supported pixel types
    virtual const char* get_type() const = 0;

    /// Returns the tile size in x direction
    virtual Uint32 get_resolution_x() const = 0;

    /// Returns the tile size in y direction
    virtual Uint32 get_resolution_y() const = 0;

    /// Returns a pointer to the raw tile data according to the pixel type of the tile.
    /// The memory is owned by a CUDA device that is specified by the ICanvas_cuda.
    ///
    /// This methods is used for fast, direct read access to the raw tile data. It is expected that
    /// the data is stored in row-major layout without any padding. In case of #mi::Color, the
    /// components are expected to be stored in RGBA order.
    ///
    /// The total size of the buffer in bytes is \code x * y * bpp \endcode where \c x is the result
    /// of #get_resolution_x(), \c y is the result of #get_resolution_y(), and \c bpp is the number
    /// of bytes per pixel. \ifnot MDL_SDK_API The number of bytes per pixel is the product of
    /// #mi::neuraylib::IImage_api::get_components_per_pixel() and
    /// #mi::neuraylib::IImage_api::get_bytes_per_component() when passing the result of #get_type()
    /// as pixel type. \endif
    virtual const void* get_data() const = 0;

    /// Returns a pointer to the raw tile data according to the pixel type of the tile.
    /// The memory is owned by a CUDA device that is specified by the ICanvas_cuda.
    ///
    /// This methods is used for fast, direct write access to the raw tile data. It is expected that
    /// the data is stored in row-major layout without any padding. In case of #mi::Color, the
    /// components are expected to be stored in RGBA order.
    ///
    /// The total size of the buffer in bytes is \code x * y * bpp \endcode where \c x is the result
    /// of #get_resolution_x(), \c y is the result of #get_resolution_y(), and \c bpp is the number
    /// of bytes per pixel. \ifnot MDL_SDK_API The number of bytes per pixel is the product of
    /// #mi::neuraylib::IImage_api::get_components_per_pixel() and
    /// #mi::neuraylib::IImage_api::get_bytes_per_component() when passing the result of #get_type()
    /// as pixel type. \endif
    virtual void* get_data() = 0;
};

/*@}*/ // end group mi_neuray_rendering

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ITILE_CUDA_H
