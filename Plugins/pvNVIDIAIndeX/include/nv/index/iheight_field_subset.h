/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief  [...]
///
#ifndef NVIDIA_INDEX_IHEIGHT_FIELD_SUBSET_H
#define NVIDIA_INDEX_IHEIGHT_FIELD_SUBSET_H

#include <mi/dice.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv
{
namespace index
{
/// Subset-data descriptor for tiled height-field subsets. This interface class is used by the NVIDIA IndeX library
/// to communicate information about tiled height-field sub-data to an application.
///
/// TODO: describe internal representation
/// TODO: describe LOD info
///
/// \ingroup nv_index_data_subsets
///
class IHeight_field_subset_data_descriptor:
        public mi::base::Interface_declare<0x88014528,0xd6b8,0x4b17,0x9d,0x12,0x74,0x9c,0xe2,0x88,0x46,0x6b,
                                           IDistributed_data_subset_data_descriptor>
{
public:
    /// Structual information regarding internal tile data representations. 
    struct Data_tile_info
    {
        mi::math::Vector_struct<mi::Sint32, 2>  tile_position;      ///< Position in tile local space of the
                                                                    ///< the particular tile (on its level-of-detail).
        mi::Uint32                              tile_lod_level;     ///< Level-of-detail of the particular tile.
    };

    /// Returns the number of levels-of-detail in the height-field dataset.
    ///
    /// \returns    The number of levels-of-detail in the height-field dataset
    ///
    virtual mi::Uint32                              get_dataset_number_of_lod_levels() const = 0;

    /// Returns the bounding box of the requested level-of-detail in local pixel coordinates.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the data bounding box.
    ///
    /// \returns    The bounding box of the requested level-of-detail in local pixel coordinates.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3>    get_dataset_lod_level_box(mi::Uint32 lod_level) const = 0;

    /// Returns the data resolution of the requested level-of-detail in 2d coordinates.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the data resolution.
    ///
    /// \returns    The data resolution of the requested level-of-detail in 2d coordinates.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_dataset_lod_level_resolution(mi::Uint32 lod_level) const = 0;

    /// Returns the dimensions of the height-field data tiles.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_subset_data_tile_dimensions() const = 0;

    /// Returns the size of the shared height-field data tile boundary.
    ///
    virtual mi::Uint32                              get_subset_data_tile_shared_border_size() const = 0;

    /// Returns the data bounds covering all height-field data tiles required for the subset on a
    /// particular level-of-detail.
    ///
    /// \param[in]  lod_level       Level-of-detail for which to return the subset-data bounding box.
    ///
    /// \returns    The data bounds covering all height-field data tiles required for this subset the
    ///             requested level-of-detail.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3>    get_subset_lod_data_bounds(mi::Uint32 lod_level) const = 0;

    /// Returns the level-of-detail range used by the subset. The components of the returned vector
    /// indicate the lowest (x-component) and hightest (y-component) level-of-detail used by the subset.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 2>  get_subset_lod_level_range() const = 0;

    /// Returns the number of height-field data-tiles available in the height-field data subset.
    ///
    virtual mi::Uint32                              get_subset_number_of_data_tiles() const = 0;

    /// Returns the height-field data tile information for a selected data tile.
    ///
    /// \param[in]  tile_index     height-field data tile index for which to return the information.
    ///
    /// \returns    The height-field data tile information (\c Data_tile_info) for the requested data tile.
    ///
    virtual const Data_tile_info                   get_subset_data_tile_info(mi::Uint32 tile_index) const = 0;
};

/// Distributed data storage class for height-field subsets.
///
/// The data import for height-field data associated with \c IHeight_field_scene_element instances using
/// NVIDIA IndeX is performed through instances of this subset class. A subset of a height field is defined
/// by all the height-field data tiles associated with a rectangular subregion of the entire scene/dataset. This
/// interface class provides methods to input tile data for one or multiple attributes of a dataset.
///
/// \ingroup nv_index_data_subsets
///
class IHeight_field_subset:
        public mi::base::Interface_declare<0xe6fc895a,0xdd21,0x4f63,0xac,0x99,0x78,0xe3,0x4a,0x95,0x68,0x26,
                                           IDistributed_data_subset>
{
public:
    /// Definition of internal buffer information.
    ///
    /// The internal buffer information can be queried using the \c access_tile_data_buffer() method
    /// to gain access to the internal buffer data for direct write operations. This enables zero-copy
    /// optimizations for implementations of, e.g., \c IDistributed_compute_technique where large parts
    /// of the data-subset buffer can be written directly without going through the \c write() methods.
    ///
    /// \note The internal layout of the buffer of a height-field data tile is in a linear IJK/XYZ
    ///       layout with the I/X-component running fastest.
    ///
    struct Data_tile_buffer_info
    {
        /// Raw memory pointer to internal elevation values.
        mi::Float32*                             data;

        /// Raw memory pointer to internal normals values.
        mi::math::Vector_struct<mi::Float32, 3>* normals;

        /// Number of values in tile.
        mi::Size                                 size;

        /// GPU device id if the buffer is located on a GPU device, -1 to indicate a host buffer.
        mi::Sint32                               gpu_device_id;
    };

    /// Transformation that should be applied on the written data.
    ///
    /// \note Experimental, subject to change!
    ///
    enum Data_transformation
    {
        DATA_TRANSFORMATION_NONE            = 0,  ///< Pass through unchanged
    };


    /// Returns the data descriptor of the subset.
    ///
    virtual const IHeight_field_subset_data_descriptor*    get_subset_data_descriptor() const = 0;

    /// Write a height-field data tile to the subset from a memory block in main memory.
    ///
    /// \param[in]  tile_subset_idx     Height-field data tile subset index for which to write pixel data.
    /// \param[in]  src_values          Source array of pixel values that will be written to
    ///                                 this data-subset buffer.
    /// \param[in]  gpu_device_id       GPU device id if the passed buffer is located on a GPU device,
    ///                                 otherwise this needs to be -1 to for a host memory buffer.
    /// \param[in]  tile_data_transform Transformation to apply to the data that is written.
    ///
    /// \returns                        True if the write operation was successful, false otherwise.
    ///
    virtual bool                            write_tile_data(
                                                mi::Uint32          tile_subset_idx,
                                                const void*         src_values,
                                                mi::Sint32          gpu_device_id,
                                                Data_transformation tile_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Write a height-field data tile to the subset from a \c IRDMA_buffer.
    ///
    /// \note The \c IRDMA_buffer can be located either in CPU or GPU memory. If a GPU-memory
    ///       buffer is passed additional memory copy operations may occur.
    ///
    /// \param[in]  tile_subset_idx    height-field data tile subset index for which to write pixel data.
    /// \param[in]  src_rdma_buffer     Source RDMA buffer of pixel values that will be written to
    ///                                 this data-subset buffer.
    /// \param[in]  tile_data_transform Transformation to apply to the data that is written.
    ///
    /// \returns                        True if the write operation was successful, false otherwise.
    ///
    virtual bool                            write_tile_data(
                                                mi::Uint32                          tile_subset_idx,
                                                const mi::neuraylib::IRDMA_buffer*  src_rdma_buffer,
                                                Data_transformation                 tile_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Write multiple height-field data tiles to the subset from a single \c IRDMA_buffer.
    /// The packing of the height-field data tiles to be written from the \c IRDMA_buffer is defined
    /// in the passed \c src_rdma_buffer_offsets parameter. It this parameter is 0, the tile
    /// sub-blocks need to be tightly packed after each other in the source buffer.
    ///
    /// \note The \c IRDMA_buffer can be located either in CPU or GPU memory. If a GPU-memory
    ///       buffer is passed additional memory copy operations may occur.
    ///
    /// \param[in]  tile_subset_indices    Subset height-field data tile indices for which to write pixel data.
    /// \param[in]  nb_input_tiles         The number of dst_range bounding boxes.
    /// \param[in]  src_rdma_buffer_offsets Array defining the offsets of the individual height-field sub-blocks
    ///                                     in the linear source buffer. These offsets need to be defined
    ///                                     as numbers of typed elements according the the pixel type of
    ///                                     the height-field tiles. If this parameter is 0, the offsets will
    ///                                     be derived from the \c dst_ranges parameter assuming a tight
    ///                                     packing of the tile sub-tiles in the source buffer.
    /// \param[in]  src_rdma_buffer         Source RDMA buffer of pixel values that will be written to
    ///                                     this subset buffer.
    /// \param[in]  tile_data_transform Transformation to apply to the data that is written.
    ///
    /// \returns                            True if the write operation was successful, false otherwise.
    ///
    virtual bool                            write_tile_data_multiple(
                                                const mi::Uint32*                   tile_subset_indices,
                                                mi::Uint32                          nb_input_tiles,
                                                const mi::Size*                     src_rdma_buffer_offsets,
                                                const mi::neuraylib::IRDMA_buffer*  src_rdma_buffer,
                                                Data_transformation                 tile_data_transform = DATA_TRANSFORMATION_NONE) = 0;

    /// Query the internal buffer information of the data-subset instance for a height-field data tile
    /// in otder to gain access to the internal buffer-data for direct write operations.
    ///
    /// \param[in]  tile_subset_idx     The height-field data tile subset index for which to query
    ///                                 internal buffer.
    ///
    /// \returns                        An instance of \c Internal_buffer_info describing the internal
    ///                                 buffer data for direct use.
    ///
    virtual const Data_tile_buffer_info    access_tile_data_buffer(mi::Uint32  tile_subset_idx) = 0;

    /// Query the internal buffer information of the data-subset instance for a height-field data tile
    /// in otder to gain access to the internal buffer-data for direct write operations.
    /// \note                           The const-qualifier of the interface method's signature leaves the
    ///                                 instance of an implemented \c IHeight_field_subset interface untouched.
    /// \param[in]  tile_subset_idx     The height-field data tile subset index for which to query
    ///                                 internal buffer.
    ///
    /// \returns                        An instance of \c Internal_buffer_info describing the internal
    ///                                 buffer data for direct use.
    ///
    virtual const Data_tile_buffer_info    access_tile_data_buffer(mi::Uint32  tile_subset_idx) const = 0;

    /// Creates an RDMA buffer that is a wrapper for of the internal buffer.
    ///
    /// \param[in]  rdma_ctx            The RDMA context used for wrapping the internal buffer.
    /// \param[in]  tile_subset_idx     Height-field data tile subset index for which to query internal buffer.
    ///
    ///
    /// \returns                    RDMA buffer wrapping the internal buffer data,
    ///                             or 0 in case of failure.
    ///
    virtual mi::neuraylib::IRDMA_buffer*    access_tile_data_buffer_rdma(
                                                mi::neuraylib::IRDMA_context* rdma_ctx,
                                                mi::Uint32                    tile_subset_idx) = 0;

    /// Returns the height value that encodes a hole in the heightfield data.
    ///
    /// \note Always use \c is_hole() to check for holes, do not compare height values with the
    /// return value of this function. Such a comparison would always fail if NaN is used, as
    /// comparing anything to NaN will always return false.
    ///
    /// \return Height value that represents holes.
    ///
    static inline mi::Float32 get_hole_value()
    {
        return mi::base::numeric_traits<mi::Float32>::quiet_NaN();
    }

    /// Returns whether the given height value represent a hole in the heightfield.
    ///
    /// \param[in] height       Heightfield height value.
    ///
    /// \return                 Returns \c true if the given height value represents a hole.
    ///
    static inline bool is_hole(mi::Float32 height)
    {
        // Use binary comparison so that it will work when holes are marked with NaN, as comparing
        // anything to NaN will always be false. This is also potentially faster than isnan().
        const mi::Uint32 height_bin = mi::base::binary_cast<mi::Uint32>(height);
        const mi::Uint32 hole_bin   = mi::base::binary_cast<mi::Uint32>(get_hole_value());

        return height_bin == hole_bin;
    }

    /// This method allows to write a compact cache file of successfully loaded subset data to the file system
    /// for more efficient future loading operations.
    ///
    /// \param[in] output_filename  Defines the output file.
    ///
    /// \note: This API is currently not supported and will change in future releases.
    ///
    virtual bool store_internal_data_representation(const char* output_filename) const = 0;

    /// This method allows to load a compact cache file of successfully loaded subset data to the file system
    /// for more efficient future loading operations.
    ///
    /// \param[in] input_filename   Defines the input file.
    ///
    /// \note: This API is currently not supported and will change in future releases.
    ///
    virtual bool load_internal_data_representation(const char*  input_filename) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IHEIGHT_FIELD_SUBSET_H
