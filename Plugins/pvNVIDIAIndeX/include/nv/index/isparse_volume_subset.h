/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subsets of sparse volume datasets.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv {
namespace index {

/// Voxel format of sparse volume voxel data.
///
/// \ingroup nv_index_data_subsets
///
enum Sparse_volume_voxel_format
{
    SPARSE_VOLUME_VOXEL_FORMAT_UINT8          = 0x00,   ///< Scalar voxel format with uint8 precision
    SPARSE_VOLUME_VOXEL_FORMAT_UINT8_2,                 ///< Vector voxel format with 2 components and uint8 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_UINT8_4,                 ///< Vector voxel format with 4 components and uint8 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_SINT8,                   ///< Scalar voxel format with sint8 precision
    SPARSE_VOLUME_VOXEL_FORMAT_SINT8_2,                 ///< Vector voxel format with 2 components and sint8 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_SINT8_4,                 ///< Vector voxel format with 4 components and sint8 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_UINT16,                  ///< Scalar voxel format with uint16 precision
    SPARSE_VOLUME_VOXEL_FORMAT_UINT16_2,                ///< Vector voxel format with 2 components and uint16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_UINT16_4,                ///< Vector voxel format with 4 components and uint16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_SINT16,                  ///< Scalar voxel format with sint16 precision
    SPARSE_VOLUME_VOXEL_FORMAT_SINT16_2,                ///< Vector voxel format with 2 components and sint16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_SINT16_4,                ///< Vector voxel format with 4 components and sint16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT16,                 ///< Scalar voxel format with float16 precision
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT16_2,               ///< Vector voxel format with 2 components and float16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT16_4,               ///< Vector voxel format with 4 components and float16 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32,                 ///< Scalar voxel format with float32 precision
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32_2,               ///< Vector voxel format with 2 components and float32 precision per component
    SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32_4,               ///< Vector voxel format with 4 components and float32 precision per component

    SPARSE_VOLUME_VOXEL_FORMAT_COUNT
};

/// Get the size in byte for a given sparse volume voxel data format.
///
/// \param[in] fmt  The sparse volume voxel format
///
/// \return Returns the size in byte for the given voxel format.
/// 
/// \ingroup nv_index_data_subsets
///
inline mi::Size get_sizeof(const nv::index::Sparse_volume_voxel_format fmt)
{
    switch (fmt)
    {
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8:     return     sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8_2:   return 2 * sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT8_4:   return 4 * sizeof(mi::Uint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8:     return     sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8_2:   return 2 * sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT8_4:   return 4 * sizeof(mi::Sint8);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16:    return     sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16_2:  return 2 * sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_UINT16_4:  return 4 * sizeof(mi::Uint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16:    return     sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16_2:  return 2 * sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_SINT16_4:  return 4 * sizeof(mi::Sint16);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32:   return     sizeof(mi::Float32);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32_2: return 2 * sizeof(mi::Float32);
    case nv::index::SPARSE_VOLUME_VOXEL_FORMAT_FLOAT32_4: return 4 * sizeof(mi::Float32);
    default:
        return 0;
    }
}


/// Volume device-data accessor interface.
///
/// An instance of this class will hold ownership of the device data. For this reason managing the life-time of an instance
/// is important for when to free up the data.
///
/// \note WORK IN PROGRESS
///  - for now uses the Sparse_volume_voxel_format enum to represent volume type
///
/// \ingroup nv_index_data_subsets
///
class IVolume_device_data_buffer :
    public mi::base::Interface_declare<0x60d74933,0xd556,0x49de,0xaa,0xbc,0x5d,0xa7,0xaa,0xc1,0x4c,0x50>
{
public:
    /// Verify if a data subset is valid.
    virtual bool                                is_valid() const = 0;

    /// Voxel format of the volume data.
    virtual Sparse_volume_voxel_format          get_data_format() const = 0;

    /// Volume data position in local volume space.
    virtual mi::math::Vector<mi::Sint32, 3>     get_data_position() const = 0;

    /// Volume data extent.
    virtual mi::math::Vector<mi::Uint32, 3>     get_data_extent() const = 0;
    
    /// Raw memory pointer to internal device-buffer data.
    virtual void*                               get_device_data() const = 0;

    /// The size of the buffer in Bytes.
    virtual mi::Size                            get_data_size() const = 0;

    /// GPU device id if the buffer is located on a GPU device,
    virtual mi::Sint32                          get_gpu_device_id() const = 0;
};

/// Attribute-set descriptor for sparse volume subsets.
/// 
/// This interface is used to configure a set of
/// attributes for a sparse volume subset to input into the NVIDIA IndeX library.
///
/// \ingroup nv_index_data_subsets
///
class ISparse_volume_attribute_set_descriptor :
    public mi::base::Interface_declare<0x728bb4d5,0x1d77,0x42da,0x81,0x1c,0x39,0xa3,0x34,0x56,0x26,0xa4,
                                       IDistributed_data_attribute_set_descriptor>
{
public:
    /// Sparse volume attribute parameters.
    ///
    /// This structure defines the basic parameters of a single attribute associated with the volume dataset.
    ///
    struct Attribute_parameters
    {
        Sparse_volume_voxel_format  format;             ///< Attribute format. See \c Sparse_volume_voxel_format.
    };

    /// Configure the parameters for an attribute for a sparse volume subset.
    ///
    /// \param[in]  attrib_index    The storage index of the attribute.
    /// \param[in]   attrib_params  The attribute parameters for the given index.
    ///
    /// \return                     True when the attribute according to the passed index could be set up, false otherwise.
    ///
    virtual bool    setup_attribute(
                        mi::Uint32                  attrib_index,
                        const Attribute_parameters& attrib_params) = 0;

    /// Get the attribute parameters of a currently valid attribute for a given index.
    ///
    /// \param[in]  attrib_index    The storage index of the attribute.
    /// \param[out] attrib_params   The attribute parameters for the given index.
    ///
    /// \return                     True when the attribute according to the passed index could be found, false otherwise.
    ///
    virtual bool    get_attribute_parameters(
                        mi::Uint32                  attrib_index,
                        Attribute_parameters&       attrib_params) const = 0;
};

/// Data descriptor for sparse volume data subsets.
///
/// This interface class is used by the NVIDIA IndeX library
/// to communicate information about sparse volume sub-data to an application. NVIDIA IndeX is storing sparse
/// volume data in a bricked format internally. This interface class gives detailed information about volume-brick
/// data contained in the subset. The application is free to fill volume-data bricks or leave them empty, hence
/// creating a sparse volume layout.
///
/// For level-of-detail volumes this interface requests volume-data bricks for potentially multiple levels-of-detail.
/// The application is responsible to fill in all requested volume-data bricks in order to enable correct level-of-detail
/// volume representation and rendering.
///
/// \ingroup nv_index_data_subsets
///
class ISparse_volume_subset_data_descriptor :
    public mi::base::Interface_declare<0xad2ab72c,0xab33,0x4357,0xb6,0xd5,0x5a,0xcd,0xb9,0xf9,0x62,0xca,
                                       IDistributed_data_subset_data_descriptor>
{
public:
    /// Internal brick structure information.
    ///  
    struct Data_brick_info
    {
        mi::math::Vector_struct<mi::Sint32, 3>  brick_position;     ///< Position in volume local space of the
                                                                    ///< the particular brick (on its level-of-detail).
        mi::Uint32                              brick_lod_level;    ///< Level-of-detail of the particular brick.
    };

public:
    /// Returns the number of levels-of-detail in the volume dataset.
    ///
    /// \returns    The number of levels-of-detail in the volume dataset
    ///
    virtual mi::Uint32                              get_dataset_number_of_lod_levels() const = 0;

    /// Returns the bounding box of the requested level-of-detail in local voxel coordinates.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the data bounding box.
    ///
    /// \returns    The bounding box of the requested level-of-detail in local voxel coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3>    get_dataset_lod_level_box(mi::Uint32 lod_level) const = 0;

    /// Returns the data resolution of the requested level-of-detail in local voxel coordinates.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the data resolution.
    ///
    /// \returns    The data resolution of the requested level-of-detail in local voxel coordinates.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 3>  get_dataset_lod_level_resolution(mi::Uint32 lod_level) const = 0;

    /// Returns the dimensions of the volume-data bricks.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 3>  get_subset_data_brick_dimensions() const = 0;

    /// Returns the size of the shared volume-data brick boundary.
    ///
    virtual mi::Uint32                              get_subset_data_brick_shared_border_size() const = 0;

    /// Returns the data bounds covering all volume-data bricks required for the subset on a
    /// particular level-of-detail.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the subset-data bounding box.
    ///
    /// \returns    The data bounds covering all volume-data bricks required for this subset the
    ///             requested level-of-detail.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3>    get_subset_lod_data_bounds(mi::Uint32 lod_level) const = 0;

    /// Returns the level-of-detail range used by the subset. The components of the returned vector
    /// indicate the lowest (x-component) and highest (y-component) level-of-detail used by the subset.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_subset_lod_level_range() const = 0;

    /// Returns the number of volume-data bricks available in the complete non-view-dependent volume-data subset.
    ///
    virtual mi::Uint32                              get_subset_number_of_data_bricks() const = 0;
    
    /// Returns the volume-data brick information for a selected data brick in the complete non-view-dependent
    /// volume subset.
    ///
    /// \param[in]  brick_index     Volume-data brick index for which to return the information.
    ///
    /// \returns    The volume-data brick information (\c Data_brick_info) for the requested data brick.
    ///
    virtual const Data_brick_info                   get_subset_data_brick_info(mi::Uint32 brick_index) const = 0;

    /// Returns the number of volume-data bricks available in the view-dependent LOD working-set of the volume-data subset.
    ///
    virtual mi::Uint32                              get_lod_subset_number_of_data_bricks() const = 0;
    
    /// Returns the subset-local volume-data brick index for a LOD data-brick in the view-dependent
    /// LOD working-set of the volume-data subset. The subset-local index then is to be used to get
    /// the brick information using the \c get_subset_data_brick_info() method.
    /// 
    /// \note LOD subset-information is not available during importer invocations. Such information will be
    ///       available for distributed compute tasks on volume scene elements (i.e., \c IDistributed_compute_technique).
    ///
    /// \param[in]  lod_brick_index LOD working-set brick index for which to return the subset-local index.
    ///
    /// \returns    The subset-local volume-data brick index for the requested data LOD brick.
    ///
    virtual mi::Uint32                              get_lod_subset_data_brick_index(mi::Uint32 lod_brick_index) const = 0;
};

/// Distributed data storage class for sparse volume subsets.
///
/// The data import for sparse volume data associated with \c ISparse_volume_scene_element instances using
/// NVIDIA IndeX is performed through instances of this subset class. A subset of a sparse volume is defined
/// by all the volume-data bricks associated with a rectangular subregion of the entire scene/dataset. This
/// interface class provides methods to input volume data for one or multiple attributes of a dataset.
///
/// \ingroup nv_index_data_subsets
///
class ISparse_volume_subset :
    public mi::base::Interface_declare<0x1dfa0274,0xcf8,0x4f4c,0xa8,0x65,0x24,0xd0,0xae,0xf5,0x89,0x31,
                                       IDistributed_data_subset>
{
public:
    /// Definition of internal buffer information.
    ///
    /// The internal buffer information can be queried using the \c get_internal_buffer_info() method
    /// to gain access to the internal buffer data for direct write operations. This enables zero-copy
    /// optimizations for implementations of, e.g., \c IDistributed_compute_technique where large parts
    /// of the data-subset buffer can be written directly without going through the \c write() methods.
    ///
    /// \note The internal layout of the buffer of a volume-data brick is in a linear IJK/XYZ layout with
    ///       the I/X-component running fastest.
    ///
    struct Data_brick_buffer_info
    {
        void*           data;               ///< Raw memory pointer to internal buffer data.
        mi::Size        size;               ///< The size of the buffer in Bytes.
        mi::Sint32      gpu_device_id;      ///< GPU device id if the buffer is located on a GPU device,
                                            ///< -1 to indicate a host buffer.
        bool            is_pinned_memory;   ///< Flag indicating if a host buffer is a pinned (page-locked)
                                            ///< memory area.
    };

    /// Transformation that should be applied on the written data.
    ///
    /// \note Experimental, subject to change!
    ///
    enum Data_transformation
    {
        DATA_TRANSFORMATION_NONE            = 0,  ///< Pass through unchanged
        DATA_TRANSFORMATION_FLIP_AXIS_ORDER = 1   ///< Change voxel storage order (x/y/z to z/y/x)
    };

    /// Definition of the state of a data-brick in the subset.
    ///
    /// Data-bricks are empty by default. They can be set to a filled state, meaning that they are
    /// filled with voxel data, or homogeneous state, which means that they do not explicitly store
    /// voxel data but are defined by a constant value.
    ///
    enum Data_brick_state
    {
        BRICK_STATE_EMPTY   = 0x00u,
        BRICK_STATE_FILLED,
        BRICK_STATE_HOMOGENEOUS
    };

public:

    /// Returns the attribute-set descriptor of the subset.
    ///
    /// \return     Returns the attribute-set descriptor interfcae.
    ///
    virtual const ISparse_volume_attribute_set_descriptor*  get_attribute_set_descriptor() const = 0;

    /// Returns the data subset descriptor of the subset.
    ///
    /// \return     Returns the data subset descriptor interface.
    ///
    virtual const ISparse_volume_subset_data_descriptor*    get_subset_data_descriptor() const = 0;

    /// Write a volume-data brick to the subset from a memory block in main memory.
    ///
    /// \param[in]  brick_subset_idx        Volume-data brick subset index for which to write voxel data.
    ///
    /// \param[in]  brick_attrib_idx        The attribute index of the volume-data brick for which to write voxel data.
    /// 
    /// \param[in]  src_values              Source array of voxel values that will be written to
    ///                                     this data-subset buffer.
    /// 
    /// \param[in]  gpu_device_id           GPU device id if the passed buffer is located on a GPU device,
    ///                                     otherwise this needs to be -1 to for a host memory buffer.
    /// 
    /// \param[in]  brick_data_transform    Transformation to apply to the data that is written
    ///
    /// \returns                            Returns \c true if the write operation was successful and \c false otherwise.
    ///
    virtual bool                            write_brick_data(
                                                mi::Uint32          brick_subset_idx,
                                                mi::Uint32          brick_attrib_idx,
                                                const void*         src_values,
                                                mi::Sint32          gpu_device_id,
                                                Data_transformation brick_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Write a volume-data brick to the subset from a \c IRDMA_buffer.
    /// 
    /// \note The \c IRDMA_buffer can be located either in CPU or GPU memory. If a GPU-memory
    ///       buffer is passed additional memory copy operations may occur.
    ///
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to write voxel data.
    /// 
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to write voxel data.
    /// 
    /// \param[in]  src_rdma_buffer     Source RDMA buffer of voxel values that will be written to
    ///                                 this data-subset buffer.
    /// 
    /// \param[in]  brick_data_transform Transformation to apply to the data that is written
    ///
    /// \returns                        True if the write operation was successful, false otherwise.
    ///
    virtual bool                            write_brick_data(
                                                mi::Uint32                          brick_subset_idx,
                                                mi::Uint32                          brick_attrib_idx,
                                                const mi::neuraylib::IRDMA_buffer*  src_rdma_buffer,
                                                Data_transformation                 brick_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Write multiple volume-data bricks to the subset from a single \c IRDMA_buffer.
    /// The packing of the volume-data bricks to be written from the \c IRDMA_buffer is defined
    /// in the passed \c src_rdma_buffer_offsets parameter. If this parameter is 0, the volume
    /// sub-blocks need to be tightly packed after each other in the source buffer.
    /// 
    /// \note The \c IRDMA_buffer can be located either in CPU or GPU memory. If a GPU-memory
    ///       buffer is passed additional memory copy operations may occur.
    ///
    /// \param[in]  brick_subset_indices    Subset volume-data brick indices for which to write voxel data.
    /// 
    /// \param[in]  brick_attrib_indices    The attribute indices of the volume-data bricks for which to write voxel data.
    /// 
    /// \param[in]  nb_input_bricks         The number of dst_range bounding boxes.
    /// 
    /// \param[in]  src_rdma_buffer_offsets Array defining the offsets of the individual volume sub-blocks
    ///                                     in the linear source buffer. These offsets need to be defined
    ///                                     as numbers of typed elements according the voxel type of
    ///                                     the volume brick. If this parameter is 0, the offsets will
    ///                                     be derived from the \c dst_ranges parameter assuming a tight
    ///                                     packing of the volume sub-blocks in the source buffer.
    /// 
    /// \param[in]  src_rdma_buffer         Source RDMA buffer of voxel values that will be written to
    ///                                     this subset buffer.
    /// 
    /// \param[in]  brick_data_transform    Transformation to apply to the data that is written
    ///
    /// \returns                            Returns \c true if the write operation was successful and \c false otherwise.
    ///
    virtual bool                            write_brick_data_multiple(
                                                const mi::Uint32*                   brick_subset_indices,
                                                const mi::Uint32*                   brick_attrib_indices,
                                                mi::Uint32                          nb_input_bricks,
                                                const mi::Size*                     src_rdma_buffer_offsets,
                                                const mi::neuraylib::IRDMA_buffer*  src_rdma_buffer,
                                                Data_transformation                 brick_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Query the internal buffer information of the data-subset instance for a volume-data brick
    /// in order to gain access to the internal buffer-data for direct write operations.
    ///
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to query internal buffer.
    /// 
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to query the
    ///                                 internal buffer information
    ///
    /// \returns                        An instance of \c Internal_buffer_info describing the internal
    ///                                 buffer data for direct use.
    ///
    virtual const Data_brick_buffer_info    access_brick_data_buffer(
                                                mi::Uint32  brick_subset_idx,
                                                mi::Uint32  brick_attrib_idx) = 0;
    /// Query the internal buffer information of the data-subset instance for a volume-data brick
    /// in order to gain access to the internal buffer-data for direct write operations.
    ///
    /// \note   Please note the const-qualifier in the #access_brick_data_buffer interfaces method's 
    ///         signature, which leaves the instance of an \c ISparse_volume_subset implementation 
    ///         untouched. 
    ///
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to query internal buffer.
    /// 
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to query the
    ///                                 internal buffer information
    ///
    /// \returns                        An instance of \c Internal_buffer_info describing the internal
    ///                                 buffer data for direct use.
    ///
    virtual const Data_brick_buffer_info    access_brick_data_buffer(
                                                mi::Uint32  brick_subset_idx,
                                                mi::Uint32  brick_attrib_idx) const = 0;

    /// Creates an RDMA buffer that is a wrapper for the internal buffer.
    ///
    /// \param[in]  rdma_ctx            The RDMA context used for wrapping the internal buffer.
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to query internal buffer.
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to query the
    ///                                 internal buffer information
    ///
    ///
    /// \returns                    RDMA buffer wrapping the internal buffer data,
    ///                             or 0 in case of failure.
    ///
    virtual mi::neuraylib::IRDMA_buffer*    access_brick_data_buffer_rdma(
                                                mi::neuraylib::IRDMA_context* rdma_ctx,
                                                mi::Uint32                    brick_subset_idx,
                                                mi::Uint32                    brick_attrib_idx) = 0;

    /// Explicitly set the state of a data-brick in the subset.
    ///
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to set the state.
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to set the state.
    /// \param[in]  brick_state         The state to set for the selected data-brick.
    ///
    virtual void                            set_brick_state(
                                                mi::Uint32       brick_subset_idx,
                                                mi::Uint32       brick_attrib_idx,
                                                Data_brick_state brick_state) = 0;
    /// Query the state of a data-brick in the subset.
    ///
    /// \param[in]  brick_subset_idx    Volume-data brick subset index for which to query the state.
    /// \param[in]  brick_attrib_idx    The attribute index of the volume-data brick for which to query the state.
    ///
    /// \returns                        The current state of the selected data-brick.
    ///
    virtual Data_brick_state                get_brick_state(
                                                mi::Uint32       brick_subset_idx,
                                                mi::Uint32       brick_attrib_idx) const = 0;

    /// This method allows to write a compact cache file of successfully loaded subset data to the file system
    /// for more efficient future loading operations.
    ///
    /// \note: This API is currently not supported and will change in future releases.
    ///
    /// \param[in]  output_filename     The file name for the data subset memory dump.
    ///
    /// \return                         Returns \c true if the memory dump was successful.
    ///
    virtual bool store_internal_data_representation(const char* output_filename) const = 0;
    /// This method allows to load a compact cache file of a data subset.
    ///
    /// \note: This API is currently not supported and will change in future releases.
    ///
    /// \param[in]  input_filename      The file name of the cache file to be loaded.
    ///
    /// \return                         Returns \c true if the cache file import was successful.
    ///
    virtual bool load_internal_data_representation(const char*  input_filename) = 0;

    // Experimental API
    // \note WORK IN PROGRESS
    // - will be refactored into a separate device-data interface exposing device available data
    //   if it is available in the particular context of usage
    // - this is future work as we have to work out how to handle the existing APIs for writing
    //   directly to the GPU (write_brick_data(), especially with IRDMA_buffer interface).
    // - for now we will expose currently required methods directly. they will return invalid
    //   GPU-resources when no GPU-device access is given at the particular point
    // - the plan is to currently have these methods working for the inferencing-technique
    //   APIs added
    //

    /// Flags used for specifying the data generation process.
    enum Sub_data_gen_flags
    {
        SUB_DATA_GEN_DEFAULT                = 0x00u,    ///<! Default generation mode.
        SUB_DATA_GEN_INCLUDE_BORDER_VOXELS  = 0x01u,    ///<! Volume data generation with included border voxel.
        SUB_DATA_GEN_NO_CLIP_REQUEST_VOLUME = 0x02u     ///<! Ignoring clip request
    };

    /// Generating a volume data buffer for use by the application.
    /// \param[in] subdata_attrib_idx   The attribute index.
    /// \param[in] subdata_position     The position as an anchor that specifies a request bounding box together with the extent.
    /// \param[in] subdata_extent       The extent that specifies a request bounding box together with the position.
    /// \param[in] subdata_flags        Flags specifying a generation mode.
    ///
    /// \return     Returns the volume data buffer encapsulated by an instance
    ///             of an \c IVolume_device_data_buffer interface implementation.
    virtual IVolume_device_data_buffer* generate_volume_sub_data_device(
                                                mi::Uint32                             subdata_attrib_idx,
                                                mi::math::Vector_struct<mi::Sint32, 3> subdata_position,
                                                mi::math::Vector_struct<mi::Uint32, 3> subdata_extent,
                                                mi::Uint32                             subdata_flags = SUB_DATA_GEN_DEFAULT) const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_SUBSET_H
