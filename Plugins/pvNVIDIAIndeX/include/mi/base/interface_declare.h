/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/base/interface_declare.h
/// \brief Mixin class template for deriving new interface declarations

#ifndef MI_BASE_INTERFACE_DECLARE_H
#define MI_BASE_INTERFACE_DECLARE_H

#include <mi/base/iinterface.h>
#include <mi/base/types.h>
#include <mi/base/uuid.h>

namespace mi {
namespace base {

/** \addtogroup mi_base_iinterface
@{
*/

// Forward declaration
class IInterface;

/// Mixin class template for deriving new interface declarations.
///
/// #mi::base::Interface_declare is a mixin class template that allows you to derive
/// new interface classes easily. It provides you with implementations for the interface ID
/// handling and support for the #mi::base::IInterface::get_interface method used 
/// by the corresponding mixin class template #mi::base::Interface_implement. 
///
/// It derives from the interface \c I, which is by default #mi::base::IInterface. 
/// Each interface needs an interface ID (represented as a universally unique identifier
/// (UUID)), which is defined here as 11 template parameter constants.
///
///   \par Include File:
///    <tt> \#include <mi/base/interface_declare.h></tt>
///
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = IInterface>
class Interface_declare : public I
{
public:
    /// Own type.
    typedef Interface_declare<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I> Self;

    /// Declares the interface ID (IID) of this interface.
    typedef Uuid_t<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11> IID;

    /// Compares the interface ID \p iid against the interface ID of this interface and of its
    /// ancestors.
    ///
    /// \return \c true if \p iid == \c IID() or is equal to one of the interface IDs of 
    /// its ancestors, and \c false otherwise.
    static bool compare_iid( const Uuid& iid) {
        if( iid == IID())
            return true;
        return I::compare_iid( iid);
    }

protected:
    // Acquires a const interface.
    //
    // Static helper function for implementing #get_interface(const Uuid&).
    //
    // \param iinterface   The interface to act on.
    // \param interface_id Interface ID of the interface to acquire.
    static const IInterface* get_interface_static(
        const IInterface* iinterface,
        const Uuid& interface_id)
    {
        if( interface_id == IID()) {
            const Self* self = static_cast<const Self *>( iinterface);
            self->retain();
            return self;
        }
        return I::get_interface_static( iinterface, interface_id);
    }

    // Acquires a mutable interface.
    //
    // Static helper function for implementing #get_interface(const Uuid&).
    //
    // \param iinterface   The interface to act on.
    // \param interface_id Interface ID of the interface to acquire.
    static IInterface* get_interface_static(
        IInterface* iinterface,
        const Uuid& interface_id)
    {
        if( interface_id == IID()) {
            Self* self = static_cast<Self*>( iinterface);
            self->retain();
            return self;
        }
        return I::get_interface_static( iinterface, interface_id);
    }
};

/*@}*/ // end group mi_base_iinterface

} // namespace base
} // namespace mi

#endif // MI_BASE_INTERFACE_DECLARE_H
