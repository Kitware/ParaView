/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Raster shape rendering order attribute

#ifndef NVIDIA_INDEX_IRENDERING_ORDER_H
#define NVIDIA_INDEX_IRENDERING_ORDER_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// Defines the rendering order of image space shape.
///
/// The attribute \c nv::index::IRendering_order defines a rendering priority 
/// for the image space shapes declared in the scene description. 
///
/// \ingroup nv_index_scene_description_attribute
///
class IRendering_order :
        public mi::base::Interface_declare<0xe734389f,0x4074,0x486e,0x8c,0xf4,0x08,0xeb,0x46,0xd7,0x03,0x38,
                                           nv::index::IAttribute>
{
public:
    /// Set drawing priority order.
    ///
    /// \param[in] offset The priority order offset.
    virtual void set_order(mi::Uint32 offset) = 0;

    /// Get current drawing priority order.
    ///
    /// \return Returns the current drawing priority order.
    virtual mi::Uint32 get_order() const = 0;

};


}} // namespace index / nv

#endif // NVIDIA_INDEX_IRENDERING_ORDER_H
