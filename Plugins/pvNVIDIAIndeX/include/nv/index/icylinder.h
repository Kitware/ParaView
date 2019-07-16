/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element representing a cylinder higher-level shape.

#ifndef NVIDIA_INDEX_ICYLINDER_H
#define NVIDIA_INDEX_ICYLINDER_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
///
/// The set of higher-level 3D shapes part of the NVIDIA IndeX library includes a 3D cylinder.
/// ICylinder is a part of the scene description.
/// A 3D cylinder is defined by its top and bottom position and a radius. Both positions 
/// need to be defined by a 3D coordinate in the cylinder's local coordinate system.
/// A cylinder can be watertight, i.e., a closed object, if caps are enabled otherwise
/// the top and the bottom of the cylinder is open. The surface of the cylinder
/// is shaded using the material and light defined in the hierarchical scene description.
///
class ICylinder :
    public mi::base::Interface_declare<0x82b63cc6,0xcd9b,0x49af,0x90,0x8f,0x3a,0xcf,0x51,0x70,0x64,0x56,
                                       nv::index::IObject_space_shape>
{
public:
    /// Get the top position of the 3D cylinder.
    ///
    /// \return             The top position is defined in the local
    ///                     coordinate system of the cylinder.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>& get_top() const = 0;
    
    /// Set the top position of the 3D cylinder.
    ///
    /// \param[in] top      The top position is defined in the local
    ///                     coordinate system of the cylinder.
    ///
    virtual void set_top(const mi::math::Vector_struct<mi::Float32, 3>& top) = 0;

    /// Get the bottom position of the 3D cylinder.
    ///
    /// \return             The bottom position is defined in the local
    ///                     coordinate system of the cylinder.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>& get_bottom() const = 0;

    /// Set the bottom position of the 3D cylinder.
    ///
    /// \param[in] bottom   The bottom position is defined in the local
    ///                     coordinate system of the cylinder.
    ///
    virtual void set_bottom(const mi::math::Vector_struct<mi::Float32, 3>& bottom) = 0;

    /// Get the radius of the 3D cylinder.
    ///
    /// \return             The radius is defined in the local
    ///                     coordinate system of the 3D cylinder.
    ///
    virtual mi::Float32 get_radius() const = 0;

    /// Set the radius of the 3D cylinder.
    ///
    /// \param[in] radius   The radius is defined in the local
    ///                     coordinate system of the 3D cylinder.
    ///
    virtual void set_radius(mi::Float32 radius) = 0;

    /// A cylinder can be a solid, watertight 3D shape, i.e., its base is closed
    /// as if a cap covers the interior. Instead of composing the capped shape
    /// manually, e.g., using a 3D disc, the shape interface class provides a
    /// flag that instructs the rendering system to render the cap automatically.
    ///
    /// \return     Returns if the cylinder shall be capped.
    ///
    virtual bool get_capped() const = 0;
    
    /// Setting the cap rendering.
    ///
    /// \param[in] is_capped    Enable or disable the cap rendering.
    ///
    virtual void set_capped(bool is_capped) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICYLINDER_H
