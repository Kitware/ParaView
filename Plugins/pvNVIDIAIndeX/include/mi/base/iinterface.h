/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/iinterface.h
/// \brief The basic extensible interface

#ifndef MI_BASE_IINTERFACE_H
#define MI_BASE_IINTERFACE_H

#include <mi/base/types.h>
#include <mi/base/uuid.h>

namespace mi {

namespace base {

/** \defgroup mi_base_iinterface Interface Framework Technology
    \ingroup mi_base

    The classes provided here support conventional modern C++ library design principles for
    component software to achieve binary compatibility across shared library boundaries and future
    extensibility. The design provides access through interfaces, which are abstract %base classes
    with pure virtual member functions.

    Interfaces derived from #mi::base::IInterface refer to reference-counted dynamic resources that
    need to be released. To avoid manual reference counting, you can use the provided handle class
    #mi::base::Handle, which is a smart-pointer implementation with automatic reference counting.
    You can also use other handle class implementations, e.g., \c std::tr1::shared_ptr<T> (or
    \c boost::shared_ptr<T>).
*/

/** \addtogroup mi_base_iinterface
@{
*/

// Forward declaration, see below for definition
class IInterface;

/** The basic extensible interface.

    The #mi::base::IInterface class is a pure virtual class with no members, a so-called interface.
    It is used as the common %base class of interface hierarchies. The purpose of this class is the
    management of object lifetime through reference counting, see
    #mi::base::IInterface::retain() and #mi::base::IInterface::release(), and the proper
    dynamic-cast like access to derived and otherwise related interfaces, see
    #mi::base::IInterface::get_interface(const Uuid&).

    In addition to these methods, the class #mi::base::IInterface has an inner type
    #mi::base::IInterface::IID, which is readily convertible to the type mi::base::Uuid. The type
    #mi::base::IInterface::IID has a default constructor which creates a value that contains the
    universally unique identifier (UUID a.k.a. GUID) of this interface. Each interface has such a
    local type #mi::base::IInterface::IID with a distinct and unique value. The value can be passed
    to the #mi::base::IInterface::get_interface(const Uuid&) methods, introduced below, and this is
    the mode in which it is normally used.

    \see 
      - #mi::base::Handle for a smart-pointer class automating the reference counting
      - #mi::base::Interface_declare for a helper class for deriving new interfaces
      - #mi::base::Interface_implement for a helper class for implementing interfaces

    \par Include File:
       <tt> \#include <mi/base/iinterface.h></tt>

    \par Example:
      Assume you have an interface pointer \c iptr for an allocator object with the corresponding
      interface #mi::base::IAllocator. You cannot directly cast the pointer to the corresponding
      interface pointer. Instead you need to use the #mi::base::IInterface::get_interface(const
      Uuid&) method with the corresponding interface ID (IID) value of type #mi::base::Uuid, cast
      the result and release the interface after you are done using it. You can obtain the necessary
      IID value by instantiating the \c IID type that is locally embedded in each interface.

    \par
      If you are not sure whether \c iptr refers to an object supporting an #mi::base::IAllocator
      interface you must check that the result of the
      #mi::base::IInterface::get_interface(const Uuid&) method call is not \c NULL.

    \code
        mi::base::IInterface* iptr = ...;
        mi::base::IAllocator* allocator = static_cast<mi::base::IAllocator*>(
                                              iptr->get_interface( mi::base::IAllocator::IID()));
        // check that the iptr object supports the mi::base::IAllocator interface
        if ( allocator) {
            ... // use allocator
            allocator->release();
        }
    \endcode
   
    Alternatively, you can use the more convenient and type-safe template version 
    that eliminates the need for the subsequent \c static_cast.
   
    \code
        mi::base::IInterface* iptr = ...;
        mi::base::IAllocator* allocator = iptr->get_interface<mi::base::IAllocator>();
        // check that the iptr object supports the mi::base::IAllocator interface
        if ( allocator) {
            ... // use allocator
            allocator->release();
        }
    \endcode

*/
class IInterface
{
public:

    /// Declares the interface ID (IID) of this interface.
    ///
    /// A local type in each interface type, which is distinct and unique for each interface. The
    /// type has a default constructor and the constructed value represents the universally unique
    /// identifier (UUID) for this interface. The local type is readily convertible to a
    /// #mi::base::Uuid.
    typedef Uuid_t<0,0,0,0,0,0,0,0,0,0,0> IID;

    /// Compares the interface ID \p iid against the interface ID of this interface.
    ///
    /// \return   \c true if \p iid == #mi::base::IInterface::IID(), and \c false otherwise.
    static bool compare_iid( const Uuid& iid)
    {
        return ( iid == IID());
    }

    /// Increments the reference count.
    ///
    /// Increments the reference count of the object referenced through this interface and returns
    /// the new reference count. The operation is thread-safe.
    ///
    /// \return   The new, incremented reference count.
    virtual Uint32 retain() const = 0;

    /// Decrements the reference count.
    ///
    /// Decrements the reference count of the object referenced through this interface and returns
    /// the new reference count. If the reference count dropped to zero, the object will be deleted.
    /// The operation is thread-safe.
    ///
    /// \return   The new, decremented reference count.
    virtual Uint32 release() const = 0;

    /// Acquires a const interface from another.
    ///
    /// If this interface supports the interface with the passed \p interface_id, then the method
    /// returns a non-\c NULL \c const #mi::base::IInterface* that can be casted via \c static_cast
    /// to an interface pointer of the interface type corresponding to the passed \p interface_id.
    /// Otherwise, the method returns \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must release the
    /// returned interface pointer at the end to prevent a memory leak.
    ///
    /// \param interface_id   Interface ID of the interface to acquire.
    virtual const IInterface* get_interface( const Uuid& interface_id ) const = 0;

    /// Acquires a const interface from another.
    ///
    /// If this interface supports the interface \c T, then the method returns a non-\c NULL
    /// \c const pointer to the interface \c T. Otherwise, the method returns \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must release the
    /// returned interface pointer at the end to prevent a memory leak.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to apply \c static_cast to the returned pointer, since
    /// the return type already is a const pointer to the type \p T specified as template parameter.
    ///
    /// \tparam T     The requested interface type.
    ///
    template <class T>
    const T* get_interface() const
    {
        return static_cast<const T*>( get_interface( typename T::IID()));
    }

    /// Acquires a mutable interface from another.
    ///
    /// If this interface supports the interface with the passed \p interface_id, then the methods
    /// returns a non-\c NULL #mi::base::IInterface* that can be casted via \c static_cast to an
    /// interface pointer of the interface type corresponding to the passed \p interface_id.
    /// Otherwise, the method returns \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must release the
    /// returned interface pointer at the end to prevent a memory leak.
    ///
    /// \param interface_id   Interface ID of the interface to acquire.
    virtual IInterface* get_interface( const Uuid& interface_id ) = 0;

    /// Acquires a mutable interface from another.
    ///
    /// If this interface supports the interface \c T, then the method returns a non-\c NULL pointer
    /// to the interface \c T. Otherwise, the method returns \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must release the
    /// returned interface pointer at the end to prevent a memory leak.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to apply \c static_cast to the returned pointer, since
    /// the return type already is a pointer to the type \p T specified as template parameter.
    ///
    /// \tparam T     The requested interface type.
    ///
    template <class T>
    T* get_interface()
    {
        return static_cast<T*>( get_interface( typename T::IID()));
    }

    /// Returns the interface ID of the most derived interface.
    virtual Uuid get_iid() const = 0;

protected:
    // Acquires a const interface.
    //
    // Static helper function for implementing #get_interface(const Uuid&). On #IInterface, the
    // method terminates the recursive call chain.
    //
    // \param iinterface     The interface to act on.
    // \param interface_id   Interface ID of the interface to acquire.
    static const IInterface* get_interface_static(
        const IInterface* iinterface, const Uuid& interface_id)
    {
        if( interface_id == IID()) {
            iinterface->retain();
            return iinterface;
        }
        return 0;
    }

    // Acquires a mutable interface.
    //
    // Static helper function for implementing #get_interface(const Uuid&). On #IInterface, the
    // method terminates the recursive call chain.
    //
    // \param iinterface     The interface to act on.
    // \param interface_id   Interface ID of the interface to acquire.
    static IInterface* get_interface_static(
        IInterface* iinterface, const Uuid& interface_id)
    {
        if( interface_id == IID()) {
            iinterface->retain();
            return iinterface;
        }
        return 0;
    }

};

/*@}*/ // end group mi_base_iinterface

} // namespace base

} // namespace mi

#endif // MI_BASE_IINTERFACE_H
