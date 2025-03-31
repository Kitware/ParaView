/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \brief Scene attribute controlling VDB rendering properties.

#ifndef NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// Filtering modes (interpolation) for VDB volume access.
/// 
/// \ingroup nv_index_scene_description_attribute
///
enum VDB_filter_mode
{
    VDB_FILTER_NEAREST      = 0x00, ///< Access a single voxel with nearest filtering (a.k.a. point filtering).
    VDB_FILTER_TRILINEAR    = 0x01  ///< Trilinear interpolation with post-classification.
};

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
    virtual void                        set_sampling_distance(
                                            mi::Float32 sample_dist) = 0;
    /// Returns the sampling distance used for a VDB scene element (\c IVDB_scene_element).
    virtual mi::Float32                 get_sampling_distance() const = 0;

    /// Set the reference sampling distance used for a VDB scene element (\c IVDB_scene_element).
    /// The default value used is 1.0f. The reference sampling distance is used during the volume rendering to steer
    /// the opacity correction and therefore the appearance of the volume display.
    /// 
    /// \param[in]  ref_sample_dist Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(
                                            mi::Float32 ref_sample_dist) = 0;
    /// Returns the reference sampling distance used for a VDB scene element (\c IVDB_scene_element).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    /// Set the volume filter mode for a VDB scene element (\c IVDB_scene_element).
    /// The default filter used is \c VDB_FILTER_NEAREST.
    /// 
    /// \param[in]  filter_mode     Filter mode (default value is \c VDB_FILTER_NEAREST).
    /// 
    virtual void                        set_filter_mode(VDB_filter_mode filter_mode) = 0;
    /// Returns the filter mode used for a VDB scene element (\c IVDB_scene_element).
    virtual VDB_filter_mode             get_filter_mode() const = 0;

    /// Set the voxel border that is applied when VDB subsets are created. SUch a border
    /// can be beneficial when using larger sampling kernels in XAC programs (e.g., for
    /// gradient generation calculations).
    /// 
    /// \note This setting is applied when NVIDIA IndeX is creating \c IVDB_scene_element subsets
    ///       and can not be changed once the volume data is loaded.
    /// 
    /// \param[in]  border_size     Voxel border size to apply to subsets (default value is 2).
    /// 
    virtual void                        set_subset_voxel_border(
                                            mi::Uint32 border_size) = 0;
    /// Returns the voxel border size used for VDB subset generation.
    virtual mi::Uint32                  get_subset_voxel_border() const = 0;

    /// Internal debugging options applied to the visualization.
    /// \param[in] o    Debug option applied to the visualization.   
    virtual void                        set_debug_visualization_option(
                                            mi::Uint32 o) = 0;
    /// Internal debugging options applied to the visualization.
    /// \return         Returns the applied debug option.  
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IVDB_RENDERING_PROPERTIES_H
