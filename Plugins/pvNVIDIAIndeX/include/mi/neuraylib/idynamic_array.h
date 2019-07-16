/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Dynamic array type.

#ifndef MI_NEURAYLIB_IDYNAMIC_ARRAY_H
#define MI_NEURAYLIB_IDYNAMIC_ARRAY_H

#include <mi/neuraylib/iarray.h>

namespace mi {

namespace neuraylib { class IFactory; }

/** \addtogroup mi_neuray_collections
@{
*/

/// This interface represents dynamic arrays, i.e., arrays with a variable number of elements.
///
/// Arrays are either typed or untyped. Typed arrays enforce all elements to be of the same type.
/// The elements of typed arrays have to be derived from #mi::IData. The type name of a typed
/// dynamic array is the type name of the element followed by \c "[]", e.g., \c "Sint32[]" for a
/// dynamic array of #mi::Sint32 elements. Initially, a dynamic array has size zero, i.e., it
/// is empty.
///
/// Untyped arrays simply store pointers of type #mi::base::IInterface. The type name of an untyped
/// dynamic array is \c "Interface[]".
///
/// \see #mi::IArray
///
class IDynamic_array :
    public base::Interface_declare<0x575af5ad,0xc7c8,0x44a1,0x92,0xb2,0xe5,0x5d,0x5b,0x9a,0x90,0xff,
                                   IArray>
{
public:
    /// Sets the size of the array to \p size.
    ///
    /// If the array size decreases, the first \p size elements of the array are maintained and
    /// the surplus elements are removed from the array. If the array size increases, the
    /// existing elements are maintained, and the additional elements are default-constructed.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    virtual void set_length( Size size) = 0;

    /// Sets the size of the array to zero.
    ///
    /// Removes all existing elements from the array (if any). Equivalent to calling #set_length()
    /// with parameter 0.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    virtual void clear() = 0;

    /// Inserts the given element at the given index.
    ///
    /// Stores \p element at slot \p index and shifts the remaining elements by one slot.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -1: \p index is out of bounds.
    ///                - -2: \p element is \c NULL or has the wrong type.
    virtual Sint32 insert( Size index, base::IInterface* element) = 0;

    /// Removes the element stored at the given index.
    ///
    /// Removes the element at \p index and shifts the remaining elements by one slot.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -1: \p index is out of bounds.
    virtual Sint32 erase( Size index) = 0;

    /// Stores the given element at the end of the array.
    ///
    /// Increases the array size by one and stores \p element at the end of the array.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -2: \p element is \c NULL or has the wrong type.
    virtual Sint32 push_back( base::IInterface* element) = 0;

    /// Removes the last element from the array and decreases the array size by one.
    ///
    /// \note If the array represents an attribute (or part thereof), you must not call this method
    ///       while holding other pointers to the attribute (or to that part of the attribute),
    ///       including pointers to array elements, to struct members of array elements, etc.
    ///
    /// \return
    ///                -  0: Success.
    ///                - -3: The array is empty.
    virtual Sint32 pop_back() = 0;

    /// Returns the last element of the array.
    ///
    /// \return        The last element, or \c NULL if the array is empty.
    virtual const base::IInterface* back() const = 0;

    /// Returns the last element of the array.
    ///
    /// \return        The last element, or \c NULL if the array is empty.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid &)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    const T* back() const
    {
        const base::IInterface* ptr_iinterface = back();
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the last element of the array.
    ///
    /// \return        The last element, or \c NULL if the array is empty.
    virtual base::IInterface* back() = 0;

    /// Returns the last element of the array.
    ///
    /// \return        The last element, or \c NULL if the array is empty.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid &)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    T* back()
    {
        base::IInterface* ptr_iinterface = back();
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the first element of the array.
    ///
    /// \return        The first element, or \c NULL if the array is empty.
    virtual const base::IInterface* front() const = 0;

    /// Returns the first element of the array.
    ///
    /// \return        The first element, or \c NULL if the array is empty.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid &)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    const T* front() const
    {
        const base::IInterface* ptr_iinterface = front();
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Returns the first element of the array.
    ///
    /// \return        The first element, or \c NULL if the array is empty.
    virtual base::IInterface* front() = 0;

    /// Returns the first element of the array.
    ///
    /// \return        The first element, or \c NULL if the array is empty.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid &)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    T* front()
    {
        base::IInterface* ptr_iinterface = front();
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }
};

/*@}*/ // end group mi_neuray_collections

} // namespace mi

#endif // MI_NEURAYLIB_IDYNAMIC_ARRAY_H
