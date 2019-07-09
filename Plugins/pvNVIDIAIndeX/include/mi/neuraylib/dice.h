/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief \NeurayApiName for implementing distributed parallel computing algorithms.

#ifndef MI_NEURAYLIB_DICE_H
#define MI_NEURAYLIB_DICE_H

#include <mi/base/handle.h>
#include <mi/base/lock.h>
#include <mi/base/interface_declare.h>
#include <mi/base/interface_implement.h>
#include <mi/neuraylib/iserializer.h>
#include <mi/neuraylib/iuser_class_factory.h>

/** \defgroup mi_neuray_dice DiCE Interfaces
    \ingroup mi_neuray

    \brief \NeurayApiName for implementing distributed parallel computing algorithms.

    The \neurayApiName represents a C++-based application programming interface that provides
    a framework for implementing parallel computing algorithms that are distributed in a cluster
    environment. The \neurayApiName provides an interface to the DiCE distributed database;
    it provides means for creating database elements and storing them in the distributed database
    and for accessing and changing elements of the distributed database.

    Furthermore, it provides mechanisms for implementing and distributing computing units
    called jobs. Jobs enable parallelizing computing tasks on a local machine or in a cluster
    environment.
*/

/** \addtogroup mi_neuray_dice
@{
*/

/// \NeurayApiName major product version number.
#define MI_NEURAYLIB_DICE_VERSION_MAJOR  3

/// \NeurayApiName minor product version number.
#define MI_NEURAYLIB_DICE_VERSION_MINOR  0

/// \NeurayApiName product version qualifier, which is something like \c "alpha1",
/// \c "beta2", or the empty string \c "", in which case the macro
/// \c MI_NEURAYLIB_DICE_VERSION_QUALIFIER_EMPTY is defined as well.
#define MI_NEURAYLIB_DICE_VERSION_QUALIFIER  ""

// This macro is defined if #MI_NEURAYLIB_DICE_VERSION_QUALIFIER is the empty string \c "".
#define MI_NEURAYLIB_DICE_VERSION_QUALIFIER_EMPTY

/// DiCE product version number in a string representation, such as \c "2016".
#define MI_NEURAYLIB_DICE_PRODUCT_VERSION_STRING  "2019"

/*@}*/ // end group mi_neuray_dice

namespace mi {

namespace neuraylib {

class IDistributed_cache;
class IElement;
class IExecution_listener;
class IFragmented_job;
class IGpu_description;
class IJob;
class IScope;
class ITransaction;
class IRDMA_buffer;
class IRDMA_context;

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface allows configuration of DiCE.
///
///For instance, the user-define database elements and jobs can be registered for serialization so
/// that hosts in the cluster can set up the distributed database.
class IDice_configuration : public
    mi::base::Interface_declare<0xfccbf7b8,0x30d1,0x4fbf,0xbd,0xc6,0x3b,0x96,0xe1,0xfb,0x40,0x6a>
{
public:
    /// Registers a serializable class with DiCE.
    ///
    /// Registering a class for serialization allows communicating class instances through the
    /// serialization mechanism. It enables, for instance, storing database elements
    /// and jobs in the distributed database or communicating job results
    /// (see #mi::neuraylib::IFragmented_job::execute_fragment_remote()).
    ///
    /// Registering a class for serialization can only be done before \neurayProductName has been
    /// started.
    ///
    /// \param class_id     The class ID of the class that shall be registered for serialization.
    /// \param factory      The class factory.
    /// \return             \c true if the class of was successfully registered
    ///                     for serialization, and \c false otherwise.
    virtual bool register_serializable_class(
        base::Uuid class_id,
        IUser_class_factory* factory) = 0;

    /// Registers a serializable class with DiCE.
    ///
    /// Registering a class for serialization allows communicating class instances through the
    /// serialization mechanism. It enables, for instance, storing database elements
    /// and jobs in the distributed database or communicating job results
    /// (see #mi::neuraylib::IFragmented_job::execute_fragment_remote()).
    ///
    /// Registering a class for serialization can only be done before \neurayProductName has been
    /// started.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It uses the default class factory #mi::neuraylib::User_class_factory
    /// specialized for T.
    ///
    /// \return             \c true if the class of type T was successfully registered
    ///                     for serialization, and \c false otherwise.
    template <class T>
    bool register_serializable_class()
    {
        mi::base::Handle<IUser_class_factory> factory( new User_class_factory<T>());
        return register_serializable_class( typename T::IID(), factory.get());
    }

    /// Enables or disables GPU detection.
    ///
    /// By default, GPU detection is enabled. GPU detection can be disabled in case it causes
    /// problems (hangs, crashes) or GPU usage is not desired at all.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \param value                \c true if GPUs are to be detected, \c false otherwise.
    /// \return                     0, in case of success, -1 in case of failure.
    virtual Sint32 set_gpu_enabled( bool value) = 0;

    /// Returns if GPU detection is enabled.
    ///
    /// \return                     The currently configured setting.
    virtual bool get_gpu_enabled() const = 0;

    /// Returns the RDMA context to be used for generating RDMA buffers
    ///
    /// \param host_id      The id of the host for which to acquire the context
    /// \return The RDMA context
    virtual IRDMA_context* get_rdma_context( Uint32 host_id) = 0;

    /// Returns the highest GPU ID.
    ///
    /// \see #get_gpu_description()
    ///
    /// \param host_id  An optional host ID. If larger than 0, the returned GPU description refers
    ///                 to a GPU installed on the machine with the given host ID, and to the local
    ///                 host otherwise.
    /// \return         The highest GPU ID installed on the specified host.
    virtual Uint32 get_highest_gpu_id( Uint32 host_id = 0) = 0;

    /// Returns the description of a GPU with a given ID.
    ///
    /// \see #get_highest_gpu_id()
    ///
    /// \param gpu_id   The GPU ID. Note that the lowest GPU ID is 1, not 0. The highest ID can be
    ///                 obtained from #get_highest_gpu_id(). Note that GPU IDs do not need to be
    ///                 consecutive.
    /// \param host_id  An optional host ID. If larger than 0, the returned GPU description refers
    ///                 to a GPU installed on the machine with the given host ID, and to the local
    ///                 host otherwise. See
    ///                 #mi::neuraylib::INetwork_configuration::register_host_callback() for
    ///                 details about host IDs of other machines in the cluster.
    /// \return         A description of the GPU with that ID, or \c NULL if \p gpu_id is not a
    ///                 valid GPU ID for the host \p host_id.
    virtual const IGpu_description* get_gpu_description( Uint32 gpu_id, Uint32 host_id = 0) = 0;
};

/*@}*/ // end group mi_neuray_configuration

/** \addtogroup mi_neuray_dice
@{
*/

/// Used to store a set of tags.
class ITag_set : public
    mi::base::Interface_declare<0xb8290f90,0x3692,0x4bb9,0x87,0x22,0x45,0x4a,0xd6,0xf1,0x1f,0xf5>
{
public:
    /// Returns the number of elements in the tag set.
    virtual Size get_length() const = 0;
    /// Returns the \p index -th element of the tag set.
    virtual Tag_struct get_tag( Size index) const = 0;
    /// Adds \p tag to the tag set.
    virtual void add_tag( Tag_struct tag) = 0;
};

/// The privacy level is an unsigned 8 bit integer. The privacy level is an integer in the range
/// from 0 to 255. The privacy level 0 is used to identify the global scope. Thus the higher the
/// privacy level is, the more private the corresponding scope.
typedef Uint8 Privacy_level;

/// Provides information about the context in which a job is executed.
class IJob_execution_context
{
public:
    /// Returns the thread ID.
    ///
    /// \return   The ID of thread the job is executed in.
    virtual Uint64 get_thread_id() const = 0;
};

/*@}*/ // end group mi_neuray_dice

/** \addtogroup mi_neuray_database_access
@{
*/

/// A transaction provides a consistent view on the database.
///
/// This view on the database is isolated from changes by other (parallel) transactions. Eventually,
/// each transaction must be either committed or aborted, i.e., all changes become either atomically
/// visible to transactions started afterwards, or not at all.
///
/// Transactions are associated with a scope of the database and can be created with
/// #mi::neuraylib::IScope::create_transaction<mi::neuraylib::IDice_transaction>().
///
/// \par Concurrent accesses to database elements within a transaction
/// Access to database elements is provided by #access() (read-only) and #edit() (for modification).
/// The interface pointers returned by these methods must be released when you are done, in
/// particular before the transaction is committed or aborted. Releasing the last interface pointer
/// obtained from #edit() makes the changes also visible to later #edit() calls for the same
/// database element.
/// \par
/// Note that it is possible to access the same database element concurrently in the same
/// transaction. Concurrently means that the interface pointer obtained from an earlier #access()
/// or #edit() call has not yet been released and the same database element is accessed once more
/// using #access() or #edit(). It is advisable to avoid such concurrent accesses since it can
/// easily lead to difficult to understand effects. The semantics are as follows:
/// <ul>
/// <li> multiple #access() calls: Since all obtained interface pointers are const there is no
///      way to modify the database elements.</li>
/// <li> #access() call after #edit() calls: This use case is not supported.</li>
/// <li> #edit() call after #access() calls: The changes done to the interface pointer returned from
///      #edit() are not observable through any interface pointer returned from the #access() calls.
///      </li>
/// <li> multiple #edit() calls: The changes done to the individual interface pointers are not
///      observable through the other interface pointers. The changes from the interface pointer
///      from the last #edit() call survive, independent of the order in which the pointers are
///      released.</li>
/// </ul>
///
/// \par Concurrent transactions
/// If the same database element is edited in multiple overlapping transactions, the changes from
/// the transaction created last survive, independent of the order in which the transactions are
/// committed. If needed, the lifetime of transactions can be serialized across hosts (see
/// #mi::neuraylib::IDatabase::lock() for details).
class IDice_transaction : public
    mi::base::Interface_declare<0x1885cbec,0x3cfc,0x4f9d,0xb2,0xe9,0xb2,0xbe,0xdc,0xc8,0x94,0x88>
{
public:
    /// Commits the transaction.
    ///
    /// Note that a commit() implicitly closes the transaction.
    /// A closed transaction does not allow any future operations and needs to be released.
    ///
    /// \return
    ///                     -  0: Success.
    ///                     - -1: Unspecified failure.
    ///                     - -3: The transaction is not open.
    virtual Sint32 commit() = 0;

    /// Aborts the transaction.
    ///
    /// Note that an abort() implicitly closes the transaction.
    /// A closed transaction does not allow any future operations and needs to be released.
    virtual void abort() = 0;

    /// Indicates whether the transaction is open.
    ///
    /// The ability to check whether the transaction is still open is particularly important for
    /// advised jobs, which shall be able to abort their operation if their results become
    /// irrelevant to the database in the meantime.
    ///
    /// \return   \c true if the transaction is still open, or \c false if the transaction is
    ///           closed, i.e., it has been committed or aborted.
    virtual bool is_open() = 0;

    /// Symbolic privacy level for the privacy level of the scope of this transaction.
    ///
    /// This symbolic constant can be passed to #store(), #store_for_reference_counting(), and
    /// #localize() to indicate the privacy level of the scope of this transaction. It has the same
    /// affect as passing the result of #mi::neuraylib::IScope::get_privacy_level(), but is more
    /// convenient.
    static const mi::Uint8 LOCAL_SCOPE = 255;

    /// Stores a new database element in the database.
    ///
    /// After a successful store operation the passed interface pointer must no longer be used,
    /// except for releasing it. This is due to the fact that after a #store() the database
    /// retains ownership of the stored data. You can obtain the stored version from the database
    /// using the #access() or #edit() methods.
    ///
    /// \param element         The database element to be stored in the database.
    /// \param tag             An optional tag used to identify the database element in the
    ///                        database. The tag must be #NULL_TAG (the default) or must have been
    ///                        obtained from previous calls to #store(),
    ///                        #store_for_reference_counting(), #name_to_tag(), or #reserve_tag().
    ///                        An existing database element/job associated with this tag will be
    ///                        replaced. See also the documentation of the return value and the note
    ///                        below.
    /// \param name            An optional name used to identify the database element in the
    ///                        database. An existing database element/job associated with this name
    ///                        will be replaced.
    /// \param privacy_level   The privacy level under which to store \p db_element (in the range
    ///                        from 0 to the privacy level of the scope of this transaction). In
    ///                        addition, the constant #LOCAL_SCOPE can be used as a shortcut to
    ///                        indicate the privacy level of the scope of this transaction without
    ///                        supplying the actual value itself.
    /// \return                The tag that identifies the stored database element, or #NULL_TAG in
    ///                        case of errors. Possible reasons for errors are:
    ///                        - The class ID of the element has not been registered (see
    ///                         #mi::neuraylib::IDice_configuration::register_serializable_class()).
    ///                        - The privacy level is invalid.
    ///                        - The element is already stored in the database.
    ///                        .
    ///                        Note that the returned tag might differ from \p tag or #name_to_tag()
    ///                        applied to \p name if there is already a database element/job
    ///                        associated with that tag, that database element/job is to be removed,
    ///                        and no other database element/job is referencing it anymore.
    ///
    /// \note The parameter \p tag is ignored if \p name is given and there is a database
    ///       element/job with that name. The tag of that database element/job is used instead of
    ///       \p tag.
    virtual Tag_struct store(
        IElement* element,
        Tag_struct tag = NULL_TAG,
        const char* name = 0,
        Privacy_level privacy_level = LOCAL_SCOPE) = 0;

    /// Stores a new database job in the database.
    ///
    /// After a successful store operation the passed interface pointer must no longer be used,
    /// except for releasing it. This is due to the fact that after a #store() the database
    /// retains ownership of the stored data.
    ///
    /// \param job             The database job to be stored in the database.
    /// \param tag             An optional tag used to identify the database job in the database.
    ///                        The tag must be #NULL_TAG (the default) or must have been obtained
    ///                        from previous calls to #store(), #store_for_reference_counting(),
    ///                        #name_to_tag(), or #reserve_tag(). An existing database element/job
    ///                        associated with this tag will be replaced. See also the documentation
    ///                        of the return value and the note below.
    /// \param name            An optional name used to identify the database job in the database.
    ///                        An existing database element/job associated with this name will be
    ///                        replaced.
    /// \param privacy_level   The privacy level under which to store \p job (in the range from 0 to
    ///                        the privacy level of the scope of this transaction). In addition, the
    ///                        constant #LOCAL_SCOPE can be used as a shortcut to indicate the
    ///                        privacy level of the scope of this transaction without supplying the
    ///                        actual value itself.
    /// \return                The tag that identifies the stored database job, or #NULL_TAG in case
    ///                        of errors. Possible reasons for errors are:
    ///                        - The class ID of the job has not been registered (see
    ///                         #mi::neuraylib::IDice_configuration::register_serializable_class()).
    ///                        - The privacy level is invalid.
    ///                        - The job is already stored in the database.
    ///                        .
    ///                        Note that the returned tag might differ from \p tag or #name_to_tag()
    ///                        applied to \p name if there is already a database element/job
    ///                        associated with that tag, that database element/job is to be removed,
    ///                        and no other database element/job is referencing it anymore.
    ///
    /// \note The parameter \p tag is ignored if \p name is given and there is a database
    ///       element/job with that name. The tag of that database element/job is used instead of
    ///       \p tag.
    virtual Tag_struct store(
        IJob* job,
        Tag_struct tag = NULL_TAG,
        const char* name = 0,
        Privacy_level privacy_level = LOCAL_SCOPE) = 0;

    /// Stores a new database element in the database (for reference counting).
    ///
    /// This method is similar to #store() except that the database element will only be kept in the
    /// database as long as other elements in the database exist that list it in their set of
    /// references returned by the #mi::neuraylib::IElement::get_references() method. The same
    /// effect can be achieved by calling #store() followed by a #remove() call on the returned tag.
    ///
    /// \see #store() for details
    virtual Tag_struct store_for_reference_counting(
        IElement* element,
        Tag_struct tag = NULL_TAG,
        const char* name = 0,
        Privacy_level privacy_level = LOCAL_SCOPE) = 0;

    /// Stores a new database job in the database (for reference counting).
    ///
    /// This method is similar to #store() except that the database job will only be kept in the
    /// database as long as other elements in the database exist that list it in their set of
    /// references returned by the #mi::neuraylib::IElement::get_references() method. The same
    /// effect can be achieved by calling #store() followed by a #remove() call on the returned tag.
    ///
    /// \see #store() for details
    virtual Tag_struct store_for_reference_counting(
        IJob* job,
        Tag_struct tag = NULL_TAG,
        const char* name = 0,
        Privacy_level privacy_level = LOCAL_SCOPE) = 0;

    /// Retrieves an element from the database.
    ///
    /// The database searches for the most recent version of the named database element visible
    /// for the current transaction. That version will be returned.
    ///
    /// If the tag identifies a database job the database ensures that the job has been executed and
    /// returns the result of the job.
    ///
    /// \param tag      The tag that identifies the requested database element.
    /// \return         The requested element in the database, or \c NULL if the transaction is
    ///                 already closed.
    virtual const base::IInterface* access( Tag_struct tag) = 0;

    /// Retrieves an element from the database.
    ///
    /// The database searches for the most recent version of the named database element visible
    /// for the current transaction. That version will be returned.
    ///
    /// If the tag identifies a database job the database ensures that the job has been executed and
    /// returns the result of the job.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const base::Uuid&)
    /// on the returned pointer, since the return type already is a const handle of the type \p T
    /// specified as template parameter.
    ///
    /// \param tag      The tag that identifies the requested database element.
    /// \tparam T       The interface type of the element to retrieve.
    /// \return         The requested element in the database, or \c NULL if the transaction is
    ///                 already closed, or the element is not of type \c T.
    template<class T>
    const T* access( Tag_struct tag)
    {
        const base::IInterface* ptr_iinterface = access( tag);
        if ( !ptr_iinterface)
            return 0;
        const T* ptr_T = static_cast<const T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Retrieves an element from the database and returns it ready for editing.
    ///
    /// The database searches for the most recent version of the named database element visible
    /// for the current transaction. The database creates a local copy which is used in this
    /// transaction (and replaces the old version when the transaction is committed).
    ///
    /// Note that database jobs including their resulting database elements cannot be edited.
    ///
    /// \param tag      The tag that identifies the requested database element.
    /// \return         The requested element in the database, or \c NULL if the transaction is
    ///                 already closed.
    virtual base::IInterface* edit( Tag_struct tag) = 0;

    /// Retrieves an element from the database and returns it ready for editing.
    ///
    /// The database searches for the most recent version of the named database element visible
    /// for the current transaction. The database creates a local copy which is used in this
    /// transaction (and replaces the old version when the transaction is committed).
    ///
    /// Note that database jobs including their resulting database elements cannot be edited.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const base::Uuid&)
    /// on the returned pointer, since the return type already is a const handle of the type \p T
    /// specified as template parameter.
    ///
    /// \param tag      The tag that identifies the requested database element.
    /// \tparam T       The interface type of the element to retrieve.
    /// \return         The requested element in the database, or \c NULL if the transaction is
    ///                 already closed, or the element is not of type \c T.
    template<class T>
    T* edit( Tag_struct tag)
    {
        base::IInterface* ptr_iinterface = edit( tag);
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Localizes a tag to the given privacy level.
    ///
    /// Creates a copy of the identified database element and stores that copy in the given privacy
    /// level.
    ///
    /// \param tag             The tag that identifies the database element to be localized.
    /// \param privacy_level   The desired privacy level of the copy (in the range from 0 to the
    ///                        privacy level of the scope of this transaction). In addition, the
    ///                        constant #LOCAL_SCOPE can be used as a shortcut to indicate the
    ///                        privacy level of the scope of this transaction without supplying the
    ///                        actual value itself.
    /// \return
    ///                        -  0: Success.
    ///                        - -2: Invalid parameters (\p tag is invalid).
    ///                        - -5: Invalid privacy level.
    virtual Sint32 localize( Tag_struct tag, Privacy_level privacy_level) = 0;

    /// Removes a \p tag from the database.
    ///
    /// Note that the element continues to be stored in the database as long as it is referenced by
    /// other elements (or jobs). If it is no longer referenced, it will be lazily removed by the
    /// garbage collection of the database. There is no guarantee when this will happen. This
    /// implies that a #remove() call might actually remove an element that was stored later under
    /// the same tag.
    ///
    /// \param tag            The tag that identifies the database element to be removed.
    /// \param only_localized If \c true, the element is only removed if it exists in the scope
    ///                       of the transaction; parent scopes are not considered.
    /// \return
    ///                       -  0: Success.
    ///                       - -1: There is no database element with tag \p tag visible in this
    ///                             transaction (\p only_localize is \c false) or there is no
    ///                             database element with tag \p tag in the scope of this
    ///                             transaction (\p only_localized is \c true).
    ///                       - -2: Invalid parameters (\p tag is invalid).
    ///                       - -3: The transaction is not open.
    virtual Sint32 remove( Tag_struct tag, bool only_localized = false) = 0;

    /// Returns the time stamp describing the current "time".
    ///
    /// \note The time stamp is not related to time in the classical meaning. It rather relates to
    ///       the current transaction and the number of database changes since the start of the
    ///       transaction.
    ///
    /// \note The time stamp is only meaningful for the current \neurayAdjectiveName instance. It
    ///       should not be put into external storage and re-used for different or later
    ///       \neurayAdjectiveName instances.
    ///
    /// \note The return value of this method is only valid until the next call of this method
    ///       (or one of its overloads) on this instance.
    ///
    /// \see has_changed_since_time_stamp(), #get_time_stamp(mi::neuraylib::Tag_struct)const
    virtual const char* get_time_stamp() const = 0;

    /// Returns the time stamp of the last change of a database element.
    ///
    /// \note The time stamp is not related to time in the classical meaning. It rather relates to
    ///       the transaction and the number of database changes since the start of the transaction
    ///       when the database element was changed last.
    ///
    /// \note The time stamp is only meaningful for the current \neurayAdjectiveName instance. It
    ///       should not be put into external storage and re-used for different or later
    ///       \neurayAdjectiveName instances.
    ///
    /// \note The return value of this method is only valid until the next call of this method
    ///       (or one of its overloads) on this instance.
    ///
    /// \see has_changed_since_time_stamp(), #get_time_stamp()
    virtual const char* get_time_stamp( Tag_struct tag) const = 0;

    /// Checks whether an element has been stored or changed in the database since a given time
    /// stamp.
    ///
    /// \note \p time_stamp should not stem from another concurrent transaction. Such changes will
    ///       never be visible in this transaction, but the method might still return \p true
    ///       depending on the start order of the two transactions.
    ///
    /// \note In case of multiple overlapping transactions the returned answer may not list
    ///       all changes due to the isolation of the transactions. If accurate results are
    ///       required, transactions changing elements should be committed before transactions
    ///       querying the journal for such changes are started.
    ///
    /// \see #get_time_stamp(), #get_time_stamp(mi::neuraylib::Tag_struct)const
    ///
    /// \param tag         The tag that identifies the database element.
    /// \param time_stamp  The time stamp obtained from #get_time_stamp() or
    ///                    #get_time_stamp(mi::neuraylib::Tag_struct)const.
    /// \return            \c true if the element has been stored or changed since the time stamp
    ///                    (or if \p tag or \p time_stamp is invalid), \c false otherwise.
    virtual bool has_changed_since_time_stamp( Tag_struct tag, const char* time_stamp) const = 0;

    /// Returns all elements (of type #mi::neuraylib::IElement) that have been stored or changed in
    /// the database since a given time stamp.
    ///
    /// \note \p time_stamp should not stem from another concurrent transaction. Such changes will
    ///       never be visible in this transaction, but the method might still return \p true
    ///       depending on the start order of the two transactions.
    ///
    /// \see #get_time_stamp(), #get_time_stamp(mi::neuraylib::Tag_struct)const
    ///
    /// \param time_stamp  The time stamp obtained from #get_time_stamp() or
    ///                    #get_time_stamp(mi::neuraylib::Tag_struct)const.
    /// \return            The set of elements that have been changed since the time stamp, or
    ///                    \c NULL in case of failure, i.e., the time stamp is invalid or there
    ///                    have been too many changes since the given time stamp.
    virtual const ITag_set* get_changed_elements_since_time_stamp(
        const char* time_stamp) const = 0;

    /// Returns the ID of this transaction.
    ///
    /// The transaction ID is of most use when debugging an application as the value returned allows
    /// one to correlate log messages and admin HTTP server output with the API actions.
    ///
    /// \return            The ID of the transaction.
    virtual const char* get_id() const = 0;

    /// Returns the scope of this transaction.
    virtual IScope* get_scope() const = 0;

    /// Returns the privacy level of the element with a given tag.
    ///
    /// \param tag         The tag of the element.
    /// \return
    ///                      - >= 0: Success. The privacy level of the element (in the range 0-255).
    ///                      -   -3: The transaction is not open.
    virtual Sint32 get_privacy_level( Tag_struct tag) const = 0;

    /// Advises the database that a given tag is required soon.
    ///
    /// If \p tag identifies a database job its execution will be triggered. If \p tag identifies a
    /// plain database element then fetching the data from other hosts or reading from disk will be
    /// triggered (if necessary).
    ///
    /// In both cases the method does not block, but returns immediately, only later accesses via
    /// #access() or #edit() might block. Thus, this method can be used, for instance, to trigger
    /// parallel execution of jobs.
    virtual void advise( Tag_struct tag) = 0;

    /// Reserves a tag.
    ///
    /// When storing a database element or database job a new tag is automatically assigned and
    /// returned. Alternatively, one can reserve a tag upfront and pass it later explicitly to the
    /// various #store() method. For example, this workflow can be used for cyclic references among
    /// database elements.
    ///
    /// \return     The reserved tag.
    virtual Tag_struct reserve_tag() = 0;

    /// Returns the name associated with a tag in the database.
    ///
    /// \note       A tag can be associated with different names in different scopes.
    ///
    /// \return     The name associated with \p tag, or \c NULL if there is no name associated with
    ///             \p tag.
    virtual const char* tag_to_name( Tag_struct tag) = 0;

    /// Returns the tag associated with a name in the database.
    ///
    /// \note       A name can be associated with different tags in different scopes.
    ///
    /// \return     The tag (or one tag) associated with \p name, or #NULL_TAG if there is no tag
    ///             associated with \p name.
    virtual Tag_struct name_to_tag( const char* name) = 0;

    /// Executes a fragmented job synchronously.
    ///
    /// The fragmented job is conceptually split into \p count fragments. Each fragment is executed
    /// exactly once in the cluster environment, either locally or remotely. The method returns
    /// when all fragments have been executed.
    ///
    /// The fragments are executed independently by any number of threads and possibly on any number
    /// of hosts in parallel (see #mi::neuraylib::IFragmented_job::get_scheduling_mode()). The
    /// number of fragments does not imply any specific parallel execution scheme. In principle, the
    /// DiCE job scheduling system may execute all fragments in parallel at the same time or all in
    /// serial order one after the other or anything in between. It is only guaranteed that each
    /// fragment is executed exactly once.
    ///
    /// \param job   The fragmented job to be executed. All fragments executed on the initiating
    ///              host operate on the given instance of the fragmented job. The fragments
    ///              executed on different hosts in the cluster operate on a replicas of the given
    ///              instance. The replicas result from serializing the instance given here and
    ///              later restoring an instance of the same class by deserializing it on the remote
    ///              host. That is, changes to the fragmented job instance applied during job
    ///              execution never become visible during remote execution and, thus, should be
    ///              avoided, unless these changes are intentional (results) or not relevant (host
    ///              local caches).
    /// \param count The number of fragments into which the job is split. This value needs to be
    ///              greater than zero unless the scheduling mode is
    ///              #mi::neuraylib::IFragmented_job::ONCE_PER_HOST. In that case a value of zero
    ///              means "as many fragments as remote hosts in the cluster".
    /// \return
    ///              -  0: Success.
    ///              - -1: Invalid parameters (\p job is \c NULL or \c count is zero but the
    ///                    scheduling mode is not #mi::neuraylib::IFragmented_job::ONCE_PER_HOST).
    ///              - -3: Invalid job priority (negative value).
    ///
    /// \see #mi::neuraylib::IScheduler::execute_fragmented() for a variant without transaction
    virtual Sint32 execute_fragmented( IFragmented_job* job, Size count) = 0;

    /// Executes a fragmented job asynchronously.
    ///
    /// This method is similar to #execute_fragmented() except that the execution happens
    /// asynchronously. A callback is used to signal the completion of the job.
    ///
    /// \note Currently, this method is restricted to local jobs (see
    ///       #mi::neuraylib::IFragmented_job::get_scheduling_mode()).
    ///
    /// \param job      The fragmented job to be executed.
    /// \param count    The number of fragments into which the job is split. This value needs to be
    ///                 greater than zero unless the scheduling mode is
    ///                 #mi::neuraylib::IFragmented_job::ONCE_PER_HOST. In that case a value of zero
    ///                 means "as many fragments as remote hosts in the cluster".
    /// \param listener The \c job_finished() method of this callback is called when the job has
    ///                 been completed (might be \c NULL).
    /// \return
    ///                 -  0: Success.
    ///                 - -1: Invalid parameters (\p job is \c NULL or \c count is zero).
    ///                 - -2: Invalid scheduling mode (asynchronous execution is restricted to local
    ///                       jobs).
    ///                 - -3: Invalid job priority (negative value).
    ///
    /// \see #mi::neuraylib::IScheduler::execute_fragmented() for a variant without transaction
    virtual Sint32 execute_fragmented_async(
        IFragmented_job* job, Size count, IExecution_listener* listener) = 0;

    /// Invalidates the results of a database job.
    ///
    /// If the result of a database is invalidated the next access to that tag will trigger
    /// re-execution of the job. Re-execution of a database job might become necessary when database
    /// elements used as input for the job have changed.
    ///
    /// \note Non-shared database jobs (see #mi::neuraylib::IJob::is_shared()) are automatically
    ///       invalidated for each transaction, i.e., their results are not shared beyond multiple
    ///       transactions.
    virtual void invalidate_job_results( Tag_struct tag) = 0;
};

/*@}*/ // end group mi_neuray_database_access

/** \addtogroup mi_neuray_dice
@{
*/

/// The scheduler allows to execute fragmented jobs.
///
/// See #mi::neuraylib::IScheduling_configuration for related configuration options.
class IScheduler : public
    mi::base::Interface_declare<0xd0754018,0xca8f,0x48e0,0x8a,0x37,0xeb,0x83,0x31,0xb7,0x85,0x20>
{
public:
    /// Executes a fragmented job synchronously (without transaction).
    ///
    /// This method is similar to
    /// #mi::neuraylib::IDice_transaction::execute_fragmented(). The difference is that here
    /// no transaction is involved, i.e., all methods of #mi::neuraylib::IFragmented_job with a
    /// transaction argument will pass \c NULL for that argument.
    ///
    /// \note Currently, this method is restricted to local jobs (see
    ///       #mi::neuraylib::IFragmented_job::get_scheduling_mode()).
    virtual Sint32 execute_fragmented(
        IFragmented_job* job, Size count) = 0;

    /// Executes a fragmented job asynchronously (without transaction).
    ///
    /// This method is similar to
    /// #mi::neuraylib::IDice_transaction::execute_fragmented_async(). The difference is that here
    /// no transaction is involved, i.e., all methods of #mi::neuraylib::IFragmented_job with a
    /// transaction argument will pass \c NULL for that argument.
    ///
    /// \note Currently, this method is restricted to local jobs (see
    ///       #mi::neuraylib::IFragmented_job::get_scheduling_mode()).
    virtual Sint32 execute_fragmented_async(
        IFragmented_job* job, Size count, IExecution_listener* listener) = 0;

    /// Notifies the scheduler that this thread suspends jobs execution (because it is going
    /// to wait for some event).
    ///
    /// The scheduler does not do anything except that it decreases the current load values
    /// accordingly. The method returns immediately if not being called from a worker thread.
    ///
    /// Usage of this method is mandatory if a child job is executed asynchronously from within
    /// a parent job, and the parent job waits for the child job to complete. Usage of this method
    /// is strongly recommended (but not mandatory) if the parent jobs waits for some event
    /// unrelated to child jobs. Usage of this method is not necessary for synchronous execution
    /// of child jobs.
    ///
    /// Failure to use this method when mandatory might lead to dead locks. Failure to use this
    /// method when recommended might lead to reduced performance.
    ///
    /// Example:
    /// \code
    /// mi::base::Condition condition;
    /// ...
    /// scheduler->suspend_current_job();
    /// condition.wait();
    /// scheduler->resume_current_job();
    /// \endcode
    ///
    /// This method needs to be used in conjunction with #resume_current_job().
    virtual void suspend_current_job() = 0;

    /// Notifies the scheduler that this worker thread resumes job execution (because it waited
    /// for some event that now happened).
    ///
    /// The scheduler does not do anything except that it increases the current load values
    /// accordingly. This method blocks if the current load values and limits do not permit instant
    /// resuming of the job. The method returns immediately if not being called from a worker
    /// thread.
    ///
    /// \see #suspend_current_job() for further details
   virtual void resume_current_job() = 0;

    /// Notifies the thread pool that this worker thread is willing to give up its resources for a
    /// while in favor of other jobs of higher priority.
    ///
    /// Yielding is similar to calling #suspend_current_job() followed by #resume_current_job(), but
    /// it takes job priorities into account and is more efficient if there is no job of higher
    /// priority in the job queue.
    virtual void yield() = 0;
};

/// This interface represents the abstract %base class for all database elements.
///
/// Each database element must be derived from this interface and the pure virtual methods must be
/// implemented.
///
/// \see The mixin #mi::neuraylib::Element can be useful as a starting point for your own
///      implementation of this abstract interface.
class IElement : public
    mi::base::Interface_declare<0x3f377a88,0xb7aa,0x487b,0x98,0xe6,0x70,0x53,0xc2,0xfa,0xb9,0xc9,
                                neuraylib::ISerializable>
{
public:
    //  Sets the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual bool set_pointer( const base::IInterface* pointer) = 0;

    //  Returns the embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not use this method.
    virtual const base::IInterface* get_pointer() const = 0;

    /// Creates a copy of the database element.
    ///
    /// Duplicating a database element is used by the database when it needs to create a new version
    /// of the given element, e.g., when someone edits an existing element. This member function
    /// must create and return a full copy of the element.
    ///
    /// \return A newly allocated instance of the database element. The pointer represents a
    ///         complete replica of the database element on which the member function was called.
    virtual IElement* copy() const = 0;

    /// Returns a human readable identifier for the class this database element belongs to.
    ///
    /// This name is \em not required for proper operation, but can be useful for debugging. For
    /// example, the name is used to display the class name in the tag table of the HTTP
    /// administration console.
    ///
    /// \return         The class name. The string has to be valid at all times while instances of
    ///                 this class are stored in the database.
    virtual const char* get_class_name() const = 0;

    /// Returns the set of referenced tags for reference counting.
    ///
    /// The tag set allows reference counting to control the lifetime of database elements. If a
    /// tag is contained in this set it is guaranteed that the database element identified by that
    /// tag is alive as long as the current version of this database elements exists.
    ///
    /// \param[out] result   The set of tags referenced by the database element. The passed-in
    ///                      tag set is empty.
    virtual void get_references( ITag_set* result) const = 0;

    /// Returns the size of the database element in bytes.
    ///
    /// The value includes all memory that directly or indirectly belongs exclusively to this
    /// database element. For simple classes, \code sizeof(*this) \endcode works.
    ///
    /// A correct implementation of this method is required if memory limits are configured.
    ///
    /// \see #mi::neuraylib::IDatabase_configuration::set_memory_limits(),
    ///      #mi::neuraylib::IDatabase_configuration::get_memory_limits()
    virtual Size get_size() const = 0;

    /// Indicates how the database element should be distributed in the cluster.
    ///
    /// If the method returns \c true the stored or edited database element is distributed to all
    /// nodes in the cluster. If the method returns \c false the database element is only
    /// distributed to the owners, i.e., a subset of the nodes as required to fulfill the configured
    /// redundancy. (Of course, it is later also distributed to those nodes that explicitly access
    /// the database element.)
    ///
    /// \see #mi::neuraylib::INetwork_configuration::set_redundancy_level().
    virtual bool get_send_to_all_nodes() const = 0;

    /// Indicates how the database element is handled when received via the network without being
    /// explicitly requested.
    ///
    /// If the method returns \c false, a host that receives such a database element will make no
    /// difference between an explicit request or an accidental reception, e.g., because the
    /// element was multicasted. If the method returns \c true, the database will be more
    /// conservative w.r.t. memory usage for elements that it received by accident. The element will
    /// be discarded if the host is not required to keep it for redundancy reasons or if the element
    /// represents the result of a database job. Next, if the disk cache is operational, the element
    /// will be offloaded to disk. Finally, in all other cases, the element will be kept in memory,
    /// as if the element was explicitly requested.
    ///
    /// \see #mi::neuraylib::IDatabase_configuration::set_disk_swapping(),
    ///      #mi::neuraylib::IDatabase_configuration::get_disk_swapping()
    virtual bool get_offload_to_disk() const = 0;
};

/// This interface represents the %base class for all database jobs.
///
/// Each database job must be derived from this interface.
///
/// Database jobs generate database elements that result from on-demand computing. Similar to
/// database elements, database jobs are identified by a tag and are stored in the database. In
/// contrast to database elements, database jobs get executed when accessing the tag by
/// #mi::neuraylib::IDice_transaction::access(), and then provide a database element representing
/// the result of the job execution, that is, a job can be considered a kind of procedural database
/// element. Subsequent accesses to the tag will not necessarily re-execute the job but the database
/// might immediately return the previously computed job result (see #is_shared, #is_local_only).
///
/// Not every procedurally generated data must be created by means of database jobs. Sometimes, some
/// member function of the database element represents an even better place to compute the required
/// data. As a rule of thumb, a database job should be introduced if:
/// -  the creation of the resulting data is not trivial and takes some time,
/// -  the creation of the data shall be done on demand and only if actually required, and
/// -  some task can be divided into smaller subtasks (individual jobs), which are independent
/// from one another.
///
/// In addition to triggering on-demand execution of database jobs by accessing the associated tag
/// it also possible to advise a database jobs for lazy execution (see
/// #mi::neuraylib::IDice_transaction::advise()).
///
/// The database has a mechanism to ensure that a job gets up-to-date information when being
/// executed. The guarantee given is: All changes done or seen on a host before doing an
/// #mi::neuraylib::IDice_transaction::advise() or #mi::neuraylib::IDice_transaction::access() on a
/// job are visible during job execution. This is also ensured if the job is executed on a remotely
/// on a different host in a cluster. This guarantee is also transitive for jobs executed by other
/// jobs.
///
/// \note Fragmented jobs represent a way to achieve parallelism and distributed computing. See
///       #mi::neuraylib::IFragmented_job for details.
///
/// \see The mixin #mi::neuraylib::Job can be useful as a starting point for your own implementation
///      of this abstract interface.
class IJob : public
    mi::base::Interface_declare<0x0edda67e,0x8e93,0x4d41,0xa8,0xdb,0x58,0x5d,0x08,0xfd,0xb1,0xde,
                                neuraylib::ISerializable>
{
public:
    /// Creates a copy of the database job.
    ///
    /// Duplicating a database job is used by the database when it needs to create a new version
    /// of the given job, e.g., when someone edits an existing job. This member function
    /// must create and return a full copy of the job.
    ///
    /// \return A newly allocated instance of the database job. The pointer represents a
    ///         complete replica of the database job on which the member function was called.
    virtual IJob* copy() const = 0;

    /// Returns a human readable identifier for the class this database job belongs to.
    ///
    /// This name is \em not required for proper operation, but can be useful for debugging. For
    /// example, the name is used to display the class name in the tag table of the HTTP
    /// administration console.
    ///
    /// \return         The class name. The string has to be valid at all times while instances of
    ///                 this class are stored in the database.
    virtual const char* get_class_name() const = 0;

    /// Returns the size of the database job in bytes.
    ///
    /// The value includes all memory that directly or indirectly belongs exclusively to this
    /// database job. For simple classes, \code sizeof(*this) \endcode works.
    ///
    /// A correct implementation of this method is required if memory limits are configured.
    ///
    /// \see #mi::neuraylib::IDatabase_configuration::set_memory_limits(),
    ///      #mi::neuraylib::IDatabase_configuration::get_memory_limits()
    virtual Size get_size() const = 0;

    /// Executes the database job and returns the resulting database element.
    ///
    /// Executing the database job returns a newly allocated database element. The type of
    /// the database element class returned by the database job may be arbitrary but must
    /// correspond to the class that is expected by code accessing the database job's result.
    ///
    /// Note that the database will execute that database job at most once in each transaction (see
    /// also #is_shared()).
    ///
    /// \param transaction  The transaction in which the job is executed. It may be used to access
    ///                     required database elements and other database jobs during the execution
    ///                     of the job. The transaction may not be used to edit or create new tags.
    /// \return             The created database element.
    virtual IElement* execute( IDice_transaction* transaction) const = 0;

    /// Indicates whether the database job is shared between transactions.
    ///
    /// A shared job is not automatically re-executed per transaction but only if
    /// - it has never been executed before or
    /// - it is explicitly invalidated by calling
    ///   #mi::neuraylib::IDice_transaction::invalidate_job_results().
    /// In contrast, a non-shared database job is done (at most) once per transaction when accessing
    /// the associated tag the first time.
    ///
    /// \return     \c true if the database job is shared between transactions,
    ///             or \c false otherwise.
    virtual bool is_shared() const = 0;

    /// Indicates whether the database job shall be executed on the local host only.
    ///
    /// A database job that is supposed to be executed locally cannot be delegated to other hosts in
    /// the cluster. In consequence, the database job must be executed on each host that accesses
    /// the associated tag.
    ///
    /// Typically, database jobs are executed locally if the inputs and/or the results too large to
    /// be transmitted over the network, or if the job is trivial enough to be executed on the fly
    /// and does not require distributed execution.
    ///
    /// \return     \c true if the job shall be executed on the local host only, or \c false
    ///             otherwise.
    virtual bool is_local_only() const = 0;

    /// Indicates whether the database job creates and stores database elements and/or database
    /// jobs.
    ///
    /// \return     \c true if the job creates and stores database elements, or \c false otherwise.
    virtual bool is_parent() const = 0;

    /// Returns the CPU load of the database job.
    ///
    /// Typically 1.0 for CPU jobs and 0.0 for GPU jobs. A value larger than 1.0 might be used for
    /// jobs that concurrently use multiple threads, e.g., if OpenMP or MPI is used.
    ///
    /// \note This value must \em never change for a given instance of the database job.
    virtual Float32 get_cpu_load() const = 0;

    /// Returns the GPU load of the database job.
    ///
    /// Typically 0.0 for CPU jobs and 1.0 for GPU jobs. A value larger than 1.0 might be used for
    /// jobs that concurrently use multiple GPUs.
    ///
    /// \note This value must \em never change for a given instance of the database job.
    virtual Float32 get_gpu_load() const = 0;
};

/// A callback interface used for asynchronous execution of fragmented jobs.
///
/// \see #mi::neuraylib::IDice_transaction::execute_fragmented_async().
class IExecution_listener : public
    mi::base::Interface_declare<0x5700dbde,0xbe1d,0x4c07,0xa7,0x0d,0xf4,0x11,0x7e,0x9f,0x73,0xf1>
{
public:
    /// Called when the execution of the job has been finished.
    virtual void job_finished() = 0;
};

/// This interface represents the %base class for all fragmented jobs.
///
/// Each fragmented job must be derived from the this interface.
///
/// Fragmented jobs enable distributing compute intensive work to the CPUs and/or GPUs available in
/// a cluster environment.
///
/// Compared to a database jobs, fragmented jobs are lightweight means to achieve distributed,
/// parallel computing in a cluster. Fragmented jobs differ from database jobs as follows:
/// - A fragmented job is not stored in the database but instantiated on demand to be executed
///   immediately.
/// - Each instance of a fragmented job splits into a pre-defined number of fragments each of which
///   executes independently possibly on different CPUs and/or GPUs in the cluster.
/// - A fragmented job does not return a result but the execution function may operate on the
///   members of the fragmented job class instance to store results. Note that it is necessary to
///   synchronize accesses to those members from different fragments because they may be executed
///   in parallel.
///
/// In general, a fragmented job should be used if the result of the parallel algorithm is used only
/// once by the host initiating the execution. For instance, a fragmented job for rendering may
/// initiate the rendering of a single frame.
///
/// \note Database jobs represent a different way to achieve parallelism in a cluster environment.
///       See #mi::neuraylib::IJob for details.
///
/// \see The mixin #mi::neuraylib::Fragmented_job can be useful as a starting point for your own
///      implementation of this abstract interface.
class IFragmented_job : public
    mi::base::Interface_declare<0x037f3783,0x187c,0x4bb7,0x92,0x50,0x6d,0xfe,0x1c,0x81,0x89,0xef,
                                neuraylib::ISerializable>
{
public:
    /// Executes one of many fragments of the fragmented job on the local host.
    ///
    /// Executes one fragment of the many fragments spawned by the fragmented job. This fragment
    /// executed on the calling host only and, thus, operates on the original instance of the
    /// fragmented job class and not on a copy.
    ///
    /// Note that other fragments operating on the same instance might be executed in parallel.
    /// Therefore all accesses to instance data need to be properly serialized.
    ///
    /// \see #execute_fragment_remote() and #receive_remote_result() are the counterpart of this
    ///      method for remote execution
    /// \see #execute_fragment_remote_rdma() and #receive_remote_result_rdma() are the counterpart
    ///      of this method for remote execution using RDMA
    ///
    /// \param transaction  The transaction in which the fragmented job is executed. The transaction
    ///                     can be used to access database elements and database jobs required for
    ///                     execution but should not be used to edit or create tags. Might be
    ///                     \c NULL if the fragmented job was started without transaction.
    /// \param index        The index of the fragment to be executed. The value is in the range from
    ///                     0 to \p count-1.
    /// \param count        The number of fragments into which the fragmented job is split.
    /// \param context      The context in which the fragmented job is executed.
    virtual void execute_fragment(
        IDice_transaction* transaction,
        Size index,
        Size count,
        const IJob_execution_context* context) = 0;

    /// Executes one of many fragments of the fragmented job on a remote host.
    ///
    /// Executes one fragment of the many fragments spawned by the fragmented job on a remote host.
    /// The remote execution requires the remote host to generate a replica of the original
    /// fragmented job instance using #serialize() on the local host and #deserialize() on the
    /// remote host.
    ///
    /// Note that all fragments of a fragmented job which are executed on the same host will use the
    /// same replica of the original fragmented job instance. That means the replica can contain
    /// data shared between different fragments, like caches, etc.
    ///
    /// As in the case of local execution other fragments executed on that host might be executed in
    /// parallel. Therefore all accesses to instance data need to be properly synchronized.
    ///
    /// Intermediate results can be returned by calling #mi::neuraylib::ISerializer::flush() on \p
    /// serializer. All data serialized until that point will be delivered in a single call to
    /// #receive_remote_result(). When #execute_fragment_remote() returns any data not yet flushed
    /// will be flushed and #receive_remote_result() will be called one last time with that data.
    ///
    /// \param serializer   The serializer used to send the result of that fragment back to the
    ///                     initiating host (see #receive_remote_result()).
    /// \param transaction  The transaction in which the fragmented job is executed. The transaction
    ///                     can be used to access database elements and database jobs required for
    ///                     execution but should not be used to edit or create tags.
    /// \param index        The index of the fragment to be executed. The value is in the range from
    ///                     0 to \p count-1.
    /// \param count        The number of fragments into which the fragmented job is split.
    /// \param context      The context in which the fragmented job is executed.
    ///
    /// \see #receive_remote_result() consumes the data produced by this method
    /// \see #execute_fragment() is the counterpart of this method and #receive_remote_result() for
    ///      local execution
    /// \see #execute_fragment_remote_rdma() and #receive_remote_result_rdma() are the counterpart
    ///      of this method and #receive_remote_result() for remote execution using RDMA
    virtual void execute_fragment_remote(
        ISerializer* serializer,
        IDice_transaction* transaction,
        Size index,
        Size count,
        const IJob_execution_context* context) = 0;

    /// Receives the result generated by the remote execution of a fragment.
    ///
    /// This method is called by the host that initiated the fragmented job execution for each
    /// fragment that has been executed on a remote host. The method receives via \p deserializer
    /// the data that has been written to the serializer on the remote host by
    /// #execute_fragment_remote().
    ///
    /// This method is called on the original instance of the fragmented job. Note that other
    /// fragments operating on the same instance might receive their result in parallel. Therefore
    /// all accesses to instance data need to be properly serialized.
    ///
    /// Essentially, the remote execution of fragments of a fragmented job using
    /// #execute_fragment_remote() on the remote host and #receive_remote_result() on the local host
    /// should have the same effect as executing #execute_fragment() on the local host.
    ///
    /// The remote execution of a fragment might produce intermediate results by flushing the data
    /// serialized so far. Each call to #mi::neuraylib::ISerializer::flush() on the remote host will
    /// result in a single call to #receive_remote_result() on the local host, even if no data was
    /// flushed. When the execution on the remote host finishes this method will be called one last
    /// time with the remaining data.
    ///
    /// \param deserializer The deserializer used to receive the result of that fragment from the
    ///                     remote host.
    /// \param transaction  The transaction in which the fragmented job is executed. The transaction
    ///                     can be used to access database elements and database jobs required for
    ///                     execution but should not be used to edit or create tags.
    /// \param index        The index of the fragment to be executed. The value is in the range from
    ///                     0 to \p count-1.
    /// \param count        The number of fragments into which the fragmented job is split.
    ///
    /// \see #execute_fragment_remote() produces the data consumed by this method
    /// \see #execute_fragment() is the counterpart of this method and
    ///      #execute_fragment_remote() for local execution
    /// \see #execute_fragment_remote_rdma() and #receive_remote_result_rdma() are the counterpart
    ///      of this method and #execute_fragment_remote() for remote execution using RDMA
    virtual void receive_remote_result(
        IDeserializer* deserializer,
        IDice_transaction* transaction,
        Size index,
        Size count) = 0;

    /// Returns an RDMA buffer to be used for the result of the fragment.
    ///
    /// Used to allocate an RDMA buffer which will be used to store the result of a fragment
    /// execution on a remote host. This method is only called for fragments which are executed on
    /// remote hosts. The RDMA buffer must be obtained from the given RDMA context.
    ///
    /// If the method returns a valid RDMA buffer, the methods #execute_fragment_remote_rdma() and
    /// #receive_remote_result_rdma() are used instead of #execute_fragment_remote() and
    /// #receive_remote_result() to execute a fragment and to receive its result.
    ///
    /// The RDMA buffer must at least be able to store the maximum expected result of the fragment
    /// execution.
    ///
    /// \note If RDMA is not available on the system, the behavior will be the same as if RDMA would
    ///       be used, only the performance might be lower.
    ///
    /// \param rdma_context RDMA context to acquire buffers from for this host.
    /// \param index        The index of the fragment to be executed.
    /// \return             The RDMA buffer or \c NULL if RDMA should not be used.
    virtual IRDMA_buffer* get_rdma_result_buffer( IRDMA_context* rdma_context, Size index) = 0;

    /// Executes one of many fragments of the fragmented job on a remote host (RDMA variant).
    ///
    /// Executes one fragment of the many fragments spawned by the fragmented job on a remote host.
    /// The remote execution requires the remote host to generate a replica of the original
    /// fragmented job instance using #serialize() on the local host and #deserialize() on the
    /// remote host.
    ///
    /// Note that all fragments of a fragmented job which are executed on the same host will use the
    /// same replica of the the original fragmented job instance. That means the replica can contain
    /// data shared between different fragments, like caches, etc.
    ///
    /// As in the case of local execution other fragments executed on that host might be executed in
    /// parallel. Therefore all accesses to instance data need to be properly synchronized.
    ///
    /// \param transaction  The transaction in which the fragmented job is executed. The transaction
    ///                     can be used to access database elements and database jobs required for
    ///                     execution but should not be used to edit or create tags.
    /// \param index        The index of the fragment to be executed. The value is in the range from
    ///                     0 to \p count-1.
    /// \param count        The number of fragments into which the fragmented job is split.
    /// \param rdma_context The RDMA context to acquire RDMA buffers from for this host.
    /// \param context      The context in which the fragmented job is executed.
    /// \return             The RDMA buffer with the result of this fragment (allocated from \p
    ///                     rdma_context) or \c NULL if no result is to be send back from this
    ///                     fragment. The same RDMA buffer can be returned from multiple fragments
    ///                     and from different jobs, in that case it will be sent back as the result
    ///                     of all the fragments using it. \n
    ///                     Note that the returned RDMA buffer has to stay unchanged as long as the
    ///                     reception has not been confirmed from the receiving side. At this time
    ///                     the system will return the buffer to the RDMA context, so that it can be
    ///                     used for further transmissions. \n
    ///                     Note that allocation of the RDMA buffer from \p rdma_context will fail
    ///                     if its size exceeds the size of the RDMA buffer allocated for this
    ///                     fragment on the receiver side.
    ///
    /// \see #receive_remote_result_rdma() produces the data consumed by this method
    /// \see #execute_fragment() is the counterpart of this method and receive_remote_result_rdma()
    ///      for local execution
    /// \see #execute_fragment_remote() and #receive_remote_result() are the counterparts
    ///      of this method and #receive_remote_result_rdma() for remote execution without RDMA
    virtual IRDMA_buffer* execute_fragment_remote_rdma(
        IDice_transaction* transaction,
        Size index,
        Size count,
        IRDMA_context* rdma_context,
        const IJob_execution_context* context)  = 0;

    /// Receives the result generated by the remote execution of a fragment (RDMA variant).
    ///
    /// This method is called by the host that initiated the fragmented job execution for each
    /// fragment that has been executed on a remote host. The method receives via \p buffer the data
    /// that has been returned in the RDMA buffer on the remote host by
    /// #execute_fragment_remote_rdma().
    ///
    /// This method is called on the original instance of the fragmented job. Note that other
    /// fragments operating on the same instance might receive their result in parallel. Therefore
    /// all accesses to instance data need to be properly serialized.
    ///
    /// Essentially, the remote execution of fragments of a fragmented job using
    /// #execute_fragment_remote_rdma() on the remote host and #receive_remote_result_rdma() on the
    /// local host should have the same effect as executing #execute_fragment() on the local host.
    ///
    /// \param buffer       The RDMA buffer obtained from #get_rdma_result_buffer(). It contains the
    ///                     data returned by #execute_fragment_remote_rdma() on the remote host. The
    ///                     data might be processed immediately, but it is also possible to retain
    ///                     the buffer and to process it later.
    /// \param transaction  The transaction in which the fragmented job is executed. The transaction
    ///                     can be used to access database elements and database jobs required for
    ///                     execution but should not be used to edit or create tags.
    /// \param index        The index of the fragment to be executed. The value is in the range from
    ///                     0 to \p count-1.
    /// \param count        The number of fragments into which the fragmented job is split.
    ///
    /// \see #execute_fragment_remote_rdma() produces the data consumed by this method
    /// \see #execute_fragment() is the counterpart of this method and
    ///      #execute_fragment_remote_rdma() for local execution
    /// \see #execute_fragment_remote() and #receive_remote_result() are the counterparts
    ///      of this method and #execute_fragment_remote_rdma() for remote execution without RDMA
    virtual void receive_remote_result_rdma(
        IRDMA_buffer* buffer,
        IDice_transaction* transaction,
        Size index,
        Size count) = 0;

    /// Serializes the fragmented job to enable remote job execution of fragments.
    ///
    /// The serialization shall include all members required for remote fragment execution. If the
    /// fragmented job references objects by means of tags it suffices to serializes the tags only.
    /// The remote execution can then access these tags and the distributed database ensures that
    /// the objects get transferred to the requesting remote host.
    ///
    /// \param serializer   The serializer to which to write the job content to.
    virtual void serialize( ISerializer* serializer) const = 0;

    /// Deserializes the fragmented job to enable remote job execution of fragments.
    ///
    /// The method creates a replica of the serialized original fragmented job instance on the
    /// remote host. The deserialized replica is used for all fragments of a fragmented job that are
    /// executed on that particular host, i.e., they all share the same data. In consequence, the
    /// class may additionally generate data required by all fragments, like caches, etc.
    ///
    /// \param deserializer   The deserializer from which to read the job content.
    virtual void deserialize( IDeserializer* deserializer) = 0;

    /// Static assignment of fragments to hosts in the cluster.
    ///
    /// The static assignment of fragments to hosts in the cluster overrides the DiCE built-in
    /// scheduling of fragments to hosts by providing an array of host IDs that are mapped to the
    /// fragments' indices, i.e., by assigning fragments to hosts explicitly.
    ///
    /// Example: The array 1, 2, 2, 2, 3 would map the first fragment with index 0 to host ID 1, the
    /// next three fragments to host ID 2, and the last fragment with index 4 to host ID 3.
    ///
    /// A typical use case for static assignments represent sort-last algorithms. Sort-last
    /// algorithms subdivide the (huge) data to be processed into smaller portions and then delegate
    /// the smaller portions to hosts once. This avoids network transfers while the processing the
    /// data later. The static assignment then ensure that the fragment for processing a certain
    /// portion of the data are scheduled for the host in the cluster where the portion resides.
    ///
    /// Care should be taken when implementing a static user-defined assignment. Usually, the DiCE
    /// built-in scheduler balances the workload of compute intensive tasks itself. The static
    /// user-defined assignment of jobs to hosts circumvents this feature which might results in an
    /// unbalanced non-scalable system when done improperly.
    ///
    /// \note In case the assigned host ID is not a valid one, DiCE will silently skip the execution
    ///       of the fragment. An application which needs to have all fragments executed should note
    ///       which fragments have been done, when the job finished, and re-schedule any missing
    ///       fragments.
    ///
    /// \param slots        Pointer to an array of host IDs. The i-th element is the host ID of the
    ///                     i-th fragment.
    /// \param nr_slots     The size of the array \p slots.
    virtual void assign_fragments_to_hosts( Uint32* slots, Size nr_slots) = 0;

    /// Constants for possible scheduling modes.
    ///
    /// \see #mi::neuraylib::IFragmented_job::get_scheduling_mode()
    enum Scheduling_mode {
        /// All fragments will be done on the local host. In consequence, dummy implementations for
        /// #serialize() and #deserialize() as well as for execute_fragment_remote() and
        /// #receive_remote_result() suffice.
        LOCAL = 0,
        /// The fragments will be spread across all hosts in the cluster.
        CLUSTER = 1,
        /// At most one fragment will be done per remote host. If the specified number of fragments
        /// is larger than zero and less than the number of remote hosts then only some hosts will
        /// get a fragment. To ensure that exactly one fragment will be done per remote host the
        /// number of fragments should be set to the special value 0. This mode is not possible for
        /// jobs which need a GPU.
        ///
        /// \note Only hosts which are in the same sub cluster are eligible for executing a
        ///       fragment of the job.
        ONCE_PER_HOST = 2,
        /// The job implements an explicit assignment of fragments to hosts and must implement the
        /// #assign_fragments_to_hosts() function to fill out all slots.
        USER_DEFINED = 3,
        SCHEDULING_MODE_FORCE_32_BIT = 0xffffffffU
    };

    /// Returns the scheduling mode.
    ///
    /// \return   The scheduling mode for the fragmented job.
    virtual Scheduling_mode get_scheduling_mode() const = 0;

    /// Returns the CPU load per fragment of the fragmented job.
    ///
    /// Typically 1.0 for CPU jobs and 0.0 for GPU jobs. A value larger than 1.0 might be used for
    /// jobs that concurrently use multiple threads per fragment, e.g., if OpenMP or MPI is used. A
    /// value between 0.0 and 1.0 might be used for jobs that do not much work themselves, but are
    /// rather used as synchronization primitive.
    ///
    /// \note This value must \em never change for a given instance of the fragmented job.
    virtual Float32 get_cpu_load() const = 0;

    /// Returns the GPU load per fragment of the fragmented job.
    ///
    /// Typically 0.0 for CPU jobs and 1.0 for GPU jobs. A value larger than 1.0 might be used for
    /// jobs that concurrently use multiple GPUs per fragment. A value between 0.0 and 1.0 might be
    /// used for jobs that do not much work themselves, but are rather used as synchronization
    /// primitive.
    ///
    /// \note This value must \em never change for a given instance of the fragmented job.
    virtual Float32 get_gpu_load() const = 0;

    /// Cancels the execution of not yet completed jobs.
    ///
    /// This method is called if the job has been submitted for execution, but not all its fragments
    /// have been completed when the transaction is closed or aborted. If the job result is no
    /// longer needed in such a case, this notification can be used to terminate currently running
    /// fragments and/or skip the execution of not yet started fragments.
    virtual void cancel() = 0;

    /// Returns the priority of the job.
    ///
    /// The smaller the value the higher the priority of the job to be executed.
    ///
    /// \note Negative values are reserved for internal purposes.
    virtual Sint8 get_priority() const = 0;

    /// Returns the maximum number of threads that should be used to execute the fragments of this
    /// job.
    ///
    /// With certain job patterns a large number of threads might be used to execute as many
    /// fragments as possible in parallel. This property can be used to limit the number of threads,
    /// potentially at the cost of performance. The special value 0 means \em no limit on the number
    /// of threads.
    virtual Size get_thread_limit() const = 0;

    /// Indicates whether chunks can be delivered out of order.
    virtual bool get_allow_non_sequential_chunks() const = 0;

};

mi_static_assert( sizeof( IFragmented_job::Scheduling_mode) == sizeof( Uint32));

/// This mixin class can be used to implement the #mi::base::IInterface interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// #mi::base::IInterface interface. It is used by the mixin classes
/// #mi::neuraylib::IElement, #mi::neuraylib::IJob, and #mi::neuraylib::IFragmented_job.
///
/// You are advised not to use this classes directly. Either use the three mixin classes
/// mentioned above, or to implement a generic interface, use #mi::base::Interface_implement.
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = base::IInterface>
class Base : public base::Interface_implement<I>
{
public:
    /// Own type.
    typedef Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I> Self;

    /// Declares the interface ID
    typedef base::Uuid_t<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11> IID;

    /// Compares the interface ID \p iid against the interface ID of this interface and its
    /// ancestors.
    ///
    /// \return \c true if \p iid == \c IID() or is equal to one of the interface IDs of
    /// its ancestors, and \c false otherwise.
    static bool compare_iid( const base::Uuid& iid)
    {
        if( iid == IID())
            return true;
        return I::compare_iid( iid);
    }

    /// Acquires a const interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL \c const #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must
    /// release the returned interface pointer at the end to prevent a memory leak.
    virtual const base::IInterface* get_interface( const base::Uuid& interface_id) const
    {
        if( interface_id == IID()) {
            const Self* self = static_cast<const Self*>( this);
            self->retain();
            return self;
        }
        return I::get_interface_static( this, interface_id);
    }

    /// Acquires a mutable interface.
    ///
    /// If this interface is derived from or is the interface with the passed
    /// \p interface_id, then return a non-\c NULL #mi::base::IInterface* that
    /// can be casted via \c static_cast to an interface pointer of the interface type
    /// corresponding to the passed \p interface_id. Otherwise return \c NULL.
    ///
    /// In the case of a non-\c NULL return value, the caller receives ownership of the new
    /// interface pointer, whose reference count has been retained once. The caller must
    /// release the returned interface pointer at the end to prevent a memory leak.
    virtual base::IInterface* get_interface( const base::Uuid& interface_id)
    {
        if( interface_id == IID()) {
            Self* self = static_cast<Self*>( this);
            self->retain();
            return self;
        }
        return I::get_interface_static( this, interface_id);
    }

    using base::Interface_implement<I>::get_interface;

    /// Returns the class ID corresponding to the template parameters of this mixin class.
    virtual base::Uuid get_class_id() const
    {
        return IID();
    }
};

/// This mixin class can be used to implement the #mi::neuraylib::IElement interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// #mi::neuraylib::IElement interface. The documentation here just lists the behavior of the
/// default implementation, see #mi::neuraylib::IElement for the documentation of the
/// methods themselves.
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = IElement>
class Element :  public Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I>
{
public:
    /// Default constructor
    Element() : m_pointer( 0) { }

    /// Copy constructor
    Element( const Element& /*other*/) : m_pointer( 0) { }

    /// Assignment operator
    Element& operator=( const Element& other)
    {
       return base::Interface_implement<I>::operator=( other);
    }

    /// Destructor
    ~Element()
    {
        mi_base_assert( m_pointer == 0);
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


    /// Empty body, i.e., leaves \p result unaltered.
    virtual void get_references( ITag_set* result) const
    {
        // avoid warnings
        (void) result;
    }

    /// Assumes that the size of the database element is given by \c sizeof.
    virtual Size get_size() const
    {
        return sizeof( *this);
    }

    /// By default, multicast distribution for database elements is enabled.
    virtual bool get_send_to_all_nodes() const
    {
        return true;
    }

    /// By default, offloading to disk is disabled.
    virtual bool get_offload_to_disk() const
    {
        return false;
    }

private:
    //  The embedded pointer.
    //
    //  The embedded pointer is used for internal purposes. Users must not access the pointer.
    mutable const base::IInterface* m_pointer;

    //  The lock that protects the embedded pointer.
    mutable base::Lock m_pointer_lock;
};

/// This mixin class can be used to implement the #mi::neuraylib::IJob interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// #mi::neuraylib::IJob interface. The documentation here just lists the behavior of the
/// default implementation, see #mi::neuraylib::IJob for the documentation of the
/// methods themselves.
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = IJob>
class Job : public Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I>
{
public:
    /// Assumes that the size of the database job is given by \c sizeof.
    virtual Size get_size() const
    {
        return sizeof( *this);
    }

    /// Returns \c false.
    virtual bool is_local_only() const { return false; }

    /// Returns \c false.
    virtual bool is_shared() const { return false; }

    /// Returns \c false.
    virtual bool is_parent() const { return false; }

    /// Returns 1.0.
    virtual Float32 get_cpu_load() const { return 1.0f; }

    /// Returns 0.0.
    virtual Float32 get_gpu_load() const { return 0.0f; }
};

/// This mixin class can be used to implement the #mi::neuraylib::IFragmented_job interface.
///
/// This mixin class provides a default implementation of some of the pure virtual methods of the
/// #mi::neuraylib::IFragmented_job interface. The documentation here just lists the behavior of the
/// default implementation, see #mi::neuraylib::IFragmented_job for the documentation of the
/// methods themselves.
template <Uint32 id1, Uint16 id2, Uint16 id3
    , Uint8 id4, Uint8 id5, Uint8 id6, Uint8 id7
    , Uint8 id8, Uint8 id9, Uint8 id10, Uint8 id11
    , class I = IFragmented_job>
class Fragmented_job :  public Base<id1,id2,id3,id4,id5,id6,id7,id8,id9,id10,id11,I>
{
public:
    /// Returns #mi::neuraylib::IFragmented_job::LOCAL.
    virtual IFragmented_job::Scheduling_mode get_scheduling_mode() const
    {
        return IFragmented_job::LOCAL;
    }

    /// Returns 1.0.
    virtual Float32 get_cpu_load() const { return 1.0f; }

    /// Returns 0.0.
    virtual Float32 get_gpu_load() const { return 0.0f; }

    /// Returns 0.
    virtual Sint8 get_priority() const { return 0; }

    /// Returns 0.
    virtual Size get_thread_limit() const { return 0; }

    /// Returns false.
    virtual bool get_allow_non_sequential_chunks() const { return false; }

    /// Empty body, i.e., leaves \p slots unaltered.
    virtual void assign_fragments_to_hosts(
        Uint32* slots,
        Size nr_slots)
    {
        // avoid warnings
        (void) slots;
        (void) nr_slots;
    }

    /// Empty body. Not used since #get_scheduling_mode() requests local execution.
    virtual void execute_fragment_remote(
        ISerializer* serializer,
        IDice_transaction* transaction,
        Size index,
        Size count,
        const IJob_execution_context* context)
    {
        // avoid warnings
        (void) serializer;
        (void) transaction;
        (void) index;
        (void) count;
        (void) context;
    }

    /// Empty body. Not used since #get_scheduling_mode() requests local execution.
    virtual void receive_remote_result(
        IDeserializer* deserializer,
        IDice_transaction* transaction,
        Size index,
        Size count)
    {
        // avoid warnings
        (void) deserializer;
        (void) transaction;
        (void) index;
        (void) count;
    }

    /// Empty body. Not used since #get_scheduling_mode() requests local execution.
    virtual IRDMA_buffer* get_rdma_result_buffer( IRDMA_context* rdma_context, Size index)
    {
        // avoid warnings
        (void) rdma_context;
        (void) index;
        return 0;
    }

    /// Empty body. Not used since #get_scheduling_mode() requests local execution.
    virtual IRDMA_buffer* execute_fragment_remote_rdma(
        IDice_transaction* transaction,
        Size index,
        Size count,
        IRDMA_context* rdma_context,
        const IJob_execution_context* context)
    {
        // avoid warnings
        (void) transaction;
        (void) index;
        (void) count;
        (void) rdma_context;
        (void) context;
        return 0;
    }

    /// Empty body. Not used since #get_scheduling_mode() requests local execution.
    virtual void receive_remote_result_rdma(
        IRDMA_buffer* buffer, IDice_transaction* transaction, Size index, Size count)
    {
        // avoid warnings
        (void) buffer;
        (void) transaction;
        (void) index;
        (void) count;
    }

    /// Empty body, i.e., no member data is serialized.
    virtual void serialize( ISerializer* serializer) const
    {
        // avoid warnings
        (void) serializer;
    }

    /// Empty body, i.e., no member data is deserialized.
    virtual void deserialize( IDeserializer* deserializer)
    {
        // avoid warnings
        (void) deserializer;
    }

    /// Empty body, i.e., canceling is not supported.
    virtual void cancel() { }
};

/*@}*/ // end group mi_neuray_dice

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_DICE_H
