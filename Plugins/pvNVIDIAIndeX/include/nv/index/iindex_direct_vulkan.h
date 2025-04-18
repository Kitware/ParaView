/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief NVIDIA IndeX Direct Vulkan interface.

#ifndef NVIDIA_INDEX_IINDEX_DIRECT_VULKAN_H
#define NVIDIA_INDEX_IINDEX_DIRECT_VULKAN_H

#include <nv/index/idistributed_data_import_callback.h>

// Vulkan forward declarations
typedef struct VkInstance_T*            VkInstance;
typedef struct VkPhysicalDevice_T*      VkPhysicalDevice;
typedef struct VkDevice_T*              VkDevice;

typedef struct VkImage_T*               VkImage;
typedef struct VkBuffer_T*              VkBuffer;
typedef struct VkSampler_T*             VkSampler;

typedef struct VkDescriptorPool_T*      VkDescriptorPool;
typedef struct VkDescriptorSetLayout_T* VkDescriptorSetLayout;
typedef struct VkDescriptorSet_T*       VkDescriptorSet;

namespace nv {
namespace index {

struct Buffer_info_vulkan
{
    VkBuffer        handle;
    mi::Size        size;
};

struct Image_info_vulkan
{
    VkImage                         handle;
    /*VkFormat*/    unsigned        format;
    /*VkImageType*/ unsigned        type;
    mi::math::Vector<mi::Uint32, 3> extent;
};

/// Provides direct access to volume data on a Vulkan device.
class IVolume_direct_vulkan :
    public mi::base::Interface_declare<0xdf314a66,0xe91d,0x4aec,0x86,0x8,0x6b,0xcf,0x5,0xfd,0x34,0x6b>
{
public:
    struct Native_resources
    {
        Buffer_info_vulkan  octree_serialization;
        Buffer_info_vulkan  octree_data;
        Buffer_info_vulkan  uniform_data;

        Image_info_vulkan*  brick_atlas_textures;
        mi::Uint32          brick_atlas_textures_count;
    };

    /// Returns the axis-aligned bounding box of the volume in voxel space.
    /// 
    virtual mi::math::Bbox_struct<mi::Float32, 3>       get_bounding_box() const = 0;

    /// Returns the volume transformation from voxel space to world space.
    /// 
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4>  get_transform() const = 0;

    /// Returns the voxel format of the volume.
    /// 
    virtual Distributed_data_attribute_format           get_voxel_format() const = 0;

    virtual Native_resources                            get_native_resources() const = 0;

    // TEMPORARY!
    virtual VkDescriptorSetLayout                       get_vk_descriptor_set_layout() const = 0;
    virtual VkDescriptorSet                             get_vk_descriptor_set() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINDEX_DIRECT_VULKAN_H
