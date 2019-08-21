/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling diagnostic rendering.

#ifndef NVIDIA_INDEX_IDIAGNOSTIC_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IDIAGNOSTIC_RENDERING_PROPERTIES_H

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Interface representing rendering properties for diagnostic rendering.
class IDiagnostic_rendering_properties :
    public mi::base::Interface_declare<0xd0c69d46,0x9511,0x4285,0x85,0x3e,0xb1,0xb2,0x70,0x99,0x45,0xe7,
                                       nv::index::IAttribute>
{
public:
    /// Property identifiers.
    enum Property
    {
        MODE,               ///< [uint]     (0)     Render mode 0:off, 1:subregions + scene, 2:subregions only
        OPACITY,            ///< [float]    (0.25)  Opacity of surfaces of subregion boxes.
        OPACITY_FRONT,      ///< [float]    (1)     Opacity factor of front (near) surfaces of subregion boxes.
        OPACITY_BACK,       ///< [float]    (1)     Opacity factor of back (far) surfaces of subregion boxes.
        WIRE_SIZE,          ///< [float]    (1)     Size of wireframe line, in pixels.
        SMOOTHING,          ///< [float]    (1)     Smoothing range of wireframe line, 0 == no smoothing.
        ITEM_LIMIT,         ///< [int]      (0)     Limit number of subregions rendered (per device), 0 == unlimited.
                            ///<                    If negative, then animate limit range from 1 to value, using FRAME_CYCLE.
        ITEM_START,         ///< [int]      (0)     Index of first subregion rendered (per device).
        FRAME_CYCLE,        ///< [int]      (0)     Switch ITEM_LIMIT after number of frames, when animating ITEM_LIMIT.
        HOST_ID,            ///< [int]      (-1)    Restrict rendering to given host id, -1 == all hosts.
        DEVICE_ID           ///< [int]      (-1)    Restrict rendering to given device id, -1 == all devices.
    };

    /// Underlying datatype of a property.
    enum Property_type
    {
        INVALID_TYPE,
        SIGNED_INT32,
        UNSIGNED_INT32,
        FLOAT32
    };

    virtual bool            set_property(mi::Sint32 prop, mi::Float64 value) = 0;
    virtual mi::Float64     get_property(mi::Sint32 prop) const = 0;
    virtual mi::Uint32      get_propert_count() const = 0;
    virtual Property_type   get_property_type(mi::Sint32 prop) const = 0;
    virtual const char*     get_property_name(mi::Sint32 prop) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IDIAGNOSTIC_RENDERING_PROPERTIES_H
