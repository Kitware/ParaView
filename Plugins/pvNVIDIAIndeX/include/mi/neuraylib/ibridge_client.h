/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief  Bridge client

#ifndef MI_NEURAYLIB_IBRIDGE_CLIENT_H
#define MI_NEURAYLIB_IBRIDGE_CLIENT_H

#include <mi/base/enums.h>
#include <mi/base/interface_implement.h>
#include <mi/neuraylib/iserializer.h>

namespace mi {

class IMap;
class IString;

namespace base { class ILogger; }

namespace neuraylib {
class IDeserializer;
class ITransaction;
class IScope;
class IProgress_callback;
class ITag_set;
}

/// Namespace for the Bridge functionality of the \neurayApiName.
/// \ingroup mi_neuray
namespace bridge {

class IClient_video_context;

/** \if IRAY_API \defgroup mi_neuray_bridge Iray Bridge
    \else \defgroup mi_neuray_bridge Bridge
    \endif
    \ingroup mi_neuray

    The Bridge is a feature of \NeurayProductName that allows simple and efficient use of the
    resources of a remote cluster with minimal integration effort. Data is transferred to the remote
    cluster in a way suitable for comparably low-bandwidth, high-latency connections such as an
    Internet connection, where normal \NeurayProductName clustering would not work.

    The Bridge supports server-side caching of data and incremental changes to minimize the amount
    of data sent, as well as low-latency, hardware H.264 video to efficiently use the available
    bandwidth.

    \if IRAY_API Rendering using Iray Bridge is exposed as a render plugin with the same interface
    as local renderers. This means that an application will require minimal configuration to use the
    resources of a remote cluster. Iray Bridge can also be used to efficiently transfer scene data
    to a remote cluster to perform other tasks such as off-line batch rendering. \endif

    \see \ref mi_neuray_bridge_client, \ref mi_neuray_bridge_server
*/

/** \defgroup mi_neuray_bridge_client Bridge client
    \ingroup mi_neuray_bridge

    The Bridge client is integrated in the application that wishes to use the resources of the
    remote cluster. The client connects to a single Bridge server. The client does not form a
    cluster with the Bridge server. The server can be part of a normal cluster of any size, which
    automatically makes the resources of the entire remote cluster available to the client. The
    client can be part of a cluster of any size, but the resources of the client-side cluster are
    used only for local jobs, not when executing jobs using a remote cluster via the Bridge.

    \if IRAY_API The API consists of two layers: a low-level API, called \em Bridge \em API, and a
    high-level API, called \em Iray \em Bridge \em API. The low-level Bridge API is generic and
    independent of domain-specific tasks like rendering. It exposes the full flexibility of the
    Bridge. The high-level Iray Bridge API is specific to rendering. It encapsulates the details of
    the Bridge API and exposes them in a simpler way sufficient for rendering. Using the high-level
    API is strongly recommended unless there are specific needs which require the use of the
    low-level API. See the API component #mi::bridge::IBridge_client for the Bridge API, and
    #mi::bridge::IIray_bridge_client for the Iray Bridge API. \else See the API component
    #mi::bridge::IBridge_client for a starting point. \endif

    \see \ref mi_neuray_bridge, \ref mi_neuray_bridge_server
*/

/** \addtogroup mi_neuray_bridge_client
@{
*/

/// Job states of Bridge jobs.
///
/// Execution of Bridge jobs often involves uploads to the server which can take a considerable
/// time. Each Bridge job goes through a sequence of states defined by this enum, starting in the
/// state #CLIENT_JOB_DETECTING_CHANGES and then progressing to the next state when some conditions
/// are met until the job execution is done and the job ends up in the state #CLIENT_JOB_DONE.
/// States will always progress to the next state in the defined order, but some states might be
/// skipped if there is no work that needs to be done in that state. Note that a lot of work is
/// done in parallel so while the job is still determining which elements that needs to be
/// updated, it will also query the cache status and start uploading data for cache misses, etc.
///
/// \see mi::bridge::IClient_job_progress::get_state()
enum Client_job_state
{
    /// Detecting which database elements needs to be updated before executing the job.
    CLIENT_JOB_DETECTING_CHANGES = 0,

    /// Calculates hashes for the elements that will be updated and to some extent serialize data
    /// that will be sent inline.
    CLIENT_JOB_PREPARING = 1,

    /// Determining how much data needs to be uploaded before executing this job.
    ///
    /// The elements that need to be updated is now known, but there are still pending cache status
    /// requests. The job might not enter this state if no cache status requests were needed, or if
    /// all requests were already answered while in state #CLIENT_JOB_PREPARING.
    CLIENT_JOB_QUERYING_CACHE_STATUS = 2,

    /// Uploading data.
    ///
    /// There are no pending cache status requests left and it is known how much data needs to be
    /// uploaded before executing the job. The job does not enter this state if there were no cache
    /// misses or if all data was already uploaded in previous states.
    CLIENT_JOB_UPLOADING = 3,

    /// Waiting for execution of the job to finish.
    ///
    /// All data needed by the job is now uploaded and the job is either executing on the server or
    /// waiting for all previous jobs to finish before it can start executing. To get progress
    /// feedback in this state it is possible for the server-side job to send partial results back
    /// to the client.
    CLIENT_JOB_PENDING = 4,

    /// The result has been received and deserialized by the client-side job and the job is done.
    CLIENT_JOB_DONE = 5,

    CLIENT_JOB_FORCE_32_BIT = 0xffffffffU
};

mi_static_assert( sizeof( Client_job_state) == sizeof( Uint32));

/// Provides detailed information about the progress of a Bridge job.
class IClient_job_progress : public
    mi::base::Interface_declare<0x7f51a745,0xfcf0,0x4b2d,0x92,0x93,0x2b,0x84,0xd7,0x8c,0xc0,0xe6>
{
public:
    /// Returns the state of the job.
    ///
    /// See #Client_job_state.
    virtual Client_job_state get_state() const = 0;

    /// Returns the number of elements that need to be updated before executing this job.
    ///
    /// In state #CLIENT_JOB_DETECTING_CHANGES this is the number of elements for which changes
    /// have been detected so far. In all other states it is the final number of elements that
    /// needs to be updated.
    virtual Size get_updated_element_count() const = 0;

    /// Returns the number of elements for which hashes needs to be calculated.
    ///
    /// Can be larger than 0 in states up to and including #CLIENT_JOB_PREPARING
    /// and is 0 in all the other states.
    virtual Size get_pending_hash_calculation_count() const = 0;

    /// Returns the number of cache status requests that the server has not yet replied to.
    ///
    /// Can be larger than 0 in states up to and including #CLIENT_JOB_QUERYING_CACHE_STATUS
    /// and is 0 in all the other states.
    virtual Size get_pending_cache_status_count() const = 0;

    /// Returns the total number of cache misses for which data needs to be uploaded.
    ///
    /// The final value is known in the state #CLIENT_JOB_UPLOADING and later.
    virtual Size get_cache_miss_count() const = 0;

    /// Returns the total amount of bytes to upload for all cache misses.
    ///
    /// The final value is known in the state #CLIENT_JOB_UPLOADING and later.
    virtual Size get_cache_miss_bytes() const = 0;

    /// Returns the number of cache misses for which data has been uploaded so far.
    ///
    /// Will be equal to #get_cache_miss_count() in states #CLIENT_JOB_PENDING and later.
    virtual Size get_uploaded_cache_miss_count() const = 0;

    /// Returns the number of bytes that has been uploaded for the cache misses so far.
    ///
    /// Will be equal to #get_cache_miss_bytes() in states #CLIENT_JOB_PENDING and later.
    virtual Size get_uploaded_cache_miss_bytes() const = 0;

    /// Returns the number of elements that have been queued up for serialization. The data
    /// needs to be serialized before it can be sent.
    ///
    /// Can be larger than 0 in states up to and including #CLIENT_JOB_UPLOADING
    /// and is 0 in all later states.
    virtual Size get_pending_data_serialization_count() const = 0;

    /// Returns the name of the currently uploaded element or \c NULL if no element is
    /// currently being uploaded.
    virtual const char* get_currently_uploaded_element_name() const = 0;

    /// Returns the size in bytes of the currently uploaded element or 0 if no element is
    /// currently being uploaded.
    virtual Size get_currently_uploaded_element_size() const = 0;

    /// Returns the number of bytes uploaded for the currently uploaded element or 0 if
    /// no element is currently being uploaded.
    virtual Size get_currently_uploaded_element_uploaded_bytes() const = 0;
};

/// Used to specify a set of elements by name.
///
/// \see #mi::bridge::IClient_job::get_references()
class IElement_set : public
    mi::base::Interface_declare<0x2242471d,0x96f3,0x4962,0x84,0x7e,0xd1,0x20,0xa9,0x6b,0xb6,0x98>
{
public:
    /// Returns the number of elements in the set.
    ///
    /// \see #get_element()
    virtual Size get_length() const = 0;

    /// Returns the \p index -th element of the element set.
    ///
    /// \see #get_length()
    virtual const char* get_element( Size index) const = 0;

    /// Adds \p element to the tag set.
    virtual void add_element( const char* element_name) = 0;
};

/// Represents the client-side part of a job that can be executed by the Bridge server.
///
/// The corresponding server-side part of the job must return the same ID from
/// #mi::neuraylib::ISerializable::get_class_id() and be registered as a Bridge job on the
/// server.
///
/// The client-side part must implement #mi::neuraylib::ISerializable::serialize() to serialize the
/// data that is received by #mi::bridge::IServer_job::deserialize(), and
/// #mi::bridge::IClient_job::receive_remote_result() to deserialize the result of the job which is
/// produced by #mi::bridge::IServer_job::execute().
///
/// It is recommended to derived your implementation from #mi::bridge::Client_job and overriding
/// only the required methods.
///
/// \see #mi::bridge::IClient_session::execute()
class IClient_job : public
    mi::base::Interface_declare<0xe02e4aeb,0x6edd,0x4e40,0xbb,0xe0,0x6c,0xc5,0xe7,0x69,0xa6,0xe,
    neuraylib::ISerializable>
{
public:
    /// Returns the database elements that will be used by this job on the server, identified by
    /// a set of tags.
    ///
    /// The Bridge will guarantee that any elements returned by this method and any elements
    /// indirectly referenced by them will be updated in the server-side cache and database before
    /// this Bridge job executes on the server.
    ///
    /// \note The elements identified by the overload #get_references(IElement_set*)const will also
    ///       be updated.
    ///
    /// \see #upload_only()
    ///
    /// \param[out] result   The set of tags referenced by the Bridge job. The passed-in tag set is
    ///                      empty.
    virtual void get_references( neuraylib::ITag_set* result) const = 0;

    /// Returns the database elements that will be used by this job on the server, identified by
    /// a set of names.
    ///
    /// The Bridge will guarantee that any elements returned by this method and any elements
    /// indirectly referenced by them will be updated in the server-side cache and database before
    /// this Bridge job executes on the server.
    ///
    /// \note The elements identified by the overload #get_references(neuraylib::ITag_set*)const
    ///       will also be updated.
    ///
    /// \see #upload_only()
    ///
    /// \param[out] result   The set of element names referenced by the Bridge job. The passed-in
    ///                      element set is empty.
    virtual void get_references( IElement_set* result) const = 0;

    /// Indicates whether the referenced elements should really be updated in the server-side
    /// database.
    ///
    /// Usually, this method returns \c false, meaning that referenced elements are updated in the
    /// server-side cache and database. In some circumstances it is not necessary to update the
    /// server-side database because only the cache is needed, e.g., when saving snapshots. In this
    /// case, one can optimize the update process by returning \c true, meaning that only the server
    /// side cache but not the database are updated before executing the job.
    ///
    /// \see #get_references()
    virtual bool upload_only() const = 0;

    /// Deserializes the job result from the execution of the job on the server.
    ///
    /// The execution of the job might produce intermediate results by flushing the data serialized
    /// so far. Each call to #mi::neuraylib::ISerializer::flush() on the server will result in a
    /// single call to #receive_remote_result() with the \c last_result flag set to \c false, even
    /// if no data was flushed. When the execution on the server finishes this method will be called
    /// one last time with the \c last_result flag set to \c true with the remaining data.
    ///
    /// See #mi::bridge::IServer_job::execute() for more information and a couple of example use
    /// cases.
    ///
    /// \param deserializer   The deserializer from which to read the (partial) result.
    /// \param last_result    \c true if this is the last chunk of result data the job will produce,
    ///                       \c false otherwise.
    virtual void receive_remote_result(
        neuraylib::IDeserializer* deserializer, bool last_result) = 0;

    /// A callback that provides progress information.
    ///
    /// This method is called whenever the state of the job changes and also periodically if the job
    /// stays in a state for a long time. During execution of the job, custom progress information
    /// can be encoded into chunks of result data.
    ///
    /// \see #Client_job_state, #receive_remote_result(), IServer_job::execute()
    ///
    /// \param job_progress   An instance that contains all progress information. The instance may
    ///                       only be used from within this callback.
    virtual void progress_callback( IClient_job_progress* job_progress) = 0;

    /// A callback that indicates an error during job execution or if the job was canceled.
    ///
    /// \param error_code
    ///                     -     0: The job was explicitly canceled or the transaction was aborted.
    ///                     -    -1: Network error.
    ///                     -    -2: Corresponding server side job does not exist.
    ///                     - <= -3: Unspecified error.
    /// \param msg          A short explanation of the error.
    virtual void error_callback( Sint32 error_code, const char* msg) = 0;
};

/// This mixin class provides a default implementation for some of the methods needed by
/// #mi::bridge::IClient_job.
///
/// It is recommended to derive from this class rather than from #mi::bridge::IClient_job directly.
template <Uint32 i_id1, Uint16 i_id2, Uint16 i_id3
, Uint8 i_id4, Uint8 i_id5, Uint8 i_id6, Uint8 i_id7
, Uint8 i_id8, Uint8 i_id9, Uint8 i_id10, Uint8 i_id11
, class I = IClient_job>
class Client_job : public base::Interface_implement<I>
{
public:
    /// Own type.
    typedef Client_job<i_id1,i_id2,i_id3,i_id4,i_id5,i_id6,i_id7,i_id8,i_id9,i_id10,i_id11,I> Self;

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
    virtual void deserialize( neuraylib::IDeserializer* deserializer)
    {
        // avoid warnings
        (void) deserializer;
    }

    /// Empty body. The default job references no elements.
    virtual void get_references( neuraylib::ITag_set* result) const
    {
        // avoid warnings
        (void) result;
    }

    /// Empty body. The default job references no elements.
    virtual void get_references( IElement_set* result) const
    {
        // avoid warnings
        (void) result;
    }

    /// Returns \c false in the default implementation.
    virtual bool upload_only() const { return false; }
};

/// Base class for Bridge jobs that only update elements without executing anything.
///
/// Derived jobs with the interface ID of this class do not need a server-side counterpart. This job
/// will work the same way as a normal Bridge job with a corresponding server-side job
/// implementation that does not execute anything and sends back an empty result.
class Update_job : public mi::bridge::Client_job<
    0xf9b3c8e2,0x7688,0x4bf8,0x91,0x36,0xcd,0x3a,0x3f,0x51,0x30,0x7a>
{
    void serialize( neuraylib::ISerializer* serializer) const
    {
        // avoid warnings
        (void) serializer;
    }

    void receive_remote_result( neuraylib::IDeserializer* deserializer, bool last_result)
    {
        // avoid warnings
        (void) deserializer;
        (void) last_result;
    }
};


/// The different states a client session can be in.
///
/// \see #mi::bridge::IClient_session::get_state()
enum Client_session_state
{
    /// The session is trying to establish a connection to the remote Bridge application for the
    /// first time.
    CLIENT_SESSION_CONNECTING,

    /// The session has successfully established a connection to the Bridge server and is ready for
    /// use.
    CLIENT_SESSION_CONNECTED,

    /// The session was disconnected unexpectedly.
    ///
    /// The session cannot be used in this state, but will automatically attempt to reconnect and
    /// resume the session. Session resume is only possible if the corresponding server-side session
    /// is still available when reconnecting, if not the session will be closed.
    ///
    /// Note that session resume is currently not supported so disconnected sessions will always
    /// be closed.
    CLIENT_SESSION_PENDING,

    /// The session has been closed in an orderly fashion, or the session has been pending but the
    /// session could not be resumed.
    ///
    /// Once a session has been closed it can not be used anymore. To attempt a reconnection a new
    /// session must be opened.
    CLIENT_SESSION_CLOSED
};

mi_static_assert( sizeof( Client_session_state) == sizeof( Uint32));

/// Abstract interface for callbacks for session state changes.
///
/// \see #mi::bridge::IClient_session::add_session_state_callback(),
///      #mi::bridge::IClient_session::remove_session_state_callback()
class IClient_session_state_callback : public
    mi::base::Interface_declare<0x9ea6d3b3,0x3d0b,0x4d10,0x89,0xc8,0x89,0xf1,0x20,0x49,0xc6,0xe1>
{
public:
    /// This method is called whenever the session changes its state.
    virtual void session_state_callback( Client_session_state state) = 0;
};

/// Abstract interface for bandwidth measurement events.
///
/// \see #mi::bridge::IClient_session::measure_bandwidth()
class IClient_measure_bandwidth_callback : public
    mi::base::Interface_declare<0x80cf9612,0x3fd4,0x4ed0,0xb1,0x63,0xc0,0xd9,0x84,0xe2,0x6,0xaa>
{
public:
    /// This method is called when the bandwidth measurement starts and then when there is progress.
    ///
    /// \param bytes_done    The number of bytes sent/received so far.
    /// \param total_bytes   The total number of bytes to send/receive.
    virtual void progress_callback( Size bytes_done, Size total_bytes) = 0;

    /// This method is called when the bandwidth measurement is done.
    ///
    /// \param total_bytes   Total number of bytes sent/received.
    /// \param total_time    The time that elapsed on the receiving side between receiving the
    ///                      beginning of the first packet and the end of the last packet.
    virtual void done_callback( Size total_bytes, Float64 total_time) = 0;

    /// This method is called if there is an error preventing the test from finishing.
    ///
    /// \param error_code
    ///                      -    -1: Network error.
    ///                      - <= -2: Unspecified error.
    virtual void error_callback( Sint32 error_code) = 0;
};

/// Represents the client side of a Bridge session.
///
/// A session can be opened by #mi::bridge::IBridge_client::get_session(). Note that repeated calls
/// of that method with the same arguments return the same session, i.e., sessions are shared
/// between different callers. Sharing the same session is vital for features such as incremental
/// changes, and to share other resources that would otherwise be duplicated. The session will be
/// destroyed and client- and server-side resources will be released when all references to the
/// session has been released.
///
/// The session will automatically connect to the server. Use #add_session_state_callback() for
/// callbacks about session state changes. A closed session can not be used anymore. Use
/// #mi::bridge::IBridge_client::get_session() to attempt to reconnect.
///
/// \see #mi::bridge::IBridge_client::get_session()
class IClient_session : public
    mi::base::Interface_declare<0x70bb8712,0x1305,0x4c76,0xb9,0x38,0xad,0x27,0x91,0xea,0xbd,0x78>
{
public:
    /// Returns the state of the session.
    /// \see #mi::bridge::Client_session_state
    virtual Client_session_state get_state() = 0;

    /// Adds a session state callback.
    ///
    /// When adding a callback it will be called immediately once with the current state, and then
    /// every time the session state changes.
    ///
    /// \see #remove_session_state_callback()
    ///
    /// \param callback    The callback to be added.
    virtual void add_session_state_callback( IClient_session_state_callback* callback) = 0;

    /// Removes a previously added session state callback.
    ///
    /// \see #add_session_state_callback()
    ///
    /// \param callback    The callback to be removed.
    virtual void remove_session_state_callback( IClient_session_state_callback* callback) = 0;

    /// Schedules the provided job for execution on the server and returns immediately.
    ///
    /// Jobs are executed asynchronously, but sequentially (per transaction) on the server in the
    /// same order as execution is requested by this method. A job is only executed after its
    /// referenced database elements have been updated from the Bridge client to the Bridge server.
    /// 
    /// Note that the same IClient_job instance must not be executed several times in parallel. 
    /// This is true both within a single transaction and in multiple transactions.
    ///
    /// \param job   The job to execute.
    /// \param transaction The transaction in which context the job will be executed.
    /// \return
    ///              -     0: Success.
    ///              -    -1: Invalid arguments (\p job or transaction is \c NULL).
    ///              -    -2: Invalid transaction state (already committed or aborted).
    ///              -    -3: Network error.
    ///              -    -4: The job reference elements that can't be accessed.
    ///              - <= -5: Unspecified error.
    virtual Sint32 execute(IClient_job* job, neuraylib::ITransaction* transaction) = 0;

    /// Marks the provided job to be canceled and returns immediately. A job progress callback
    /// will be made to inform when the job is canceled or completed. Note that it is not always 
    /// possible to cancel a job depending on the timing so a job can still be completed 
    /// even after successfully calling this method.
    ///
    /// Known limitation: Canceling jobs doing uploads doesn't cancel the upload itself and 
    /// even if the job is canceled successfully some or all uploaded elements might be
    /// stored in the database. To make sure that elements from a job is not persisted in 
    /// the server side database the transaction must be aborted. This will automatically 
    /// cancel all Bridge jobs running in that transaction.
    /// 
    /// \param job   The job to cancel.
    /// \return
    ///              -     0: Job canceled.
    ///              -     1: Job not executing or already canceled.
    ///              -    -1: Invalid arguments (\p job is \c NULL).
    ///              -    -3: Network error.
    ///              - <= -4: Unspecified error.
    virtual Sint32 cancel(IClient_job* job) = 0;

    /// Creates a new video context.
    ///
    /// The video context will be assigned an id which needs to be transferred to the server, along
    /// with any other data required to set up the video source properly. The server use this id to
    /// get the corresponding server-side video context which can be used to stream video frames to
    /// the client. The recommended way of transferring the id to the server is by executing a
    /// Bridge job.
    ///
    /// \return The created video context, or \c NULL in case of failure.
    virtual IClient_video_context* create_video_context() = 0;

    /// Measures the bandwidth to the Bridge server.
    ///
    /// The bandwidth is measured by sending a specified number of packages with a specified size in
    /// bytes. This method is asynchronous, the provided callback interface will be called when
    /// there is progress and when the measurement is done.
    ///
    /// \note For meaningful results the method should only be called when the session is not used
    /// to upload or download other real data at the same time.
    ///
    /// \param progress_callback   A callback interface that will be called with progress
    ///                            information.
    /// \param upload              \c true for measuring upload bandwidth, \c false for measuring
    ///                            download bandwidth.
    /// \param package_size        The size in bytes of a single package.
    /// \param package_count       The number of packages to upload or download.
    /// \return                    0 if the measurement was successfully started, -1 otherwise.
    virtual Sint32 measure_bandwidth(
        IClient_measure_bandwidth_callback* progress_callback,
        bool upload,
        Uint32 package_size,
        Uint32 package_count = 1) = 0;

    /// Sets the receiving logger for log messages forwarded from the Bridge server.
    ///
    /// If the provided logger is \c NULL then forwarded log entries will be written to the general
    /// configured logger as if they were generated locally, but with a prefix identifying the
    /// server that they originated from (default behavior).
    ///
    /// \see #set_receiving_logger()
    ///
    /// \param logger      The receiving logger. It is valid to pass \c NULL in which case logging
    ///                    is reset to be done to the general configured logger.
    virtual void set_receiving_logger( base::ILogger* logger) = 0;

    /// Returns the receiving logger for log messages forwarded from the Bridge server.
    ///
    /// Note that if no receiving logger has been set, this method returns \c NULL and logging is
    /// done using the currently general configured logger.
    ///
    /// \see #set_receiving_logger()
    virtual base::ILogger* get_receiving_logger() const = 0;

    /// Sets the log level of messages that are forwarded from the Bridge server.
    ///
    /// Note that this does not affect the log level of the server, nor the log level of the client.
    /// It only filters the messages forwarded by the server to the client. Also note that the
    /// server will never send out log messages more verbose than the configured limit for the
    /// bridge application the session is connected to.
    ///
    /// \see #get_forwarding_log_level()
    virtual Sint32 set_forwarding_log_level( base::Message_severity level) = 0;

    /// Returns the currently set log level for messages forwarded from the server.
    ///
    /// \see #set_forwarding_log_level
    virtual base::Message_severity get_forwarding_log_level() const = 0;

    /// Returns the number of bytes written to the server so far.
    ///
    /// \see #get_bytes_read()
    virtual Size get_bytes_written() const = 0;

    /// Returns the number of bytes read from the server so far.
    ///
    /// \see #get_bytes_written()
    virtual Size get_bytes_read() const = 0;

    /// Allows the data for specific elements to be overridden by specifying the hash of the 
    /// data to use regardless of the actual data stored in the db. The replacements will only be
    /// done when executing a bridge job that returns the provided top_level_element in its 
    /// get_references() method and only when executed in the same scope as the scope of the 
    /// provided transaction. When an edited element is detected for the job the overrides
    /// map will be consulted to see if an override for the element with that name exists. If 
    /// it does the element won't be serialized and the override hash from the map will
    /// be used instead. The server then use the hash when loading the data from the cache, meaning
    /// that it can be different than the actual data on the client. For this to work, the data
    /// for the hash must be guaranteed to already be in the server side cache. This can be 
    /// achieved by storing snapshots in the server side cache with all required data before 
    /// rendering the scene containing hash overrides.
    ///
    /// \param overrides An IMap with the name of the element to override as key and the hash
    ///                  as string value. The map must be of type Map\<String\>. Call this 
    ///                  method with a \c NULL override map to release it or replace it with a 
    ///                  new one. The map instance is retained by bridge to avoid expensive copying
    ///                  and must not be modified after this call.
    /// \param transaction The hash overrides will be applied only for jobs executing in the same
    ///                    scope as this transaction. The top level element must be visible in 
    ///                    this transaction.
    /// \param top_level_element The top level element. The overrides applies to this element 
    ///                          and all elements it reference when executing a job that returns
    ///                          the specified top level element in its get_references() method.
    /// \return
    ///              -     0: Success.
    ///              -    -1: Invalid arguments (\p job or transaction is \c NULL).
    ///              -    -2: Invalid transaction state (already committed or aborted).
    ///              - <= -2: Unspecified error.
    virtual Sint32 set_hash_overrides(
        const IMap* overrides, 
        const char* top_level_element,
        neuraylib::ITransaction* transaction) = 0;
    
};

/// API component that serves as entry point for the client-side Bridge API.
///
/// Can be used to create sessions to a Bridge application.
class IBridge_client : public
    mi::base::Interface_declare<0xbe270827,0xad68,0x4044,0x94,0x6e,0x9d,0x41,0x4d,0xf,0x75,0x65>
{
public:
    /// Returns a session to the provided Bridge application running on a Bridge server.
    ///
    /// Bridge supports session sharing, so subsequent calls to this method with the same
    /// URL and security token will return an already existing valid session. The session will
    /// be considered the same if the URL and security token strings match exactly, otherwise a
    /// new session will be created. If a session is closed then calling get_session() with the
    /// same URL and security token will create a new session and attempt a new connection to the
    /// server.
    ///
    /// The session will connect to the specified URL automatically. To find out if the session is
    /// connected or if the connection fails add a session state callback by calling
    /// #mi::bridge::IClient_session::add_session_state_callback(). This callback will be called
    /// immediately with the current state, and then when the state changes.
    ///
    /// \see #mi::bridge::IBridge_server::create_application()
    ///
    /// \param application_url  The WebSocket URL to the server-side Bridge application. If
    ///                         the HTTP server listens on host \c "somehost" and port 80, and the
    ///                         application path on the server side is set to \c "/myapp", then the
    ///                         client will connect using the URL \c "ws://somehost:80/myapp". If
    ///                         the connection is encrypted using the SSL protocol, then the prefix
    ///                         \c "wss" needs to be used instead of \c "ws". Note that the port
    ///                         must be specified. The standard port for WebSockets is 80 and 443
    ///                         for secure WebSockets.
    /// \param security_token   An optional security token that can be inspected by the server-side
    ///                         application to decide whether the session should be accepted or
    ///                         rejected.
    virtual IClient_session* get_session(
        const char* application_url, const char* security_token = 0) = 0;

    /// Returns the Bridge protocol version.
    virtual const char* get_bridge_protocol_version() const = 0;
};

/*@}*/ // end group mi_neuray_bridge_client

} // namespace bridge

} // namespace mi

#endif // MI_NEURAYLIB_IBRIDGE_CLIENT_H
