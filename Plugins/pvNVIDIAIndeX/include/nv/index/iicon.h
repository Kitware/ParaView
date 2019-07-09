/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene elements representing image space icons.

#ifndef NVIDIA_INDEX_IICON_SHAPES_H
#define NVIDIA_INDEX_IICON_SHAPES_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_scene_description_shape
///
/// Icons are common means for annotating a 3D scene using images.
/// The 3D icon is defined in 3D space and its orientation is affected by the
/// transformations applied by the scene description.
/// The size is defined in 3D object space length.
///
class IIcon_3D :
        public mi::base::Interface_declare<0x1f5f4ed8,0xf0ac,0x4aed,0x8d,0x16,0x80,0x86,0x19,0x9d,0xe9,0x26,
                                           nv::index::IObject_space_shape>
{
public:
    /// The position and orientation in its local coordinate system is defined by
    /// an anchor point that lies inside the plane that the icon lies in and
    /// defines the icon's lower left corner,
    /// the normal vector perpendicular to the icon, and the up vector.
    ///
    /// \param[in] position         The position defines the lower left
    ///                             corner of the icon.
    ///
    /// \param[in] right_vector     The right vector of the icon
    ///                             together with the up vector defines the
    ///                             orientation of the icon in 3D space.
    ///
    /// \param[in] up_vector        The up vector of the icon together with the
    ///                             right vector defines the orientation 
    ///                             of the icon in 3D space.
    ///
    /// \param[in] height           The height of the icon along the up vector
    ///                             in 3D object space length.
    ///
    /// \param[in] width            The width of the icon along the right vector 
    ///                             in 3D object space length. If the
    ///                             width is not set (or set a negative value)
    ///                             then the width of the icon will be computed by the 
    ///                             rendering system.
    ///
    virtual void set_geometry(
        const mi::math::Vector_struct<mi::Float32, 3>& position,
        const mi::math::Vector_struct<mi::Float32, 3>& right_vector,
        const mi::math::Vector_struct<mi::Float32, 3>& up_vector,
        mi::Float32                                    height,
        mi::Float32                                    width = -1.0f) = 0;

    /// The position and orientation in local coordinate system is defined by
    /// an anchor point that lies inside plane the icon lies in and
    /// defines the icon's lower left corner,
    /// the right and up vectors to the icon, and the up vector.
    /// The direction that the normal points to  defines the front face
    /// of the icon that displays the image.
    ///
    /// \param[out] position        The position defines the lower left
    ///                             corner of the icon.
    ///
    /// \param[out] right_vector    The right vector of the icon
    ///                             together with the up vector defines the
    ///                             orientation of the icon in 3D space.
    ///
    /// \param[out] up_vector       The up vector of the icon together with the
    ///                             right vector defines the orientation
    ///                             of the icon in 3D space.
    ///
    /// \param[in] height           The height of the icon along the up vector
    ///                             in the 3D object space length.
    ///
    /// \param[in] width            The width of the icon along the right vector 
    ///                             in the 3D object space length.
    ///
    virtual void get_geometry(
        mi::math::Vector_struct<mi::Float32, 3>& position,
        mi::math::Vector_struct<mi::Float32, 3>& right_vector,
        mi::math::Vector_struct<mi::Float32, 3>& up_vector,
        mi::Float32&                             height,
        mi::Float32&                             width) const = 0;
};


/// @ingroup nv_index_scene_description_shape
///
/// Icons are common means for annotating a 3D scene using images.
/// The 2D icon is defined in image space and always faces towards the viewer,
/// similar to a billboard; the label is parallel to the view plane.
/// The size is defined in 2D image space (screen space) in pixel.
///
class IIcon_2D :
        public mi::base::Interface_declare<0xfea64c6d,0xb200,0x4955,0x80,0x35,0xe3,0x1e,0x3a,0xd9,0xc1,0x13,
                                           nv::index::IImage_space_shape>
{
public:
    /// The position of the icon is defined in its local coordinate system by the
    /// anchor point that lies inside the plane that the icon lies in and
    /// defines the icon's lower left corner. The orientation of the icon is defined 
    /// by a right and an up vector. Both these vectors are defined in 2D image space.
    ///
    /// \param[in] position         The position defines the lower left
    ///                             corner of the icon.
    ///
    /// \param[in] right_vector     The right vector of the icon
    ///                             together with the up vector defines the
    ///                             orientation of the icon in 2D image space.
    ///
    /// \param[in] up_vector        The up vector of the icon together with the
    ///                             right vector defines the orientation 
    ///                             of the icon in 2D image space.
    ///
    /// \param[in] height           The height of the icon along the up vector
    ///                             in image space, pixel.
    ///
    /// \param[in] width            The width of the icon along the right vector 
    ///                             in image space, pixel. If the width is not set 
    ///                             (or set a negative value)
    ///                             then the width of the icon will be computed by the 
    ///                             rendering system.
    ///
    virtual void set_geometry(
        const mi::math::Vector_struct<mi::Float32, 3>& position,
        const mi::math::Vector_struct<mi::Float32, 2>& right_vector,
        const mi::math::Vector_struct<mi::Float32, 2>& up_vector,
        mi::Float32                                    height,
        mi::Float32                                    width = -1.0f) = 0;

    /// The position of the icon is defined in its local coordinate system by the
    /// anchor point that lies inside plane that the icon lies in and
    /// defines the icon's lower left corner. The orientation of the icon is defined 
    /// by a right and an up vector. Both these vectors are defined in 2D image space.
    ///
    /// \param[out] position        The position defines the lower left
    ///                             corner of the icon.
    ///
    /// \param[out] right_vector    The right vector of the icon
    ///                             together with the up vector defines the
    ///                             orientation of the icon in 2D space.
    ///
    /// \param[out] up_vector       The up vector of the icon together with the
    ///                             right vector defines the orientation
    ///                             of the icon in 2D space.
    ///
    /// \param[in] height           The height of the icon along the up vector
    ///                             in image space, pixel.
    ///
    /// \param[in] width            The width of the icon along the right vector 
    ///                             in image space, pixel.
    ///
    virtual void get_geometry(
        mi::math::Vector_struct<mi::Float32, 3>& position,
        mi::math::Vector_struct<mi::Float32, 2>& right_vector,
        mi::math::Vector_struct<mi::Float32, 2>& up_vector,
        mi::Float32&                             height,
        mi::Float32&                             width) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IICON_SHAPES_H
