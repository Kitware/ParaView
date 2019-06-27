/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute representing user programmable rendering kernel
///        components such as surface intersection programs or volume sample programs.

#ifndef NVIDIA_INDEX_IRENDERING_KERNEL_PROGRAMS_H
#define NVIDIA_INDEX_IRENDERING_KERNEL_PROGRAMS_H

#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv {
namespace index {

/// @ingroup nv_index_scene_description_attribute
/// The interface class representing user-programmable rendering kernel components.
///
/// There are two sub-classes from this interface representing rendering kernel programs applied to volumetric
/// primitives, such as regular volumes (\c IVolume_sample_program), and applied to surface primitives, such as
/// regular heightfields or planes (\c ISurface_sample_program). These classes act as scene attributes to the
/// particular scene elements.
///
/// These programs are not compiled in-place but on each rendering node depending on the encountered GPU-devices and
/// their particular compute capabilities. Failure to compile a rendering kernel program results in an error
/// containing the compile log. In order to indicate such a failure to the user, the primitives the particular program
/// attribute is applied to are rendered in a signal color (deep-pink color).
///
/// \note For best performance of the compiled rendering kernels, methods defined in device code need to be
/// decorated with special qualifiers. The qualifiers required by NVIDIA IndeX are covered by the
/// \c NV_IDX_DEVICE_INLINE_MEMBER macro. While it is not strictly required to use this macro, it is advised to do so.
///
class IRendering_kernel_program :
    public mi::base::Interface_declare<0x4953ee6c,0x1099,0x47de,0x94,0xa8,0x5e,0xb7,0x17,0xe4,0x83,0x7d,
                                       nv::index::IAttribute>
{
public:
    struct Program_options
    {
        mi::Sint32                                  max_registers;  ///< Define the maximum number of registers to use for the program.
                                                                    ///< Special values:  0 - use NVIDIA IndeX internal limit
                                                                    ///<                 -1 - no limit, let CUDA decide the register count
                                                                    ///< (default value: 0)
        mi::math::Vector_struct<mi::Uint32, 2>      block_size;     ///< Define the execution block size of the rendering kernel using
                                                                    ///< the program.
                                                                    ///< Special values: (0, 0) - let NVIDIA IndeX decide
                                                                    ///< the block size)
                                                                    ///< (default value: (0, 0))
        mi::Uint32                                  debug_flags;    ///< Internal debug options.
                                                                    ///< (default value: 0)
    };

public:
    /// Sets the source code for the rendering kernel program.
    ///
    /// \param[in] prog_src     String containing the program source code.
    ///
    virtual void set_program_source(const char* prog_src) = 0;

    /// Returns the source code for the rendering kernel program.
    ///
    /// \return String containing the program source code.
    ///
    virtual const char* get_program_source() const = 0;

    /// Sets the program options for the rendering kernel program.
    ///
    /// \param[in] prog_options Program options to set for the kernel program.
    ///
    virtual void set_program_options(const Program_options& prog_options) = 0;

    /// Returns the program options of the rendering kernel program.
    ///
    /// \return The currently set program options.
    ///
    virtual const Program_options& get_program_options() const = 0;
};

/// @ingroup nv_index_scene_description_attribute
/// Defines headers that can be included by rendering kernel programs. The attribute is applied to a
/// scene element and then affects any \c IRendering_kernel_program that is also applied.
///
/// Multiple headers can be defined, each identified by the name by which it can be included in the
/// program. While the name may look like a file path such as <tt>/includes/common/utils.h</tt> and the
/// header can be included as <tt>\#include "/includes/common/utils.h"</tt>, it is really just an
/// identifier. This attribute does not perform any file access.
///
/// When multiple instances of this attribute are active for a rendered scene element, only the
/// instance that is defined closest to the scene element in the scene description hierarchy will be
/// used.
///
class IRendering_kernel_program_headers :
    public mi::base::Interface_declare<0xa5edda1a,0x9a97,0x40c1,0xa2,0xe7,0xf2,0x44,0x2d,0x2d,0x55,0xc7,
                                       nv::index::IAttribute>
{
public:
    /// Stores a header in the attribute. Any existing header with the same name will be
    /// overwritten.
    ///
    /// \param[in] include_name   Name by which the header can be included. Must not be null or an
    ///                           empty string.
    /// \param[in] header_src     Header source code. If null, any existing entry will be removed.
    ///
    virtual void set_header(
        const char* include_name,
        const char* header_src) = 0;

    /// Returns the header source code for a given name.
    ///
    /// \param[in] include_name   Name by which the header can be included.
    /// \return Header source code, or null if no header is defined for the name.
    ///
    virtual const char* get_header(const char* include_name) const = 0;

    /// Returns the total number of headers stored in the attribute.
    ///
    /// \return Number of entries.
    ///
    virtual mi::Uint32 get_nb_headers() const = 0;

    /// Returns the name and header source code for the header with the given index.
    ///
    /// \param[in]  index             Index of the header, must be less than \c get_nb_headers().
    /// \param[out] include_name_out  Will be filled with a pointer to the name of the entry, or
    ///                               null if the index is invalid.
    /// \param[out] header_src_out    Will be filled with a pointer to the header source code.
    ///
    virtual void get_header(
        mi::Uint32   index,
        const char** include_name_out,
        const char** header_src_out) const = 0;

    /// Removes all headers from the attributes.
    ///
    virtual void clear() = 0;
};

/// @ingroup nv_index_scene_description_attribute
/// The interface class representing user-defined parameter buffers for user-programmable rendering kernel components.
///
/// User-defined parameter buffers allow the application to define custom input to \c IRendering_kernel_program
/// instances. Such input can be utilized to implement custom rendering kernel programs independent of the input made
/// available through the NVIDIA IndeX rendering kernel interfaces. For instance, an application is enabled to
/// implement a specialized material shading method that is not pre-defined by any existing NVIDIA IndeX rendering
/// properties. The required material parameters, unknown to the actual rendering system, are passed through this
/// interface class to NVIDIA IndeX, which handles the distribution of such data to each rendering node and rendering
/// device. The interface class acts as a scene attribute to scene elements enabled to utilize rendering kernel components
/// (\c IRendering_kernel_program).
///
/// Each instance of this interface allows for a certain number of data buffers to be added. The buffer data is
/// assigned to so called buffer slots. These buffer slots are a way to identify the actual buffers and access them
/// in the rendering kernel programs, as shown in the following example code snippet of a volume sample program.
///
/// This example illustrates the input of clip-plane parameters on the application side:
///
/// \code
/// struct Plane_data_struct
/// {
///     mi::math::Vector<mi::Float32, 4>   plane_equation;
/// };
///
/// // Define two clip planes
/// Plane_data_struct planes[2];
///
/// // First clip plane
/// mi::math::Vector<mi::Float32, 3> clip_plane_0_normal = mi::math::Vector<mi::Float32, 3>(1.0f, 1.0f, -1.0f);
/// clip_plane_0_normal.normalize();
/// planes[0].plane_equation = mi::math::Vector<mi::Float32, 4>(clip_plane_0_normal, -11.0f);
///
/// // Second clip plane
/// mi::math::Vector<mi::Float32, 3> clip_plane_1_normal = mi::math::Vector<mi::Float32, 3>(-1.0f, 1.0f, 1.0f);
/// clip_plane_1_normal.normalize();
/// planes[1].plane_equation = mi::math::Vector<mi::Float32, 4>(clip_plane_1_normal, 10.0f);
///
/// // Add the two defined planes to the rendering kernel parameter buffers
/// kernel_param_buffers->set_buffer_data(CLIP_PLANE_PARAM_BUFFER_SLOT, planes, sizeof(Plane_data_struct) * 2);
/// \endcode
///
/// This example illustrates how to access clip-plane information defined by the application in a rendering kernel
/// program:
///
/// \code
/// // Define the user-defined data structure
/// struct Plane_data_buffer
/// {
///    float4 plane_equation;
/// };
///
/// class Volume_sample_program
/// {
///     NV_IDX_VOLUME_SAMPLE_PROGRAM
///
/// public:
///     // Define variables to bind the contents of the user-defined buffer to
///     const Plane_data_buffer*  m_plane_array_buffer; ///< access the entire array of user data
///     const Plane_data_buffer*  m_plane_buffer_0;     ///< access a specific entry of the user data
///     const Plane_data_buffer*  m_plane_buffer_1;
///
/// public:
///     NV_IDX_DEVICE_INLINE_MEMBER
///     void initialize()
///     {
///         // Bind the contents of the buffer slot 0 to the variable
///         m_plane_array_buffer = state.bind_parameter_buffer<Plane_data_buffer>(0);
///
///         // Bind the contents of the buffer slot 0 to the variable pointing to the first entry
///         m_plane_buffer_0 = state.bind_parameter_buffer<Plane_data_buffer>(0, 0 /*array offset*/);
///         // Bind the contents of the buffer slot 0 to the variable pointing to the second entry
///         m_plane_buffer_1 = state.bind_parameter_buffer<Plane_data_buffer>(0, 1 /*array offset*/);
///     }
/// ...
///     NV_IDX_DEVICE_INLINE_MEMBER
///     int execute(
///        const Sample_info_self&     sample_info,
///              Sample_output&        sample_output)
///     {
///         const float dist_p_00 = dot(m_plane_array_buffer[0].plane_equation, make_float4(sample_info.sample_position, 1.0f));
///         const float dist_p_01 = dot(m_plane_array_buffer[1].plane_equation, make_float4(sample_info.sample_position, 1.0f));
/// \endcode
///
/// or to access the same data through the separate entries:
///
/// \code
///         const float dist_p_00 = dot(m_plane_buffer_0->plane_equation, make_float4(sample_info.sample_position, 1.0f));
///         const float dist_p_01 = dot(m_plane_buffer_1->plane_equation, make_float4(sample_info.sample_position, 1.0f));
/// ...
/// };
/// \endcode
///
/// As shown in the above example the association of data input through this interface class and the actual data
/// at runtime happens through the buffer slot identifiers. Further, the example illustrates that a buffer slot can
/// actually hold an array of structures (in this example an array of \c Plane_data_buffer instances). It is possible
/// to access such arrays in a straightforward way through the array index operator or by binding the contents of the
/// array to individual variables. The function to perform the binding (\c state.bind_parameter_buffer()) allows both use
/// cases:
///
/// \code
/// NV_IDX_DEVICE_INLINE_MEMBER
/// const T* bind_parameter_buffer(
///             unsigned buffer_slot,
///             unsigned buffer_slot_offset = 0u);
/// \endcode
///
/// The \c buffer_slot_offset parameter allows to select a particular array element. The offset unit is the actual
/// number of array elements to offset the binding.
///
/// It is important to note that the contents of the buffer data input through this interface must conform to certain
/// rules to ensure data consistency between the host and device data. Especially, data alignment of buffer contents
/// must be regarded. For example, certain vector data types require alignment to special boundaries inside structures:
/// http://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#vector-types__alignment-requirements-in-device-code
///
/// Example for misaligned data between host and device:
///
/// \code
/// struct wrong_alignment
/// {
///     float3  normal;
///     float4  color; // misaligned
/// };
/// \endcode
///
/// In this example the 'color' entry of the structure is not correctly aligned to the struct. A \c float4 variable requires
/// a 4 byte alignment. Such cases can be corrected by manually adding padding data:
///
/// \code
/// struct correct_alignment
/// {
///     float3  normal;
///     char    padding_data[4]; // align the next entry to a 4 byte boundary
///     float4  color;
/// };
/// \endcode
///
/// Similarly, when storing multiple elements in an array of structures, the individual structures require alignment to
/// a certain boundary. This required array offset alignment can be queried through this interface using the
/// \c get_array_offset_alignment() method. In order to achieve correct structure alignment, padding data can be used:
///
/// \code
/// struct array_element
/// {
///     float material_property_0;
///     float material_property_1;
///     float material_property_2;
///     char  padding[4]; // pad structure to multiple of 16 bytes
/// };
/// \endcode
///
class IRendering_kernel_program_parameters :
    public mi::base::Interface_declare<0x82ad1cdf,0xcbac,0x452f,0x9b,0x63,0x13,0xe4,0x94,0xc9,0xbf,0xaa,
                                       nv::index::IAttribute>
{
public:
    /// Returns the number of valid buffer slots available to assign user-defined data to.
    ///
    /// \returns Returns the number of valid buffer slots available.
    ///
    virtual mi::Uint32 get_nb_buffer_slots() const = 0;

    /// Returns the maximum size of an individual buffer assignable to an individual buffer slot.
    ///
    /// \returns Maximum size of an individual buffer assignable to a buffer slot.
    ///
    virtual mi::Size get_max_buffer_size() const = 0;

    /// Returns the alignment in bytes of array components in a single buffer assigned to a buffer slot.
    ///
    /// \returns Alignment in bytes of array components in a single buffer.
    ///
    virtual mi::Size get_array_offset_alignment() const = 0;

    /// Assigns the buffer contents to a particular buffer slot.
    ///
    /// \note       An instance of the \c IRendering_kernel_program_parameters class does not take ownership of the
    ///             user data passed through the \c set_buffer_data() method. The buffer data is copied for internal use.
    ///
    /// \param[in]  slot_idx   The buffer slot index to assign the buffer data to.
    /// \param[in]  data       A raw pointer to the buffer data to assign to the buffer slot.
    /// \param[in]  data_size  The size in bytes of the buffer data to assign to the buffer slot.
    ///
    /// \returns    True on successful input of the user buffer data to the specified buffer slot, false otherwise.
    ///
    virtual bool set_buffer_data(
        mi::Uint32       slot_idx,
        const void*      data,
        mi::Size         data_size) = 0;
};

/// @ingroup nv_index_scene_description_attribute
/// Scene attribute to map scene elements to slots that are accessible by user-programmable
/// rendering kernel components.
///
/// The attribute is similar to \c IRendering_kernel_program_parameters, but instead of providing a
/// user-defined buffer, it provides a fixed number of \e slots, where scene elements can be mapped
/// to. These scene elements (e.g., \c IRegular_volume can then be accessed inside a
/// user-defined program using \c state.scene.access().
///
/// For example, consider the mapping attribute is applied to \c IPlane, mapping an existing \c
/// IRegular_volume to slot 1 by calling <tt>mapping_attribute.set_mapping(1, volume_tag)</tt>. A \c
/// IVolume_sample_program also applied to the plane may then access the volume data as follows:
///
/// \code
/// NV_IDX_XAC_VERSION_1_0
///
/// class Volume_sample_program
/// {
///     NV_IDX_VOLUME_SAMPLE_PROGRAM
/// public:
///     NV_IDX_DEVICE_INLINE_MEMBER
///     void initialize() {}
///
///     NV_IDX_DEVICE_INLINE_MEMBER
///     int execute(
///        const Sample_info_self&     sample_info,
///              Sample_output&        sample_output)
///     {
///         const int SLOT_ID = 1;
///         const auto volume = state.scene.access<xac::Regular_volume>(SLOT_ID);
///         const float v = volume.sample(sample_info.sample_position);
///         sample_output.color = make_float4(v, v, v, 1.f);
///         return NV_IDX_PROG_OK;
///     }
/// };
/// \endcode
///
/// The scene element that should be mapped to a slot must be part of the scene description. When
/// mapping attributes (\c IAttribute), they must be enabled and also assigned to the scene element
/// to which this mapping attribute is assigned.
///
class IRendering_kernel_program_scene_element_mapping :
    public mi::base::Interface_declare<0x8ecf51a1,0xdf74,0x47a1,0x84,0x27,0xae,0xb0,0x14,0xee,0x52,0xc8,
                                       nv::index::IAttribute>
{
public:
    /// Returns the number of slots available to assign mappings to.
    ///
    /// \return Number of slots available.
    ///
    virtual mi::Uint32 get_nb_slots() const = 0;

    /// Maps a scene element to a slot.
    ///
    /// \param[in]  slot_idx   Slot index, must be less than \c get_nb_slots().
    /// \param[in]  data_tag   Tag of a scene element. If \c NULL_TAG, an existing mapping is removed.
    ///
    /// \return     True is \c slot_idx is valid.
    ///
    virtual bool set_mapping(
        mi::Uint32                 slot_idx,
        mi::neuraylib::Tag_struct  data_tag) = 0;

    ///
    enum Query_reference
    {
        RELATIVE_QUERY_BOUNDING_BOX = 0, ///< 
        ABSOLUTE_QUERY_BOUNDING_BOX = 1  ///< 
    };

    /// Maps a scene element to a slot.
    ///
    /// \param[in]  slot_idx    Slot index, must be less than \c get_nb_slots().
    /// \param[in]  data_tag    Tag of a scene element. If \c NULL_TAG, an existing mapping is removed.
    /// \param[in]  query_bbox  Bounding box ensuring that all data inside becomes available for 
    ///                         use by the kernel program through the given slot id.
    /// \param[in]  reference   The query reference indicates whether the query bounding box shall be
    ///                         applied relative to each data point or whether the bounding box represents a 
    ///                         absolute spatial area for distributed data accessing.
    ///
    /// \return     True is \c slot_idx is valid.
    ///
    virtual bool set_mapping(
        mi::Uint32                                   slot_idx,
        mi::neuraylib::Tag_struct                    data_tag,
        const mi::math::Bbox_struct<mi::Float32, 3>& query_bbox,
        Query_reference                              reference = RELATIVE_QUERY_BOUNDING_BOX) = 0;

    /// Returns the mapping of a given slot.
    ///
    /// \param[in]  slot_idx   Slot index, must be less than \c get_nb_slots().
    ///
    /// \return     Tag of the scene element that is mapped to the slot, or \c NULL_TAG if it is not
    ///             mapped or the slot index is invalid.
    ///
    virtual mi::neuraylib::Tag_struct get_mapping(
        mi::Uint32                 slot_idx) const = 0;
};

/// @ingroup nv_index_scene_description_attribute
/// An interface class representing rendering kernel programs applied to volume primitives (e.g., \c IRegular_volume).
///
/// The programs applied to volumes are evaluated for each sample taken during the rendering process. An example of a
/// volume sample program is:
///
/// \code
/// NV_IDX_XAC_VERSION_1_0
///
/// class Volume_sample_program
/// {
///     NV_IDX_VOLUME_SAMPLE_PROGRAM
///
/// public:
///     NV_IDX_DEVICE_INLINE_MEMBER
///     void initialize() {}
///
///     NV_IDX_DEVICE_INLINE_MEMBER
///     int execute(
///        const Sample_info_self&     sample_info,
///              Sample_output&        sample_output)
///     {
///         // Scalar volume sample
///         const float  volume_sample = state.volume.sample(sample_info.sample_position);
///         // Look up color and opacity values from colormap
///         const float4 sample_color  = state.colormap.lookup(volume_sample);
///
///         if (0.3f < volume_sample && volume_sample < 0.7f)
///         {
///             return NV_IDX_PROG_DISCARD_SAMPLE;
///         }
///
///         // Output swizzled color
///         sample_output.color = make_float4(
///                                    sample_color.y,
///                                    sample_color.z,
///                                    sample_color.x,
///                                    sample_color.w);
///
///         return NV_IDX_PROG_OK;
///     }
/// };
/// \endcode
///
/// Currently the class representing a volume sample program needs to follow the template set in the example above. It
/// has to be called \c Volume_sample_program and is required to expose two public methods called
/// \c initialize(), which is executed once per rendering instance of a subregion, and \c execute(),
/// which is then executed for each sample taken along a viewing ray.
///
/// Each volume sample program is also required to have the \c NV_IDX_VOLUME_SAMPLE_PROGRAM macro as the first
/// definition inside the program class. This macro initializes the internal state of the program exposed to the user.
/// Later revisions of the rendering kernel programs feature will make the definition more straightforward.
///
/// The \c initialize() method can be used to initialize variables used in the per-sample computation of the
/// \c execute() method. The initializations can encompass the calculation of static data such as lighting or material
/// properties.
///
/// The \c execute() method always receives the current sample position as input and the output color as output. Through
/// the return value of this method the rendering kernel can be informed to use the generated output sample or to
/// discard the current sample. For this purpose there are two return values for the \c execute() method:
/// - \c NV_IDX_PROG_OK
/// - \c NV_IDX_PROG_DISCARD_SAMPLE
///
/// The list of return values may be extended in the future to, for instance, discard entire ray segments.
///
/// Each instance of a volume sample program has access to a state variable allowing access to the current volume data
/// set as well as the current colormap. The contents of this state is derived from the scene definition. The state
/// is accessed through the \c state variable available to each instance. The state contains a \c volume and \c colormap
/// variable, which can be used to sample the volume and lookup color and opacity values from a colormap, as demonstrated
/// in the example above. It is important to note that the return value of the \c sample() method is dependent on the
/// actual volume type. A scalar volume, such as \c uint8 or \c float32 volumes return a simple float value, whereas RGBA-
/// volumes return a \c float4 value.
///
class IVolume_sample_program :
    public mi::base::Interface_declare<0x4c2b9169,0xb377,0x452a,0x8f,0xfa,0x87,0x70,0x35,0xd9,0xa2,0xe8,
                                       nv::index::IRendering_kernel_program>
{
};

/// @ingroup nv_index_scene_description_attribute
/// An interface class representing rendering kernel programs applied to surface primitives (e.g., \c IRegular_heightfield).
///
/// The programs applied to surface geometries are evaluated for each surface intersection encountered during the
/// rendering process. An example of a surface sample program is:
///
/// \code
/// NV_IDX_XAC_VERSION_1_0
///
/// class Surface_sample_program
/// {
///     NV_IDX_SURFACE_SAMPLE_PROGRAM
///
///     float3 light_direction;
///     float3 material_ka;     // ambient term
///
/// public:
///     NV_IDX_DEVICE_INLINE_MEMBER
///     void initialize()
///     {
///         light_direction = normalize(make_float3(1.0f, 1.0f, 1.0f));
///         material_ka     = make_float3(0.1f, 0.1f, 0.1f);
///     }
///
///     NV_IDX_DEVICE_INLINE_MEMBER
///     int execute(
///        const   Sample_info_self&  sample_info,         // read-only
///                Sample_output&     sample_output)       // write-only
///     {
///         if (sample_info.sample_color.w < 0.1f)
///         {
///             return NV_IDX_PROG_DISCARD_SAMPLE;
///         }
///
///         const float n_l          = dot(sample_normal, light_direction);
///
///         const bool  soft_diffuse = false;
///         const float diffuse      =   (soft_diffuse)
///                                    ? n_l * 0.5f + 0.5f
///                                    : max(0.0f, n_l);
///
///         const float3 kd      = make_float3(sample_info.sample_color);
///         const float3 s_lit   = material_ka + kd * diffuse;
///
///         sample_output.color = make_float4(s_lit, sample_info.sample_color.w);
///
///         return NV_IDX_PROG_OK;
///     }
///
/// };
/// \endcode
///
/// Currently the class representing a surface sample program needs to follow the template set in the example above. It
/// has to be called \c Surface_sample_program and is required to expose two public methods called
/// \c initialize(), which is executed once per rendering instance of a subregion, and execute(), which is then
/// executed for each surface intersection encountered along a viewing ray.
///
/// Each surface sample program is also required to have the \c NV_IDX_SURFACE_SAMPLE_PROGRAM macro as the first
/// definition inside the program class. This macro initializes the internal state of the program exposed to the user.
/// Later revisions of the rendering kernel programs feature will make the definition more straightforward.
///
/// The \c initialize() method can be used to initialize variables used in the per-intersection computation of the
/// \c execute() method. The initializations can encompass the calculation of static data such as lighting or material
/// properties.
///
/// The \c execute() method always receives the current sample position, normal and color as input and the output color as
/// output. The sample distance is passed as a modifiable reference to the method, which allows surface sample programs
/// to override the intersection distance value individually. This feature can be utilized to implement custom depth-offset
/// techniques to applicable surface geometries. Through the return value of this method the rendering kernel can
/// be informed to use the generated output sample or to discard the current sample. For this purpose there are two
/// return values for the \c execute() method:
/// - \c NV_IDX_PROG_OK
/// - \c NV_IDX_PROG_DISCARD_SAMPLE
///
/// The list of return values may be extended in the future to, for instance, discard entire ray segments.
///
/// In contrast to \c IVolume_sample_program instances, a surface sample program does currently not expose an internal
/// state variable. This is subject to change in future revisions of this feature.
///
class ISurface_sample_program :
    public mi::base::Interface_declare<0x861e254b,0x6ade,0x4579,0x93,0x15,0x23,0x2d,0x5c,0x3f,0xe5,0xfd,
                                       nv::index::IRendering_kernel_program>
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IRENDERING_KERNEL_PROGRAMS_H
