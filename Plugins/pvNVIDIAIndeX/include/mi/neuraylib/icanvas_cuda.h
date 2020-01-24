/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for CUDA canvases

#ifndef MI_NEURAYLIB_ICANVAS_CUDA_H
#define MI_NEURAYLIB_ICANVAS_CUDA_H

#include <mi/neuraylib/icanvas.h>

namespace mi {

namespace neuraylib {

class ITile_cuda;

/** \addtogroup mi_neuray_rendering
@{
*/

/// Abstract interface for a canvas with CUDA device memory storage.
///
/// The provided interface is similar to the #mi::neuraylib::ICanvas interface except that all
/// memory resides in CUDA device memory for a single CUDA device identified by its CUDA device
/// id. Using this interface instead of #mi::neuraylib::ICanvas allows the result of the rendering
/// operation to remain on the GPU instead of transferring it back to main memory.
/// From there it can be processed further, or be downloaded to main memory for other operations.
class ICanvas_cuda : public
    mi::base::Interface_declare<0x211963f4,0x31c1,0x4583,0x81,0x4b,0x5,0x24,0x69,0xa8,0xc,0x27,
                                neuraylib::ICanvas_base>
{
public:
    /// Returns the CUDA device id that owns the device memory storage for this canvas
    /// and its tiles.
    virtual Sint32 get_cuda_device_id() const = 0;

    /// Returns the tile size in x direction.
    virtual Uint32 get_tile_resolution_x() const = 0;

    /// Returns the tile size in y direction.
    virtual Uint32 get_tile_resolution_y() const = 0;

    /// Returns the number of tiles in x direction.
    virtual Uint32 get_tiles_size_x() const = 0;

    /// Returns the number of tiles in y direction.
    virtual Uint32 get_tiles_size_y() const = 0;

    /// Returns the tile which contains a given pixel.
    ///
    /// \param pixel_x   The x coordinate of pixel with respect to the canvas.
    /// \param pixel_y   The y coordinate of pixel with respect to the canvas.
    /// \param layer     The layer of the pixel in the canvas.
    /// \return          The tile that contains the pixel, or \c NULL in case of invalid
    ///                  parameters.
    virtual const ITile_cuda* get_tile( Uint32 pixel_x, Uint32 pixel_y, Uint32 layer = 0) const = 0;

    /// Returns the tile which contains a given pixel.
    ///
    /// \param pixel_x   The x coordinate of pixel with respect to the canvas.
    /// \param pixel_y   The y coordinate of pixel with respect to the canvas.
    /// \param layer     The layer of the pixel in the canvas.
    /// \return          The tile that contains the pixel, or \c NULL in case of invalid
    ///                  parameters.
    virtual ITile_cuda* get_tile( Uint32 pixel_x, Uint32 pixel_y, Uint32 layer = 0) = 0;
};

/*@}*/ // end group mi_neuray_rendering

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ICANVAS_CUDA_H
