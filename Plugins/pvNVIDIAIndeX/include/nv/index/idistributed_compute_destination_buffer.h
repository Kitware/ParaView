 /******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Asynchronous texture generation for use with shapes.

#ifndef NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_DESTINATION_BUFFER_H
#define NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_DESTINATION_BUFFER_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/idistributed_data_subset.h>

#include <nv/index/iirregular_volume_subset.h>

#include <nv/index/isparse_volume_subset.h>

namespace nv {
namespace index {

/// The interface class enables the asynchronous generation of 2D texture
/// buffers and 3D volume data. Once the generation of compute-destination buffers
/// has been triggered, for example, by means of the interface class
/// \c IDistributed_compute_technique, the user-defined technique can populate the buffer
/// contents with the values required for later texture mapping and rendering.
/// Meanwhile, the rendering system may proceed to render datasets that
/// don't require a particular user-defined texturing technique.
///
/// In particular, the interface class enables leveraging designated compute
/// clusters for data generation while hiding the common network transfer and latency
/// issues.
///
/// If a compute-destination buffer has been filled the buffer can be made available to
/// the calling rendering system.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer :
    public mi::base::Interface_declare<0xfaae5c5,0x2701,0x442e,0x8f,0x77,0xe3,0x3,0xed,0xd,0x6f,0x5c>
{
public:
    /// The interface method returns the bounding box of the subregion for which
    /// the compute technique has been invoked. The rendering system
    /// initializes the bounding box that then can be accessed by an
    /// user-defined technique to control the texture generation.
    ///
    /// \return     The bounding box of the subregion, which is defined in the
    ///             global coordinate system.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3>   get_subregion_bbox() const = 0;

    /// Returns the 2D area that the subregion covers in screen space. Based on
    /// the given screen-space area a user-defined technique can choose, for
    /// instance, the least compute-intensive generation technique.
    ///
    /// The rendering system computes and initializes the screen-space coverage.
    ///
    /// \return     The screen-space area that the subregion covers defined in
    ///             window coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Uint32, 2>    get_screen_space_area() const = 0;

    /// GPU-device id if the destination buffer is located on a GPU-device.
    ///
    /// \returns GPU-device id, negative values indicate that the data is currently not stored on any device.
    ///
    virtual mi::Sint32                              get_gpu_device_id() const = 0;

    /// Unique identifier that corresponds to the frame for which the compute was issued. 
    struct Compute_frame_id {
        mi::Uint32      frame_id;   ///< Globally unique frame identifier
        mi::Uint32      scope_id;   ///< Scope id the compute task is issued under
        mi::Uint32      host_id;    ///< Host-id of the host issuing the compute task
    };

    /// Compute operations run asynchronous to a frame, which is uniquely defined by an id.
    ///
    /// \return Returns the frame-id the compute task was issued under
    ///
    virtual Compute_frame_id                        get_compute_frame_id() const = 0;

};

/// Compute-destination buffer for 3D sparse-volume generation techniques.
///
/// Upon applying an \c IDistributed_compute_technique attribute to a \c ISparse_volume_scene_element scene element
/// a \c IDistributed_compute_destination_buffer_3d_sparse_volume is passed to the \c IDistributed_compute_technique::launch_compute()
/// method.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer_3d_sparse_volume :
    public mi::base::Interface_declare<0x9da831bb,0x8425,0x4590,0xaa,0x10,0x8,0xcc,0x8d,0x5f,0x90,0xc1,
                                       nv::index::IDistributed_compute_destination_buffer>
{
public:
    /// Returns the bounding box of the volume subset for which the compute technique
    /// is required to generate values.
    ///
    /// The rendering system computes and initializes the bounding box of the volume subset.
    ///
    /// \return     The bounding box of the volume subset, defined in non-normalized
    ///             volume coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3> get_volume_subset_data_bbox() const = 0;

    /// Returns the clipped bounding box of the volume subset for which the compute technique
    /// is required to generate values. This bounding box describes the part of the 
    /// complete volume subset which is potentially visible due to clipping the volume
    /// dataset by the global or volume defined clip-regions.
    ///
    /// The clipped bounding box describes the parts of the volume that are required to
    /// be written in order to achieve a consistent rendered image of the volume dataset.
    ///
    /// The rendering system computes and initializes the bounding box of the volume subset.
    ///
    /// \return     The clipped bounding box of the volume subset, defined in non-normalized
    ///             volume coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3> get_clipped_volume_subset_data_bbox() const = 0;

    /// Returns the bounding box of the volume subset for which the compute technique
    /// is required to generate values containing already computed voxel values.
    ///
    /// This bounding box can be used to reduce the computing workload when parts of
    /// the volume subset were already generated by a previous invocation of the compute
    /// technique, for instance when the global or volume defined regions of interest
    /// have changed.
    ///
    /// The rendering system computes and initializes the bounding box of the volume subset.
    ///
    /// \return     The bounding box of the volume subset containing already computed voxel
    ///             values, defined in non-normalized volume coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3> get_already_computed_volume_subset_data_bbox() const = 0;

    /// Returns the volume data-subset for which the compute technique is required to
    /// generate voxel values.
    ///
    /// \return     An interface pointer to an instance of \c ISparse_volume_subset.
    ///
    virtual ISparse_volume_subset* get_distributed_data_subset() = 0;
};

/// Compute-destination buffer for irregular volume generation techniques.
///
/// Upon applying an \c IDistributed_compute_technique attribute to a \c IIrregular_volume_scene_element scene element
/// a \c IDistributed_compute_destination_buffer_irregular_volume is passed to the \c IDistributed_compute_technique::launch_compute()
/// method.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer_irregular_volume :
    public mi::base::Interface_declare<0x384f6eca,0xd09c,0x47a1,0xa0,0x91,0x1e,0x5f,0xc3,0xfb,0x47,0x6,
                                       nv::index::IDistributed_compute_destination_buffer>
{
public:
    /// Returns the bounding box of the volume subset for which the compute technique
    /// is required to generate values.
    ///
    /// The rendering system computes and initializes the bounding box of the volume subset.
    ///
    /// \return     The bounding box of the volume subset, defined in non-normalized
    ///             volume coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 3> get_volume_subset_data_bbox() const = 0;

    /// Returns the volume data-subset for which the compute technique is required to
    /// generate data values.
    ///
    /// \return     An interface pointer to an instance of \c IIrregular_volume_subset.
    ///
    virtual IIrregular_volume_subset* get_distributed_data_subset() = 0;
};

/// The interface class exposes intersections points that can be used, for
/// instance, to texture or shade the surface of geometry.
///
/// The IndeX library implements the serializable interface, which enables
/// user-defined classes to distribute an instance of the class to different
/// hosts in the cluster environment using DiCE's serialization and
/// deserialization mechanism.  Furthermore, the interface class exposes the
/// intersections points through the interface method
/// generate_intersection_points(). The internal implementation of this method
/// creates the intersection points on the fly whenever the method is
/// called. That is, all the intersection points never need to be communicated
/// to nodes in the cluster. Instead, the instance of the interface class itself
/// is sent to compute the intersection points on the remote machines on
/// request.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_intersection_points :
    public mi::base::Interface_declare<0xd985cb6a,0xe5c2,0x4c40,0x95,0xe3,0x29,0xa7,0x22,0x3e,0x35,0x9f,
                                       mi::neuraylib::ISerializable>
{
public:
    /// The interface method computes the intersections of rays with the
    /// geometry and returns a buffer of all intersection points to a
    /// user-defined computing class.  The renderer computes the intersection
    /// points based on the present viewing parameters so that all intersection
    /// points match the internal rendering.
    ///
    /// \param[out] nb_points The number of intersection points returned by the
    ///                       interface call.
    ///
    /// \return               The intersection points. They are defined in the local
    ///                       coordinate system of the geometry, that is, x and
    ///                       y will be in the range [0, extent], while the
    ///                       z-component will be 0. The ownership remains with the
    ///                       interface class.
    ///
    virtual const mi::math::Vector_struct<mi::Float32, 3>* generate_intersection_points(
        mi::Uint32& nb_points) = 0;
};

/// Compute-destination buffer for 2D texture generation techniques.
///
/// Upon applying an \c IDistributed_compute_technique attribute to a \c IPlane or \c IRegular_heightfield scene element
/// a \c IDistributed_compute_destination_buffer_2d_texture is passed to the \c IDistributed_compute_technique::launch_compute()
/// method.
///
/// Based on the 2D area returned by \c get_screen_space_area() a user-defined compute technique can choose, for
/// instance, an appropriate level-of-detail or the least compute-intensive generation technique, for example,
/// by either rendering into a 2D buffer or a buffer that corresponds to the ray/geometry intersection.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer_2d_texture :
    public mi::base::Interface_declare<0x1ea87c20,0xdae0,0x4b81,0xa3,0x16,0xa0,0x52,0x2d,0xf7,0xb4,0x3c,
                                       nv::index::IDistributed_compute_destination_buffer>
{
public:
    /// The layout of the buffer.
    ///
    /// The buffer's layout is defined by the user-defined texture generation
    /// technique.
    ///
    /// Setting the buffer layout to \c TWO_DIMENSIONAL_ARRAY is in most
    /// situations the most appropriate way to represent a texture tile. Then,
    /// all texture values are stored in consecutive order in x-first and y-last
    /// order and the value x=0 and y=0 represents the lower left texture value
    /// of the texture tile.
    ///
    /// The texture generation technique could set the layout to \c
    /// ONE_DIMENSIONAL_ARRAY to indicate that every entry in the texture buffer
    /// corresponds to a texture value for precomputed ray/object
    /// intersection. A ray/object intersection is only meaningful if the screen
    /// space area covered by the subregion is small compared to the size of the
    /// tile.
    ///
    enum Buffer_layout
    {
        ONE_DIMENSIONAL_ARRAY = 0,  ///< Buffer layout that corresponds to a 1D array of ray/geometry intersections
        TWO_DIMENSIONAL_ARRAY = 1   ///< Buffer layout that corresponds to a 2D array
    };

    /// Texture destination buffer format description.
    ///
    /// The buffer format defines the type and number of components of each texture element.
    ///
    enum Buffer_format
    {
        INTENSITY_UINT8 = 0,        ///< 8-bit integer used for indexing a color table of 256 entries
        RGBA_UINT8      = 1,        ///< 8-bit integer per RGBA component
        RGBA_FLOAT32    = 2         ///< 32-bit float per RGBA component
    };

    /// Texture buffer configuration.
    ///
    /// This structure defines the basic buffer parameters required by the rendering system to
    /// allocate the required buffer storage (see \c generate_buffer_storage).
    ///
    /// The covered area is the 2D area of the geometry's surface that a 2D texture tile covers.
    /// The covered area is typically larger than the object's surface area because the returned
    /// 2D tile has to have integral width and height resolution while the surface area is given
    /// in floating point values. (See \c get_surface_area().)
    ///
    /// The associated compute technique has to compute and set the covered area. The covered area
    /// is required to enable the calling rendering system to compute the texture coordinates to
    /// correctly texture map the tile onto the geometry's 2D surface area.
    ///
    /// If the texture buffer layout specifies a 2D array, then the 2D resolution of the 2D tile needs
    /// to be specified. The resolution needs to be known to the calling rendering system to enable
    /// valid accesses to the texture's 2D array.
    ///
    /// If the texture buffer layout specifies an array of isolated texture values, then the resolution
    /// needs to be specified as (number of intersection points, 0) and each buffer entry at the end needs
    /// to correspond to exactly one intersection point and the number of texture values has to correspond to
    /// the number of intersections.
    ///
    struct Buffer_config
    {
        Buffer_layout                           layout;         ///<! The buffer layout can be either 1- or 2-dimensional.
        /// Texture destination buffer format.
        Buffer_format                           format;         ///<! Buffer format desprition.
        mi::math::Bbox_struct<mi::Float32, 2>   covered_area;   ///<! Covered 2d area in screen space.
        mi::math::Vector_struct<mi::Uint32, 2>  resolution;     ///<! Resolution of the buffer.
    };

    /// Generate buffer storage.
    ///
    /// This function initializes the buffer storage according to the passed instance of \c Buffer_config.
    /// At any point during the lifetime of an \c IDistributed_compute_destination_buffer_2d_texture instance there can only be a
    /// single valid buffer storage. Multiple calls to this function will invalidate previous instances of the buffer
    /// storage and generate a new valid instance according the newly passed buffer config.
    ///
    /// \param[in]  buffer_cfg      The texture buffer config parameters.
    ///
    /// \return                     True if a valid storage was generated, false otherwise.
    ///
    virtual bool generate_buffer_storage(const Buffer_config& buffer_cfg) = 0;

    /// Get the texture buffer config parameters of the currently valid buffer storage.
    ///
    /// When this function is called before initializing a valid buffer storage (\c generate_buffer_storage()) the
    /// returned instance of \c Buffer_config will describe an invalid mesh (i.e. covered_area and resolution will
    /// describe an empty buffer).
    ///
    /// \return     The texture buffer config parameters of the currently valid buffer storage.
    /// 
    virtual const Buffer_config& get_buffer_config() const = 0;

    /// Get the current valid texture buffer storage.
    ///
    /// When this function is called before initializing a valid buffer storage (\c generate_buffer_storage()) the 
    /// returned value will be invalid (i.e. value \c 0). The ownership with the returned buffer remains with
    /// the instance of \c IDistributed_compute_destination_buffer_2d_texture.
    ///
    /// \return     The currently valid texture buffer storage.
    ///
    virtual mi::Uint8* get_buffer_storage() const = 0;

    /// Returns the per-subregion data of the geometry associated with this
    /// buffer.  The data type depends on the type of geometry:
    /// - For heightfields, the data type is \c IRegular_heightfield_patch,
    ///   which allows access to individual height values.
    /// - Planes do not provide per-subregion data.
    ///
    /// \return Per-subregion data or 0 if no data is available.
    virtual const IDistributed_data_subset* get_subregion_geometry_data() const = 0;


    /// Returns the 2D surface area for which the generation of the texture
    /// buffer is requested.
    ///
    /// It is not required that the texture generation technique computes a
    /// texture that covers the 2D area exactly, but the texture usually is
    /// slightly larger. For that, the covered area has to be set appropriately
    /// by the user-defined texturing technique. (See set_covered_area().)
    ///
    /// \return The 2D area that shall be textured by the returned texture.  The
    ///         area is defined in the object's local coordinate system.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 2> get_surface_area() const = 0;

    /// The texture generation can rely on pre-computed ray/geometry
    /// intersections. The interface method exposes all ray/geometry
    /// intersection points with respect to the given subregion. The
    /// intersection points enable the technique to compute the corresponding
    /// texture values.
    ///
    /// \return An instance of the class \c IDistributed_compute_intersection_points that contains all
    ///         ray/geometry intersection points given in the object's local
    ///         coordinate system. The caller is responsible to releasing the
    ///         object.
    ///
    virtual IDistributed_compute_intersection_points* get_intersection_points() const = 0;
};

/// \todo Documentation missing.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer_2d_texture_LOD_configuration :
    public mi::base::Interface_declare<0xfb33be9,0x8b65,0x400d,0xb3,0xfe,0xc2,0x3a,0x47,0x2d,0x4e,0xcf>
{
public:
    /// Structural information regarding the level-of-detail data. 
    struct LOD_level_info
    {
        mi::math::Vector_struct<mi::Uint32, 2>  mip_level_image_resolution;     ///< Image resolution for specified mipmap-level
        mi::math::Bbox_struct<mi::Uint32, 2>    mip_level_image_tile_bounds;    ///< Image-tile bounds for specified mipmap-level
        mi::Float32                             mip_level_selection_distance;   ///< World-distance for selection of specified mipmap-level
    };

    /// Number of mipmap-levels in the configuration.
    ///
    /// \return Returns the number of mipmap levels.
    ///
    virtual mi::Uint32 get_nb_mipmap_levels() const = 0;
    /// Number of mipmap-levels in the configuration.
    ///
    /// \param[in] nb_mipmaps   Defines the number of mipmap levels.
    ///
    virtual void set_nb_mipmap_levels(mi::Uint32 nb_mipmaps) = 0;

    /// Information regarding the mipmap-levels in the configuration.
    ///
    /// \param[in] mip_level    The mipmap level for which information shall be provided.
    ///
    /// \return Returns the information regarding a given mipmap level.
    ///
    virtual LOD_level_info get_mip_level_info(mi::Uint32 mip_level) const = 0;
    /// Information regarding the mipmap-levels in the configuration.
    ///
    /// \param[in] mip_level        The mipmap level for which information shall be set.
    ///
    /// \param[in] mip_level_info   The mipmap level information to be set for the given mipmap level.
    ///
    virtual void set_mip_level_info(
        mi::Uint32            mip_level,
        const LOD_level_info& mip_level_info) = 0;
};

/// Compute-destination buffer for 2D LOD-texture generation techniques.
/// 
/// Upon applying an \c IDistributed_compute_technique_LOD attribute to a \c
/// IPlane scene element a \c
/// IDistributed_compute_destination_buffer_2d_texture_LOD is passed to the \c
/// IDistributed_compute_technique::launch_compute() method.
/// 
/// Based on a \c
/// IDistributed_compute_destination_buffer_2d_texture_LOD_configuration, either
/// generated through the \c generate_LOD_configuration() method as the best
/// fitting proposal from NVIDIA IndeX or entirely defined by the user-defined
/// compute-technique, an instance of this class is configured by setting up the
/// LOD-buffer storage (\c generate_LOD_buffer_storage()). This configuration is
/// used by NVIDIA IndeX to request actually required mipmap-levels for a compute-
/// texture tile. The actual required mipmap-levels directly depend on the
/// current camera and view on the current scene.
/// 
/// It is possible that an existing, in a previous \c
/// IDistributed_compute_technique::launch_compute() configured, instance of this
/// class is again passed into the \c
/// IDistributed_compute_technique::launch_compute() method in order to fill in
/// missing, previously not required mipmap-level data. In such cases the method
/// \c is_LOD_buffer_storage_initialized() should be used to prevent re-
/// initialization of internal texture-data storage and repeated texture-mipmap
/// data inputs.
/// 
/// This class also allows a user-defined implementation to set an active LOD-
/// level range if it does not want to send certain mipmap-level data to NVIDIA
/// IndeX. For example, for previewing a certain compute-technique it may be
/// desired to first send low-resolution data to preview preliminary compute
/// results before generating and sending the high-resolution data. For such use-
/// cases the \c set_active_LOD_level_range() method can be used to limit the
/// active LOD-range.
/// 
/// After configuring the destination buffer and potentially setting the active
/// LOD-range the actually required compute-tile mipmap-levels can be queried
/// using the \c get_required_LOD_levels() method. The requested data can then be
/// written to the data-buffer returned by the \c get_LOD_level_buffer_storage()
/// method.
///
/// \ingroup nv_index_data_computing
///
class IDistributed_compute_destination_buffer_2d_texture_LOD :
    public mi::base::Interface_declare<0xb25d2721,0xab95,0x450c,0x8d,0x64,0xd0,0xc5,0xd1,0xaa,0xac,0xca,
                                       nv::index::IDistributed_compute_destination_buffer>
{
public:
    /// Texture destination buffer format description.
    ///
    /// The buffer format defines the type and number of components of each texture element.
    ///
    enum Buffer_format
    {
        FORMAT_INVALID          = 0x00,
        FORMAT_SCALAR_UINT8     = 0x01,        ///< 8-bit integer used for indexing a color table of 256 entries
        FORMAT_RGBA_UINT8       = 0x02,        ///< 8-bit integer per RGBA component
        FORMAT_RGBA_FLOAT32     = 0x03         ///< 32-bit float per RGBA component
    };

    /// Generate an instance of \c IDistributed_compute_destination_buffer_2d_texture_LOD_configuration.
    /// 
    /// The returned instance of the LOD-configuration can directly be used to
    /// configure and generate the internal LOD-buffer storage (\c
    /// generate_LOD_buffer_storage()), or a user-defined compute-technique can fill
    /// in custom requirements to the LOD-configuration before configuring the
    /// storage.
    /// 
    /// \note   In order to ensure correct memory management and reference counting
    ///         use \c mi::base::Handle to manage the lifetime of the returned interface
    ///         pointer.
    /// 
    /// \param [in] global_texture_resolution   The global texture resulution for which to
    ///                                         generate the compute-texture tile LOD-
    ///                                         configuration.
    /// \param [in] texture_tile_surface_area   The tile-surface area for which to generate
    ///                                         the compute-texture tile LOD-configuration.
    ///                                         This usually corresponds to the values
    ///                                         returned but the \c get_surface_area() method.
    /// \param [in] texture_tile_border         In order to allow for the implementation of
    ///                                         custom filter-kernels in rendering kernel
    ///                                         programs it may be required to have an
    ///                                         overlapping border between compute-texture
    ///                                         tile data. This allows a user-defined compute-
    ///                                         technique to directly set the required tile
    ///                                         border.
    ///
    /// \return A valid interface-pointer to an instance of \c
    ///         IDistributed_compute_destination_buffer_2d_texture_LOD_configuration. In case
    ///         of failure a 0-pointer is returned.
    ///
    virtual IDistributed_compute_destination_buffer_2d_texture_LOD_configuration* generate_LOD_configuration(
        const mi::math::Vector_struct<mi::Uint32, 2>& global_texture_resolution,
        const mi::math::Bbox_struct<mi::Float32, 2>&  texture_tile_surface_area,
        mi::Uint32                                    texture_tile_border) const = 0;

    /// \brief Generate LOD-buffer storage.
    ///        
    /// This function initializes the buffer storage according to the passed instance
    /// of \c IDistributed_compute_destination_buffer_2d_texture_LOD_configuration
    /// and the requested \c Buffer_format. At any point during the lifetime of an \c
    /// IDistributed_compute_destination_buffer_2d_texture_LOD instance there can
    /// only be a single valid buffer storage. Multiple calls to this function will
    /// invalidate previous instances of the buffer storage and generate a new valid
    /// instance according the newly passed configuration.
    /// 
    /// \param [in] lod_config                  The LOD-configuration parameters.
    /// \param [in] texture_buffer_format       The texture buffer format.
    ///             
    /// \return     True if a valid storage was generated, false otherwise.
    ///         
    virtual bool generate_LOD_buffer_storage(
        const IDistributed_compute_destination_buffer_2d_texture_LOD_configuration* lod_config,
        Buffer_format                                                               texture_buffer_format) = 0;

    /// Query if the LOD-buffer storage is initialized.
    ///
    /// \returns    True if a valid storage is initialized, false otherwise.
    ///             
    virtual bool is_LOD_buffer_storage_initialized() const = 0;

    /// \brief Returns the currently active \c IDistributed_compute_destination_buffer_2d_texture_LOD_configuration.
    /// 
    /// \note   In order to ensure correct memory management and reference counting use
    ///         \c mi::base::Handle to manage the lifetime of the returned interface pointer.
    ///            
    /// \return The currently active \c
    ///         IDistributed_compute_destination_buffer_2d_texture_LOD_configuration, or a 0-
    ///         pointer if the current storage was not yet configured.
    ///         
    virtual const IDistributed_compute_destination_buffer_2d_texture_LOD_configuration* get_active_LOD_configuration() const = 0;


    /// Returns the currently active \c Buffer_format.
    /// 
    /// \returns The currently active \c Buffer_format, or a \c FORMAT_INVALID if the
    ///          current storage was not yet configured.
    /// 
    virtual Buffer_format get_texture_buffer_format() const = 0;

    /// Set the active LOD-level range to be used by NVIDIA IndeX.
    /// 
    /// The active LOD-level range limits the data NVIDIA IndeX is requesting to be written
    /// to this LOD-texture instance and accesses during rendering.
    /// 
    /// \param[in]  lod_level_range     The LOD-range to be used by NVIDIA IndeX.
    ///             
    virtual void set_active_LOD_level_range(
        const mi::math::Vector_struct<mi::Uint32, 2>& lod_level_range) = 0;

    /// Returns the currently active LOD-level range to be used by NVIDIA IndeX.
    /// 
    /// \returns    The currently active LOD-level range.
    /// 
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_active_LOD_level_range() const = 0;

    /// Query the required LOD-levels for which to generate and write data to this LOD-texture instance.
    /// 
    /// \param[out] nb_required_levels          The number of required LOD-levels.
    /// \param[out] required_levels             An array containing the required LOD-level indices.
    /// 
    virtual void get_required_LOD_levels(
        mi::Uint32&  nb_required_levels,
        mi::Uint32*& required_levels) const = 0;

    /// Query the currently valid texture buffer storage for a particular compute-tile LOD-level.
    /// 
    /// When this function is called before initializing a valid buffer storage (\c generate_LOD_buffer_storage()) the
    /// returned value will be invalid (i.e. value \c 0). The ownership with the returned buffer remains with
    /// the instance of \c IDistributed_compute_destination_buffer_2d_texture_LOD.
    ///
    /// \param[in]  lod_level                   The LOD-level for which to return the valid storage pointer.
    ///             
    /// \return     The currently valid texture buffer storage for the requested LOD-level.
    ///
    virtual mi::Uint8* get_LOD_level_buffer_storage(
        mi::Uint32 lod_level) = 0;

    /// Returns the per-subregion data of the geometry associated with this
    /// buffer.  The data type depends on the type of geometry:
    /// - For heightfields, the data type is \c IRegular_heightfield_patch,
    ///   which allows access to individual height values.
    /// - Planes do not provide per-subregion data.
    ///
    /// \return Per-subregion data or 0 if no data is available.
    virtual const IDistributed_data_subset* get_subregion_geometry_data() const = 0;

    /// Returns the 2D surface area for which the generation of the texture
    /// buffer is requested.
    ///
    /// It is not required that the texture generation technique computes a
    /// texture that covers the 2D area exactly, but the texture usually is
    /// slightly larger. For that, the covered area has to be set appropriately
    /// by the user-defined texturing technique. (See set_covered_area().)
    ///
    /// \return The 2D area that shall be textured by the returned texture.  The
    ///         area is defined in the object's local coordinate system.
    ///
    virtual mi::math::Bbox_struct<mi::Float32, 2> get_surface_area() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IDISTRIBUTED_COMPUTE_DESTINATION_BUFFER_H
