/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for regular volume scene elements

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_SCENE_ELEMENT_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>
#include <nv/index/islice_scene_element.h>

namespace nv
{

namespace index
{

/// The abstract interface class representing a regular volume scene element.
///
/// Each regular volume is a scene element as part of the scene description, i.e.,
/// in \c IScene. These regular volume scene elements are an abstract representation of the
/// 'physical' volume data uploaded to the cluster. They contain generic details
/// about the volumes, but do not store the voxel data, which is handled by \c
/// IRegular_volume_brick_data.
///
/// Each regular volume has a <em>region of interest</em> which defines the part of the
/// volume data considered for rendering. It is defined as a bounding box in local 3D
/// space by calling \c set_IJK_region_of_interest(). If no region of interest is set, the entire
/// volume is rendered. The region of interest is used by \c get_XYZ_clipped_bounding_box()
/// to clip the scene element bounding box before transforming it from local IJK to global XYZ
/// space. In contrast, \c get_XYZ_bounding_box() returns the unclipped scene element bounding box.
///
/// Slices (i.e., an \c ISection_scene_element or \c IVertical_profile_scene_element) must be
/// assigned to a specific regular volume as part of the scene description, hence there are
/// methods for creating, storing and accessing them.
/// @ingroup nv_index_scene_description_shape
///
class IRegular_volume : public mi::base::Interface_declare<0x761c9e3b, 0x255e, 0x442c, 0xbd, 0x36,
                          0xe9, 0xeb, 0x0d, 0xe9, 0x83, 0x57, nv::index::IDistributed_data>
{
public:
  /// Sets the user-defined name of the scene element.
  ///
  /// This can be useful to distinguish several scene elements of the same type.
  /// However, setting a name is optional.
  ///
  /// \param[in] name User-defined name. This should not be 0.
  virtual void set_name(const char* name) = 0;

  /// Returns the user-defined name of the scene element.
  /// \return User-defined name. An empty string, if no name has been set.
  virtual const char* get_name() const = 0;

  /// Returns the transformation matrix of the scene element.
  ///
  /// \return Transformation matrix from IJK (local) to XYZ (global) space.
  virtual mi::math::Matrix_struct<mi::Float32, 4, 4> get_transform() const = 0;

  /// Returns the size of the entire (unclipped) volume.
  ///
  /// \return The size of the volume in each dimension.
  virtual mi::math::Vector_struct<mi::Uint32, 3> get_volume_size() const = 0;

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Bounding box and region of interest
  ///@{

  /// Returns the IJK (local) bounding box of the entire (unclipped) volume.
  /// Since no clipping from the region of interest is applied, the \c min vector
  /// of the result will always be zero.
  ///
  /// \return IJK bounding box
  virtual mi::math::Bbox_struct<mi::Uint32, 3> get_IJK_bounding_box() const = 0;

  /// Sets the region of interest of the volume in IJK (local) coordinates.
  ///
  /// \param[in] ijk_roi_bbox Region of interest
  virtual void set_IJK_region_of_interest(
    const mi::math::Bbox_struct<mi::Float32, 3>& ijk_roi_bbox) = 0;

  /// Returns the region of interest of the volume in IJK (local) coordinates.
  ///
  /// \return IJK region of interest
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_IJK_region_of_interest() const = 0;

  /// Returns the XYZ (global) bounding box of the entire (unclipped) volume.
  /// It is calculated by transforming the IJK bounding box to XYZ according to the
  /// current transformation matrix of the scene element.
  ///
  /// \return XYZ bounding box
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_XYZ_bounding_box() const = 0;

  /// Returns the XYZ (global) bounding box clipped against the region of interest
  /// of the volume. It is calculated by first clipping and then transforming the
  /// IJK bounding box to XYZ according to the current transformation matrix of the
  /// scene element.
  ///
  /// \return XYZ bounding box
  virtual mi::math::Bbox_struct<mi::Float32, 3> get_XYZ_clipped_bounding_box() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Slices
  /// Section slices or vertical profiles can be assigned to a volume scene element.
  ///@{

  /// Creates and adds a new section slice to the scene element.
  /// This will create a new \c ISection_scene_element instance and store it in the database.
  ///
  /// \param[in] dice_transaction DiCE transaction to use
  /// \param[in] orientation      Slice orientation
  /// \param[in] position         Slice position
  /// \param[in] colormap         Slice colormap tag
  /// \param[in] enabled          True when rendering should be enabled
  ///
  /// \return Tag of the stored \c ISection_scene_element
  virtual mi::neuraylib::Tag_struct add_section(mi::neuraylib::IDice_transaction* dice_transaction,
    ISection_scene_element::Slice_orientation orientation, mi::Float32 position,
    mi::neuraylib::Tag_struct colormap, bool enabled = true) = 0;

  /// Creates and adds a new vertical profile slice to the scene element.
  /// This will create a new \c IVertical_profile_scene_element instance and store it
  /// in the database.
  ///
  /// \param[in] dice_transaction DiCE transaction to use
  /// \param[in] vertices         Array of 2D vertices defining the profile
  /// \param[in] nb_vertices      Number of vertices in the array
  /// \param[in] colormap         Colormap for vertical profile
  /// \param[in] enabled          True when rendering should be enabled
  ///
  /// \return Tag of the stored \c IVertical_profile_scene_element
  virtual mi::neuraylib::Tag_struct add_vertical_profile(
    mi::neuraylib::IDice_transaction* dice_transaction,
    const mi::math::Vector_struct<mi::Float32, 2>* vertices, mi::Uint32 nb_vertices,
    mi::neuraylib::Tag_struct colormap, bool enabled = true) = 0;

  /// Adds an existing section or vertical profile slice to the scene element.
  ///
  /// \param[in] tag Tag of an \c ISection_scene_element or \c IVertical_profile_scene_element.
  ///            If the given slice was already added to this scene element, the call will be
  ///            ignored.
  virtual void add_slice(mi::neuraylib::Tag_struct tag) = 0;

  /// Removes a section or vertical profile slice from the scene element.
  ///
  /// \param[in] tag Tag of an \c ISection_scene_element or \c IVertical_profile_scene_element.
  ///            If the given slice is not stored in this scene element, the call will be
  ///            ignored.
  virtual void remove_slice(mi::neuraylib::Tag_struct tag) = 0;

  /// Returns the number of section or vertical profile slices stored in the scene element.
  /// \return number of slices
  virtual mi::Uint32 get_nb_slices() const = 0;

  /// Returns the tag of the given section or vertical profile slice stored in the scene element.
  ///
  /// \param[in] index Index of the slice, must be in [0, #get_nb_slices()-1].
  ///
  /// \return The tag of the \c ISection_scene_element or \c IVertical_profile_scene_element. If the
  ///         index is out of range, \c NULL_TAG is returned.
  virtual mi::neuraylib::Tag_struct get_slice(mi::Uint32 index) const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Colormap
  ///@{

  /// Assigns a colormap for rendering.
  ///
  /// The colormap will be used to assign color and opacity to volume amplitude values during
  /// rendering.
  ///
  /// \param[in] colormap_tag Tag of an \c IColormap
  virtual void assign_colormap(mi::neuraylib::Tag_struct colormap_tag) = 0;

  /// Returns the tag of the currently assigned colormap.
  ///
  /// \return Tag of an \c IColormap or \c NULL_TAG
  virtual mi::neuraylib::Tag_struct assigned_colormap() const = 0;
  ///@}

  ///////////////////////////////////////////////////////////////////////////////////
  /// \name Handling of 'invalid' values
  /// A certain amplitude value can be defined as meaning 'invalid'.
  ///@{

  /// Controls whether there is a special 'invalid' value in the data.
  /// When this is active, the amplitude value set by \c set_invalid_value() will be
  /// considered as marking an 'invalid' value, which than can be handled differently
  /// than 'normal' values. What actually happens to 'invalid' values is
  /// implementation-specific to the used rendering or computation method.
  /// \todo This is currently not used
  ///
  /// \param[in] has_invalid True when 'invalid' amplitude values should be handled
  virtual void has_invalid_value(bool has_invalid) = 0;

  /// Returns true when a special 'invalid' value has been set.
  /// \todo This is currently not used
  /// \return True when invalid value has beend set
  virtual bool has_invalid_value() const = 0;

  /// Set the 'invalid' value.
  /// This setting will only be considered when \c has_invalid_value() was activated.
  /// \todo This is currently not used
  ///
  /// \param[in] invalid_value Amplitude value to be considered 'invalid'
  virtual void set_invalid_value(mi::Uint8 invalid_value) = 0;

  /// Returns currently set 'invalid' value.
  /// \todo This is currently not used
  ///
  /// \return Amplitude value to be considered 'invalid'
  virtual mi::Uint8 get_invalid_value() const = 0;
  ///@}
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_SCENE_ELEMENT_H
