/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Static array type.

#ifndef MI_NEURAYLIB_IARRAY_H
#define MI_NEURAYLIB_IARRAY_H

#include <mi/neuraylib/idata.h>

namespace mi
{

/** \addtogroup mi_neuray_collections
@{
*/

/// This interface represents static arrays, i.e., arrays with a fixed number of elements.
///
/// Arrays are either typed or untyped. Typed arrays enforce all elements to be of the same type.
/// The elements of typed arrays have to be derived from #mi::IData. The type name of a typed
/// array is the type name of the element followed by \c "[", the size of the array (a non-negative
/// integer), and finally \c "]", e.g., \c "Sint32[10]" for an array of 10 #mi::Sint32 elements.
/// The elements of a typed array are default-constructed.
///
/// Untyped arrays simply store pointers of type #mi::base::IInterface. The type name of an untyped
/// array is \c "Interface[" followed by the size of the array (a non-negative integer) and finally
/// \c "]. The elements of an untyped array are default-constructed as instance of #mi::IVoid.
///
/// The keys of an array (see #mi::IData_collection) are the indices converted to strings.
///
/// \see #mi::IDynamic_array
///
class IArray : public base::Interface_declare<0x329db537, 0x9892, 0x488c, 0xa2, 0xf4, 0xf5, 0x37,
                 0x1a, 0x35, 0xcf, 0x39, IData_collection>
{
public:
  /// Returns the size of the array.
  ///
  /// The size of an array is the number of elements in the array.
  virtual Size get_length() const = 0;

  /// Returns the \p index -th element of the array.
  ///
  /// \param index   The index of the requested element.
  /// \return        The requested element, or \c NULL if \p index is equal to or larger than
  ///                the size of the array.
  virtual const base::IInterface* get_element(Size index) const = 0;

  /// Returns the \p index -th element of the array.
  ///
  /// \param index   The index of the requested element.
  /// \return        The requested element, or \c NULL if \p index is equal to or larger than
  ///                the size of the array.
  ///
  /// This templated member function is a wrapper of the non-template variant for the user's
  /// convenience. It eliminates the need to call
  /// #mi::base::IInterface::get_interface(const Uuid &)
  /// on the returned pointer, since the return type already is a pointer to the type \p T
  /// specified as template parameter.
  template <class T>
  const T* get_element(Size index) const
  {
    const base::IInterface* ptr_iinterface = get_element(index);
    if (!ptr_iinterface)
      return 0;
    const T* ptr_T = static_cast<const T*>(ptr_iinterface->get_interface(typename T::IID()));
    ptr_iinterface->release();
    return ptr_T;
  }

  /// Returns the \p index -th element of the array.
  ///
  /// \param index   The index of the requested element.
  /// \return        The requested element, or \c NULL if \p index is equal to or larger than
  ///                the size of the array.
  virtual base::IInterface* get_element(Size index) = 0;

  /// Returns the \p index -th element of the array.
  ///
  /// \param index   The index of the requested element.
  /// \return        The requested element, or \c NULL if \p index is equal to or larger than
  ///                the size of the array.
  ///
  /// This templated member function is a wrapper of the non-template variant for the user's
  /// convenience. It eliminates the need to call
  /// #mi::base::IInterface::get_interface(const Uuid &)
  /// on the returned pointer, since the return type already is a pointer to the type \p T
  /// specified as template parameter.
  template <class T>
  T* get_element(Size index)
  {
    base::IInterface* ptr_iinterface = get_element(index);
    if (!ptr_iinterface)
      return 0;
    T* ptr_T = static_cast<T*>(ptr_iinterface->get_interface(typename T::IID()));
    ptr_iinterface->release();
    return ptr_T;
  }

  /// Sets the \p index -th element of the array.
  ///
  /// The object \p element is stored as \p index -th element of the array.
  ///
  /// \param index        The index where the object should be stored. The method call gets
  ///                     ignored if \p index is equal to or larger than the size of the array.
  /// \param element      The object to be stored.
  /// \return
  ///                     -  0: Success.
  ///                     - -1: \p index is out of bounds.
  ///                     - -2: \p element is \c NULL or has the wrong type.
  virtual Sint32 set_element(Size index, base::IInterface* element) = 0;

  /// Checks whether the array is empty.
  ///
  /// Equivalent to #get_length() == 0.
  virtual bool empty() const = 0;
};

/*@}*/ // end group mi_neuray_collections

} // namespace mi

#endif // MI_NEURAYLIB_IARRAY_H
