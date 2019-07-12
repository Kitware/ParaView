/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Attribute defining a clipping region.

#ifndef NVIDIA_INDEX_ICLIP_REGION_H
#define NVIDIA_INDEX_ICLIP_REGION_H

#include <mi/math/bbox.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// @ingroup nv_index_scene_description_attribute

/// Defines a generic clipping region that is applied to shapes.
/// Clipping is only supported for some shapes, as described in the documentation of the individual
/// shape scene elements.
///
class IClip_region :
    public mi::base::Interface_declare<0x7693cba2,0x8c85,0x4023,0xa2,0x96,0xab,0x56,0xab,0x53,0xc8,0x5c,
                                       nv::index::IAttribute>
{
public:
    /// Sets the bounding box that is used for clipping.
    ///
    /// \param[in] bbox  Bounding box
    virtual void set_clip_bounding_box(const mi::math::Bbox_struct<mi::Float32, 3>& bbox) = 0;

    /// Returns the bounding box that is used for clipping.
    ///
    /// \return clip bounding box.
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_clip_bounding_box() const = 0;
};


}} // namespace

#endif // NVIDIA_INDEX_ICLIP_REGION_H
