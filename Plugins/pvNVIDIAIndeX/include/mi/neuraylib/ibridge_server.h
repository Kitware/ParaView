/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief  Bridge server

#ifndef MI_NEURAYLIB_IBRIDGE_SERVER_H
#define MI_NEURAYLIB_IBRIDGE_SERVER_H

#include <mi/base/enums.h>
#include <mi/base/handle.h>
#include <mi/base/interface_implement.h>
#include <mi/base/uuid.h>
#include <mi/neuraylib/iserializer.h>
#include <mi/neuraylib/itransaction.h>
#include <mi/neuraylib/iuser_class_factory.h>

namespace mi {

class IString;
class IMap;

namespace http { class IServer; }

namespace neuraylib { class ICanvas; class IImport_result; class ITag_set; class IExport_result; }

namespace bridge {

class IApplication;
class IApplication_session_handler;
class IElement_set;
class IServer_session;
class IServer_transaction;
class IServer_video_context;

/** \defgroup mi_neuray_bridge_server Bridge server
    \ingroup mi_neuray_bridge

    The Bridge server is a single remote host that makes its resources available to Bridge
    clients through a proprietary network protocol over a single TCP/IP connection. The client uses
    the connection to send changes and delegate work to the Bridge server; the server uses the
    connection to send job results and other data back to the client.

    A Bridge server can serve multiple clients at the same time. This is equivalent to starting
    multiple jobs simultaneously on a host in a normal local cluster.

    \if IRAY_API The API consists of two layers: a low-level API, called \em Bridge \em API, and a
    high-level API, called \em Iray \em Bridge \em API. The low-level Bridge API is generic and
    independent of domain-specific tasks like rendering. It exposes the full flexibility of the
    Bridge. The high-level Iray Bridge API is specific to rendering. It encapsulates the details of
    the Bridge API and exposes them in a simpler way sufficient for rendering. Using the high-level
    API is strongly recommended unless there are specific needs which require using the low-level
    API. See the API component #mi::bridge::IBridge_server for the Bridge API, and
    #mi::bridge::IIray_bridge_server for the Iray Bridge API. \else See the API component
    #mi::bridge::IBridge_server for a starting point. \endif

    \see \ref mi_neuray_bridge, \ref mi_neuray_bridge_client
*/

/** \addtogroup mi_neuray_bridge_server
@{
*/

class IServer_job_info;

/// Represents the server-side part of a job that can be executed by the Bridge server.
///
/// The corresponding client-side part of the job must return the same ID from
/// #mi::neuraylib::ISerializable::get_class_id() and be registered as a Bridge job on the
/// client.
///
/// The server-side part must implement #mi::neuraylib::ISerializable::deserialize() to deserialize
/// the data that is sent by #mi::bridge::IClient_job::serialize(), and
/// #mi::bridge::IServer_job::execute() to serialize the result of the job which is consumed by
/// #mi::bridge::IClient_job::receive_remote_result(),
///
/// It is recommended to derived your implementation from #mi::bridge::Server_job and overriding
/// only the required methods.
class IServer_job : public
    mi::base::Interface_declare<0x555dea0f,0x4eeb,0x44b9,0xba,0x81,0x5a,0x42,0x3d,0xe5,0xf8,0x30,
    neuraylib::ISerializable>
{
public:
    /// Executes a job on behalf of a Bridge client.
    ///
    /// Intermediate results can be sent to the client by calling
    /// #mi::neuraylib::ISerializer::flush() on \p serializer. All data serialized until that point
    /// will be delivered in a single call to #mi::bridge::IClient_job::receive_remote_result() on
    /// the client with the \c last_result flag set to \c false. When #execute() returns any data
    /// not yet flushed will be flushed and #mi::bridge::IClient_job::receive_remote_result() will
    /// be called one last time with \c last_result flag set to \c true.
    ///
    /// Example 1: No intermediate results are required. The \c execute() method simply serializes
    ///            the result and returns. The \c receive_remote_result() method will be called once
    ///            with the full result and \c last_result flag set to \c true.
    ///
    /// Example 2: Progress feedback. During execution of the job periodically one #mi::Float32
    ///            representing the percentage of the work done is serialized periodically and the
    ///            serializer is flushed. Finally, when the job is done the final result is
    ///            serialized and the method returns. On the client side \c receive_remote_result()
    ///            parses a single #mi::Float32 for every call with the \c last_result flag set to
    ///            \c false and updates a progress bar. When \c last_result is \c true it
    ///            deserializes the entire result and is done.
    ///
    /// Example 3: Custom protocol. Whenever the server is done with a partial result it is flushed
    ///            as a chunk to the client for processing. The server is preparing multiple
    ///            different types of results, so the chunk always begins with an #mi::Uint8 which
    ///            identifies the type of the chunk and the data to expect. After flushing the last
    ///            chunk the \c execute() method returns without any pending data. On the client
    ///            side \c receive_remote_result() will check the \c last_result flag. If \c true,
    ///            then the job is done, no more data to parse. If \c false the first #mi::Uint8 is
    ///            deserialized and the rest of the data is deserialized depending on the type of
    ///            the chunk and then processed.
    ///
    /// \param transaction   The Bridge transaction to be used for the job execution.
    /// \param serializer    The serializer to be used to send the result back to the client.
    /// \param job_info      Additional information about this job.
    virtual void execute(IServer_transaction* transaction, neuraylib::ISerializer* serializer,
        IServer_job_info* job_info) = 0;

    /// Called if the Bridge transaction is aborted or if an error occurs.
    ///
    /// The job execution should be stopped as quickly as possible, no result will be written to the
    /// client.
    virtual void cancel() = 0;
};

/// This mixin class provides a default implementation for some of the methods needed by
/// #mi::bridge::IServer_job.
///
/// It is recommended to derive from this class rather than from #mi::bridge::IServer_job directly.
template <Uint32 i_id1, Uint16 i_id2, Uint16 i_id3
, Uint8 i_id4, Uint8 i_id5, Uint8 i_id6, Uint8 i_id7
, Uint8 i_id8, Uint8 i_id9, Uint8 i_id10, Uint8 i_id11
, class I = IServer_job>
class Server_job : public base::Interface_implement<I>
{
public:
    /// Own type.
    typedef Server_job<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I> Self;

    /// Declares the interface ID
    typedef base::Uuid_t<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11> IID;

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

    /// Empty body. This method from the base class is not needed for client jobs.
    virtual void serialize( neuraylib::ISerializer* serializer) const
    {
        // avoid warnings
        (void) serializer;
    }

    /// Does nothing in the default implementation.
    virtual void cancel() { }
};

/// Provides additional information about a bridge job.
class IServer_job_info : public
    mi::base::Interface_declare<0xa28b5525,0x728b,0x447a,0x89,0x9f,0x15,0x64,0xdf,0x14,0xc7,0xdc>
{
public:
    /// Returns the Bridge job instance. If the job has not been deserialized yet \c NULL is 
    /// returned. This is the case if the job data contains tags in which case it can't be 
    /// deserialized until the server-side database transaction has been started.
    virtual IServer_job* get_job() const = 0;

    /// Returns the Bridge job instance.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    T* get_job() const
    {
        IServer_job* ptr_job = get_job();
        if ( !ptr_job)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_job->get_interface( typename T::IID()));
        ptr_job->release();
        return ptr_T;
    }

    /// Returns the id of this job. The id is only guaranteed to be unique among the 
    /// currently active jobs of the transaction and is suitable only for log messages
    /// and the like.
    virtual const char* get_id() const = 0;

    /// Returns the universally unique identifier (UUID or GUID) of the Bridge job. This 
    /// will always be available even if the job has not been deserialized yet. The UUID
    /// defines the job class and client side and server side job implementations share 
    /// the same class id.
    virtual base::Uuid get_job_uuid() = 0;
};

/// A list of IServer_job instances.
class IServer_job_list : public
    mi::base::Interface_declare<0xde301678,0x11e9,0x4bed,0x9b,0xc3,0x60,0x1e,0xcf,0xa3,0x88,0xa9>
{
public:

    /// Returns the number of jobs in the list.
    virtual Size get_size() = 0;

    /// Returns the job at the given index, or 0 if the index is out of bounds. The job must 
    /// be released when not needed anymore.
    virtual IServer_job_info* get_job(Size index) = 0;
};

/// Abstract interface for callbacks for transaction commit or abort events.
///
/// \see #mi::bridge::IServer_transaction::add_transaction_callback(),
///      #mi::bridge::IServer_transaction::remove_transaction_callback()
class IServer_transaction_callback : public
    mi::base::Interface_declare<0x7796c406,0xff8f,0x423d,0x8f,0x53,0x1a,0x66,0x50,0xcf,0x83,0x86>
{
public:
    /// This method is called when the Bridge transaction has been successfully committed.
    ///
    /// At this point any changes mirrored via the Bridge transaction have been committed to the
    /// server-side database.
    virtual void transaction_committed_callback() = 0;

    /// This method is called when the Bridge transaction has been aborted.
    ///
    /// Any changes mirrored via the Bridge transaction have \em not been committed to the
    /// server-side database.
    ///
    /// \param error_code
    ///                     -     0: The transaction was aborted by request of the client.
    ///                     -    -1: The transaction was canceled because of a connection error.
    ///                     - <= -2: The transaction was canceled because of an unspecified error.
    /// \param message      A short explanation of the error.
    virtual void transaction_aborted_callback( Sint32 error_code, const char* message) = 0;
};

/// Context for incremental snapshots.
///
/// \see #mi::bridge::IServer_transaction::create_incremental_snapshot_context()
class IIncremental_snapshot_context : public
    mi::base::Interface_declare<0x5ec24e12,0x9ce9,0x4f39,0x9f,0xd5,0x35,0xf1,0xe5,0x41,0x34,0x2e>
{
public:
    /// Creates an incremental snapshot.
    ///
    /// The incremental snapshot contains only the changes since the last call to this method on
    /// this context (or to the creation of this context for the first call).
    ///
    /// \param transaction        The transaction to be used.
    /// \param[out] snapshot_id   The ID of the snapshot will be stored here if the operation is
    ///                           successful.
    /// \return
    ///                           -     0: Success.
    ///                           - <= -1: Unspecified error.
    virtual Sint32 create_snapshot( IServer_transaction* transaction, IString* snapshot_id) = 0;
};

/// Database transactions started on the client will be automatically mirrored to the server 
/// and exposed as instances of this class to executing Bridge jobs. The relative start and 
/// commit order of the mirrored server side transactions will always be the same as on the 
/// client. Note however that the server side transactions are started and committed 
/// asynchronously so even after committing the client side transaction the mirrored server 
/// side transaction might not even have been started yet. For the client to know when a 
/// server side transaction has finished it must wait for the last Bridge job executed in the 
/// transaction to complete.
class IServer_transaction : public
    mi::base::Interface_declare<0x67fd848e,0xce43,0x4675,0x8b,0x14,0xb2,0x54,0xd,0xd2,0x29,0x63>
{
public:
    /// Returns the session of the transaction.
    virtual IServer_session* get_session() const = 0;

    /// Returns the local transaction corresponding to this Bridge transaction, or \c NULL if this
    /// transaction has not yet been started on the client. The IServer_transaction instance 
    /// passed to #mi::bridge::IServer_job::execute() will always be started. IServer_transaction 
    /// instances returned by #mi::bridge::IServer_session::get_pending_transactions() are 
    /// however not always started yet, in which case this method will return \c NULL.
    ///
    /// \return the database transaction or \c NULL if this bridge transaction has not yet 
    ///         been started.
    virtual neuraylib::ITransaction* get_database_transaction() const = 0;

    /// Returns the local transaction corresponding to this Bridge transaction.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It eliminates the need to call
    /// #mi::base::IInterface::get_interface(const Uuid&)
    /// on the returned pointer, since the return type already is a pointer to the type \p T
    /// specified as template parameter.
    template<class T>
    T* get_database_transaction() const
    {
        neuraylib::ITransaction* ptr_itransaction = get_database_transaction();
        if ( !ptr_itransaction)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_itransaction->get_interface( typename T::IID()));
        ptr_itransaction->release();
        return ptr_T;
    }

    /// Adds a transaction callback.
    ///
    /// The callback will be called when the transaction has been committed or aborted by the
    /// client, or if there is an error causing the transaction to be aborted.
    ///
    /// \see #remove_transaction_callback()
    ///
    /// \param callback   The callback to be added.
    /// \return
    ///                   -     0: Success.
    ///                   -    -1: Invalid argument (\p callback is \c NULL).
    ///                   -    -2: The transaction is not open.
    ///                   -    -3: The transaction callback was already added.
    ///                   - <= -4: Unspecified error.
    virtual Sint32 add_transaction_callback( IServer_transaction_callback* callback) = 0;

    /// Removes a transaction callback.
    ///
    /// \see #add_transaction_callback()
    ///
    /// \param callback   The callback to be removed.
    /// \return
    ///                   -     0: Success.
    ///                   -    -1: Invalid argument (\p callback is \c NULL).
    ///                   -    -2: There is no such callback.
    ///                   - <= -3: Unspecified error.
    virtual Sint32 remove_transaction_callback( IServer_transaction_callback* callback) = 0;

    /// Creates a snapshot of some database elements.
    ///
    /// The snapshot will save the provided element and all elements it references in their current
    /// state into the disk cache. The snapshot will only reference data already in the disk cache
    /// so storage cost of snapshots is very low. Data in the disk cache is guaranteed not to be
    /// removed for as long as there exists at least one snapshot that references the data.
    ///
    /// \see #create_incremental_snapshot_context(),
    ///      #mi::bridge::IBridge_server::create_snapshot_context()
    ///
    /// \param element            The top level element to save the snapshot for.
    /// \param[out] snapshot_id   The ID of the snapshot will be stored here if the operation is
    ///                           successful.
    /// \return
    ///                           -     0: Success.
    ///                           -    -1: Invalid arguments (\p element or \p snapshot_id is
    ///                                    \c NULL).
    ///                           -    -2: The specified element does not exist.
    ///                           -    -3: Failed to create the snapshot.
    ///                           - <= -4: Unspecified error.
    virtual Sint32 create_snapshot( const char* element, IString* snapshot_id) = 0;

    /// Creates a \em base snapshot and a context for subsequent incremental snapshots.
    ///
    /// In contrast to regular snapshots, incremental snapshots contain only those elements that
    /// have been changed since the last incremental snapshot of the same context. The first
    /// incremental snapshot, also called \em base snapshot, of a given context contains all
    /// elements, just as the regular snapshot.
    ///
    /// Incremental snapshots must be imported in the same order as they were created, starting with
    /// the base snapshot.
    ///
    /// \note The current transaction is only used for the base snapshot, the context itself is not
    ///       bound to this transaction.
    ///
    /// \see #create_snapshot(),
    ///      #mi::bridge::IBridge_server::create_snapshot_context()
    ///
    /// \param element            The top level element that incremental snapshots will be created
    ///                           for.
    /// \param[out] snapshot_id   The ID of the snapshot will be stored here if the operation is
    ///                           successful.
    /// \param[out] context       The context to be used for incremental snapshots.
    /// \return
    ///                           -     0: Success.
    ///                           -    -1: Invalid parameters (\p element, \p snapshot_id, or
    ///                                    \p context is \c NULL).
    ///                           -    -2: The specified element does not exist.
    ///                           -    -3: Failed to create the snapshot.
    ///                           - <= -4: Unspecified error.
    virtual Sint32 create_incremental_snapshot_context(
        const char* element, IString* snapshot_id, IIncremental_snapshot_context** context) = 0;

    /// Returns the number of database elements updated by this transaction so far.
    ///
    /// This number includes elements that have just been uploaded to the disk cache as well as
    /// elements that also have been replicated in the database.
    virtual Size get_updated_element_count() const = 0;

    /// Returns the list of currently pending jobs for this transaction in the order they will 
    /// be executed. Note that jobs added after the call to this method won't be present in the 
    /// list and that jobs might have completed execution when this list is processed.
    /// Also note that jobs within this transaction will execute in sequence but jobs belonging 
    /// to other transactions might execute in parallel if the transactions execute in parallel.
    ///
    /// Two transactions will execute in sequence if the second transaction is started after  
    /// the previous transaction was committed. Example: start A, commit A, start B commit B. 
    /// They will execute in parallel if the second transaction is started before the previous 
    /// transaction is committed. For instance start A, start B, commit A, commit B.
    ///
    /// \return A list of pending jobs in the order they will be executed.
    virtual IServer_job_list* get_pending_jobs() const = 0;

    /// Returns the id of this Bridge transaction. The id is only guaranteed to be unique among 
    /// the currently active transactions and is suitable only for log messages and the like. 
    /// Note that the Bridge transaction id is not the same as the id of the database transaction
    /// returned by get_database_transaction().
    virtual const char* get_id() const = 0;
};

/// The different states a server session can be in.
///
/// \see #mi::bridge::IServer_session::get_state()
enum Server_session_state
{
    /// A client has connected to the application but has not yet been approved.
    SERVER_SESSION_CONNECTING = 0,

    /// The session has successfully established a connection to the Bridge application.
    SERVER_SESSION_CONNECTED  = 1,

    /// The session was disconnected unexpectedly and is waiting for the client to reconnect.
    SERVER_SESSION_PENDING    = 2,

    /// The session has been closed.
    SERVER_SESSION_CLOSED     = 3,

    SERVER_SESSION_FORCE_32_BIT = 0xffffffffU
};

mi_static_assert( sizeof( Server_session_state) == sizeof( Uint32));

/// Abstract interface for callbacks for session state changes.
///
/// \see #mi::bridge::IServer_session::add_session_state_callback(),
///      #mi::bridge::IServer_session::remove_session_state_callback()
class IServer_session_state_callback : public
    mi::base::Interface_declare<0x12a50ba1,0x9cfc,0x4a12,0x9a,0x4b,0x52,0x13,0xf6,0x98,0x81,0x11>
{
public:
    /// This method is called whenever the session changes its state.
    virtual void session_state_callback( IServer_session* session) = 0;
};

/// A list of IServer_transaction instances.
class IServer_transaction_list : public
    mi::base::Interface_declare<0xe97c6925,0x780,0x40b7,0x8c,0xc0,0xa9,0x2f,0x73,0xfe,0xb7,0x8
>
{
public:

    /// Returns the number of transactions in the list.
    virtual Size get_size() = 0;

    /// Returns the job at the given index, or 0 if the index is out of bounds.
    virtual IServer_transaction* get_transaction(Size index) = 0;
};

/// Represents the server side of a Bridge session.
///
/// \see #mi::bridge::IServer_session_state_callback::session_state_callback()
///      #mi::bridge::IApplication_session_handler::on_session_connect()
class IServer_session : public
    mi::base::Interface_declare<0x42574f4a,0xfab1,0x4fdc,0xa0,0xc7,0x52,0x48,0xba,0xfa,0x8e,0x7d>
{
public:
    /// Returns the state of this session.
    virtual Server_session_state get_state() const = 0;

    /// Returns the application this session belongs to.
    virtual IApplication* get_application() const = 0;

    /// Returns the video context for a given ID.
    ///
    /// Video contexts must first be created by the client and can then be retrieved on the server
    /// via this method. The video context ID is assigned when the video context is created on the
    /// client side and must be transferred to the server before calling this method.
    /// The recommended way to do this is to execute a Bridge job containing the video context ID
    /// and optionally other data that is required for the server-side application to set up
    /// the video source and to start producing frames.
    ///
    /// \param context_id   The video context ID.
    /// \return             The video context or \c NULL if no video context with the provided ID
    /// exists.
    virtual IServer_video_context* get_video_context( Sint32 context_id) = 0;

    /// Adds a session state callback.
    ///
    /// When adding a callback it will be called immediately once with the current state, and then
    /// every time the session state changes.
    ///
    /// \see #remove_session_state_callback()
    ///
    /// \param callback    The callback to be added.
    virtual void add_session_state_callback( IServer_session_state_callback* callback) = 0;

    /// Removes a previously added session state callback.
    ///
    /// \see #add_session_state_callback()
    ///
    /// \param callback    The callback to be removed.
    virtual void remove_session_state_callback( IServer_session_state_callback* callback) = 0;

    /// Returns the security token specified by the client.
    virtual const char* get_security_token() const = 0;

    /// Returns the session ID.
    virtual const char* get_session_id() const = 0;

    /// Returns build number of the client.
    virtual const char* get_client_build_number() = 0;

    /// Returns the client Bridge protocol version. This is usually the same as the server 
    /// Bridge protocol version, but can differ if the server for instance accepts older 
    /// clients.
    virtual const char* get_client_bridge_protocol_version() = 0;

    /// Returns a list of the currently pending Bridge transactions for this session in the
    /// order they were started on the client. This is also the order the transactions will
    /// be started on the server, but some transactions in the list might not have been 
    /// started yet on the server which means there won't be a database transaction yet.
    ///
    /// Note that even though transactions will be started in the order specified by this 
    /// list some transactions will execute in sequence and others in parallel, depending 
    /// on how the transactions were started on the client.
    ///
    /// Also note that even though all transactions in the list were pending 
    /// when this call was made they might have been already committed or aborted when the 
    /// list is processed and that any transactions added after this call won't be present 
    /// in the list.
    ///
    /// Two transactions will execute in sequence if the second transaction is started after  
    /// the previous transaction was committed. Example: start A, commit A, start B commit B. 
    /// They will execute in parallel if the second transaction is started before the previous 
    /// transaction is committed. For instance start A, start B, commit A, commit B.
    ///
    /// \return A list of pending transactions in the order they were started on the client.
    virtual IServer_transaction_list* get_pending_transactions() = 0;
};

/// Abstract interface that can be used to control which sessions to a Bridge application are
/// established, based on credentials supplied by the client.
///
/// \see #mi::bridge::IApplication::set_session_handler()
class IApplication_session_handler : public
    mi::base::Interface_declare<0x8913c078,0xeba2,0x4e0b,0x83,0x44,0x1,0xcb,0x2b,0x57,0x67,0x3c>
{
public:
    /// This method is called when a session to a Bridge application is to be established.
    ///
    /// Use #mi::bridge::IServer_session::get_security_token() to obtain the security token string
    /// supplied by the client when connecting. This token can be used to decide whether the client
    /// is allowed to connect.
    ///
    /// \param session   The session that is to be established.
    /// \return          \c true to accept the session, \c false to reject it.
    virtual bool on_session_connect( IServer_session* session) = 0;
};

/// This class represents a Bridge application.
///
/// When a client opens a session to a Bridge server it also specifies the application it wants to
/// connect to. The features of the application are defined by the Bridge jobs registered with the
/// application.
///
/// \see #mi::bridge::IBridge_server::create_application()
class IApplication : public
    mi::base::Interface_declare<0x84c2d806,0x6e1f,0x402d,0xb2,0xa,0x2f,0xcf,0x47,0xd1,0xf,0x2e>
{
public:
    /// Registers a Bridge job with the application.
    ///
    /// All jobs to be executed via the Bridge need to be registered with the corresponding
    /// application.
    ///
    /// \see #unregister_job()
    ///
    /// \param job_class_id   The class ID of the job. You can simply pass IID() of your class
    ///                       derived from #mi::bridge::IServer_job.
    /// \param factory        The factory method of the job.
    /// \return               \c 0 in case of success, \c <0 in case of failure.
    virtual Sint32 register_job(
        const base::Uuid& job_class_id, neuraylib::IUser_class_factory* factory) = 0;

    /// Registers a Bridge job with the application.
    ///
    /// All jobs to be executed via the Bridge need to be registered with the corresponding
    /// application.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience. It uses the default class factory #mi::neuraylib::User_class_factory
    /// specialized for T.
    ///
    /// \see #unregister_job()
    ///
    /// \return               \c 0 in case of success, \c <0 in case of failure.
    template <class T>
    Sint32 register_job()
    {
        mi::base::Handle<mi::neuraylib::IUser_class_factory> factory(
            new neuraylib::User_class_factory<T>());
        return register_job( typename T::IID(), factory.get());
    }

    /// Unregisters a Bridge job with the application.
    ///
    /// \see #register_job()
    ///
    /// \param job_class_id   The class id of the job.
    /// \return               \c 0 in case of success, \c <0 in case of failure.
    virtual Sint32 unregister_job( const base::Uuid& job_class_id) = 0;

    /// Unregisters a Bridge job with the application.
    ///
    /// This templated member function is a wrapper of the non-template variant for the user's
    /// convenience.
    ///
    /// \see #register_job()
    ///
    /// \return               \c 0 in case of success, \c <0 in case of failure.
    template <class T>
    Sint32 unregister_job()
    {
        return unregister_job( typename T::IID());
    }

    /// Sets the disk cache to use.
    ///
    /// \note The disk cache must be set before the application can be opened for client sessions.
    ///       The disk cache can not be changed after it has been successfully set.
    ///
    /// \see #get_disk_cache(), #mi::neuraylib::ICache_manager_factory
    ///
    /// \param location   The location of the disk cache. This can be either a directory on the
    ///                   local machine (prefix \c "path:") or the address of some cache manager
    ///                   (prefix \c "address:").
    /// \return
    ///                   -     0: Success.
    ///                   -    -1: Invalid argument (\p disk_cache is \c NULL or has an incorrect
    ///                            format).
    ///                   -    -2: No disk cache found at the specified location.
    ///                   -    -3: The disk cache was already set.
    ///                   - <= -4: Unspecified error.
    virtual Sint32 set_disk_cache( const char* location) = 0;

    /// Returns the disk cache location.
    ///
    /// \see #set_disk_cache()
    virtual const char* get_disk_cache() const = 0;

    /// Opens the application so that clients can open sessions to it.
    ///
    /// \return
    ///           -     0: Success
    ///           -    -1: Mandatory configuration is incomplete (\see #set_disk_cache()).
    ///           -    -2: The application is already open.
    ///           - <= -3: Unspecified error.
    virtual Sint32 open() = 0;

    /// Closes the application and any open sessions.
    ///
    /// \return
    ///          -     0: Success.
    ///          -    -1: The application is already closed.
    ///          - <= -2: Unspecified error.
    virtual Sint32 close() = 0;

    /// Indicates whether the application is open, i.e., whether it accepts client sessions.
    virtual bool is_open() = 0;

    /// Sets the session handler that will be called when clients connect.
    ///
    /// The session handler will be called as part of the handshake between client and server to
    /// decide whether to accept or to reject the client, e.g., based on a security token supplied
    /// by the client. The server will always accept clients if no session handler is set (default
    /// behavior). The session handler can also be used to keep track of which sessions are
    /// connected to the application and their state by adding an
    /// #mi::bridge::IServer_session_state_callback to the session.
    ///
    /// \param handler The handler to be set or \c NULL to remove the current handler.
    virtual Sint32 set_session_handler( IApplication_session_handler* handler) = 0;

    /// Returns the current session handler, or \c NULL if no session handler has been set.
    virtual IApplication_session_handler* get_session_handler() const = 0;

    /// Sets the maximum verbosity of log messages that will be forwarded to the client.
    ///
    /// Note that the log level that will be forwarded is requested by the client, and this setting
    /// will only override the client request in case the server needs to restrict logging further,
    /// for instance because of security considerations. So if the client requests that info level
    /// messages and more severe will be forwarded and the server sets the limit to warning
    /// severity, then only warning log messages will be forwarded.
    ///
    /// Defaults to #mi::base::details::MESSAGE_SEVERITY_DEBUG which will allow the client to
    /// request forwarding of all log messages. Set to #mi::base::details::MESSAGE_SEVERITY_FATAL
    /// to turn of log forwarding.
    ///
    /// \see #get_log_forwarding_limit()
    ///
    /// \param limit    The log message limit to set.
    virtual Sint32 set_log_forwarding_limit( base::Message_severity limit) = 0;

    /// Returns the maximum verbosity of log messages that will be forwarded to the client.
    ///
    /// \see #set_log_forwarding_limit()
    ///
    /// \return         The current log forwarding limit
    virtual base::Message_severity get_log_forwarding_limit() const = 0;
};

/// Context to import, export, or remove snapshots.
///
/// \see #mi::bridge::IBridge_server::create_snapshot_context()
class IBridge_snapshot_context : public
    mi::base::Interface_declare<0x9a9ceafe,0x876b,0x4647,0xbc,0xae,0xb6,0x4,0x9c,0x7b,0x9,0xfc>
{
public:
    /// Imports a previously saved snapshot from the disk cache.
    ///
    /// \if IRAY_API This method is similar to using #mi::neuraylib::IImport_api::import_elements(),
    /// but will load a previously saved snapshot from the disk cache. \endif
    ///
    /// In the case of incremental snapshots the base snapshot must be loaded first, and then each
    /// incremental snapshot in sequence.
    /// 
    /// Currently no importer options are supported.
    ///
    /// \param transaction        The transaction into which to import the elements.
    /// \param snapshot_id        The ID of the snapshot to import.
    /// \param importer_options   Optional importer options.
    /// \return                   Result of the import operation with the following error codes:
    ///                           -       0: Success.
    ///                           -    4001: Invalid parameters.
    ///                           -    4002: Failed to open the disk cache.
    ///                           -    4003: There is no snapshot with the given ID in this disk
    ///                                      cache.
    ///                           -    4004: Snapshot has an incompatible Bridge version.
    ///                           -    4005: Failed to import the snapshot elements.
    ///                           - >= 4006: Unspecified error.
    virtual neuraylib::IImport_result* import_snapshot(
        neuraylib::ITransaction* transaction,
        const char* snapshot_id,
        const IMap* importer_options = 0) = 0;

    /// Removes a snapshot from the disk cache.
    ///
    /// \param snapshot_id        The ID of the snapshot to remove.
    /// \return
    ///                           -     0: Success.
    ///                           -    -1: Invalid parameters.
    ///                           -    -2: Failed to open the disk cache.
    ///                           -    -3: There is no snapshot with the given ID in this disk
    ///                                    cache.
    ///                           - <= -4: Unspecified error
    virtual Sint32 remove_snapshot( const char* snapshot_id) = 0;

    /// Exports a set of elements to the disk cache.
    ///
    /// \if IRAY_API This method is similar to using #mi::neuraylib::IExport_api::export_elements(),
    /// but will save a snapshot in the disk cache specified. \endif
    ///
    /// The following optional exporter options are supported:
    /// - \c "recurse" of type #mi::IBoolean: If \c true, any elements referenced by the elements
    ///   in the \p elements array are exported as well. Default: true.
    ///
    /// \param transaction         The transaction to use for the export.
    /// \param elements            The elements to export. Only supports element names as IString.
    /// \param exporter_options    The exporter options.
    /// \param[in,out] snapshot_id The ID of the snapshot to use. If an empty string is passed an 
    ///                            automatically generated unique identifier will be assigned to 
    ///                            this string.
    /// \return                    Result of the export operation with the following error codes:
    ///                            -       0: Success.
    ///                            -    6001: Invalid parameters.
    ///                            -    6002: Disk cache IO error.
    ///                            -    6003: Failed to serialize a database element.
    ///                            -    6004: Failed to export snapshot.
    ///                            - >= 6005: Unspecified error.
    virtual neuraylib::IExport_result* export_snapshot(
        neuraylib::ITransaction* transaction,
        const IArray* elements,
        const IMap* exporter_options,
        IString* snapshot_id) = 0;
};

/// API component that serves as entry point for the server-side Bridge API.
///
/// Can be used to create Bridge application and a context to import or remove snapshots.
class IBridge_server : public
    mi::base::Interface_declare<0x1fd8a3ac,0xa70c,0x4273,0xa9,0x1a,0x67,0x57,0xdf,0xc7,0xa5,0xb>
{
public:
    /// Creates a Bridge application.
    ///
    /// The application will listen on the provided path on the given HTTP server for clients. The
    /// application has to be opened for connections before clients can open a session to it.
    ///
    /// \see #mi::bridge::IBridge_client::get_session()
    ///
    /// \param application_path   The path component of the WebSocket URL to the application. This
    ///                           identifier allows to run different Bridge applications on the same
    ///                           HTTP server. Note that the application path must begin with a
    ///                           slash.
    /// \param http_server        The HTTP server instance that handles WebSocket requests for
    ///                           this application.
    /// \return                   The created Bridge application, or \c NULL in case of failure
    ///                           (invalid arguments, \p application_path does not start with a
    ///                           slash, or an application for these arguments exists already).
    virtual IApplication* create_application(
        const char* application_path, http::IServer* http_server) = 0;

    /// Creates a snapshot context for importing or deleting snapshots.
    ///
    /// \see #mi::bridge::IServer_transaction::create_snapshot(),
    ///      #mi::bridge::IServer_transaction::create_incremental_snapshot_context()
    ///
    /// \param disk_cache    The location of the disk cache. This can be either a directory on the
    ///                      local machine (prefix \c "path:") or the address of some cache manager
    ///                      (prefix \c "address:").
    /// \return              The snapshot context.
    virtual IBridge_snapshot_context* create_snapshot_context( const char* disk_cache) = 0;

    /// Returns the Bridge protocol version string.
    virtual const char* get_bridge_protocol_version() const = 0;

    /// Returns the hash and serialized size for the provided element.
    ///
    /// \param transaction   The transaction to use when looking up the element.
    /// \param element       The name of the element.
    /// \param o_hash        The hash will be written to this IString on success.
    /// \param o_size        The serialized size in bytes will be assigned to this variable on
    ///                      success.
    /// \return
    ///                      -     0: Success.
    ///                      -    -1: Invalid arguments.
    ///                      -    -2: Failed to look up element with the provided name.
    ///                      - <= -2: Unspecified error.
    virtual Sint32 calculate_hash(
        neuraylib::ITransaction* transaction,
        const char* element,
        IString* o_hash,
        Size* o_size) = 0;
};

/*@}*/ // end group mi_neuray_bridge_server

} // namespace bridge

} // namespace mi

#endif // MI_NEURAYLIB_IBRIDGE_SERVER_H
