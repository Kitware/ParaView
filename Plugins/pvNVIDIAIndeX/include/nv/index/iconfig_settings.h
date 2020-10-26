/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Setting that configure NVIDIA IndeX's rendering and computing.

#ifndef NVIDIA_INDEX_ICONFIG_SETTINGS_H
#define NVIDIA_INDEX_ICONFIG_SETTINGS_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/idepth_test.h>

namespace nv
{
namespace index
{

/// An abstract interface for sharing global settings that configure NVIDIA IndeX
/// rendering and computing functionality.
///
/// \ingroup nv_index_configuration
///
class IConfig_settings :
    public mi::base::Interface_declare<0xf7d37400,0xdf43,0x49f2,0x8e,0x12,0x67,0xe3,0x62,0x90,0xad,0xee,
                                       mi::neuraylib::IElement>
{
public:
    /// NVIDIA IndeX supports both a CUDA-based GPU or CPU renderers. The renderers can be switched
    /// at runtime. The GPU renderer gives the best performance especially when rendering large-scale
    /// data. Moreover, the GPU renderer has the complete and more comprehensive feature set compared
    /// to the CPU renderer. The CPU renderer is restricted to volume and heightfield rendering only.
    ///
    /// \deprecated CPU rendering mode is deprecated
    enum Rendering_mode
    {
        /// CPU rendering mode.
        CPU_RENDERING        = 0,
        /// GPU rendering mode (using CUDA).
        GPU_RENDERING        = 1,
        /// GPU rendering with automatic fallback to CPU when no GPU is available.
        GPU_OR_CPU_RENDERING = 2
    };

    /// The workload for compositing of the rendering results can be distributed to all machines,
    /// to the local viewer/head machine only, or to all but the local machine.
    enum Compositing_mode
    {
        /// Perform compositing on all available cluster machines.
        COMPOSITING_ALL         = 0,
        /// Perform compositing on the local (viewer or head) cluster machine only.
        COMPOSITING_LOCAL_ONLY  = 1,
        /// Perform compositing on all cluster machines except the local (viewer or head) cluster machine.
        COMPOSITING_REMOTE_ONLY = 2,
        /// Disable compositing for performance testing.
        NO_COMPOSITING          = 3
    };

    /// The heightfield vertex normal calculation mode.
    enum Heightfield_normal_calculation_mode
    {
        /// One ring neighbor triangle face normal average.
        FACE_NORMAL_AVERAGE                = 0,
        /// Triangle area weighted one ring neighbor triangle face normal average.
        AREA_WEIGHTED_FACE_NORMAL_AVERAGE  = 1,
    };

    /// Settings for data transfer over the network
    struct Data_transfer_config
    {
        Data_transfer_config()
          : span_compression_level(1),
            tile_compression_level(1),
            span_image_encoding(true),
            tile_image_encoding(true),
            span_alpha_channel(false),
            span_compression_mode(0),
            tile_compression_mode(0),
            span_compression_threads(0),
            tile_compression_threads(0),
            z_buffer_compression_level(1),
            z_buffer_compression_mode(0),
            z_buffer_compression_threads(0)
        {}

        /// Compression level for transfer of horizontal span image data. The level can have values
        /// between 1 (fast) and 9 (best), while 0 disables compression (default: 1).
        mi::Uint8 span_compression_level;

        /// Compression level for transfer of intermediate tile image data. The level can have
        /// values between 1 (fast) and 9 (best), while 0 disables compression (default: 1).
        mi::Uint8 tile_compression_level;

        /// Use special image encoding to improve compression for horizontal span image data
        /// (default: true).
        bool span_image_encoding;

        /// Use special image encoding to improve compression for intermediate tile image data
        /// (default: true).
        bool tile_image_encoding;

        /// Enable transfer of the alpha channel for horizontal span image data. Unless it is
        /// required for compositing the final image, it should be disabled to reduce the amount of
        /// data sent of the network (default: false).
        bool span_alpha_channel;

        /// Compression mode for horizontal span image data, experimental (default: 0).
        mi::Uint32 span_compression_mode;

        /// Compression mode for intermediate tile image data, experimental (default: 0).
        mi::Uint32 tile_compression_mode;

        /// Number of threads for compression of horizontal span image data, 0 means to use the
        /// default.
        mi::Uint8 span_compression_threads;

        /// Number of threads for compression of intermediate tile image data, 0 means to use the
        /// default.
        mi::Uint8 tile_compression_threads;

        /// Compression level for transfer of z-buffer data. The level can have values
        /// between 1 (fast) and 9 (best), while 0 disables compression (default: 1).
        mi::Uint8 z_buffer_compression_level;

        /// Compression mode for z-buffer data, experimental (default: 0).
        mi::Uint32 z_buffer_compression_mode;

        /// Number of threads for compression of z-buffer data, 0 means to use the default.
        mi::Uint8 z_buffer_compression_threads;
    };

    /// Configuration settings for sparse volume scene elements.
    struct Sparse_volume_config
    {
        /// Controls the dimensions of sparse volume bricks and therefore the granularity of
        /// the sparse volume renderer for empty-space leaping. The actual internally used brick
        /// size is a combination of the brick dimensions and the chosen shared brick border size.
        /// (it is calculated as \c brick_dimensions + 2 * \c brick_shared_border_size). The
        /// actual brick size should be powers of two in each dimension for optimal operation.
        mi::math::Vector_struct<mi::Uint32, 3>      brick_dimensions;

        /// Controls the size of shared border around volume bricks. The default border size is 1,
        /// which enables tri-linear filtering of voxel values. For more complex filters (using
        /// wider kernels) a larger border size may be required.
        mi::Uint32                                  brick_shared_border_size;
    };

    /// Configuration settings for corner-point grid scene elements.
    struct Corner_point_grid_config
    {
        /// Controls the dimensions of corner-point grid layer tiles and therefore the granularity of
        /// the corner-point grid renderer for empty-space leaping.
        mi::math::Vector_struct<mi::Uint32, 2>      tile_dimensions;
    };

    /// Configuration settings for tiled/level-of-detail height-field scene elements.
    struct Height_field_lod_config
    {
        /// Controls the dimensions of height-field tiles. The actual internally used tile
        /// size is a combination of the tile dimensions and the chosen shared tile border size.
        /// (it is calculated as \c tile_dimensions + 2 * \c tile_shared_border_size). The
        /// actual tile size should be powers of two in each dimension for optimal operation.
        mi::math::Vector_struct<mi::Uint32, 2>      tile_dimensions;

        /// Controls the size of shared border around height-field tiles. The default border,
        /// size is 1 which enables bi-linear filtering of height-field values. For more
        /// complex filters (using wider filtering kernels) a larger border size may be required.
        mi::Uint32                                  tile_shared_border_size;
    };

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name System configuration settings
    /// \brief The set methods of the system configuration settings
    ///        can only be called before rendering of the first frame
    ///        is started. Instead, the get methods can be called
    ///        any time.
    /// @{

    /// Sets the sparse volume renderer configuration.
    ///
    /// \param[in]  svol_config     The new sparse volume configuration.
    ///
    /// \return    Return \c true if the configuration succeeded, false otherwise.
    ///
    virtual bool set_sparse_volume_configuration(
        const Sparse_volume_config&     svol_config) = 0;

    /// Returns the current sparse volume renderer configuration.
    ///
    /// \return The current sparse volume renderer configuration (c.f., \c Sparse_volume_config).
    ///
    virtual const Sparse_volume_config& get_sparse_volume_configuration() const = 0;

    /// Sets the corner-point grid renderer configuration.
    ///
    /// \param[in]  cpg_config      The new corner-point grid configuration.
    ///
    /// \return    Return \c true if the configuration succeeded, false otherwise.
    ///
    virtual bool set_corner_point_grid_configuration(
        const Corner_point_grid_config& cpg_config) = 0;

    /// Returns the current corner-point grid renderer configuration.
    ///
    /// \return The current corner-point grid renderer configuration (c.f., \c Corner_point_grid_config).
    ///
    virtual const Corner_point_grid_config& get_corner_point_grid_configuration() const = 0;

    /// Sets the level-of-detail height-field renderer configuration.
    ///
    /// \param[in]  hf_lod_config   The new level-of-detail height-field configuration.
    ///
    /// \return    Return \c true if the configuration succeeded, false otherwise.
    ///
    virtual bool set_height_field_lod_configuration(
        const Height_field_lod_config&  hf_lod_config) = 0;

    /// Returns the current level-of-detail height-field renderer configuration.
    ///
    /// \return The current level-of-detail height-field renderer configuration (c.f., \c Height_field_lod_config).
    ///
    virtual const Height_field_lod_config& get_height_field_lod_configuration() const = 0;

    /// Sets the size of the logical subcubes into which the volume dataset is split up. The
    /// subcubes may shrink if volume rotation or scaling is enabled.
    ///
    /// To get the internal subcube size for memory allocation, the border (see
    /// set_subcube_border()) is added to the given subcube size. For example, with an initial
    /// subcube size of 510 and a border size of 1 (on each side) subcubes with a size of 512 would
    /// be allocated. These are also the default settings.
    ///
    /// \note This method may only be called before rendering of the first frame is started. When
    /// calling it after that, the behavior is undefined.
    ///
    /// \param[in] initial_subcube_size     The initial subcube size to use. It actually used size might
    ///                                     become smaller is rotation or scaling is enabled.
    ///                                     (default: 510)
    /// \param[in] subcube_border_size      Controls the size of the border around each subcube, for which
    ///                                     neighboring voxel data is stored. The default border size is 1,
    ///                                     which makes trilinear filtering possible. For more complex
    ///                                     filters (using larger kernels) a larger border size may be required.
    /// \param[in] support_continuous_volume_translation  Controls whether a translation of volume scene elements
    ///                                     should be supported for arbitrary float values (true) or
    ///                                     just for integer values (false). Since enabling this feature reduces the
    ///                                     usable subcube size, it should only be enabled when needed.
    ///                                     (default: false)
    /// \param[in] support_volume_rotation  Controls whether rotation of volume scene elements
    ///                                     should be supported. Since enabling this feature reduces the
    ///                                     usable subcube size, it should only be enabled when needed.
    ///                                     (default: false)
    /// \param[in] minimal_volume_scaling   The minimum scaling factor for volume scene elements that
    ///                                     should be supported. Since any value less than 1 reduces the
    ///                                     usable subcube size, the scaling factor should be chosen only
    ///                                     as small as absolutely needed. The default of (1, 1, 1)
    ///                                     should be used if no scaling is required. Values given here
    ///                                     must be greater than 0.0 and less or equal to 1.0. When a
    ///                                     scene element is scaled by a negative scaling factor then the
    ///                                     absolute value of this factor should be used to determine the
    ///                                     minimal volume scaling.
    /// \return                             Return \c true if the configuration succeeded.
    ///
    virtual bool set_subcube_configuration(
        const mi::math::Vector_struct<mi::Uint32, 3>&  initial_subcube_size,
        mi::Uint32                                     subcube_border_size,
        bool                                           support_continuous_volume_translation,
        bool                                           support_volume_rotation,
        const mi::math::Vector_struct<mi::Float32, 3>& minimal_volume_scaling) = 0;

    /// Returns the subcube size. This is the size of the logical subcubes into which the volume
    /// dataset is split up.
    ///
    /// \param[out] subcube_size        Current subcube size It can be smaller than the value that was
    ///                                 passed to set_subcube_configuration() if rotation or scaling
    ///                                 were enabled. The returned value does not include the size of
    ///                                 the border.
    virtual void get_subcube_size(mi::math::Vector_struct<mi::Uint32, 3>& subcube_size) const = 0;

    /// Returns whether continuous translation of volume scene elements is currently supported.
    /// \return true when continuous translation is supported, false if only integer translation is
    ///         supported.
    virtual bool get_continuous_volume_translation_supported() const = 0;

    /// Returns whether rotation of volume scene elements is currently supported.
    /// \return true when rotation is supported
    virtual bool get_volume_rotation_supported() const = 0;

    /// Returns the minimum scaling factor for volume scene elements currently supported. The
    /// default of (1, 1, 1) means these scene elements will only be made larger but not smaller.
    ///
    /// \param[out] minimal_scaling     The minimal scaling factors for each axis
    ///
    virtual void get_minimal_volume_scaling(mi::math::Vector_struct<mi::Float32, 3>& minimal_scaling) const = 0;

    /// Returns the size of the subcube border.
    /// \return subcube border size
    virtual mi::Uint32 get_subcube_border_size() const = 0;

    /// Controls additional internal runtime checks for potential CUDA errors.
    ///
    /// \note These checks potentially impair runtime performance and should be only enabled
    /// to generate additional debug information in order to track down certain problems
    /// related to otherwise hard to track CUDA errors. By default the
    /// additional runtime checks are disabled.
    ///
    /// \param[in] debug_checks Enable or disable the runtime checks.
    ///
    virtual void set_cuda_debug_checks_enabled(bool debug_checks) = 0;

    /// Returns whether additional CUDA runtime checks are enabled or disabled.
    ///
    /// \return True when runtime checks are enabled, false otherwise.
    ///
    virtual bool is_cuda_debug_checks_enabled() const = 0;

    /// Controls the state of dynamic memory management. The dynamic memory management is
    /// enabled by default.
    ///
    /// \note The dynamic memory management ensures that the assignment of GPU-memory resources
    ///       to individual rendering primitives can be adapted at runtime to the actual
    ///       requirements. Dynamic memory allocations and de-fragmentation procedures might
    ///       cause occasional but perceivable performance fluctuations. However, by disabling
    ///       the dynamic memory management NVIDIA IndeX will determine static memory assignments
    ///       based on the initial scene configuration, any changes to the scene at runtime will
    ///       not trigger the system to re-assign memory resources. This can lead to either free
    ///       resources to go unused or resource allocation failures.
    ///
    /// \param[in] dyn_mm   Enable or disable the dynamic memory management.
    ///
    virtual void set_dynamic_memory_management_enabled(bool dyn_mm) = 0;

    /// Returns whether the dynamic memory management is enabled or disabled.
    ///
    /// \return True when the dynamic memory management is enabled, false otherwise.
    ///
    virtual bool is_dynamic_memory_management_enabled() const = 0;
    /// @}

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Performance settings
    /// \brief These settings are used for performance tuning depends
    /// on your system and data. See also the span settings.
    ///@{

    /// Returns whether GPU or CPU rendering is used.
    /// \return rendering mode (GPU or CPU)
    virtual Rendering_mode get_rendering_mode() const = 0;
    /// Sets the rendering mode to either GPU or CPU.
    /// (default: GPU_RENDERING)
    /// \param[in] mode Rendering mode to use
    virtual void set_rendering_mode(Rendering_mode mode) = 0;

    /// Returns the number of CPU threads used for rendering.
    /// \return number of CPU threads
    virtual mi::Uint32 get_cpu_thread_count() const = 0;
    /// Sets the number of CPU threads to be used for rendering.
    /// (default: 12)
    /// \param[in] count Number of CPU threads
    virtual void set_cpu_thread_count(mi::Uint32 count) = 0;

    /// Returns the workload distribution mode used for compositing intermediate rendering results
    /// \return workload distribution mode
    virtual Compositing_mode get_compositing_mode() const = 0;

    /// Sets the workload distribution mode used for compositing intermediate rendering results
    /// (default: COMPOSITING_ALL)
    /// \param[in] mode New compositing mode
    virtual void set_compositing_mode(Compositing_mode mode) = 0;

    /// Returns true when rendering and compositing are performed in parallel
    /// \return true when rendering and compositing are parallelized
    virtual bool is_parallel_rendering_and_compositing() const = 0;

    /// Controls if rendering and compositing should be performed in parallel.
    /// (default: true)
    /// \param[in] is_parallel If true, parallel rendering and compositing will be used
    ///
    virtual void set_parallel_rendering_and_compositing(bool is_parallel) = 0;

    /// Sets the size of the rendering result queue.
    ///
    /// Intermediate rendering results are not immediately transferred to other cluster nodes, but
    /// are temporarily stored in a rendering result queue. Only when this queue is full is the
    /// transfer started. This reduces overhead, because a single transfer can be used for multiple
    /// results. (default: 1)
    ///
    /// \param[in] size Number of Rendering results in the queue
    ///
    /// \return    Returns \c true on success.
    ///
    virtual bool set_size_of_rendering_results_in_queue(mi::Uint32 size) = 0;

    /// Returns the size of the rendering result queue.
    ///
    /// \return rendering result queue length
    virtual mi::Uint32 get_size_of_rendering_results_in_queue() const = 0;

    /// Returns if detailed performance monitoring is active.
    ///
    /// \return true when performance monitoring is active
    virtual bool is_monitor_performance_values() const = 0;

    /// Enables or disables detailed performance monitoring.
    /// (default: false)
    ///
    /// \param[in] monitor Monitoring state
    ///
    virtual void set_monitor_performance_values(bool monitor) = 0;

    /// Return the current settings for data transfer over the network
    ///
    /// \return data transfer configuration mode
    virtual Data_transfer_config get_data_transfer_config() const = 0;

    /// Controls the settings for data transfer over the network
    /// (default: see the Data_transfer_config)
    ///
    /// \param[in] config network data transfer configuration (e.g., compression mode, encoding mode.)
    ///
    virtual void set_data_transfer_config(const Data_transfer_config& config) = 0;
    ///@}

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Span settings
    /// \brief These span composition settings affects the system performance
    ///        also.
    ///@{

    /// Returns if automatic span control is enabled.
    ///
    /// \return true if the automatic span control is enabled.
    ///
    virtual bool is_automatic_span_control() const = 0;

    /// Enable or disable the automatic span control
    /// (default: true)
    ///
    /// \param[in] automatic_span_control True to enable the automatic span control mechanism.
    ///
    virtual void set_automatic_span_control(bool automatic_span_control) = 0;

    /// Returns the maximum number of spans per machine
    /// \returns maximum number of spans
    virtual mi::Uint32 get_max_spans_per_machine() const = 0;

    /// Set the Maximum number of spans per machine
    /// (default: 4)
    ///
    /// \param max_spans_per_machine        Maximum number of spans per machine.
    /// \return                             Returns \c true on success.
    ///
    virtual bool set_max_spans_per_machine(mi::Uint32 max_spans_per_machine) = 0;

    /// Returns the number of horizontal spans.
    /// \return number of spans
    virtual mi::Uint32 get_nb_spans() const = 0;

    /// Sets the current number of horizontal spans.
    /// The rendering job will be split up in this many vertically stacked rectangles, to improve
    /// workload distribution. This setting can have a great influence on overall rendering
    /// performance, especially when running with a large number of hosts.
    /// (default: 1)
    ///
    /// \param[in] nb_spans Number of horizontal spans to use
    ///
    /// \return    Returns \c true on success.
    ///
    virtual bool set_nb_spans(mi::Uint32 nb_spans) = 0;
    ///@}

    /// NVIDIA IndeX uses the pixel centers to perform pick operations.
    /// If the lower left corner of the pixel shall be used then the
    /// following call could be used. (default: true)
    ///
    /// \deprecated Shall only be used to be compatible with former applications.
    ///
    /// \param[in]  pixel_center    If \c true, then the pixel center will be used.
    ///
    virtual void set_pick_origin(bool pixel_center) = 0;

    /// NVIDIA IndeX uses the pixel centers to perform pick operations.
    ///
    /// \deprecated Shall only be used to be compatible with former applications.
    ///
    /// \return                     Returns \c true if the pixel center is used for picking.
    ///
    virtual bool get_pick_origin() const = 0;

    ///////////////////////////////////////////////////////////////////////////////////
    /// \name Rendering settings
    /// \brief Rendering settings alter the rendering quality. This
    /// also affects the system performance since there usually is a
    /// trade off between the rendering quality and the performance.
    ///@{

    /// Returns the number of samples per pixel used during rendering.
    /// \returns current number samples.
    virtual mi::Uint32 get_rendering_samples() const = 0;

    /// Set the number of samples per pixel used during rendering.
    /// Using a sample value of 1 or less effectively disables the full screen antialiasing.
    /// (default: 1)
    ///
    /// \param[in] samples The number of samples to be used for each pixel when rendering the scene.
    ///
    virtual void set_rendering_samples(mi::Uint32 samples) = 0;

    /// Returns the current boost geometry color opacity mode.
    /// \return true when boost geometry color opacity mode
    virtual bool is_boost_geometry_colormap_opacity() const = 0;

    /// Set boost geometry color opacity mode.
    ///
    /// This will increase the opacity of colormaps for geometry such as slices, so that the same
    /// colormap can be used for volume and geometric data, without the geometry becoming
    /// invisible, because it is too thin to be visible with the standard colormap settings.
    ///
    /// When this mode is enabled, an internal copy of the active colormap is created where the
    /// opacity values are modified as follows:
    /// \par
    /// <tt>alpha_dest = min(1.0, 0.3 + (alpha_src * 100.0) * 0.7)</tt>
    /// 
    /// (default: false)
    /// 
    /// \deprecated Only intended for testing of slices and heightfields.
    /// \attention Not supported by all shape types.
    ///
    /// \param[in] boost_opacity true when opacity boosting is on
    virtual void set_boost_geometry_colormap_opacity(bool boost_opacity) = 0;

    /// Set global depth test mode. 
    ///
    /// This mode affects raster object and non-raster
    /// object. Non-raster object can be specified per-object test
    /// mode via IDepth_test attribute node.
    /// (default: nv::index::IDepth_test::TEST_LESS_EQUAL)
    ///
    ///
    /// \see IDepth_test
    ///
    /// \param[in] test_mode depth test operator
    virtual void set_depth_test(nv::index::IDepth_test::Depth_test_mode test_mode) = 0;

    /// Get current global depth test mode. 
    ///
    /// \see IDepth_test
    ///
    /// \return current depth test operator
    virtual nv::index::IDepth_test::Depth_test_mode get_depth_test() const = 0;

    ///@}

    /// \name Data loading and workload balancing
    ///@{

    /// Returns if an upload of all scene data is forced.
    ///
    /// \return true when upload data is forced
    ///
    virtual bool is_forced_data_upload() const = 0;

    /// Enable or disable the forced upload of scene data.
    /// (default: false)
    ///
    /// \param[in] upload   True enables the forced data upload.
    ///
    virtual void set_force_data_upload(bool upload) = 0;

    /// Returns if the workload balancing is enabled.
    ///
    /// \return true if the workload balancing is enabled.
    ///
    virtual bool is_workload_balancing_enabled() const = 0;

    /// Enable or disable the workload balancing.
    /// (default: false)
    ///
    /// \param[in] enable   True enables the workload balancing.
    ///
    virtual void set_workload_balancing(bool enable) = 0;
    ///@}

    /// \name Experimental settings
    /// \brief These settings are only experimental. They are not officially supported.
    ///@{

    /// Time step animation settings
    ///
    /// \deprecated Replaced by \c ITime_step_assignment.
    ///
    /// \return total number of timesteps
    virtual mi::Uint32 get_nb_timesteps() const = 0;

    /// Set total number of timesteps (default: 0)
    ///
    /// \deprecated Replaced by \c ITime_step_assignment.
    ///
    /// \param[in] nb_timesteps total number of timesteps
    virtual void set_nb_timesteps(mi::Uint32 nb_timesteps) = 0;
    
    /// \deprecated Replaced by \c ITime_step_assignment.
    ///
    /// \return current timestep
    virtual mi::Uint32 get_current_timestep() const = 0;

    /// Set current timestep (default: 0)
    ///
    /// \deprecated Replaced by \c ITime_step_assignment.
    ///
    /// \param[in] current_timestep current timestep to be set
    virtual void set_current_timestep(mi::Uint32 current_timestep) = 0;

    /// Frame buffer blending allows for colored backgrounds 
    /// also in a non GL frame buffer. 
    ///
    /// \param enable_flag      Enables or disables blending.
    ///
    virtual void enable_frame_buffer_blending(bool enable_flag) = 0;

    /// \return     Returns 'true' if blending is enabled.
    virtual bool is_frame_buffer_blending_enabled() const = 0;

    /// Set heightfield normal calculation mode
    ///
    /// \param[in] calculation_mode The mode of normal calculation.
    virtual void set_heightfield_normal_calculation_mode(Heightfield_normal_calculation_mode calculation_mode) = 0;

    /// Get heightfield normal calculation mode
    ///
    /// \return current heightfield normal calculation mode
    virtual Heightfield_normal_calculation_mode get_heightfield_normal_calculation_mode() const = 0;

    /// Triggers recompilation of all custom CUDA programs on next use.
    virtual void flush_custom_cuda_programs() = 0;

    /// Set threshold and mode for picking inside a volume.
    ///
    /// There are two threshold modes: When the threshold value is negative or 0 then the picking
    /// will return the first volume sample that has a opacity above the absolute threshold, i.e.
    /// sample >= fabs(threshold).
    ///
    /// When the threshold value is positive then the volume picking will use color accumulation and
    /// will return when the accumulated opacity is above the threshold. This the same approach as
    /// for volume rendering, meaning that picking result will match the rendering.
    ///
    /// The default threshold value is -1.
    ///
    /// \note This API is experimental and subject to change.
    ///
    /// \param[in] threshold Threshold value.
    ///
    virtual void set_volume_picking_threshold(mi::Float32 threshold) = 0;

    /// Return the threshold for picking inside a volume.
    ///
    /// \return Threshold value.
    virtual mi::Float32 get_volume_picking_threshold() const = 0;

    ///@}
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICONFIG_SETTINGS_H
