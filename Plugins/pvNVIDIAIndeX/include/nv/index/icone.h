/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief A higher-level shape representing a 3D cone.

#ifndef NVIDIA_INDEX_ICONE_H
#define NVIDIA_INDEX_ICONE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/ishape.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
///
/// A 3D cone is defined by its base's center and a tip. Both need
/// to be defined by a 3D coordinate in the cone's local coordinate
/// system. A cone can be a closed object if caps are enabled,
/// otherwise the base of the cone is open. The surface of the cone
/// is shaded using the material and light defined in the scene
/// description.
///
class ICone :
    public mi::base::Interface_declare<0xe2e95b32,0x858a,0x4cb7,0x80,0x91,0xf5,0xf7,0x42,0x5e,0x9a,0x04,
                                       nv::index::IObject_space_shape>
{
public:
    /// Sets center of the cone's base in its local coordinate system.
    ///
    /// \param[in]  center      Center of the cone's base
    ///
    virtual void set_center(const mi::math::Vector_struct<mi::Float32, 3>& center) = 0;

    /// Returns center of the cone's base in its local coordinate system.
    ///
    /// \return     Center of the cone's base
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>& get_center() const = 0;

    /// Sets the position of the tip in the cone's local coordinate system.
    ///
    /// \param[in] tip Position of the tip of cone
    ///
    virtual void set_tip(const mi::math::Vector_struct<mi::Float32, 3>& tip) = 0;

    /// Returns the position of the tip in the cone's local coordinate system.
    ///
    /// \return     Tip of the cone.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>& get_tip() const = 0;

    /// Sets the cone's base radius
    ///
    /// \param[in] radius Base radius
    virtual void set_radius(mi::Float32 radius) = 0;

    /// Returns the cone's base radius.
    ///
    /// \return     Base radius
    ///
    virtual mi::Float32 get_radius() const = 0;

    /// Controls whether the cone is capped.
    /// A cone can have closed base as if a cap covers the interior.
    ///
    /// \param[in] is_capped True when the cone should be capped.
    virtual void set_capped(bool is_capped) = 0;

    /// Returns whether the cone is capped.
    ///
    /// \return     True if the cone is capped.
    ///
    virtual bool get_capped() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICONE_H
