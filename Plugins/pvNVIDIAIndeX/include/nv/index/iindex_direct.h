/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces for the low-level NVIDIA IndeX Direct interface.

#ifndef NVIDIA_INDEX_IINDEX_DIRECT_H
#define NVIDIA_INDEX_IINDEX_DIRECT_H

#include <nv/index/idistributed_data_import_callback.h>
#include <nv/index/idistributed_data_subset.h>
#include <nv/index/iindex_direct_vulkan.h>

// CUDA forward declarations required for the interfaces below
struct cudaArray;
struct cudaChannelFormatDesc;

namespace nv {
namespace index {

enum Volume_direct_type
{
    VOLUME_DIRECT_TYPE_INVALID          = 0x00u,

    VOLUME_DIRECT_TYPE_SPARSE_VOLUME,   ///< NVIDIA IndeX sparse volume-octree (SVO) representation
    VOLUME_DIRECT_TYPE_NANOVDB          ///< NanoVDB volume representation
};

/// Define an opaque handle type without exposing the underlying struct.
struct Volume_direct_data;

/// Direct access to volume data structure.
typedef const Volume_direct_data* Volume_direct_data_handle;

/// Provides direct access to volume data.
class IVolume_direct_host :
    public mi::base::Interface_declare<0x99cd79a5,0x3ad,0x4a93,0x8c,0xca,0xd8,0x8e,0x2b,0xf1,0x19,0xc2,
                                       mi::neuraylib::IElement>
{
public:
    /// Returns the axis-aligned bounding box of the volume in voxel space.
    /// 
    virtual mi::math::Bbox_struct<mi::Float32, 3>       get_bounding_box() const = 0;

    /// Returns the volume transformation from voxel space to world space.
    /// 
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4>  get_transform() const = 0;

    /// Returns the voxel format of the volume.
    /// 
    virtual Distributed_data_attribute_format           get_voxel_format() const = 0;

    /// Returns the direct data-handle to the volume representation used by the
    /// NVIDIA IndeX_direct data access and sampling facilities.
    /// 
    virtual Volume_direct_data_handle                   get_direct_handle() const = 0;

    /// Returns the data subst associated with the \c IVolume_direct_host instance.
    ///
    /// This exposes the internal data subset instance used for representing the
    /// NVIDIA IndeX internal volume data that is made accessible through this
    /// volume-direct interface.
    ///
    virtual IDistributed_data_subset*                   get_data_subset() const = 0;
};

class IVolume_direct_resources_device :
    public mi::base::Interface_declare<0x35b08e00,0xce92,0x41bb,0x9f,0xad,0x54,0x8,0x70,0x48,0x42,0x1d>
{
public:
    struct Buffer
    {
        mi::Sint32      device_id;              // = -1

        void*           device_data;            // = nullptr;
        mi::Size        device_data_size;       // = 0ull

        const void*     allocation_user_data;   // = nullptr (optional, based on registered ICuda_memory_allocator implementation)
    };

    struct Array
    {
        mi::Sint32      device_id;              // = -1

        cudaArray*                              device_array;           // = nullptr;
        const cudaChannelFormatDesc*            device_array_format;
        mi::math::Vector_struct<mi::Uint32, 3>  device_array_extent;

        const void*     allocation_user_data;   // = nullptr (optional, based on registered ICuda_memory_allocator implementation)
    };
};

class IVolume_direct_resources_device_sparse_volume :
    public mi::base::Interface_declare<0x56677429,0xc0c6,0x4db7,0xb6,0x5c,0x40,0x8d,0xf8,0x38,0xd7,0x20,
                                       IVolume_direct_resources_device>
{
public:
    virtual Buffer      get_octree_serialization_buffer() const = 0;
    virtual Buffer      get_octree_node_data_buffer() const = 0;
    virtual Buffer      get_uniform_buffer() const = 0;

    virtual Array*      get_brick_atlas_textures() const = 0;
    virtual mi::Uint32  get_brick_atlas_textures_count() const = 0;
};

class IVolume_direct_resources_device_NanoVDB :
    public mi::base::Interface_declare<0xbfc8514b,0xa153,0x48a5,0x97,0x19,0xe3,0xe9,0xfa,0x1b,0xa5,0x3e,
                                       IVolume_direct_resources_device>
{
public:
    virtual Buffer      get_nanovdb_buffer() const = 0;
};

/// Provides direct access to volume data on a CUDA device.
class IVolume_direct_cuda :
    public mi::base::Interface_declare<0xb12ed2be,0x80bf,0x4fc8,0xba,0xb9,0x46,0x37,0x81,0x48,0x88,0xc>
{
public:
    /// Returns the axis-aligned bounding box of the volume in voxel space.
    /// 
    virtual mi::math::Bbox_struct<mi::Float32, 3>       get_bounding_box() const = 0;

    /// Returns the volume transformation from voxel space to world space.
    /// 
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4>  get_transform() const = 0;

    /// Returns the voxel format of the volume.
    /// 
    virtual Distributed_data_attribute_format           get_voxel_format() const = 0;

    /// Returns the device id of the GPU the volume was uploaded to.
    /// 
    virtual mi::Uint32                                  get_device_id() const = 0;

    /// Returns the direct data-handle to the volume representation used by the
    /// NVIDIA IndeX_direct data access and sampling facilities.
    /// 
    virtual Volume_direct_data_handle                   get_direct_handle() const = 0;

    virtual Volume_direct_type                          get_volume_type() const = 0;
    virtual IVolume_direct_resources_device*            get_resources() const = 0;
};

class IIndex_direct_config :
    public mi::base::Interface_declare<0x72acbf4a,0x9abc,0x4e6d,0x8c,0xd3,0xe1,0x8e,0xc0,0x66,0xc3,0x7e>
{
public:
    /// Configuration settings for internal volume representation.
    /// 
    /// The brick size denotes the full size of the internal volume bricks with the
    /// brick border reducing the usable part of the brick
    /// (useable size = brick_size - 2*brick_border).
    ///
    struct Sparse_volume_config
    {
        mi::math::Vector_struct<mi::Uint32, 3>  brick_size;     ///<! Volume brick size.
        mi::Uint32                              brick_border;   ///<! Shared border size between neighboring bricks.
    };

    struct Vulkan_config
    {
        VkInstance                              vk_instance;    ///<! Vulkan instance used for handling NVIDIA IndeX Direct
                                                                ///<! exported volume data
        // ...
    };

    /// Returns the default volume configuration used by IndeX_direct.
    /// 
    virtual Sparse_volume_config        default_volume_config() const = 0;

    /// Returns the currently active sparse-volume configuration used by IndeX Direct.
    /// 
    virtual void                        set_sparse_volume_config(const Sparse_volume_config& vol_cfg) = 0;
    virtual Sparse_volume_config        get_sparse_volume_config() const = 0;

    virtual void                        set_vulkan_config(const Vulkan_config& vk_cfg) = 0;
    virtual Vulkan_config               get_vulkan_config() const = 0;

};

/// Simplified scene management for volumes with direct access to low-level data.
class IIndex_direct :
    public mi::base::Interface_declare<0x62216bae,0xf97e,0x4dda,0xaa,0xfa,0xe5,0x70,0x14,0xec,0xc0,0x67>
{
public:
    /// Initializes the direct access functionality. Must be called before \c load_volume() but
    /// after \c IIndex::start().
    ///
    /// \param[in] idx_direct_config    Configuration settings for the IndeX direct instance.
    ///
    virtual bool                        initialize(
                                            const IIndex_direct_config* idx_direct_config) = 0;

    /// The current configuration settings.
    ///
    /// \return     Returns the configuration settings for the IndeX direct instance.
    ///
    virtual const IIndex_direct_config* get_config() const = 0;

    /// The NVIDIA IndeX Direct product name.
    ///
    /// \return     Returns the NVIDIA IndeX Direct product name
    ///             as a null-terminated string.
    ///
    virtual const char*                 get_product_name() const = 0;

    /// The product version of the NVIDIA IndeX Direct interface. Please also
    /// refer to the product version in support requests.
    ///
    /// \return     Returns the NVIDIA IndeX Direct product version
    ///             as a null-terminated string.
    ///
    virtual const char*                 get_version() const = 0;

    /// The NVIDIA IndeX Direct revision number indicates the build. Please also
    /// refer to the product version in support requests.
    ///
    /// \return     Returns NVIDIA IndeX Direct product revision number
    ///             as a null-terminated string.
    ///
    virtual const char*                 get_revision() const = 0;

    /// Loads a volume using the given importer.
    ///
    /// \param[in] volume_importer    Importer that implements the data loading.
    ///
    /// \return Volume representation that can be used to access the data, or 0 on error.
    ///
    virtual IVolume_direct_host*        load_volume(
                                            nv::index::IDistributed_data_import_callback* volume_importer) = 0;

    /// Frees up all resources of a particular host volume instance.
    ///
    /// \param[in] volume_direct_inst Instance of \c IVolume_direct to free.
    ///
    /// \return True if resources were freed up successfully, false otherwise.
    ///
    virtual bool                        free_volume(
                                            IVolume_direct_host* volume_direct_inst) = 0;

    /// Frees up all resources of a particular device volume instance. 
    /// 
    /// \param[in] volume_direct_inst Instance of \c volume_direct_inst to free.
    ///
    /// \return True if resources were freed up successfully, false otherwise.
    ///
    virtual bool                        free_volume(
                                            IVolume_direct_cuda* volume_direct_inst) = 0;

    /// Upload the volume data to a particular CUDA device.
    ///
    /// \param[in] cuda_device_id   CUDA device where the data resides.
    ///
    /// \return Handle to device data, or 0 when the data does not exist on the given device.
    ///
    virtual IVolume_direct_cuda*        upload_volume_to_device(
                                            const IVolume_direct_host*  volume_direct,
                                            mi::Uint32                  cuda_device_id) = 0;

    /// Upload the volume data to a particular Vulkan device.
    ///
    /// \param[in] vk_phys_device   Physical Vulkan device to match to internal CUDA device for the upload.
    /// \param[in] vk_device        Logical Vulkan device where the data should reside.
    /// \param[in] volume_direct    Instance of \c IVolume_direct to upload.
    ///
    /// \return Handle to device data, or 0 when the data does not exist on the given device.
    ///
    virtual IVolume_direct_vulkan*      upload_volume_to_vulkan(
                                            VkPhysicalDevice            vk_phys_device,
                                            VkDevice                    vk_device,
                                            const IVolume_direct_host*  volume_direct) = 0;

    /// Get an information string containing information about internal memory usage.
    ///
    /// \returns An information string containing information about internal memory usage
    ///
    virtual mi::IString*                get_memory_usage_info() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINDEX_DIRECT_H
