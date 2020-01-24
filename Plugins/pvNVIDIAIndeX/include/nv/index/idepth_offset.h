/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Depth offset shape rendering attribute

#ifndef NVIDIA_INDEX_IDEPTH_OFFSET_H
#define NVIDIA_INDEX_IDEPTH_OFFSET_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// Defines the depth-offset for shapes following this attribute in the scene
/// description.  The default depth offset is 0.0. This attribute only affects
/// raster (2D) shapes.
///
/// \note A shape rendering attribute provides the renderer with a hint from an
/// application related to shape rendering.  Such a hint could, for example,
/// describe how the renderer should handle a numerical problem with a z-value.
///
/// \ingroup nv_index_scene_description_attribute
///
class IDepth_offset :
        public mi::base::Interface_declare<0x09b90263,0x58b2,0x4be4,0xbf,0xb1,0xae,0xd1,0xae,0xab,0xbc,0x95,
                                           nv::index::IAttribute>
{
public:
    /// Set depth-offset value of this attribute.
    ///
    /// \param[in] offset depth offset value
    virtual void set_depth_offset(mi::Float32 offset) = 0;

    /// Get current depth-offset value of this attribute.
    ///
    /// \return current depth-offset value
    virtual mi::Float32 get_depth_offset() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDEPTH_OFFSET_H
