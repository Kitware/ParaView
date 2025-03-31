/***************************************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for CUDA canvases

#ifndef MI_NEURAYLIB_ICANVAS_CUDA_H
#define MI_NEURAYLIB_ICANVAS_CUDA_H

#include <mi/neuraylib/icanvas.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_rendering
@{
*/

/// Abstract interface for a canvas with CUDA device memory storage.
///
/// This interface is similar to a #mi::neuraylib::ICanvas interface with only a single tile,
/// except that all memory resides in CUDA device memory for a single CUDA device identified by
/// its CUDA device id.
class ICanvas_cuda : public
    mi::base::Interface_declare<0x211963f4,0x31c1,0x4583,0x81,0x4b,0x5,0x24,0x69,0xa8,0xc,0x27,
                                neuraylib::ICanvas_base>
{
public:
    /// Returns the CUDA device id that owns the device memory storage for this canvas.
    virtual Sint32 get_cuda_device_id() const = 0;

    /// Returns the number of pixels in x direction
    virtual Uint32 get_resolution_x() const = 0;

    /// Returns the number of pixels in y direction
    virtual Uint32 get_resolution_y() const = 0;

    /// Returns a pointer to the raw pixel data according to the pixel type of the canvas.
    ///
    /// This methods is used for fast, direct read access to the raw data. It is expected that
    /// the data is stored in row-major layout without any padding. In case of #mi::Color, the
    /// components are expected to be stored in RGBA order.
    ///
    /// The total size of the buffer in bytes is \code x * y * bpp \endcode where \c x is the result
    /// of #get_resolution_x(), \c y is the result of #get_resolution_y(), and \c bpp is the number
    /// of bytes per pixel. \ifnot MDL_SDK_API The number of bytes per pixel is the product of
    /// #mi::neuraylib::IImage_api::get_components_per_pixel() and
    /// #mi::neuraylib::IImage_api::get_bytes_per_component() when passing the result of #get_type()
    /// as pixel type. \endif
    ///
    /// \param layer     The layer of the pixel in the canvas.
    virtual const void* get_data(Uint32 layer = 0) const = 0;

    /// Returns a pointer to the raw pixel data according to the pixel type of the canvas.
    ///
    /// This methods is used for fast, direct write access to the raw data. It is expected that
    /// the data is stored in row-major layout without any padding. In case of #mi::Color, the
    /// components are expected to be stored in RGBA order.
    ///
    /// The total size of the buffer in bytes is \code x * y * bpp \endcode where \c x is the result
    /// of #get_resolution_x(), \c y is the result of #get_resolution_y(), and \c bpp is the number
    /// of bytes per pixel. \ifnot MDL_SDK_API The number of bytes per pixel is the product of
    /// #mi::neuraylib::IImage_api::get_components_per_pixel() and
    /// #mi::neuraylib::IImage_api::get_bytes_per_component() when passing the result of #get_type()
    /// as pixel type. \endif
    ///
    /// \param layer     The layer of the pixel in the canvas.
    virtual void* get_data(Uint32 layer = 0) = 0;
};

/**@}*/ // end group mi_neuray_rendering

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ICANVAS_CUDA_H
