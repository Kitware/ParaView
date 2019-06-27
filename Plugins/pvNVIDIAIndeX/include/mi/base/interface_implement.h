/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/interface_implement.h
/// \brief Mixin class template for deriving interface implementations.

#ifndef MI_BASE_INTERFACE_IMPLEMENT_H
#define MI_BASE_INTERFACE_IMPLEMENT_H

#include <mi/base/types.h>
#include <mi/base/uuid.h>
#include <mi/base/atom.h>

namespace mi {
namespace base {

/** \addtogroup mi_base_iinterface
@{
*/

// Forward declaration
class IInterface;

/// Mixin class template for deriving interface implementations.
///
/// #mi::base::Interface_implement is a mixin class template that allows you to derive
/// interface class implementations easily. It provides you with the full implementation
/// of reference counting and the #mi::base::IInterface::get_interface(const Uuid&)
/// method. It requires that you used interfaces derived from the corresponding mixin
/// class template #mi::base::Interface_declare.
///
/// #mi::base::Interface_implement is derived from the interface \c I.
///
/// \tparam I The interface class that this class implements.
///
///   \par Include File:
///    <tt> \#include <mi/base/interface_implement.h></tt>
///
template <class I>
class Interface_implement : public I
{
public:
    /// Constructor.
    ///
    /// \param initial   The initial reference count (defaults to 1).
    Interface_implement( Uint32 initial=1)
        : m_refcnt( initial)
    {
    }

    /// Copy constructor.
    ///
    /// Initializes the reference count to 1.
    Interface_implement( const Interface_implement<I>& other)
        : m_refcnt( 1)
    {
        // avoid warning
        (void) other;
    }

    /// Assignment operator.
    ///
    /// The reference count of \c *this and \p other remain unchanged.
    Interface_implement<I>& operator=( const Interface_implement<I>& other)
    {
        // Note: no call of operator= on m_refcount
        // avoid warning
        (void) other;
        return *this;
    }

    /// Increments the reference count.
    ///
    /// Increments the reference count of the object referenced through this interface
    /// and returns the new reference count. The operation is thread-safe.
    virtual Uint32 retain() const
    {
        return ++m_refcnt;
    }

    /// Decrements the reference count.
    ///
    /// Decrements the reference count of the object referenced through this interface
    /// and returns the new reference count. If the reference count dropped to
    /// zero, the object will be deleted. The operation is thread-safe.
    virtual Uint32 release() const
    {
        Uint32 cnt = --m_refcnt;
        if( !cnt)
            delete this;
        return cnt;
    }

    /// Acquires a const interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL \c const #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual const IInterface* get_interface( const Uuid& interface_id) const
    {
        return I::get_interface_static( this, interface_id);
    }

    /// Acquires a mutable interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual IInterface* get_interface( const Uuid& interface_id)
    {
        return I::get_interface_static( this, interface_id);
    }

    using I::get_interface;

    /// Returns the interface ID of the most derived interface.
    Uuid get_iid() const
    {
        return typename I::IID();
    }

protected:
    virtual ~Interface_implement() {}

private:
    mutable Atom32 m_refcnt;
};


/// Mixin class template for deriving interface implementations from two interfaces.
///
/// #mi::base::Interface_implement_2 is a mixin class template that allows you to derive
/// interface class implementations easily. It provides you with the full implementation
/// of reference counting and the #mi::base::IInterface::get_interface(const Uuid&)
/// method. It requires that you used interfaces derived from the corresponding mixin
/// class template #mi::base::Interface_declare.
///
/// #mi::base::Interface_implement is derived from the interface \c I1 and \c I2.
/// In case of ambiguities, interface \c I1 is preferred.
///
/// \tparam I1 First  interface class that this class implements.
/// \tparam I2 Second interface class that this class implements.
///
///   \par Include File:
///    <tt> \#include <mi/base/interface_implement.h></tt>
///
template <class I1, class I2>
class Interface_implement_2 : public I1, public I2
{
public:
    /// Constructor.
    ///
    /// \param initial   The initial reference count (defaults to 1).
    Interface_implement_2( Uint32 initial=1)
        : m_refcnt( initial)
    {
    }

    /// Copy constructor.
    ///
    /// Initializes the reference count to 1.
    Interface_implement_2( const Interface_implement_2<I1,I2>& other)
        : m_refcnt( 1)
    {
        // avoid warning
        (void) other;
    }

    /// Assignment operator.
    ///
    /// The reference count of \c *this and \p other remain unchanged.
    Interface_implement_2<I1,I2>& operator=( const Interface_implement_2<I1,I2>& other)
    {
        // Note: no call of operator= on m_refcount
        // avoid warning
        (void) other;
        return *this;
    }

    /// Increments the reference count.
    ///
    /// Increments the reference count of the object referenced through this interface
    /// and returns the new reference count. The operation is thread-safe.
    virtual Uint32 retain() const
    {
        return ++m_refcnt;
    }

    /// Decrements the reference count.
    ///
    /// Decrements the reference count of the object referenced through this interface
    /// and returns the new reference count. If the reference count dropped to
    /// zero, the object will be deleted. The operation is thread-safe.
    virtual Uint32 release() const
    {
        Uint32 cnt = --m_refcnt;
        if( !cnt)
            delete this;
        return cnt;
    }

    /// Acquires a const interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL \c const #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual const IInterface* get_interface( const Uuid& interface_id) const
    {
        const IInterface* iptr = I1::get_interface_static( static_cast<const I1*>(this),
                                                           interface_id);
        if ( iptr == 0)
            iptr = I2::get_interface_static( static_cast<const I2*>(this), interface_id);
        return iptr;
    }

    /// Acquires a mutable interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual IInterface* get_interface( const Uuid& interface_id)
    {
        IInterface* iptr = I1::get_interface_static(static_cast<I1*>(this),interface_id);
        if ( iptr == 0)
            iptr = I2::get_interface_static( static_cast<I2*>(this), interface_id);
        return iptr;
    }

    using I1::get_interface;

    /// Returns the interface ID of the most derived interface.
    Uuid get_iid() const
    {
        return typename I1::IID();
    }

protected:
    virtual ~Interface_implement_2() {}

private:
    mutable Atom32 m_refcnt;
};


/// Mixin class template for deriving singleton interface implementations,
/// where the reference count is fixed to one.
///
/// #mi::base::Interface_implement is a mixin class template that allows you to derive
/// interface class implementations easily. It provides you with a special implementation
/// of reference counting and the #mi::base::IInterface::get_interface(const Uuid&)
/// method. It requires that you used interfaces derived from the corresponding mixin
/// class template #mi::base::Interface_declare.
///
/// The reference counting is fixed to have a count of always one. It is suitable, for
/// example, for singleton objects that get once allocated and never deallocated.
///
/// #mi::base::Interface_implement is derived from the interface \c I.
///
/// \tparam I The interface class that this class implements.
///
template <class I>
class Interface_implement_singleton : public I
{
public:
    /// Returns the fixed reference count of one.
    ///
    /// Implements #mi::base::IInterface::retain() with a constant reference
    /// count of one.
    virtual Uint32 retain() const
    {
        return 1;
    }

    /// Returns the fixed reference count of one.
    ///
    /// Implements #mi::base::IInterface::release() with a constant reference
    /// count of one. The object will never be deleted through a release call.
    virtual Uint32 release() const
    {
        return 1;
    }

    /// Acquires a const interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL \c const #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual const IInterface* get_interface( const Uuid& interface_id) const
    {
        return I::get_interface_static( this, interface_id);
    }

    /// Acquires a mutable interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the
    /// new interface pointer, whose reference count has been retained once. The caller
    /// must release the returned interface pointer at the end to prevent a memory leak.
    virtual IInterface* get_interface( const Uuid& interface_id)
    {
        return I::get_interface_static( this, interface_id);
    }

    using I::get_interface;

    /// Returns the interface ID of the most derived interface.
    Uuid get_iid() const
    {
        return typename I::IID();
    }

protected:
    virtual ~Interface_implement_singleton() {}
};


/*@}*/ // end group mi_base_iinterface

} // namespace base
} // namespace mi

#endif // MI_BASE_INTERFACE_IMPLEMENT_H
