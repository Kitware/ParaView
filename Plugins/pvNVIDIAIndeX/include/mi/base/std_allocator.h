/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/std_allocator.h
/// \brief Standard STL %allocator implementation.
///
/// The implementation is based on the #mi::base::IAllocator interface.
/// See \ref mi_base_iinterface.

#ifndef MI_BASE_STD_ALLOCATOR_H
#define MI_BASE_STD_ALLOCATOR_H

#include <mi/base/default_allocator.h>
#include <mi/base/iallocator.h>
#include <mi/base/types.h>

namespace mi
{

namespace base
{

/** \addtogroup mi_base_iallocator
@{
*/

/** An adaptor class template that implements a standard STL allocator.

    The implementation of the STL allocator is based on the #mi::base::IAllocator interface.

    \tparam T The value type of the allocator.

    \par Include File:
        <tt> \#include <mi/base/std_allocator.h></tt>
*/
template <class T>
class Std_allocator
{
  // Allocator interface used for memory management.
  IAllocator* m_alloc;

public:
  typedef T value_type;                   ///< Value type allocated by this allocator.
  typedef T* pointer;                     ///< Pointer type.
  typedef const T* const_pointer;         ///< Const pointer type.
  typedef T& reference;                   ///< Reference type.
  typedef const T& const_reference;       ///< Const reference type.
  typedef std::size_t size_type;          ///< Size type.
  typedef std::ptrdiff_t difference_type; ///< Difference type.

  /// Rebind helper struct to define a new class instance of this allocator template
  /// instantiated for the new value type \c T1.
  template <class T1>
  struct rebind
  {
    /// Rebind type, defines a new class instance of this allocator template
    /// instantiated for the new value type \c T1.
    typedef Std_allocator<T1> other;
  };

  /// Default constructor.
  ///
  /// Uses #mi::base::Default_allocator.
  Std_allocator() throw()
    : m_alloc(Default_allocator::get_instance())
  {
  }

  /// Constructor.
  ///
  /// Constructor from an #mi::base::IAllocator interface.
  ///
  /// \param allocator An implementation of the #mi::base::IAllocator interface.
  ///        Can be \c NULL in which case the #mi::base::Default_allocator
  ///        will be used.
  Std_allocator(base::IAllocator* allocator) throw()
    : m_alloc(allocator ? allocator : Default_allocator::get_instance())
  {
  }

  /// Copying constructor template for alike allocators of different value type.
  template <class T1>
  Std_allocator(const Std_allocator<T1>& other) throw()
    : m_alloc(other.get_allocator())
  {
  }

  /// Returns address of object \c x allocated through this allocator.
  pointer address(reference x) const { return &x; }

  /// Returns const address of object \c x allocated through this allocator.
  const_pointer address(const_reference x) const { return &x; }

  /// Allocate uninitialized dynamic memory for \c n elements of type \c T.
  /// \return The pointer to the allocated memory. Can be \c NULL if the
  ///         underlying IAllocator implementation fails to allocate the memory.
  T* allocate(size_type n, const void* = 0) throw()
  {
    return reinterpret_cast<T*>(m_alloc->malloc(n * sizeof(value_type)));
  }

  /// Frees uninitialized  dynamic memory at location \p p that has previously
  /// been allocated with \c %allocate().
  /// \param p the memory to be freed. If \p p is equal to \c NULL,
  ///          no operation is performed. However, note that according to
  //           the standard allocator concept \p p must not be \c NULL.
  void deallocate(pointer p, size_type) { m_alloc->free(p); }

  /// Returns the maximum number of elements of type \c T that can be allocated
  /// using this allocator.
  size_type max_size() const throw() { return SIZE_MAX_VALUE / sizeof(value_type); }

  /// Calls the copy constructor of \c T on the location \p p with the argument
  /// \p value.
  void construct(pointer p, const_reference value) { new (p) T(value); }

  /// Calls the destructor of \c T on the location \p p.
  void destroy(pointer p) { p->~T(); }

  /// Returns the interface of the underlying allocator.
  IAllocator* get_allocator() const { return m_alloc; }

  /// Equality comparison.
  ///
  /// Returns \c true if the underlying IAllocator interface
  /// implementations are the same, because then these allocators can be used
  /// interchangeably for allocation and deallocation.
  template <class T2>
  bool operator==(Std_allocator<T2> other) const throw()
  {
    return m_alloc == other.get_allocator();
  }

  /// Inequality comparison.
  ///
  /// Returns \c false if the underlying IAllocator interface
  /// implementations are the same.
  template <class T2>
  bool operator!=(Std_allocator<T2> other) const throw()
  {
    return !((*this) == other);
  }
};

/*@}*/ // end group mi_base_iallocator

} // namespace base

} // namespace mi

#endif // MI_BASE_STD_ALLOCATOR_H
