/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene elements representing object space and image space labels.

#ifndef NVIDIA_INDEX_ILABEL_SHAPES_H
#define NVIDIA_INDEX_ILABEL_SHAPES_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>
#include <nv/index/ifont.h>
#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// A label layout defined a label's appearance including, for instance, the padding around the test
/// and
/// the foreground and background colors.
///
class ILabel_layout : public mi::base::Interface_declare<0x408633d3, 0xbbe1, 0x4e01, 0xa1, 0x27,
                        0xb2, 0x2c, 0xa3, 0x44, 0x5c, 0x55, nv::index::IAttribute>
{
public:
  /// Set the padding around the label text.
  ///
  /// \param[in]      padding The padding in label space.
  ///                 The size is interpreted by in which space the label defined.
  ///
  virtual void set_padding(mi::Float32 padding) = 0;

  /// Get the padding around the label text.
  ///
  /// \return         The padding in label space.
  ///
  virtual mi::Float32 get_padding() const = 0;

  /// Set/Clear the auto flip mode. When setting auto flip mode the text of the label will be flip
  /// when seen from back face so it always read from left to right.
  ///
  /// \param[in]      auto_flip set to true/false to enable/disable auto flip mode.
  ///
  virtual void set_auto_flip(bool auto_flip) = 0;

  /// Returns the state of the auto flip mode. When setting auto flip mode the text of the label
  /// will be flip when seen from back face so it always read from left to right.
  ///
  /// \return         the state of the auto flip mode.
  ///
  virtual bool get_auto_flip() const = 0;

  /// A label defines it foreground color, i.e., the color of the text shown on the label, and
  /// its background color, i.e., the color that surrounds the label.
  ///
  /// \param[out]     foreground      Returns the foreground color.
  ///
  /// \param[out]     background      Returns the background color.
  ///
  virtual void get_color(
    mi::math::Color_struct& foreground, mi::math::Color_struct& background) const = 0;

  /// A label defines it foreground color, i.e., the color of the text shown on the label, and
  /// its background color, i.e., the color that surrounds the label.
  ///
  /// \param[in]      foreground      The foreground color.
  ///
  /// \param[in]      background      The background color.
  ///
  virtual void set_color(
    const mi::math::Color_struct& foreground, const mi::math::Color_struct& background) = 0;
};

/// @ingroup nv_index_scene_description_shape
///
/// A label defined in object space. The orientation of the label is affected by the transformations
/// applied
/// by the scene description.
/// The dimension (position, height, ...) is defined in the 3D object space.
///
class ILabel_3D : public mi::base::Interface_declare<0x4967ddd0, 0xbc19, 0x45d4, 0x88, 0x4d, 0x82,
                    0xfa, 0x68, 0xd0, 0x13, 0x35, nv::index::IObject_space_shape>
{
public:
  /// Get the text of the label.
  ///
  /// \return     The text of the label.
  ///
  virtual const char* get_text() const = 0;
  /// The Set text of the label.
  ///
  /// \param      text        The text of the label.
  ///
  virtual void set_text(const char* text) = 0;

  /// The position and orientation in object space is defined by
  /// an anchor point that lies inside plane that the label lies in and defines the label's lower
  /// left corner,
  /// the normal vector perpendicular to the label, and the up vector.
  ///
  /// \param[in] position         The position defines the lower left
  ///                             corner of the label.
  ///
  /// \param[in] right_vector     The right vector of the label
  ///                             together with the up vector defines the
  ///                             orientation of the label in 3D space.
  ///
  /// \param[in] up_vector        The up vector of the label together with the
  ///                             right vector defines the orientation
  ///                             of the label in 3D space.
  ///
  /// \param[in] height           The height of the label along the up vector
  ///                             in local space. The size is defined in the 3D object space.
  ///
  /// \param[in] width            The width of the label along the right vector
  ///                             in local space. If the width is not set (or set to a negative
  ///                             value)
  ///                             then the width of the label will be computed automatically by the
  ///                             rendering system. The size is defined in the 3D object space.
  ///
  virtual void set_geometry(const mi::math::Vector_struct<mi::Float32, 3>& position,
    const mi::math::Vector_struct<mi::Float32, 3>& right_vector,
    const mi::math::Vector_struct<mi::Float32, 3>& up_vector, mi::Float32 height,
    mi::Float32 width = -1.f) = 0;

  /// The position and orientation in object space is defined by
  /// an anchor point that lies inside plane the label lies in and defines the label's lower left
  /// corner,
  /// the right and up vectors to the label, and the up vector.
  /// The direction that the normal points to  defines the front face
  /// of the label that displays the text.
  ///
  /// \param[out] position        The position defines the lower left
  ///                             corner of the label.
  ///
  /// \param[out] right_vector    The right vector of the label
  ///                             together with the up vector defines the
  ///                             orientation of the label in 3D space.
  ///
  /// \param[out] up_vector       The up vector of the label together with the
  ///                             right vector defines the orientation
  ///                             of the label in 3D space.
  ///
  /// \param[in] height           The height of the label along the up vector
  ///                             in local space. The size is defined in the 3D object space.
  ///
  /// \param[in] width            The width of the label along the right vector
  ///                             in local space. The size is defined in the 3D object space.
  ///
  virtual void get_geometry(mi::math::Vector_struct<mi::Float32, 3>& position,
    mi::math::Vector_struct<mi::Float32, 3>& right_vector,
    mi::math::Vector_struct<mi::Float32, 3>& up_vector, mi::Float32& height,
    mi::Float32& width) const = 0;

  /// Compute the width of the label based on the given font.
  /// Note: not yet implemented and supported!
  ///
  /// \param[in]      font    The font based on which the label width shall be computed.
  ///
  /// \param[in]      layout  The label layout used by this label.
  ///
  /// \return         Returns the computed width of the label in label space.
  ///
  virtual mi::Float32 compute_label_width(const IFont* font, const ILabel_layout* layout) const = 0;
};

/// @ingroup nv_index_scene_description_shape
///
/// A label defined in image space. The label always faces towards the viewer, i.e., the label is
/// parallel to the view plane.
/// ILabel_2D is a part of the scene description.
/// The dimension (position, height, ...) is defined in the 2D image space (2D screen space).
class ILabel_2D : public mi::base::Interface_declare<0x641854bd, 0xb7a7, 0x45b0, 0x94, 0x44, 0x25,
                    0x0d, 0x3e, 0x10, 0x6f, 0xae, nv::index::IImage_space_shape>
{
public:
  /// Get the text of the label.
  ///
  /// \return     The text of the label.
  ///
  virtual const char* get_text() const = 0;
  /// The Set text of the label.
  ///
  /// \param      text    The text of the label.
  ///
  virtual void set_text(const char* text) = 0;

  /// The position and orientation in object space is defined by
  /// an anchor point that lies inside plane that the label lies in and defines the label's lower
  /// left corner,
  /// the normal vector perpendicular to the label, and the up vector.
  ///
  /// \param[in] position         The position defines the lower left
  ///                             corner of the label.
  ///
  /// \param[in] right_vector     The right vector of the label
  ///                             together with the up vector defines the
  ///                             orientation of the label in 2D image space.
  ///
  /// \param[in] up_vector        The up vector of the label together with the
  ///                             right vector defines the orientation
  ///                             of the label in 2D image space.
  ///
  /// \param[in] height           The height of the label along the up vector
  ///                             in image space. The size is defined in the 2D image space.
  ///                             (pixels)
  ///
  /// \param[in] width            The width of the label along the right vector
  ///                             in image space. If the width is not set (or set to a negative
  ///                             value)
  ///                             then the width of the label will be computed automatically by the
  ///                             rendering system. The size is defined in the 2D image space.
  ///                             (pixels)
  ///
  virtual void set_geometry(const mi::math::Vector_struct<mi::Float32, 3>& position,
    const mi::math::Vector_struct<mi::Float32, 2>& right_vector,
    const mi::math::Vector_struct<mi::Float32, 2>& up_vector, mi::Float32 height,
    mi::Float32 width = -1.f) = 0;

  /// The position and orientation in object space is defined by
  /// an anchor point that lies inside plane the label lies in and defines the label's lower left
  /// corner,
  /// the right and up vectors to the label, and the up vector.
  /// The direction that the normal points to  defines the front face
  /// of the label that displays the text.
  ///
  /// \param[out] position        The position defines the lower left
  ///                             corner of the label.
  ///
  /// \param[out] right_vector    The right vector of the label
  ///                             together with the up vector defines the
  ///                             orientation of the label in 2D space.
  ///
  /// \param[out] up_vector       The up vector of the label together with the
  ///                             right vector defines the orientation
  ///                             of the label in 2D space.
  ///
  /// \param[in] height           The height of the label along the up vector
  ///                             in image space. The size is defined in the 2D image space.
  ///                             (pixels)
  ///
  /// \param[in] width            The width of the label along the right vector
  ///                             in image space. The size is defined in the 2D image space.
  ///                             (pixels)
  ///
  virtual void get_geometry(mi::math::Vector_struct<mi::Float32, 3>& position,
    mi::math::Vector_struct<mi::Float32, 2>& right_vector,
    mi::math::Vector_struct<mi::Float32, 2>& up_vector, mi::Float32& height,
    mi::Float32& width) const = 0;

  /// Compute the width of the label based on the given font.
  /// Note: not yet implemented and supported!
  ///
  /// \param[in]      font    The font based on which the label width shall be computed.
  ///
  /// \param[in]      layout  The label layout used by this label.
  ///
  /// \return         Returns the computed width of the label in image space.
  ///
  virtual mi::Float32 compute_label_width(const IFont* font, const ILabel_layout* layout) const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ILABEL_SHAPES_H
