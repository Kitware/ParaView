/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for allocators.

#ifndef MI_NEURAYLIB_IALLOCATOR_H
#define MI_NEURAYLIB_IALLOCATOR_H

#include <mi/base/iallocator.h>
#include <mi/base/interface_declare.h>
#include <mi/base/types.h>

namespace mi
{

namespace neuraylib
{

/** \addtogroup mi_neuray_ineuray
@{
*/

/// The %Allocator interface class supports allocating and releasing memory dynamically
/// and querying the amount of extra memory used in the integrating application.
///
/// You can provide your own allocator object implementing this interface at
/// initialization time of the \neurayLibraryName to have the \neurayApiName
/// use your memory management.
///
class IAllocator : public mi::base::Interface_declare<0x952af060, 0xe2a6, 0x4bd7, 0xa2, 0x52, 0x9f,
                     0x6d, 0x39, 0xfb, 0x50, 0xa3, base::IAllocator>
{
public:
  /// Allocates a memory block of the given size.
  ///
  /// The memory must be aligned to an address which can accommodate any type of object on the
  /// current platform.
  ///
  /// An allocation of zero bytes returns a valid non- \c NULL pointer which must be freed in the
  /// end. However, dereferencing this pointer gives undefined behavior.
  ///
  /// This function can be called at any time from any thread, including concurrent calls from
  /// several threads at the same time.
  ///
  /// If the requested memory is not available this function must return \c NULL. In this case
  /// \neurayProductName will try to reduce the memory it uses and retry the allocation. If
  /// allocation still fails \neurayProductName will give up. In this case \neurayProductName can
  /// not be used anymore and must be shut down. \NeurayProductName will try to release as much
  /// memory as possible but it can not be guaranteed that all memory is returned. It is not
  /// possible to restart the library after a failure without restarting the process.
  ///
  /// Plugins for the \neurayApiName will get an allocator implementation that uses
  /// \neurayProductName memory management to give the stronger guarantee that allocation can
  /// never fail.
  ///
  /// \param size     The requested size of memory in bytes. It may be zero.
  /// \return         The allocated memory block.
  virtual void* malloc(Size size) = 0;

  /// Releases the given memory block.
  ///
  /// This function can be called at any time from any thread, including concurrent calls from
  /// several threads at the same time.
  ///
  /// \param memory   A memory block previously allocated by a call to #malloc().
  ///                 If \c memory is \c NULL, no operation is performed.
  virtual void free(void* memory) = 0;

  /// This function is used by \neurayProductName to inquire the amount of extra memory currently
  /// in use in the application.
  ///
  /// This function may be called frequently and must be implemented efficiently.
  ///
  /// This function can be called at any time from any thread, including concurrent calls from
  /// several threads at the same time.
  virtual Size get_used_extra_memory() = 0;
};

/*@}*/ // end group mi_neuray_ineuray

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IALLOCATOR_H
