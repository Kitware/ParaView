/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief  Interface for accessing the monitored performance values.

#ifndef NVIDIA_INDEX_IPERFORMANCE_VALUES_H
#define NVIDIA_INDEX_IPERFORMANCE_VALUES_H

#include <mi/dice.h>

namespace nv {

namespace index {

/// @ingroup nv_index_performance_measurement
/// Interface class that exposes the performance values and statistics of a single horizontal span.
///
/// For each span, detailed information about the compositing time, the transferred image data (memory), the
/// number of subcubes considered, etc. are gathered.
///
class IPer_span_statistics :
    public mi::base::Interface_declare<0x323be64a,0xbd51,0x4078,0xb3,0x29,0xf6,0xe8,0x04,0x93,0x70,0xeb,
                                       mi::neuraylib::ISerializable>
{
public:
    /// Returns the id of the span for which the present performance values have been gathered.
    /// \return span id
    virtual mi::Uint32 get_span_id() const = 0;

    /// Returns the id of the cluster node that was responsible for the span compositing.
    /// \return cluster node id
    virtual mi::Uint32 get_cluster_node_id() const = 0;

    /// Returns the number of type names for performance values.
    /// \return number of type names
    virtual mi::Uint32 get_nb_type_names() const = 0;

    /// Returns type name string by the given index.
    /// \param[in] index index of type name
    /// \return statistics type name, or 0 when an invalid index is given.
    virtual const char* get_type_name(mi::Uint32 index) const = 0;

    /// Returns the value of the given performance counter.
    /// \param[in] type_name type name
    /// \return value
    virtual mi::Uint64 get(const char* type_name) const = 0;

    /// Returns the value of the given performance value, interpreted as time in milliseconds.
    /// \param[in] type_name type name
    /// \return time in milliseconds
    virtual mi::Float32 get_time(const char* type_name) const = 0;
};

/// @ingroup nv_index_performance_measurement
/// Interface class to query overall system performance values and statistics.
class IPerformance_values :
    public mi::base::Interface_declare<0xa3ed753a,0x5a9c,0x4157,0xa4,0x71,0x52,0x86,0x5f,0x5f,0x43,0xd9>
{
public:
    /// Internal resolution of time values (in 1/milliseconds).
    static const mi::Uint64  TIME_RESOLUTION     = 1000000;

    /// Value returned by \c get() when an invalid type was specified.
    static const mi::Uint64  INVALID_VALUE       = ~0ULL;

    /// Value returned by \c get_time() when an invalid type was specified.
    /// \return invalid time value
    static mi::Float32 INVALID_VALUE_FLOAT() { return  -1.f; }

    /// Returns the value of the given performance counter.
    ///
    /// The following types of performance counters are available:
    /// - nb_subcubes_rendered: Number of subcubes that were rendered,
    ///   i.e., are visible and not empty.
    /// - size_volume_data_rendered: Size (in bytes) of the volume data
    ///   accessed during rendering.
    /// - size_horizon_data_rendered: Size (in bytes) of the heightfield data
    ///   accessed during rendering.
    /// - nb_horizon_triangles_rendered: Approximation of the number
    ///   of horizon triangles rendered.
    /// - time_rendering_horizon: Time just for rendering the horizon
    ///   data.
    /// - time_rendering_volume: Time just for rendering the volume
    ///   data.
    /// - time_rendering_volume_and_horizon: Time just for rendering
    ///   volume and heightfield data (in the same subcube).
    /// - time_rendering_only: Time just for the basic rendering call
    ///   without per-subcube initialization (sum of
    ///   time_rendering_*).
    /// - time_gpu_upload: Time for uploading data to the GPU
    ///   (e.g. volume data).
    /// - time_gpu_download: Time for downloading data from the GPU
    ///   (e.g. rendering results).
    /// - time_rendering: Time for rendering the subcubes, with
    ///   per-subcube initialization.
    /// - time_rendering_total_sum: Summed-up rendering time of all
    ///   GPU or CPU threads.
    /// - size_volume_data_upload: Size (in bytes) of the uploaded
    ///   volume data.
    /// - size_rendering_results_download: Size (in bytes) of the
    ///   downloaded rendering results.
    /// - size_pinned_host_memory: Size (in bytes) of the allocated
    ///   page-locked (pinned) host memory.
    /// - size_unpinned_host_memory: Size (in bytes) of the allocated
    ///   normal (unpinned) host memory.
    /// - size_gpu_memory_used: Size (in bytes) of the device memory on
    ///   the GPUs that is allocated by IndeX.
    /// - size_gpu_memory_total: Size (in bytes) of the total device
    ///   memory on the GPUs.
    /// - size_gpu_memory_available: Size (in bytes) of the available (free)
    ///   device   memory on the GPU.
    /// - size_system_memory_usage: Size (in bytes) of the
    ///   allocated system memory (on the host). The approximate value is
    ///   queried using a system call.
    /// - size_transfer_compositing: Amount of image data (in bytes)
    ///   sent to remote hosts for compositing.
    /// - nb_fragments: Total number of GPUs or number of CPU threads
    ///   used for rendering.
    /// - is_using_gpu: Number of GPUs used for rendering, zero for
    ///   all-CPU rendering.
    /// - time_total_rendering: Time for the entire rendering,
    ///   including all initialization.
    /// - time_total_compositing: Time for compositing the rendering
    ///   results into the final image.
    /// - time_total_final_compositing: Time for compositing the final
    ///   result into the user-defined framebuffer.
    /// - time_complete_frame: Time for generating a complete frame,
    ///   includes initialization, rendering and compositing.
    /// - time_frame_setup: Time for global initialization before
    ///   rendering is finished.
    /// - time_frame_finish: Time for global deinitialization after
    ///   compositing.
    /// - time_avg_rendering: Average rendering time per GPU or CPU
    ///   thread.
    /// - frames_per_second: Rendered frames per second (corresponding
    ///   to time_complete_frame).
    /// - nb_horizontal_spans: Number of horizontal spans used.
    /// - size_zbuffer_transfer: Size of the z-buffer used for OpenGL
    ///   integration.
    /// - size_zbuffer_transfer_compressed: Size of the compressed z-buffer
    ///   that is transferred over the network.
    /// - time_zbuffer_serialize: Time taken for processing the z-buffer,
    ///   e.g. compression.
    ///
    /// The types that have names starting with "time_" as well as the type "frames_per_second" are
    /// to be accessed using \c get_time(), for all other types \c get() should be used.
    ///
    /// \param type_name Name of the performance counter type
    /// \param host_id   If 0, request globally accumulated values, else return per-host values for
    ///                  the given host id.
    /// \return Raw integer value of the performance counter, or #INVALID_VALUE if the given type
    ///         does not exist. For time values you need to multiply the result with
    ///         #TIME_RESOLUTION to get milliseconds (or just call get_time()).
    virtual mi::Uint64 get(const char* type_name, mi::Uint32 host_id = 0) const = 0;

    /// Returns the value of the given performance value, interpreted as time in milliseconds.
    ///
    /// See \c get() for a description of the supported performance counter types.
    ///
    /// \param type_name Name of the performance counter type
    /// \param host_id   If 0, request globally accumulated values, else return per-host values for
    ///                  the given host id.
    /// \return milliseconds of the given performance counter, or #INVALID_VALUE_FLOAT if the given
    ///         type does not exist
    virtual mi::Float32 get_time(const char* type_name, mi::Uint32 host_id = 0) const = 0;

    /// Returns the number of different performance value types.
    ///
    /// \return number of performance value types
    virtual mi::Uint32 get_nb_type_names() const = 0;

    /// Returns the name of the given performance value type.
    ///
    /// \param index Specifies the type
    /// \return name of the specified type, or 0 for invalid index
    virtual const char* get_type_name(mi::Uint32 index) const = 0;

    /// Returns the size of host id array.
    ///
    /// \return size of the host id array.
    virtual mi::Size get_nb_host_ids() const = 0;

    /// Returns the host id array.
    ///
    /// \param[out] host_id_array A host id array to be filled by this
    /// method. The application must pre-allocate the array before
    /// calling this method. The size must be at least the size of the
    /// value returned by \c get_nb_host_ids().
    virtual void get_host_id_array(mi::Uint32 host_id_array[]) const = 0;

    /// Returns performance statistics for the given horizontal span.
    /// The range of span_id is [0, nb_horizontal_spans], where
    /// nb_horizontal_spans can be accessed with the \c get() method.
    ///
    /// \param[in] span_id  The requested span id.
    /// \return per span statistics object associated with the span_id,
    /// or 0 when span_id is invalid.
    virtual IPer_span_statistics* get_per_span_statistics(mi::Uint32 span_id) const = 0;

};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPERFORMANCE_VALUES_H
