/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Abstract interface for user-defined classes.

#ifndef MI_NEURAYLIB_IUSER_CLASS_H
#define MI_NEURAYLIB_IUSER_CLASS_H

#include <mi/base/interface_declare.h>
#include <mi/base/interface_implement.h>
#include <mi/base/lock.h>
#include <mi/neuraylib/iserializer.h>

namespace mi {

class IArray;

namespace neuraylib {

class IDeserializer;
class ITransaction;

/** \addtogroup mi_neuray_plugins
@{
*/

/// Abstract interface for user-defined classes.
///
/// All user-defined classes have to be derived from this interface.
/// The mixing #mi::neuraylib::User_class is helpful to implement this interface (or interfaces
/// derived from it). User-defined classes have to be registered via
/// #mi::neuraylib::IExtension_api::register_class().
class IUser_class : public
    mi::base::Interface_declare<0xa8bbfac5,0xf1b0,0x4ab5,0x99,0x18,0x9a,0x46,0xf8,0xb8,0x32,0x2b,
                                neuraylib::ISerializable>
{
public:
    /// Creates a copy of the database element.
    ///
    /// Duplicating a database element is used by the database when it needs to create a new version
    /// of the given element, e.g., when someone edits an existing element. This member function
    /// must create and return a full copy of the element.
    ///
    /// \return A newly allocated instance of the database element. The pointer represents a
    ///         complete replica of the database element on which the member function was called
    ///         and is owned by the database.
    virtual IUser_class* copy() const = 0;

    /// Returns a human readable identifier for the class this database element belongs to.
    ///
    /// This name is \em not required for proper operation, but can be useful for debugging. For
    /// example, the name is used to display the class name in the tag table of the HTTP
    /// administration console.
    ///
    /// \return         The class name.
    virtual const char* get_class_name() const = 0;

    /// Returns the list of elements referenced by this element.
    ///
    /// The database ensures that elements will not be removed from the database as long as they
    /// are referenced by other elements, even if they have been scheduled for removal
    /// by \ifnot DICE_API #mi::neuraylib::ITransaction::remove(). \else
    /// #mi::neuraylib::IDice_transaction::remove(). \endif
    ///
    /// \param transaction   A transaction that can be used to create the return value or temporary
    ///                      values.
    /// \return              An array of strings, where each string is the name of a referenced
    ///                      element. Can be \c NULL if the element does not refer other elements.
    virtual IArray* get_references( ITransaction* transaction) const = 0;

    //  Sets the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual bool set_pointer( const base::IInterface* pointer) = 0;

    //  Returns the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual const base::IInterface* get_pointer() const = 0;
};

/// This mixin class should be used to implement the \c %IUser_class interface.
///
/// This interface provides a default implementation of some of the pure virtual methods of the
/// #mi::neuraylib::IUser_class interface.
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = IUser_class>
class User_class : public base::Interface_implement<I>
{
public:
    /// Declares the class ID
    typedef base::Uuid_t<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11> IID;

    /// Default constructor
    User_class() : m_pointer( 0) { }

    /// Copy constructor
    User_class( const User_class& other) : base::Interface_implement<I>( other), m_pointer( 0) { }

    /// Assignment operator
    User_class& operator=( const User_class& other)
    {
        base::Interface_implement<I>::operator=( other);
        return *this;
    }

    /// Destructor
    ~User_class()
    {
        mi_base_assert( m_pointer == 0);
    }

    /// Returns a human readable class name
    virtual const char* get_class_name() const
    {
        return "User class";
    }

    /// Returns the class ID corresponding to the template parameters of this mixin class.
    virtual base::Uuid get_class_id() const
    {
        return IID();
    }

    //  Overrides the standard release() implementation.
    //
    //  If the release count drops to 1, and the embedded pointer is set, release it.
    virtual Uint32 release() const
    {
        base::Lock::Block block( &m_pointer_lock);
        base::Interface_implement<I>::retain();
        Uint32 count = base::Interface_implement<I>::release();
        if( count == 1) {
            block.release();
            return base::Interface_implement<I>::release();
        }
        if(( count == 2) && m_pointer) {
            m_pointer->release();
            m_pointer = 0;
        }
        return base::Interface_implement<I>::release();
    }

    //  Sets the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual bool set_pointer( const base::IInterface* pointer)
    {
        base::Lock::Block block( &m_pointer_lock);
        if( m_pointer)
            return false;
        m_pointer = pointer;
        if( m_pointer)
            m_pointer->retain();
        return true;
    }

    //  Returns the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual const base::IInterface* get_pointer() const
    {
        base::Lock::Block block( &m_pointer_lock);
        if( m_pointer)
            m_pointer->retain();
        return m_pointer;
    }

private:
    //  The embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not access the pointer.
    mutable const base::IInterface* m_pointer;

    //  The lock that protects the embedded pointer.
    mutable base::Lock m_pointer_lock;
};

/*@}*/ // end group mi_neuray_plugins

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IUSER_CLASS_H
