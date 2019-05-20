/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute representing a texture element

#ifndef NVIDIA_INDEX_ITEXTURE_H
#define NVIDIA_INDEX_ITEXTURE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_attribute

/// A base class that defines the properties of a texture of resolution [x,y]
/// for a given pixel format.  The origin of the texture coordinate system is
/// defined to be the lower-left corner.  \note The texture is only used in
/// combination with an IIcon_2D.
///
class ITexture : public mi::base::Interface_declare<0x633c679, 0x5eb7, 0x4e0d, 0x9e, 0xab, 0xdc,
                   0x6e, 0xed, 0x87, 0x56, 0x38, nv::index::IAttribute>
{
public:
  /// Get the horizontal resolution of the texture.
  ///
  /// \return         Returns the texture's x resolution.
  ///
  virtual mi::Uint32 get_resolution_x() const = 0;

  /// Get the vertical resolution of the texture.
  ///
  /// \return         Returns the texture's y resolution.
  ///
  virtual mi::Uint32 get_resolution_y() const = 0;

  /// Pixel format
  enum Pixel_format
  {
    /// RGBA color, every component unsigned 8-bit integer
    RGBA_UINT8 = 0,
    /// RGBA color, every component a 32-bit IEEE-754 single-precision
    /// normalized floating-point number
    RGBA_FLOAT32 = 1
  };

  /// Get the pixel format of the texture.
  ///
  /// \return         Returns the pixel format.
  ///
  virtual Pixel_format get_pixel_format() const = 0;

  /// Set the texture data. It resets the texture with a pointer to a raw
  /// texture data according to the pixel format.
  ///
  /// \param[in] pixel_data  A pointer to the raw pixel data
  ///
  /// \param[in] width       The horizontal resolution of the texture
  ///
  /// \param[in] height      The vertical resolution of the texture
  ///
  /// \param[in] format      The pixel format of the texture:
  ///                        RGBA_UINT8 (default) or RGBA_FLOAT32
  ///
  /// The total size of the buffer in bytes is \code x * y * sizeof(t)
  /// \endcode where \c x is the result of #get_resolution_x(), \c y is the
  /// result of #get_resolution_y(), and \c t is the size of type as returned
  /// by #get_pixel_format().
  ///
  /// \return  pixel data.
  ///
  virtual void set_pixel_data(
    const void* pixel_data, mi::Uint32 width, mi::Uint32 height, Pixel_format format) = 0;

  /// Get the texture data. It returns a pointer to the raw texture data
  /// according to the pixel format.
  ///
  /// The total size of the buffer in bytes is \code x * y * sizeof(t)
  /// \endcode where \c x is the result of #get_resolution_x(), \c y is the
  /// result of #get_resolution_y(), and \c t is the size of type as returned
  /// by #get_pixel_format().
  ///
  /// \return  Returns the raw pointer to the pixel data.
  ///
  virtual const void* get_pixel_data() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ITEXTURE_H
