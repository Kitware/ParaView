/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Database scopes.

#ifndef MI_NEURAYLIB_ISCOPE_H
#define MI_NEURAYLIB_ISCOPE_H

#include <mi/base/interface_declare.h>

#include <mi/neuraylib/itransaction.h>

namespace mi {

namespace neuraylib {

/** \addtogroup mi_neuray_database_access
@{
*/

/// A scope is the context which determines the visibility of database elements.
///
/// Scopes are organized in a tree-like fashion and have a so-called \em privacy \em level. The root
/// of the tree is called \em global \em scope and has privacy level 0. On each path from the global
/// scope to one of the leafs of the tree the privacy levels are strictly increasing. Scopes are
/// identified with a cluster-unique ID which can be used to access a scope on any host in the
/// cluster.
///
/// A database element stored in a given scope is only visible in this scope and all child scopes.
/// For example, a database element stored in the global scope is visible in all scopes. This
/// visibility concept for database scopes and elements is similar to scopes and the lifetime of
/// stack variables in programming languages.
///
/// Any database element can exist in multiple versions (at most one version per scope). In this
/// case the scope at hand does not just determine the visibility itself but also determines which
/// version is visible. The version from the current scope has highest priority, next is the version
/// from the parent scope, etc., until the global scope is reached. Again, this is similar to
/// shadowing of variables with the same name in programming languages.
///
/// For scope management see the methods on #mi::neuraylib::IDatabase.
///
/// \if MDL_SDK_API
/// \note The MDL SDK currently supports only \em one scope, the global scope. It also supports only
///       one transaction at a time.
/// \endif
class IScope : public
    mi::base::Interface_declare<0x578df0c5,0xab97,0x460a,0xb5,0x0a,0x2c,0xf8,0x54,0x22,0x31,0xb9>
{
public:
    /// Creates a new transaction associated with this scope.
    ///
    /// \if DICE_API DiCE users should treat \c ITransaction as an opaque type. Instead, you should
    /// use #mi::neuraylib::IDice_transaction which is better suited for the needs of DiCE. To
    /// create such a DiCE transaction call the templated variant
    /// #mi::neuraylib::IScope::create_transaction<mi::neuraylib::IDice_transaction>(). \endif
    ///
    /// \if MDL_SDK_API \note The MDL SDK currently supports only one transaction at a time. \endif
    ///
    /// \return   A transaction associated with this scope.
    virtual ITransaction* create_transaction() = 0;

    /// Creates a new transaction associated with this scope.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    ///
    /// \if MDL_SDK_API \note The MDL SDK currently supports only one transaction at a time. \endif
    ///
    /// \tparam T   The interface type of the transaction to create.
    /// \return     A transaction associated with this scope.
    template<class T>
    T* create_transaction()
    {
        ITransaction* ptr_itransaction = create_transaction();
        if ( !ptr_itransaction)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_itransaction->get_interface( typename T::IID()));
        ptr_itransaction->release();
        return ptr_T;
    }

    /// Returns the ID of the scope.
    ///
    /// Can be used to retrieve the scope from the database later.
    ///
    /// \return   The ID of the scope.
    virtual const char* get_id() const = 0;

    /// Returns the privacy level of the scope.
    ///
    /// The global scope has privacy level 0, all other scopes have higher privacy levels. On each
    /// path from the global scope to any other scope in the scope tree the privacy levels are
    /// strictly increasing.
    ///
    /// \return   The privacy level of the scope.
    virtual Uint8 get_privacy_level() const = 0;

    /// Returns the name of the scope.
    ///
    /// \return   The name of the scope, or \c NULL if the scope has no name.
    virtual const char* get_name() const = 0;

    /// Returns the parent scope.
    ///
    /// \return   The parent scope or \c NULL if the scope is the global scope.
    virtual IScope* get_parent() const = 0;
};

/*@}*/ // end group mi_neuray_database_access

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ISCOPE_H
