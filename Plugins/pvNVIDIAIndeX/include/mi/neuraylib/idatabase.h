/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component that provides access to the database.

#ifndef MI_NEURAYLIB_IDATABASE_H
#define MI_NEURAYLIB_IDATABASE_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

class IScope;

/** \defgroup mi_neuray_database_access Database Access
    \ingroup mi_neuray

    Provides access to the database using concepts like scopes and transactions.
*/

/** \addtogroup mi_neuray_database_access
@{
*/

/// This interface is used to interact with the distributed database.
///
/// \if MDL_SDK_API
/// \note The MDL SDK currently supports only \em one scope, the global scope. It also supports only
///       one transaction at time.
/// \endif
class IDatabase : public
    mi::base::Interface_declare<0x814ae637,0xde35,0x4870,0x8e,0x5b,0x7e,0x28,0x9d,0x30,0xfb,0x82>
{
public:
    /// Returns the global scope which is the root of a tree of scopes
    ///
    /// \return  The global scope which is guaranteed to exist after startup of the
    ///          system.
    virtual IScope* get_global_scope() const = 0;

    /// \ifnot MDL_SDK_API
    /// Creates a new optionally temporary scope at the given privacy level with the
    /// given parent scope ID.
    ///
    /// \note A scope continues to exist if the pointer returned by this method is released. Use
    ///       #remove_scope() to remove a scope.
    ///
    /// \param parent         The parent scope for this scope. If the value is \c NULL the created
    ///                       scope will be a child of the global scope.
    /// \param privacy_level  The privacy level of the scope. This must be higher than the
    ///                       privacy level of the parent scope. The privacy level of the global
    ///                       scope is 0 (and the global scope is the only scope with privacy level
    ///                       0). The default value of 0 indicates the privacy level of the parent
    ///                       scope plus 1.
    /// \param temp           A bool indicating if the scope is temporary. If the scope is
    ///                       temporary, then when the host that created the scope is removed
    ///                       from the cluster the scope and all data contained in the scope
    ///                       will be removed. If the scope is not temporary, the default,
    ///                       then when the creating host is removed from the cluster the
    ///                       scope and all contained data will remain in the database.
    /// \return               The created scope or \c NULL if something went wrong.
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual IScope* create_scope( IScope* parent, Uint8 privacy_level = 0, bool temp = false) = 0;

    /// Looks up and returns a scope with a given ID.
    ///
    /// \param id             The ID of the scope as returned by #mi::neuraylib::IScope::get_id().
    /// \return               The found scope or \c NULL if no such scope exists.
    virtual IScope* get_scope( const char* id) const = 0;

    /// \ifnot MDL_SDK_API
    /// Removes a scope with the specified ID.
    ///
    /// Note that scopes are reference counted. The actual removal will not happen before all
    /// elements referencing the scope have been released, e.g., child scopes, transactions,
    /// database elements, including handles to the scope itself.
    ///
    /// It is not possible to remove the global scope.
    ///
    /// \param id             The ID of the scope as returned by #mi::neuraylib::IScope::get_id().
    /// \return               0, in case of success, -1 in case of failure.
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual Sint32 remove_scope( const char* id) const = 0;

    /// \ifnot MDL_SDK_API
    /// Acquires a DB lock.
    ///
    /// The method blocks until the requested lock has been obtained. Recursively locking the
    /// same lock from within the same thread on the same host is supported.
    ///
    /// If the host holding a lock leaves the cluster, the lock is automatically released.
    ///
    /// \param lock_id   The lock to acquire.
    ///
    /// \note The locking mechanism is kind of a co-operative locking mechanism: The lock does not
    ///       prevent other threads from accessing or editing the DB. It only prevents other threads
    ///       from obtaining the same lock.
    ///
    /// \note DB locks are not restricted to threads on a single host, they apply to all threads on
    ///       all hosts in the cluster.
    ///
    /// \note DB locks are an expensive operation and should only be used when absolutely necessary.
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual void lock( Uint32 lock_id) = 0;

    /// \ifnot MDL_SDK_API
    /// Releases a previously obtained DB lock.
    ///
    /// If the lock has been locked several times from within the same thread on the same host,
    /// it simply decrements the lock count. If the lock count reaches zero, the lock is released.
    ///
    /// \param lock_id   The lock to release.
    /// \return          0, in case of success, -1 in case of failure, i.e, the lock is not held
    ///                  by this thread on this host
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual Sint32 unlock( Uint32 lock_id) = 0;

    /// \ifnot MDL_SDK_API
    /// Creates or retrieves a new named scope at the given privacy level with the given parent
    /// scope ID.
    ///
    /// \param name           A name which can be used to lookup the scope.
    ///                       If a scope with the same name exists already then it will be returned
    ///                       if the parent and privacy level are identical. Otherwise
    ///                       creating the scope will fail.
    /// \param parent         The parent scope for this scope. If the value is \c NULL the created
    ///                       scope will be a child of the global scope.
    /// \param privacy_level  The privacy level of the scope. This must be higher than the
    ///                       privacy level of the parent scope. The privacy level of the global
    ///                       scope is 0 (and the global scope is the only scope with privacy level
    ///                       0). The default value of 0 indicates the privacy level of the parent
    ///                       scope plus 1.
    /// \return               The created scope or \c NULL if something went wrong.
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual IScope* create_or_get_named_scope(
        const char* name, IScope* parent = 0,  Uint8 privacy_level = 0) = 0;

    /// \ifnot MDL_SDK_API
    /// Looks up and returns a scope with a given name.
    ///
    /// \param name           The name of the scope
    /// \return               The found scope or \c NULL if no such scope exists.
    /// \else
    /// This operation is not supported.
    /// \endif
    virtual IScope* get_named_scope( const char* name) const = 0;

    /// Triggers a synchronous garbage collection run.
    ///
    /// The method sweeps through the entire database and removes all database elements which have
    /// been marked for removal and are no longer referenced. Note that it is not possible to remove
    /// database elements if there are open transactions in which such an element is still
    /// referenced.
    ///
    /// To mark an element for removal use \ifnot DICE_API #mi::neuraylib::ITransaction::remove().
    /// \else #mi::neuraylib::IDice_transaction::remove() or
    /// #mi::neuraylib::IDice_transaction::store_for_reference_counting(). \endif
    virtual void garbage_collection() = 0;
};

/*@}*/ // end group mi_neuray_database_access

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDATABASE_H
