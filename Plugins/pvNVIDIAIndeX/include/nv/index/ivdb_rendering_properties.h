/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \brief Scene attribute controlling VDB rendering properties.

#ifndef NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Rendering properties for VDB data.
///
/// \ingroup nv_index_scene_description_attribute
///
class IVDB_rendering_properties :
    public mi::base::Interface_declare<0xed7b65f1,0xea15,0x46db,0xaa,0x69,0xc0,0xb2,0xca,0x2f,0x78,0xbb,
                                       nv::index::IAttribute>
{
public:
    /// Set the sampling distance used for a VDB scene element (\c IVDB_scene_element)
    /// when using the direct volume rendering mode.
    /// The default value used is 1.0f.
    /// 
    /// \param[in]  sample_dist     Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_sampling_distance(mi::Float32 sample_dist) = 0;
    /// Returns the sampling distance used for a VDB scene element (\c IVDB_scene_element).
    virtual mi::Float32                 get_sampling_distance() const = 0;

    /// Set the reference sampling distance used for a VDB scene element (\c IVDB_scene_element).
    /// The default value used is 1.0f. The reference sampling distance is used during the volume rendering to steer
    /// the opacity correction and therefore the appearance of the volume display.
    /// 
    /// \param[in]  ref_sample_dist Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(mi::Float32 ref_sample_dist) = 0;
    /// Returns the reference sampling distance used for a VDB scene element (\c IVDB_scene_element).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    /// Internal debugging options applied to the visualization.
    /// \param[in] o    Debug option applied to the visualization.   
    virtual void                        set_debug_visualization_option(mi::Uint32 o) = 0;
    /// Internal debugging options applied to the visualization.
    /// \return         Returns the applied debug option.  
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H
