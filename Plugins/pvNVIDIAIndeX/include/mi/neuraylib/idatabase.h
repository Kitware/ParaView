/***************************************************************************************************
 * Copyright 2023 NVIDIA Corporation. All rights reserved.
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

\if IRAY_API

\section mi_neuray_database_limitations Database limitations

The database does not support certain usage patterns. These patterns can not be rejected
programmatically, but need to be ensured by the user for proper operation.

\note This section mentions some DB internals, e.g. \c tags, which are not further explained here.


\subsection mi_neuray_database_reuse_of_names Re-use of names of DB elements eligible for garbage
collection

The database requires that database elements that are eligible for removal by the asynchronous
garbage collection must not be accessed under any circumstances. Failure to observe this limitation
can have fatal consequences, i.e., lead to crashes.

When is a DB element eligible for garbage collection? For simplicity, we assume that there is only
a single scope, and no parallel transactions. A DB element becomes eligible for garbage collection
if (a) the DB element has been flagged for removal via
#mi::neuraylib::ITransaction::remove() in this or a past transaction, and if (b) at the end of a
transaction the DB element is no longer referenced by other DB elements. From this point on, the
asynchronous garbage collection may remove the element at any point in time and the element must
not be accessed anymore under any circumstances.

Example (invalid re-use of names):
\code
mi::base::Handle<mi::neuraylib::IDatabase> database(
    neuray->get_api_component<mi::neuraylib::IDatabase>());

mi::base::Handle<mi::neuraylib::IScope> scope( database->get_global_scope());

mi::base::Handle<mi::neuraylib::ITransaction> transaction1(
    scope->create_transaction());
{
    mi::base::Handle<mi::neuraylib::IGroup> group(
        transaction1->create<mi::neuraylib::IGroup>( "Group"));
    // For simplicity, creation and flagging for removal in the very same transaction.
    transaction1->store( group.get(), "foo");
    transaction1->remove( "foo");
}
// The DB element "foo" is eligible for garbage collection after this call.
transaction1->commit();

mi::base::Handle<mi::neuraylib::ITransaction> transaction2(
    scope->create_transaction());
// Invalid re-use of the name "foo".
mi::base::Handle<const mi::neuraylib::IGroup> group(
    transaction2->access<mi::neuraylib::IGroup>( "foo"));
transaction2->commit();
\endcode

Complying with this limitation can be quite difficult since names might not be under the user's
control, e.g. due to the name occurring in a given scene, or if names are determined by Iray itself
as for many MDL elements.

<b>Strategy A: Ensure that the DB element has actually been removed before its name is re-used.
</b> This can be achieved by an enforced, synchronous run of the garbage collection between the end
of the transaction in which the DB becomes eligible for garbage collection and the next use of that
name (see #mi::neuraylib::IDatabase::garbage_collection()). Keep in mind though that other,
parallel transactions might delay the point in time at which a DB element becomes eligible for
garbage collection. While this approach is usually rather simple to implement, it causes a delay
due to ending the current and starting a new transaction, and the synchronous garbage collection
run itself. In addition it might not have to intended effect due to parallel transactions.

<b>Strategy B: Avoid that the DB element becomes eligible for garbage collection.</b> An obvious
approach not to flag DB elements for removal at all is usually not an option. But it is possible to
delay the eligibility for garbage collection to a later point in time when strategy A can be easily
implemented: Create an attribute container with a user-defined attribute of type \c "Ref[]" which
references all DB elements flagged for removal. At a favorable time for synchronous garbage
collection, flag this attribute container for removal, and use strategy A. Care needs to be taken
that the attribute container is not edited from parallel transactions (see
#mi::neuraylib::ITransaction for the semantics of edit operations in parallel transactions). Note
that this strategy just delays the problem, but does not solve it by itself.

Most often these invalid re-uses of names occur when one scene is unloaded in order to load another
scene. These situations are especially susceptible to timings that cause the fatal consequences
like crashes. While not a full strategy in itself, there is simple way to avoid the invalid re-use
in these situations:

<b>Strategy C: Use a different scope for each scene.</b> The recommendation is not to use the
global scope at all, but different child scopes for each scene. The different scope means that
names from other scopes are not visible for the current scope, and, therefore, do not count as
re-use of that name. Besides, using separate scopes has also the benefit that one can easily get
rid of a particular scene without the risk of leaving some remnants behind due to missing removal
flags.




\subsection mi_neuray_database_scopes Identical names due to different scopes

See #mi::neuraylib::IScope for general documentation about scopes.

Be careful when \em storing DB elements of the same name in different scopes. If the scopes are not
in a parent-child relation, then the following limitation does not apply since no transaction will
ever see both elements. But if the scopes are in a parent-child relation, then it is required that
the store operation happens first in the parent scope and that this element is visible in the child
scope before the store operation in that child scope occurs (in the same transaction, or with
different transactions where the first one is committed before the second one is started).
Otherwise, this results in different DB elements of the same name (and not just different versions
of the same DB elements as it would happen when the correct order is observed).

Example (creation in wrong order):
\code
mi::base::Handle<mi::neuraylib::IDatabase> database(
    neuray->get_api_component<mi::neuraylib::IDatabase>());

mi::base::Handle<mi::neuraylib::IScope> parent_scope( database->get_global_scope());

mi::base::Handle<mi::neuraylib::IScope> child_scope(
    database->create_scope( parent_scope.get()));

// DB element with name "foo" is created first in the child scope ...
mi::base::Handle<mi::neuraylib::ITransaction> child_transaction(
    child_scope->create_transaction());
{
    mi::base::Handle<mi::neuraylib::IGroup> group(
        child_transaction->create<mi::neuraylib::IGroup>( "Group"));
    child_transaction->store( group.get(), "foo");
}
child_transaction->commit();

mi::base::Handle<mi::neuraylib::ITransaction> parent_transaction(
    parent_scope->create_transaction());
{
    mi::base::Handle<mi::neuraylib::IGroup> group(
        parent_transaction->create<mi::neuraylib::IGroup>( "Group"));
    // ... and is not visible here.
    parent_transaction->store( group.get(), "foo");
}
parent_transaction->commit();

// The admin HTTP server shows now that the name "foo" maps to two different tags. Both tags
// are visible from the child scope via tag references (not part of this example), but only
// one of them via name.
\endcode

While such a situation is not necessarily a problem for the database itself, it leads to unexpected
behavior on the user side, as accesses might return different instances, depending on the details
of the access method.

This limitation does not apply if there is already a DB element with name accessible at the time of
the store operation, i.e., the method does not create a new DB element, but essentially
overwrites/edits an existing one.


\subsection mi_neuray_database_transactions Identical names due to parallel transactions

See #mi::neuraylib::ITransaction for general documentation about transactions.

The database does not support the \em storing of DB elements in parallel transactions, unless these
DB elements have different names or the scopes are different and are not in a parent-child
relation to each other. Failure to observe this limitation results in different DB elements of the
same name (and not just different versions of the same DB elements as it would happen with
serialized transactions).

Example (wrong creation in parallel transactions):
\code
mi::base::Handle<mi::neuraylib::IDatabase> database(
     neuray->get_api_component<mi::neuraylib::IDatabase>());

mi::base::Handle<mi::neuraylib::IScope> scope( database->get_global_scope());

mi::base::Handle<mi::neuraylib::ITransaction> transaction1(
    scope->create_transaction());
mi::base::Handle<mi::neuraylib::ITransaction> transaction2(
    scope->create_transaction());

{
    mi::base::Handle<mi::neuraylib::IGroup> group(
        transaction1->create<mi::neuraylib::IGroup>( "Group"));
    transaction1->store( group.get(), "foo");
}

{
    mi::base::Handle<mi::neuraylib::IGroup> group(
        transaction2->create<mi::neuraylib::IGroup>( "Group"));
    transaction2->store( group.get(), "foo");
}

transaction1->commit();
transaction2->commit();

// The admin HTTP server shows now that the name "foo" maps to two different tags. Both tags are
// visible via tag references from the global scope (not part of this example), but only one of
// them via name.
\endcode

While such a situation is not necessarily a problem for the database itself, it leads to unexpected
behavior on the user side, as accesses might return different instances, depending on the details
of the access method.

This limitation does not apply if there is already a DB element with name accessible at the time of
the store operations, i.e., the method does not create new DB elements, but essentially
overwrites/edits an existing one.

Note that \em editing (as opposed to storing) the very same DB element in parallel transactions is
supported by the database, but it is discouraged, since the semantics might not be as desired (see
#mi::neuraylib::ITransaction).


\subsection mi_neuray_database_references References to elements in more private scopes

See #mi::neuraylib::IScope for general documentation about scopes.

Be careful when creating references to DB elements that exist in a different scope than the
referencing element. References to elements in parent scopes (or the the same scope) are perfectly
fine. But you must not create references to DB elements that exist only in a more private scope.
Typically, this happens when using #mi::neuraylib::ITransaction::store() with an (explicit) wrong
privacy level.

Example (invalid reference to element in more private scope):
\code
mi::base::Handle<mi::neuraylib::IDatabase> database(
    neuray->get_api_component<mi::neuraylib::IDatabase>());

mi::base::Handle<mi::neuraylib::IScope> parent_scope( database->get_global_scope());

mi::base::Handle<mi::neuraylib::IScope> child_scope(
    database->create_scope( parent_scope.get()));

mi::base::Handle<mi::neuraylib::ITransaction> child_transaction(
    child_scope->create_transaction());
{
    mi::base::Handle<mi::neuraylib::IGroup> bar(
        child_transaction->create<mi::neuraylib::IGroup>( "Group"));
    check_success( 0 == child_transaction->store( bar.get(), "bar"));
    mi::base::Handle<mi::neuraylib::IGroup> foo(
        child_transaction->create<mi::neuraylib::IGroup>( "Group"));
    check_success( 0 == foo->attach( "bar"));
    // Triggers an error message since "foo" is to be stored in the parent scope (due to explicit
    // privacy level 0), but references "bar" in the child scope.
    check_success( 0 == child_transaction->store( foo.get(), "foo", 0));
}
child_transaction->commit();

mi::base::Handle<mi::neuraylib::ITransaction> parent_transaction(
    parent_scope->create_transaction());
{
    mi::base::Handle<const mi::neuraylib::IGroup> foo(
        parent_transaction->access<mi::neuraylib::IGroup>( "foo"));
    // Triggers a fatal message about an invalid tag access.
    const char name = foo->get_element( 0);
}
parent_transaction->commit();
\endcode

A reference to an element in a more private scope triggers an error message when the referencing
element is stored in the DB, but does not prevent the operation from being completed nor is it
signaled via a return code. As soon as the incorrect reference is used, this triggers a fatal error
message (and the process aborts if it returns from the logger callback). Even if the incorrect
reference is never used, its existence hints at a conceptual error in the way the application uses
scopes.

\endif

*/

/// This interface is used to interact with the distributed database.
///
/// \if MDL_SDK_API
/// \note The MDL SDK currently supports only \em one scope, the global scope.
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
    /// database elements, including handles to the scope itself. Even when all these conditions
    /// are met, scope removal might actually happen at a later point in time, depending on the
    /// timing of past and current transactions, even in unrelated scopes.
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

    /// Priorities for synchronous garbage collection runs.
    enum Garbage_collection_priority {

        /// Low priority for synchronous garbage collection runs. Use this priority if the
        /// performance of other concurrent DB operations is more important than a fast synchronous
        /// garbage collection.
        PRIORITY_LOW = 0,

        /// Medium priority for synchronous garbage collection runs. This priority attempts to
        /// maintain a balance between the synchronous garbage collection and other concurrent DB
        /// operations.
        PRIORITY_MEDIUM = 1,

        /// High priority for synchronous garbage collection runs. Other concurrent DB operations
        /// will experience a large performance drop. Therefore, this priority should not be used
        /// in multi-user settings.
        PRIORITY_HIGH = 2,

        //  Undocumented, for alignment only.
        PRIORITY_FORCE_32_BIT = 0xffffffffU
    };

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
    ///
    /// \param priority   The intended priority of the synchronous garbage collection run.
    ///                   \if MDL_SDK_API The MDL SDK does not support different priorities, and
    ///                   the synchronous garbage collection always runs at highest priority.
    ///                   \endif
    virtual void garbage_collection( Garbage_collection_priority priority = PRIORITY_MEDIUM) = 0;
};

mi_static_assert( sizeof( IDatabase::Garbage_collection_priority) == sizeof( Uint32));

/**@}*/ // end group mi_neuray_database_access

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IDATABASE_H
