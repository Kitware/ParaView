/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling sparse volume rendering properties.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// @ingroup nv_index_scene_description_attribute
/// 
/// Filtering modes (interpolation) for sparse volume access.
/// 
enum Sparse_volume_filter_mode
{
    SPARSE_VOLUME_FILTER_NEAREST                = 0x00, ///< Access a single voxel with nearest filtering (a.k.a. point filtering).
    SPARSE_VOLUME_FILTER_TRILINEAR_POST         = 0x01, ///< Trilinear interpolation with post-classification.
    SPARSE_VOLUME_FILTER_TRILINEAR_PRE          = 0x02, ///< Trilinear interpolation with pre-classification.
    SPARSE_VOLUME_FILTER_TRICUBIC_CATMULL_POST  = 0x03, ///< Tricubic Catmull-Rom interpolation with post-classification.
    SPARSE_VOLUME_FILTER_TRICUBIC_CATMULL_PRE   = 0x04, ///< Tricubic Catmull-Rom interpolation with pre-classification.
    SPARSE_VOLUME_FILTER_TRICUBIC_BSPLINE_POST  = 0x05, ///< Tricubic B-spline interpolation with post-classification.
    SPARSE_VOLUME_FILTER_TRICUBIC_BSPLINE_PRE   = 0x06  ///< Tricubic B-spline interpolation with pre-classification.
};

/// @ingroup nv_index_scene_description_attribute
/// 
/// The interface class representing rendering properties for sparse volume data.
///
class ISparse_volume_rendering_properties :
    public mi::base::Interface_declare<0x718c9406,0x99a7,0x41cc,0xb1,0x48,0xe,0x36,0x8d,0x67,0x85,0x19,
                                       nv::index::IAttribute>
{
public:
    /// Set the sampling distance used for a sparse volume scene element (\c ISparse_volume_scene_element).
    /// The default value used is 1.0f.
    /// 
    /// \param[in]  sample_dist     Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_sampling_distance(mi::Float32 sample_dist) = 0;
    /// Returns the sampling distance used for a sparse volume scene element (\c ISparse_volume_scene_element).
    virtual mi::Float32                 get_sampling_distance() const = 0;

    /// Set the reference sampling distance used for a sparse volume scene element (\c ISparse_volume_scene_element).
    /// The default value used is 1.0f. The reference sampling distance is used during the volume rendering to steer
    /// the opacity correction and therefore the appearance of the volume display.
    /// 
    /// \param[in]  s   Sampling distance (default value is 1.0f).
    /// 
    virtual void                        set_reference_sampling_distance(mi::Float32 s) = 0;
    /// Returns the reference sampling distance used for a sparse volume scene element (\c ISparse_volume_scene_element).
    virtual mi::Float32                 get_reference_sampling_distance() const = 0;

    /// Set the voxel offsets used for a sparse volume scene element (\c ISparse_volume_scene_element).
    /// The default values used are (0.0, 0.0, 0.0). The input values are limited to the range [0.0, 0.5].
    ///
    /// \param[in]  voxel_offsets   Voxel offsets (default value is (0.0, 0.0, 0.0));
    ///
    virtual void                        set_voxel_offsets(
                                            const mi::math::Vector_struct<mi::Float32, 3>& voxel_offsets) = 0;
    /// Returns the voxel offsets used for a sparse volume scene element (\c ISparse_volume_scene_element).
    virtual mi::math::Vector_struct<mi::Float32, 3> get_voxel_offsets() const = 0;

    /// Set the volume filter mode for a sparse volume scene element (\c ISparse_volume_scene_element).
    /// The default filter used is \c SPARSE_VOLUME_FILTER_NEAREST.
    /// 
    /// \param[in]  filter_mode     Filter mode (default value is \c SPARSE_VOLUME_FILTER_NEAREST).
    /// 
    virtual void                        set_filter_mode(Sparse_volume_filter_mode filter_mode) = 0;
    /// Returns the filter mode used for a sparse volume scene element (\c ISparse_volume_scene_element).
    virtual Sparse_volume_filter_mode   get_filter_mode() const = 0;

    /// Set the mode for the level-of-detail (LOD) rendering for a sparse volume scene element
    /// (\c ISparse_volume_scene_element). The default setting of disabled.
    ///
    /// \param[in]  enable_lod_render   Enables or disabled the LOD-rendering.
    ///
    virtual void                        set_lod_rendering_enabled(bool enable_lod_render) = 0;
    /// Returns if level-of-detail rendering is enabled or disabled.
    virtual bool                        get_lod_rendering_enabled() const = 0;

    /// The pixel-threshold allows to control the dynamic level-of-detail selection. It feeds into an internal
    /// heuristic determining the amount of pixels a voxel spans in the viewport. A larger threshold allows for
    /// a more coarse LOD selection displaying lower resolution data closer to the view point.
    ///
    /// \param[in]  pixel_threshold     Scalar pixel threshold.
    ///
    virtual void                        set_lod_pixel_threshold(mi::Float32 pixel_threshold) = 0;
    /// Returns the currently set scalar pixel threshold.
    virtual mi::Float32                 get_lod_pixel_threshold() const = 0;

#if 0 // EXTENSION LOD active level range control
    /// Set the active LOD-level range to be used by NVIDIA IndeX.
    ///
    /// The active LOD-level range limits the data NVIDIA IndeX is requesting to be written
    /// to this LOD-texture instance and accesses during rendering.
    ///
    /// \param[in]  lod_level_range     The LOD-range to be used by NVIDIA IndeX.
    ///
    virtual void                            set_active_LOD_level_range(
                                                const mi::math::Vector_struct<mi::Uint32, 2>& lod_level_range) = 0;

    /// Returns the currently active LOD-level range to be used by NVIDIA IndeX.
    ///
    /// \returns    The currently active LOD-level range.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_active_LOD_level_range() const = 0;
#endif


    /// Enable or disable pre-integrated volume rendering for a sparse volume scene element (\c ISparse_volume_scene_element).
    /// Per default this is disabled.
    /// 
    /// \param[in] enable           Enables the pre-integrated volume rendering technique
    ///                             if (\c true) otherwise the technique will be disabled (\c false).
    /// 
    virtual void                        set_preintegrated_volume_rendering(bool enable) = 0;
    /// Returns if the pre-integrated volume rendering is enabled or disabled.
    virtual bool                        get_preintegrated_volume_rendering() const = 0;

    /// Internal debugging options.
    virtual void                        set_debug_visualization_option(mi::Uint32 o) = 0;
    virtual mi::Uint32                  get_debug_visualization_option() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_RENDERING_PROPERTIES_H
