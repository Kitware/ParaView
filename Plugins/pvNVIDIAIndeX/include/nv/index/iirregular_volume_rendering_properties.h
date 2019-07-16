/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/**
   \file
   \brief        Scene attribute controlling irregular volume rendering.
*/


#ifndef NVIDIA_INDEX_IIRREGULAR_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IIRREGULAR_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>
#include <nv/index/iattribute.h>


namespace nv {
namespace index {

/// Interface representing rendering properties for irregular volumes.
class IIrregular_volume_rendering_properties :
    public mi::base::Interface_declare<0x72327639,0xd6ed,0x4fc9,0xba,0x2f,0x92,0xf3,0x94,0x2a,0xee,0x7c,
                                       nv::index::IAttribute>
{
public:
    struct Rendering {
        mi::Uint32  sampling_mode;                      ///< Mode for sampling (0 = preintegrated colormap; 1 = discrete sampling)
        mi::Float32 sampling_segment_length;            ///< Segment length for sampling.
        mi::Float32 sampling_reference_segment_length;  ///< Segment length for sampling.
    };

    struct Diagnostics {
        mi::Uint32  mode;   ///< If not 0, a diagnostic rendering is performed instead of normal rendering (1:wireframe, 2:run path)
        mi::Uint32  flags;  ///< Bit flags to enable various diagnostics.
    };

    struct Wireframe {
        mi::Float32 wire_size;          ///< World space size of wireframe mode lines.
        mi::Float32 color_mod_begin;    ///< Distance from camera where color modulation starts.
        mi::Float32 color_mod_factor;   ///< Distance is multiplied by factor for color modulation. 0 disables color modulation.
    };


    /// Set or get rendering properties.
    virtual void set_rendering(const Rendering&) = 0;
    virtual void get_rendering(Rendering&) const = 0;
    
    /// Set or get diagnostic rendering mode for irregular volume.
    virtual void set_diagnostics(const Diagnostics&) = 0;
    virtual void get_diagnostics(Diagnostics&) const = 0;

    /// Set or get wireframe mode properties.
    virtual void set_wireframe(const Wireframe&) = 0;
    virtual void get_wireframe(Wireframe&) const = 0;
};

}} //nv::index

#endif
