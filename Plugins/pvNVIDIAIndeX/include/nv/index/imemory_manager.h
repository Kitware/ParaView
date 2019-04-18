/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef NVIDIA_INDEX_IMEMORY_MANAGER_H
#define NVIDIA_INDEX_IMEMORY_MANAGER_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

namespace nv
{
namespace index
{

class ICuda_memory_buffer : public mi::base::Interface_declare<0xa1defe77, 0xb3ef, 0x4652, 0x9c,
                              0x49, 0x97, 0x5f, 0xe3, 0x7a, 0xb3, 0xe0>
{
public:
  virtual char* get() const = 0;
};

/// Interface class for allocating NVIDIA IndeX library-side buffers for use also on the
/// application-side.
///
class ICuda_memory_manager : public mi::base::Interface_declare<0x636c6df4, 0x2aaf, 0x4144, 0x8f,
                               0x92, 0xb4, 0x22, 0x00, 0xa6, 0x8a, 0xcd>
{
public:
  virtual ICuda_memory_buffer* request_linear_device_memory(
    mi::Size elements, mi::Size type_size) const = 0;

  template <typename T>
  ICuda_memory_buffer* request_linear_device_memory(mi::Size elements) const
  {
    const mi::Size type_size = sizeof(T);
    return request_linear_device_memory(elements, type_size);
  }
};

/// Interface class for allocating NVIDIA IndeX library-side buffers for use also on the
/// application-side.
///
class IMemory_manager : public mi::base::Interface_declare<0xe0a59103, 0x275c, 0x43e1, 0xa5, 0x6d,
                          0xda, 0xc2, 0x8f, 0xb4, 0x42, 0x55>
{
public:
  virtual ICuda_memory_manager* get_cuda_memory_manager() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IMEMORY_MANAGER_H
