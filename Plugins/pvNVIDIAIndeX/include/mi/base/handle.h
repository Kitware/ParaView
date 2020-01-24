/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/handle.h
/// \brief Smart-pointer handle class for interfaces, const and non-const version.

#ifndef MI_BASE_HANDLE_H
#define MI_BASE_HANDLE_H

#include <mi/base/assert.h>
#include <mi/base/config.h> // for MI_CXX_FEATURE_RVALUE_REFERENCES
#include <mi/base/iinterface.h>

namespace mi {
namespace base {

/** \addtogroup mi_base_iinterface
@{
*/

// Helper type to define Dup_interface
struct Dup_interface_helper {};

/// Type for a symbolic constant to trigger a special constructor in the %Handle class.
///
/// \see #mi::base::Handle::Handle(Interface* ptr,Dup_interface)
typedef const Dup_interface_helper* Dup_interface;

/// Symbolic constant to trigger a special constructor in the %Handle class.
///
/// \see #mi::base::Handle::Handle(Interface* ptr,Dup_interface)
static const Dup_interface DUP_INTERFACE = 0;

/// %Handle class template for interfaces, automatizing the lifetime control via reference counting.
///
/// The Handle class is smart-pointer class that handles the reference counting of interface classes
/// automatically. A handle stores internally a pointer to the underlying interface class.
///
/// Template parameter:
///   - \c Interface: an interface class, i.e., either the
///     mi::base::IInterface class itself or a class derived from it.
///
/// \note
/// The Handle class is const correct: Use Handle< const I > for a const pointer to an interface
/// class \c I and Handle< I > for a mutable pointer to an interface class \c I.
///
/// The Handle class has two constructors which differ in the way they handle ownership of the
/// interface pointer they are constructed from (actually, there is a third constructor, the default
/// constructor, which constructs an invalid handle). In the first form mi::base::Handle<I>(I*) the
/// Handle instance takes ownership of the interface. In the second form
/// mi::base::Handle<I>(I*,Dup_interface) it does not take ownership of the interface, but
/// duplicates it.
///
/// The following two examples, based on the neuraylib API, illustrate the use of both constructors.
/// The first example demonstrates the prevailing use case where you want to locally store the
/// interface pointer returned from an API function for subsequent usage. In this case you should
/// use the first form of the constructor which takes ownership of the interface.
///
/// \code
///     mi::base::Handle<mi::neuraylib::INeuray> neuray( mi_neuray_factory());
///     neuray->start( true);
/// \endcode
///
/// On the other hand, assume that you want to store a pointer to an interface whose lifetime you do
/// not control. This typically happens when a pointer is passed as parameter to a function. By
/// convention such pointers are owned by the function caller.
///
/// \code
///     void foo( mi::base::IInterface* interface)
///     {
///         mi::base::Handle<mi::base::IInterface> handle( interface, mi::base::DUP_INTERFACE);
///         // do something with handle
///     }
///
///     mi::base::IInterface* interface = ...
///     foo( interface);
///     // do more things with interface
/// \endcode
///
/// If you had not used the second form of the handle constructor in this example, the handle
/// destructor would have decremented the reference counter of \c interface to 0 at the end of
/// foo(). Therefore, the corresponding interface would have been destroyed and the \c interface
/// pointer would be invalid after the foo() call.
///
/// In contrast, the second form of the handle constructor does not take ownership of the \c
/// interface pointer, but increments the reference count once more. Consequently, when the handle
/// is destroyed, the reference count does not drop to 0.
///
/// Note that this use case often shows up when you store a pointer passed in via a member function
/// as a class member.
/// 
/// \if IRAY_API See also [:ipmlink handle_class %Handle class] for an extended example (and
/// [:ipmlink reference_counting Reference counting] for the same example without handle class).
/// \endif
/// \if DICE_API See also \ref mi_neuray_handle for an extended example (and \ref
/// mi_neuray_refcounting for the same example without handle class). \endif
/// \if MDL_SDK_API See also \ref mi_neuray_handle for an extended example (and \ref
/// mi_neuray_refcounting for the same example without handle class). \endif
///
///    \par Include File:
///    <tt> \#include <mi/base/handle.h></tt>
///
/// \sa
///     #make_handle() and #make_handle_dup() for creating a typed handle from a typed
///     interface %pointer
template <class Interface>
class Handle
{
public:
    /// Own type.
    typedef Handle<Interface> Self;

    /// Type of the underlying interface.
    typedef Interface Interface_type;

    // STL iterator inspired typedef names

    /// Type of the underlying interface.
    typedef Interface  value_type;

    /// Difference type (signed integral type to hold pointer differences).
    typedef Difference difference_type;

    /// Mutable-pointer type to underlying interface.
    typedef Interface* pointer;

    /// Mutable-reference type to underlying interface.
    typedef Interface& reference;

private:
    // Pointer to underlying interface, can be \c NULL
    Interface* m_iptr;

public:
    /// Default constructor, initializes handle to hold an invalid interface.
    Handle() : m_iptr( 0) { }

    /// Constructor from interface pointer, takes ownership of interface.
    ///
    /// The constructor does not increment the reference count of \p ptr assuming it is already set
    /// properly, e.g., by a corresponding get_interface() call. It therefore takes over the
    /// ownership of the interface pointer.
    explicit Handle( Interface* ptr) : m_iptr( ptr) { }

    /// Constructor from interface pointer, does not take ownership of interface but duplicates it.
    ///
    /// The constructor increments the reference count of \p ptr so that it does not influence the
    /// interface when it decrements the reference count later on. You can use this constructor for
    /// example to hold interfaces that are passed into functions as parameters because by
    /// convention they are owned by the function caller. You can pass the constant DUP_INTERFACE as
    /// the second argument.
    Handle( Interface* ptr, Dup_interface)
      : m_iptr( ptr)
    {
        if( m_iptr)
            m_iptr->retain();
    }

    /// Copy constructor, increments reference count if interface is valid.
    Handle( const Self& other)
      : m_iptr( other.m_iptr)
    {
        if( m_iptr)
            m_iptr->retain();
    }

    /// Copy constructor template which allows the construction from assignment compatible interface
    /// pointers, increments reference count if interface is valid.
    ///
    /// This constructor allows specifically the construction of a <tt>Handle< const I ></tt> from a
    /// <tt>Handle< I ></tt> value, which corresponds to the assignment of a mutable pointer to a
    /// const pointer. In addition, promotion of derived interfaces to %base interfaces is allowed.
    template <class Interface2>
    Handle( const Handle<Interface2>& other)
      : m_iptr( other.get())
    {
        if( m_iptr)
            m_iptr->retain();
    }

#ifdef MI_CXX_FEATURE_RVALUE_REFERENCES
    /// Move constructor.
    Handle( Self&& other)
      : m_iptr( other.m_iptr)
    {
        other.m_iptr = 0;
    }
#endif

    /// Swap two interfaces.
    void swap( Self& other)
    {
        Interface* tmp_iptr = m_iptr;
        m_iptr = other.m_iptr;
        other.m_iptr = tmp_iptr;
    }

    /// Assignment operator, releases old interface and increments reference count of the new
    /// interface if interface is valid.
    Self& operator=( const Self& other)
    {
        Self( other).swap( *this);
        return *this;
    }

    /// Assignment operator template, releases old interface and increments reference count of the
    /// new interface if interface is valid.
    ///
    /// This assignment operator allows specifically the assignment of a <tt>Handle< I ></tt> to a
    /// <tt>Handle< const I ></tt> value, which corresponds to the assignment of a mutable pointer
    /// to a const pointer. In addition, promotion of derived interfaces to %base interfaces is
    /// allowed.
    template <class Interface2>
    Self& operator=( const Handle<Interface2>& other)
    {
        Self( other).swap( *this);
        return *this;
    }

#ifdef MI_CXX_FEATURE_RVALUE_REFERENCES
    /// Move assignment operator, releases old interface.
    Self& operator=( Self&& other)
    {
        if( this != &other) {
            if( m_iptr)
                m_iptr->release();
            m_iptr = other.m_iptr;
            other.m_iptr = 0;
        }
        return *this;
    }
#endif

    /// Assignment operator from interface pointer, releases old interface and assigns new interface
    /// \p ptr, takes ownership of interface.
    ///
    /// Does not increment reference count of \p ptr assuming it is already set properly, e.g., by a
    /// corresponding get_interface() call.
    Self& operator=( Interface* ptr)
    {
        Self( ptr).swap( *this);
        return *this;
    }

    /// Releases the current interface, decrementing the reference count.
    void reset()
    {
        if( m_iptr) {
            m_iptr->release();
            m_iptr = 0;
        }
    }

    /// Destructor, releases the interface if it is valid, which decrements the reference count, and
    /// triggers thus the deletion of the interface implementation once the reference count reaches
    /// zero.
    ~Handle()
    {
        if( m_iptr)
            m_iptr->release();
    }

    /// Returns \c true if the interface is valid.
    bool is_valid_interface() const { return m_iptr != 0; }

    /// Access to the interface. Returns 0 for an invalid interface.
    Interface* get() const { return  m_iptr; }

    /// The dereference operator accesses the interface.
    ///
    /// \pre is_valid_interface().
    Interface& operator*() const
    {
        mi_base_assert_msg( is_valid_interface(), "precondition");
        return *m_iptr;
    }

    /// The arrow operator accesses the interface.
    ///
    /// \pre is_valid_interface().
    Interface* operator->() const
    {
        mi_base_assert_msg( is_valid_interface(), "precondition");
        return m_iptr;
    }

    /// Returns a new handle for a possibly different interface type, which is the equivalent of a
    /// dynamic cast.
    ///
    /// Returns a handle with an invalid interface if the requested interface type is not supported
    /// by the underlying interface implementation or if this interface is itself already invalid.
    template <class New_interface>
    Handle<New_interface> get_interface() const
    {
        if( !is_valid_interface())
            return Handle<New_interface>( 0);
        return Handle<New_interface>( static_cast< New_interface*>(
            m_iptr->get_interface( typename New_interface::IID())));
    }

    /// Helper typedef.
    ///
    /// This typedef represent the type of #is_valid_interface() used by the
    /// #bool_conversion_support() operator.
    typedef bool (Handle::*bool_conversion_support)() const;

    /// Helper function for the conversion of a Handle<Interface> to a bool.
    ///
    /// This helper function allows to write
    ///   \code
    ///     Handle<T> h(...);
    ///     if( h) ...
    ///   \endcode
    /// instead of
    ///   \code
    ///     Handle<T> h(...);
    ///     if( h.is_valid_interface()) ...
    ///   \endcode
    operator bool_conversion_support() const
    {
        return is_valid_interface() ? &Handle<Interface>::is_valid_interface : 0;
    }

    /// Returns \c true if the underlying interface pointer of \p lhs is equal to \p rhs
    friend bool operator==( const Handle<Interface>& lhs, const Interface* rhs)
    {
        return lhs.get() == rhs;
    }

    /// Returns \c true if \p lhs is equal to the underlying interface pointer of \p rhs
    friend bool operator==( const Interface* lhs, const Handle<Interface>& rhs)
    {
        return lhs == rhs.get();
    }

    /// Returns \c true if the underlying interface pointer of \p lhs is not equal to \p rhs
    friend bool operator!=( const Handle<Interface>& lhs, const Interface* rhs)
    {
        return !( lhs == rhs);
    }

    /// Returns \c true if \p lhs is not equal to the underlying interface pointer of \p rhs
    friend bool operator!=( const Interface* lhs, const Handle<Interface>& rhs) {
        return !( lhs == rhs);
    }
};

/// Returns \c true if the underlying interface pointers are equal
template <class Interface1, class Interface2>
inline bool operator==( const Handle<Interface1>& lhs, const Handle<Interface2>& rhs)
{
    return lhs.get() == rhs.get();
}

/// Returns \c true if the underlying interface pointers are not equal
template <class Interface1, class Interface2>
inline bool operator!=( const Handle<Interface1>& lhs, const Handle<Interface2>& rhs)
{
    return !( lhs == rhs);
}

/// Returns a handle that holds the interface pointer passed in as argument.
///
/// This helper function template simplifies the creation of handles with an interface type that
/// matches the type of the interface argument. In contrast to #make_handle_dup(), the handle takes
/// ownership of the interface.
template <class Interface>
inline Handle<Interface> make_handle( Interface* iptr)
{
    return Handle<Interface>( iptr);
}

/// Converts passed-in interface pointer to a handle, without taking interface over.
///
/// This helper function template simplifies the creation of handles with an interface type that
/// matches the type of the interface argument. In contrast to #make_handle(), the handle does not
/// take ownership of the interface.
template <class Interface>
inline Handle<Interface> make_handle_dup( Interface* iptr)
{
    return Handle<Interface>( iptr, DUP_INTERFACE);
}

/*@}*/ // end group mi_base_iinterface

} // namespace base
} // namespace mi

#endif // MI_BASE_HANDLE_H
