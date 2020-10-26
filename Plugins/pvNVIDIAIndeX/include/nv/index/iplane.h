/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Plane shape with texturing functionality.

#ifndef NVIDIA_INDEX_IPLANE_H
#define NVIDIA_INDEX_IPLANE_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// Interface class representing a rectangular cutout of an infinite plane as part of the scene
/// description.
///
/// A plane is a geometric structure defined by a position, a orientation, and a rectangular cutout. 
/// The position and orientation of the plane are defined by a point and a normal vector. The
/// rectangular cutout stretches from the given point into the directions of the up vector and the
/// right vector (which is perpendicular to normal and up). The x-component of the extent specifies
/// the size in the direction of the right vector and the y-component corresponds to the up vector.
/// 
/// The texture implemented by a \c IDistributed_compute_technique attribute may be assigned to the
/// plane.
///
/// \ingroup nv_index_scene_description_object_shape
///
class IPlane :
    public mi::base::Interface_declare<0x70c0a5e0,0x2ce2,0x4bd8,0xbe,0x4b,0xb0,0x34,0x32,0x83,0x11,0xd8,
                                       nv::index::IObject_space_shape>
{
public:
    /// Sets the point defining the position of the plane and also the origin of the rectangular
    /// cutout.
    ///
    /// \param[in] point Defines the lower left corner of the rectangular cutout.
    ///
    virtual void set_point(const mi::math::Vector_struct<mi::Float32, 3>& point) = 0;

    /// Returns the point defining the position of the plane and also the origin of the rectangular
    /// cutout.
    ///
    /// \return The point that defines the lower left corner of the rectangular cutout.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3> get_point() const = 0;

    /// Sets the plane's normal vector.
    ///
    /// \param[in] normal The plane's normal vector.
    ///
    virtual void set_normal(const mi::math::Vector_struct<mi::Float32, 3>& normal) = 0;

    /// Returns the plane's normal vector.
    ///
    /// \return The plane's normal vector.
    //
    virtual mi::math::Vector_struct<mi::Float32, 3> get_normal() const = 0;

    /// Sets the up vector of the rectangular cutout, corresponding to the y-component of the plane
    /// extent.
    ///
    /// \param[in] up The up vector.
    ///
    virtual void set_up(const mi::math::Vector_struct<mi::Float32, 3>& up) = 0;

    /// Returns the up vector of the rectangular cutout, corresponding to the y-component of the
    /// plane extent.
    ///
    /// \return The up vector.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 3> get_up() const = 0;

    /// Sets the extent of the plane's rectangular cutout.
    ///
    /// \note Currently, a plane cannot be infinite.
    ///
    /// \param[in] extent The 2D extent, with the x-component giving the size in the direction of
    ///                   the right vector and the y-component corresponding to the up vector.
    ///
    virtual void set_extent(const mi::math::Vector_struct<mi::Float32, 2>& extent) = 0;

    /// Returns the extent of the plane's rectangular cutout.
    ///
    /// \return The 2D extent.
    ///
    virtual mi::math::Vector_struct<mi::Float32, 2> get_extent() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPLANE_H
