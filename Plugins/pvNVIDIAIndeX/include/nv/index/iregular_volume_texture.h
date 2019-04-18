/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute for texturing geometric primitives using a regular
///        volume dataset.

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_TEXTURE_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_TEXTURE_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// The interface class representing a regular volume texture attribute.
///
class IRegular_volume_texture : public mi::base::Interface_declare<0xcffdcde9, 0x3f88, 0x4147, 0xa4,
                                  0xfa, 0x4b, 0xc9, 0xe2, 0x63, 0xb6, 0x53, nv::index::IAttribute>
{
public:
  /// Defines how a texture access outside of the volume boundary is handled.
  enum Volume_boundary_mode
  {
    /// A texture access outside of the volume boundary will be considered fully transparent
    /// and therefore be ignored for rendering
    CLAMP_TO_TRANSPARENT = 0,
    /// A texture access outside of the volume boundary will return fully opaque white, i.e.
    /// RGBA [1.0, 1.0, 1.0, 1.0], which may be further modulated by a material property.
    CLAMP_TO_OPAQUE = 1,
  };

  /// Sets the reference to the regular volume scene element to be used as the texturing source.
  ///
  /// \param[in] elem_tag     Tag of an IRegular_volume scene element.
  ///
  virtual void set_volume_element(mi::neuraylib::Tag_struct elem_tag) = 0;

  /// Returns the reference to the regular volume scene element.
  ///
  /// \return Tag of an IRegular_volume scene element.
  ///
  virtual mi::neuraylib::Tag_struct get_volume_element() const = 0;

  /// Sets the mode for handling textures accesses outside of the volume boundary.
  ///
  /// \param[in] mode Volume boundary mode
  virtual void set_volume_boundary_mode(Volume_boundary_mode mode) = 0;

  /// Returns the mode for handling texture accesses outside of the volume boundary.
  ///
  /// \return Current volume boundary mode
  virtual Volume_boundary_mode get_volume_boundary_mode() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_TEXTURE_H
