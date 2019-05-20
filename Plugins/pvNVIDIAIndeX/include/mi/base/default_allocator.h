/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/default_allocator.h
/// \brief Default allocator implementation based on global new and delete.
///
/// See \ref mi_base_iinterface.

#ifndef MI_BASE_DEFAULT_ALLOCATOR_H
#define MI_BASE_DEFAULT_ALLOCATOR_H

#include <mi/base/iallocator.h>
#include <mi/base/interface_implement.h>
#include <mi/base/types.h>
#include <new>

namespace mi
{

namespace base
{

/** \addtogroup mi_base_iallocator
@{
*/

/** A default allocator implementation based on global new and delete.

    This implementation realizes the singleton pattern. An instance of the
    default allocator can be obtained through the static inline method
    #mi::base::Default_allocator::get_instance().

       \par Include File:
       <tt> \#include <mi/base/default_allocator.h></tt>

*/
class Default_allocator : public Interface_implement_singleton<IAllocator>
{
  Default_allocator() {}
  Default_allocator(const Default_allocator&) {}
public:
  /** Allocates a memory block of the given size.

      Implements #mi::base::IAllocator::malloc through a global non-throwing
      \c operator \c new call.

      \param size   The requested size of memory in bytes. It may be zero.
      \return       The allocated memory block.
  */
  virtual void* malloc(Size size)
  {
    // Use non-throwing new call, which may return NULL instead
    return ::new (std::nothrow) char[size];
  }

  /** Releases the given memory block.

      Implements #mi::base::IAllocator::free through a global
      \c operator \c delete call.

      \param  memory   A memory block previously allocated by a call to #malloc().
                       If \c memory is \c NULL, no operation is performed.
  */
  virtual void free(void* memory) { ::delete[] reinterpret_cast<char*>(memory); }

  /// Returns the single instance of the default allocator.
  static IAllocator* get_instance()
  {
    // We claim that this is multithreading safe because the
    // Default_allocator has an empty default constructor.
    // Whatever number of threads gets into the constructor, there
    // should be no way to screw up the initialization in each
    // thread. The optimizer might even be able to eliminate all
    // code here.
    static Default_allocator allocator;
    return &allocator;
  }
};

/*@}*/ // end group mi_base_iallocator

} // namespace base
} // namespace mi

#endif // MI_BASE_DEFAULT_ALLOCATOR_H
