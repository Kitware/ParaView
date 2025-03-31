/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Custom memory allocator.

#ifndef NVIDIA_INDEX_IMEMORY_ALLOCATOR_H
#define NVIDIA_INDEX_IMEMORY_ALLOCATOR_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

// CUDA forward declarations required for the interfaces below
struct cudaArray;
struct cudaChannelFormatDesc;

namespace nv {
namespace index {

/// Forward declarations
class ICuda_memory_allocator;

class IMemory_allocator_configuration :
    public mi::base::Interface_declare<0x35cb18dd,0x6523,0x4362,0x92,0xb2,0xda,0xc4,0xb,0x3a,0x91,0xfd>
{
public:
    /// Register a CUDA-memory allocator for use of this IndeX instance.
    ///
    /// \param[in] alloc                CUDA-memory allocator instance (taking ownership).
    ///
    /// \return                         Returns 0 on success.
    ///
    virtual mi::Sint32                  set_memory_allocator(
                                            ICuda_memory_allocator* alloc) = 0;

    /// Retrieve the registered CUDA-memory allocator for use of this IndeX instance.
    ///
    /// \return                         Currently registered CUDA-memory allocator, nullptr otherwise.
    ///
    virtual ICuda_memory_allocator*     get_memory_allocator() const = 0;
};

/// Interface class allowing applications to implement a custom CUDA-memory allocation mechanism.
/// 
/// A custom, application-implemented allocator can be registered to an NVIDIA IndeX instance and
/// allows to use application defined CUDA-memory allocations for buffers and texture arrays.
/// 
class ICuda_memory_allocator :
    public mi::base::Interface_declare<0xea832c47,0x6dfa,0x41ba,0xb3,0x1c,0xa3,0xcf,0x4e,0x35,0x53,0x91>
{
public:
    struct Buffer_data {
        mi::Sint32  device_id;      ///< Device-id the allocation was performed on.
        void*       data;           ///< Pointer to the allocated buffer data.
        const void* user_data;      ///< Optional pointer to application defined user-data
                                    ///< (e.g., native data handles for interop allocations,
                                    ///< nullptr if unused).

        mi::Size    used_pitch;     ///< The used data pitch in number of Bytes for pitched allocations.
                                    ///< (if unused this value will be \c 0ull)
    };

    struct Array_data {
        mi::Sint32  device_id;      ///< Device-id the allocation was performed on.
        cudaArray*  array;          ///< Pointer to the allocated array/texture.
        const void* user_data;      ///< Optional pointer to application defined user-data
                                    ///< (e.g., native data handles for interop allocations,
                                    ///< nullptr if unused).
    };

public:
    virtual Buffer_data device_alloc(
                            mi::Sint32  device_id,
                            mi::Size    data_size) = 0;
    virtual Buffer_data device_alloc_pitched(
                            mi::Sint32  device_id,
                            mi::Uint32  size_x_in_bytes,
                            mi::Uint32  size_y) = 0;

    virtual Array_data  device_alloc_array(
                            mi::Sint32                                     device_id,
                            const mi::math::Vector_struct<mi::Uint32, 3>&  arr_dim,
                            const cudaChannelFormatDesc*                   arr_chdesc,
                            mi::Uint32                                     arr_flags) = 0;

    virtual void        device_free(
                            Buffer_data&    buffer_data) = 0;
    virtual void        device_free(
                            Array_data&     array_data) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IMEMORY_ALLOCATOR_H
