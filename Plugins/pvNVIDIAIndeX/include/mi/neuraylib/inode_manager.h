/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Node manager API

#ifndef MI_NEURAYLIB_INODE_MANAGER_H
#define MI_NEURAYLIB_INODE_MANAGER_H

#include <mi/base/interface_declare.h>

namespace mi
{

class IString;
class IMap;

namespace neuraylib
{

/// \defgroup mi_neuray_node_manager Node manager
/// \ingroup mi_neuray
///
/// This module represents the node manager, a service to control the formation of clusters of
/// worker nodes based on their properties. The node manager is part of the DiCE library and can be
/// used by any application integrating DiCE. In the following a client is an application based on
/// DiCE which wants to make use of additional worker nodes to offload work. The node manager allows
/// to allocate and manage those worker nodes.
///
/// For using the node manager, a node manager process must be running on the worker nodes to be
/// used by client applications to delegate work to them. This process running on the worker nodes
/// can be built based on the DiCE library, too. This library offers an API which allows to register
/// properties at runtime, including the possibility to change them dynamically. The node manager
/// process running on the worker nodes can for example detect local capabilities, e.g., the number
/// of available CPU cores, the number of GPUs, or the amount of physical memory present and set
/// them as properties of the worker node. Those and other arbitrarily chosen properties will be
/// announced by DiCE to the client nodes.
///
/// On the client nodes, the node manager API can be used to control formation and/or joining
/// clusters of worker nodes. This can happen before the start of the DiCE library and also later,
/// in order to add a cluster of worker nodes to a running application or to join an already running
/// cluster.
///
/// The application running on the client nodes has full control over which cluster to join or which
/// worker nodes to select for the formation of a cluster. This can be achieved by writing a custom
/// filter class to which DiCE offers eligible clusters respectively worker nodes along with their
/// properties which have been set by the node manager process running on the worker nodes. Such a
/// filter can then return either true or false. True is returned if the cluster respectively worker
/// node in question should be chosen, or false, otherwise. In addition a client application can
/// specify a minimum and maximum amount of worker nodes which need to be in the cluster for the
/// cluster creation to be successful.
///
/// Each cluster created using the node manager API is associated with a multicast address which is
/// automatically chosen and which can be passed to DiCE for forming a DiCE cluster. In addition to
/// that a command string which is used to start child processes on the worker nodes is associated
/// with the cluster.
///
/// A cluster can be shut down automatically when no client is using the cluster anymore. Shutting
/// down can also be delayed by a timeout which can be set by the client application. In addition it
/// is possible to shut down a cluster immediately, even if there are still client nodes using the
/// clusters or the timeout has not elapsed.
///
/// The node manager API allows a client node to form or join any number of clusters at the same
/// time or at different times.
///
/// The node manager can be operated in two network modes: multicasting and TCP networking with a
/// discovery host. Multicasting is the default. TCP networking can be used in network environments
/// where switches/routers do not allow UDP multicasting and establishing a connection between node
/// manager instances does not work. With TCP networking, a head node is used to allow node manager
/// instances to find each other. There can be only one head node and it needs to be the first
/// instance that is started. A node manager instance that is started in TCP mode and where the
/// address that follows is the local IP address will become the head node. Other nodes specify the
/// head node's IP address as well and will obtain the list of known nodes from there.
///
/// Keepalive PDU for the child process watchdog
///
/// struct keepalive {
///    int type;
///    int sequence_number;
/// };

/** \addtogroup mi_neuray_node_manager
@{
*/

/// This interface describes a worker node and its properties.
class IWorker_node_descriptor : public mi::base::Interface_declare<0x29a6d6a5, 0xfaa9, 0x48dc, 0x87,
                                  0xc5, 0xee, 0xa5, 0x83, 0x2c, 0xe9, 0xb3>
{
public:
  /// Returns the IP address of the worker node.
  virtual const IString* get_address() const = 0;

  /// Indicates whether the worker node is currently a member of a cluster.
  virtual bool is_in_cluster() const = 0;

  /// Returns the properties of the worker node.
  ///
  /// The type of the map is \c "Map<String>".
  ///
  /// The following properties are defined by default.
  /// - \c "dice_host_name" \n
  ///   The host name of the worker node.
  /// - \c "dice_number_of_cpus" \n
  ///   The number of CPUs of the worker node.
  /// - \c "dice_number_of_gpus" \n
  ///   The number of GPUs of the worker node.
  /// - \c "dice_platform_os" \n
  ///   The platform/operating system identifier of the worker node.
  /// - \c "dice_platform_version" \n
  ///   The version of \neurayProductName used by the worker node.
  ///
  /// Other properties with the name prefix \c dice are reserved for future use.
  virtual const IMap* get_properties() const = 0;
};

/// This interface describes a cluster and its properties.
class ICluster_descriptor : public mi::base::Interface_declare<0xf83a075b, 0xf3d1, 0x46a9, 0xb6,
                              0x75, 0x91, 0x10, 0x1c, 0xa3, 0x5a, 0x9f>
{
public:
  /// Returns the multicast address reserved for the cluster.
  ///
  /// This multicast address may be used for communication among the child processes running in
  /// that cluster. It may also be used by client nodes to communicate with these child
  /// processes.
  ///
  /// \return    The multicast address and port.
  virtual const IString* get_multicast_address() const = 0;

  /// Returns the keep-alive timeout for the cluster.
  ///
  /// The keep-alive timeout specifies the amount of time (in seconds) the cluster will remain
  /// operational after the last client left the cluster.
  ///
  /// \see #mi::neuraylib::INode_manager_cluster::set_timeout()
  virtual Float64 get_timeout() const = 0;

  /// Returns the properties of the cluster.
  ///
  /// The type of the map is \c "Map<String>".
  ///
  /// \return    A map containing the properties associated with the cluster.
  virtual const IMap* get_properties() const = 0;

  /// Returns the number of worker nodes in the cluster.
  virtual Size get_number_of_worker_nodes() const = 0;

  /// Returns a descriptor for a worker node in the cluster.
  ///
  /// \note The set of worker nodes in the cluster can change at any time. That is, this function
  ///       can return \c NULL even if \p index is smaller than the result of the last call to
  ///       #get_number_of_worker_nodes().
  ///
  /// \param index   The index of the worker node (from 0 to #get_number_of_worker_nodes()-1).
  /// \return        The descriptor for the specified worker node, or \c NULL if \p index is
  ///                out of bounds.
  virtual const IWorker_node_descriptor* get_worker_node(Size index) const = 0;

  /// Returns the head node.
  ///
  /// The head node is a cluster member flagged by the node manager for applications
  /// that need support for a head node. It can be referenced by a placeholder substring in the
  /// argument string passed to #mi::neuraylib::INode_manager_client::join_or_create_cluster().
  ///
  /// \see #mi::neuraylib::INode_manager_client::join_or_create_cluster() for how to reference the
  ///      head node in the argument string.
  ///
  /// \return        The descriptor for the head node, or \c NULL if no head node has been
  ///                flagged.
  virtual const IWorker_node_descriptor* get_head_node() const = 0;

  /// Returns the number of client nodes connected to the cluster.
  virtual Size get_number_of_client_nodes() const = 0;
};

/// Abstract interface for signaling changed cluster properties.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then be
/// registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
///
/// \see #mi::neuraylib::INode_manager_cluster
class ICluster_property_callback : public mi::base::Interface_declare<0x36a1317b, 0xfbc1, 0x4ef5,
                                     0xbf, 0x38, 0x57, 0x1a, 0xca, 0x6d, 0x22, 0x9f>
{
public:
  /// This function is called when a cluster property change was communicated.
  ///
  /// It should return as soon as possible because it may block further network operations.
  ///
  /// \param cluster_descriptor      The descriptor of the cluster, containing all properties.
  /// \param changed_property_name   The name of the changed property. This value is \c NULL if
  ///                                the callback is triggered to communicate the current set of
  ///                                properties instead of a property change.
  virtual void property_callback(
    const ICluster_descriptor* cluster_descriptor, const char* changed_property_name) = 0;
};

/// Abstract interface for signaling changed worker node properties.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then be
/// registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
///
/// \see #mi::neuraylib::INode_manager_cluster
class IWorker_node_property_callback : public mi::base::Interface_declare<0x3c14c356, 0xde2c,
                                         0x4991, 0x9a, 0x7f, 0x50, 0x53, 0x5d, 0x2a, 0x9f, 0x5a>
{
public:
  /// This function is called when a worker node property change was communicated.
  ///
  /// It should return as soon as possible because it may block further network operations.
  ///
  /// \param worker_descriptor       The descriptor of the worker node, containing all properties.
  /// \param changed_property_name   The name of the changed property. This value is \c NULL if
  ///                                the callback is triggered to communicate the current set of
  ///                                properties instead of a property change.
  virtual void property_callback(
    const IWorker_node_descriptor* worker_descriptor, const char* changed_property_name) = 0;
};

/// Abstract interface for signaling changed cluster members.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then be
/// registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
///
/// \see #mi::neuraylib::INode_manager_cluster
class IClient_node_callback : public mi::base::Interface_declare<0x441ca19c, 0xa7d7, 0x46fa, 0x92,
                                0xc3, 0x14, 0xe0, 0x4b, 0x66, 0x13, 0x55>
{
public:
  /// This function is called when a remote client joined or left the cluster.
  ///
  /// It should return as soon as possible because it may block further network operations.
  ///
  /// \param address          The address of the client joining or leaving.
  /// \param flag             \c true in case of a joining client, \c false in case of a leaving
  ///                         client.
  virtual void membership_callback(const char* address, bool flag) = 0;
};

/// Abstract interface for signaling changed cluster members.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then be
/// registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
///
/// \see #mi::neuraylib::INode_manager_cluster
class IWorker_node_callback : public mi::base::Interface_declare<0xd5472198, 0xf755, 0x4db8, 0x82,
                                0x49, 0x74, 0xf7, 0x95, 0xb5, 0x58, 0xee>
{
public:
  /// This function is called when a worker node joined or left the cluster.
  ///
  /// It should return as soon as possible because it may block further network operations.
  ///
  /// \param worker_descriptor  The descriptor of the worker node joining or leaving.
  /// \param flag               \c true in case of a joining worker node , \c false in case of a
  ///                           leaving worker node.
  virtual void membership_callback(IWorker_node_descriptor* worker_descriptor, bool flag) = 0;
};

/// Abstract interface for signaling a change of the cluster application head node.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then
/// be registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
///
/// \see #mi::neuraylib::INode_manager_cluster::get_head_node to retrieve the cluster head node.
class IHead_node_callback : public mi::base::Interface_declare<0xf07bba0e, 0x249f, 0x4c6d, 0x97,
                              0x57, 0x48, 0xa7, 0xf8, 0xe7, 0xe6, 0x5a>
{
public:
  /// This function is called when a cluster node becomes cluster head node. This should not be
  /// confused with the head node in TCP networking as it is unrelated.
  ///
  /// It should return as soon as possible because it may block further network operations.
  ///
  /// \param worker_descriptor  The descriptor of the cluster head node.
  virtual void head_node_callback(IWorker_node_descriptor* worker_descriptor) = 0;
};

class INode_manager_cluster;

/// Abstract interface for signaling a request to shutdown all clients and workers.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then
/// be registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
class IShutdown_node_managers_callback : public mi::base::Interface_declare<0x9e876854, 0x04a9,
                                           0x467f, 0x85, 0xe9, 0xa4, 0xb4, 0xd1, 0x2d, 0x28, 0x8d>
{
public:
  /// This function is called when a request to shutdown all clients and workers is received.
  ///
  /// It should return as soon as possible because it may block further network operations.
  virtual void shutdown_node_managers_callback() = 0;
};

/// Abstract interface for signaling a request to shutdown a cluster.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then
/// be registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
class IShutdown_cluster_callback : public mi::base::Interface_declare<0xadf38762, 0x86db, 0x4ba9,
                                     0x9d, 0xde, 0x1d, 0x13, 0xee, 0x85, 0xa0, 0x45>
{
public:
  /// This function is called when a request to shutdown a cluster is being received.
  ///
  /// It should return as soon as possible because it may block further network operations.
  virtual void shutdown_cluster_callback(const ICluster_descriptor* cluster) = 0;
};

/// Abstract interface for indicating that a worker process has been fully started.
///
/// Its aim is to be derived from by the application writer. The concrete implementation can then
/// be registered with the \neurayApiName and will after that be used by the \neurayLibraryName.
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
class IWorker_process_started_callback : public mi::base::Interface_declare<0x42b4ec43, 0x9562,
                                           0x42fc, 0x8b, 0x8d, 0xc8, 0x55, 0x6b, 0x5e, 0x50, 0x3c>
{
public:
  /// This function is called when the worker process on the node represented by the
  /// worker_descriptor has been fully started.
  ///
  /// It should return as soon as possible because it may block further network operations.
  virtual void worker_process_started_callback(IWorker_node_descriptor* worker_descriptor) = 0;
};

/// The interface to a cluster created and managed by the node manager.
///
/// As long as an application holds this interface, the application will be part of the cluster.
/// Releasing the last handle to this interface gives up cluster membership. When that happens,
/// the node manager will check if there are no more clients. If there are still clients, the
/// cluster will be kept alive. If there are none, then the optional timeout will be started.
/// After the timeout elapsed without another client joining, or immediately if there is no timeout,
/// the cluster will be shut down.
class INode_manager_cluster : public mi::base::Interface_declare<0x0a58b727, 0x0ed4, 0x4ecf, 0x90,
                                0x9a, 0x30, 0x7c, 0x65, 0xd4, 0x47, 0xe8>
{
public:
  /// Returns the cluster descriptor for the cluster.
  virtual const ICluster_descriptor* get_cluster_descriptor() const = 0;

  /// Possible cluster states.
  ///
  /// \see #get_cluster_status().
  enum Cluster_status
  {
    CLUSTER_ESTABLISHED = 0, ///< The cluster has been successfully established.
    CLUSTER_SHUTDOWN = 1,    ///< The cluster has been shutdown.
    CLUSTER_FAILURE = 2,     ///< The cluster has failed for unspecified reasons.
    CLUSTER_FORCE_32_BIT = 0xffffffffU
  };

  /// Returns the status of the cluster.
  virtual Cluster_status get_cluster_status() const = 0;

  /// Sets the keep-alive timeout for the cluster.
  ///
  /// The keep-alive timeout specifies the amount of time (in seconds) the cluster will remain
  /// operational after the last client left the cluster. If no timeout is set, the
  /// value defaults to 0, meaning the cluster will be shut down as soon as the last connected
  /// client disconnects.
  ///
  /// \see #mi::neuraylib::ICluster_descriptor::get_timeout()
  virtual void set_timeout(Float64 timeout) = 0;

  /// Sets a cluster property.
  ///
  /// This change will be forwarded immediately to all clients.
  ///
  /// \see #remove_cluster_property() to remove cluster properties,
  ///      #get_cluster_descriptor() and #mi::neuraylib::ICluster_descriptor to retrieve cluster
  ///      properties
  ///
  /// \param name    The name of the property to set.
  /// \param value   The value string to be set for the property.
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointers).
  virtual Sint32 set_cluster_property(const char* name, const char* value) = 0;

  /// Removes a cluster property.
  ///
  /// This change will be forwarded immediately to all clients.
  ///
  /// \see #set_cluster_property() to set cluster properties,
  ///      #get_cluster_descriptor() and #mi::neuraylib::ICluster_descriptor to retrieve cluster
  ///      properties
  ///
  /// \param name   The name of the property to remove.
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointer).
  ///                - -2: There is no property with the given name.
  virtual Sint32 remove_cluster_property(const char* name) = 0;

  /// Starts a worker program if none has been started when the cluster has been created.
  ///
  /// The method is to be used if \p program_name in
  /// #mi::neuraylib::INode_manager_client::join_or_create_cluster() was \c NULL.
  ///
  /// \param program_name                           The name of the program to run on the worker
  ///                                               nodes. If \c NULL, a cluster is created
  ///                                               without child processes being forked by the
  ///                                               worker nodes.
  ///
  /// \param argument_string                        Arguments to \p program_name. The string may
  ///                                               include the substring \%m which will be
  ///                                               replaced by the multicast address of the
  ///                                               cluster (use \%\% to escape percent signs).
  ///                                               It may also contain the substring \%h which
  ///                                               will be expanded into the head node, a node
  ///                                               marked within the cluster for applications
  ///                                               that need support for it. The\%H substring
  ///                                               will be expanded to 1 on the head node, to
  ///                                               0 on all other hosts. The \%n substring will
  ///                                               be expanded to the number of nodes in the
  ///                                               cluster. If the child_process_timeout
  ///                                               parameter is defined, the command line also
  ///                                               needs to contain the substrings \%w, which
  ///                                               will be substituted with the name of the
  ///                                               named pipe and \%t which will be replaced
  ///                                               with the timeout value. If a substring of
  ///                                               the format \%p:property is included, it will
  ///                                               be expanded to the value for the property
  ///                                               set by the worker. Please note that adding
  ///                                               \%p placeholders dismisses nodes during
  ///                                               recruitment that do not have the assigned
  ///                                               property set.
  ///
  /// \param child_process_timeout                  A parameter indicating whether there should
  ///                                               be a watchdog controlling the child process
  ///                                               on worker nodes. The value specifies the
  ///                                               timeout in seconds after which the node
  ///                                               manager assumes the child process is hanging.
  ///                                               It needs to be >= 5 seconds.
  ///                                               This parameter only has an effect when
  ///                                               creating new clusters. If it is defined, the
  ///                                               argument_string parameter needs to contain
  ///                                               the substrings \%w and \%t.
  /// \return                                       0 in case of success, -1 if no cluster has
  ///                                               been created, -2 if the worker program can
  ///                                               not be started because the cluster is not
  ///                                               flagged 'reusable' and -3 if no program name
  ///                                               was specified.
  virtual Sint32 start_worker_program(
    const char* program_name, const char* argument_string, Uint32 child_process_timeout = 0) = 0;

  /// Shuts down the program that was started on all worker nodes when the cluster was created.
  ///
  /// The cluster with all worker nodes is retained. This call will usually be followed by a call
  /// to #restart_worker_program().
  ///
  /// \return
  ///                -  0: Success.
  ///                - -1: An error occurred while shutting down.
  ///
  virtual Sint32 shutdown_worker_program() = 0;

  /// Restarts the program that is supposed to run on all workers.
  ///
  /// The program command line used will be the same that was passed to
  /// #mi::neuraylib::INode_manager_client::join_or_create_cluster() when the cluster was created.
  ///
  /// \return
  ///                -  0: Success.
  ///                - -1: An error occurred while restarting.
  virtual Sint32 restart_worker_program() = 0;

  /// Initiates the shutdown of the cluster.
  ///
  /// This method shuts down the cluster unconditionally, independent of the keep-alive timeout
  /// or the number of client nodes using the cluster.
  virtual void shutdown() = 0;

  /// Returns the head node.
  ///
  /// The head node is a cluster member flagged by the node manager for applications
  /// that need support for a head node. It can be referenced by a placeholder substring in the
  /// argument string passed to #mi::neuraylib::INode_manager_client::join_or_create_cluster().
  ///
  /// \see #mi::neuraylib::INode_manager_client::join_or_create_cluster() for how to reference the
  ///      head node in the argument string.
  ///
  /// \return        The descriptor for the head node, or \c NULL if no head node has been
  ///                flagged.
  virtual IWorker_node_descriptor* get_head_node() = 0;

  /// Adds a callback to be called when a property of a worker node changes.
  ///
  /// When adding a callback it will be called immediately once for all worker nodes existing at
  /// this point. The descriptor contains all properties for the worker node known at this point.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_worker_property_callback()
  virtual void add_worker_property_callback(IWorker_node_property_callback* callback) = 0;

  /// Removes a previously added callback for property changes of worker nodes.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_worker_property_callback()
  virtual void remove_worker_property_callback(IWorker_node_property_callback* callback) = 0;

  /// Adds a callback to be called when a cluster property changes.
  ///
  /// When adding a callback it will be called immediately once. The cluster descriptor will
  /// contain all cluster properties known at this point.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_cluster_property_callback()
  virtual void add_cluster_property_callback(ICluster_property_callback* callback) = 0;

  /// Removes a previously added callback for cluster property changes.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_cluster_property_callback()
  virtual void remove_cluster_property_callback(ICluster_property_callback* callback) = 0;

  /// Adds a callback to be called when a worker node joins or leaves the cluster.
  ///
  /// Note that when adding a callback, it will be called automatically for all worker nodes
  /// already in the cluster.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_worker_node_callback()
  virtual void add_worker_node_callback(IWorker_node_callback* callback) = 0;

  /// Removes a previously added callback for joined or left worker nodes.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_worker_node_callback()
  virtual void remove_worker_node_callback(IWorker_node_callback* callback) = 0;

  /// Adds a callback to be called when a client joins or leaves the cluster.
  ///
  /// Note that when adding a callback, it will be called automatically for all client nodes
  /// already in the cluster.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_client_node_callback()
  virtual void add_client_node_callback(IClient_node_callback* callback) = 0;

  /// Removes a previously added callback for joined or left client nodes.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_client_node_callback()
  virtual void remove_client_node_callback(IClient_node_callback* callback) = 0;

  /// Adds a callback to be called when a new node becomes head node.
  ///
  /// When adding a callback, it will be called immediately once.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_client_node_callback()
  virtual void add_head_node_callback(IHead_node_callback* callback) = 0;

  /// Removes a previously added callback for notification about a new head node.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_client_node_callback()
  virtual void remove_head_node_callback(IHead_node_callback* callback) = 0;

  /// Grows the cluster by one node, if a worker node is available.
  ///
  /// The new node will be added to the cluster by starting a worker process on
  /// that node using the worker_node_filter, program_name, argument_string
  /// and child_process_timeout parameters passed to
  /// #mi::neuraylib;;INode_manager_client::join_or_create_cluster() to create the cluster.
  ///
  /// \return The descriptor of the worker node that has been added to the
  ///         cluster or \c NULL if no worker node was available.
  virtual const IWorker_node_descriptor* grow() = 0;

  /// Shrinks the cluster by one node.
  ///
  /// \param remove_node  The descriptor of the node that is supposed to be
  ///                     removed from the cluster.
  ///
  /// \return   -  0 Success.
  ///           - -1 The specified node is not a member of the cluster.
  ///           - -2 The specified node is invalid (\c NULL pointer).
  virtual Sint32 shrink(const IWorker_node_descriptor* remove_node) = 0;
};

mi_static_assert(sizeof(INode_manager_cluster::Cluster_status) == sizeof(Uint32));

/// A filter used to decide whether a cluster is eligible to be joined.
class ICluster_filter : public mi::base::Interface_declare<0x63a3ced9, 0x9ae6, 0x4c3a, 0x80, 0xc2,
                          0x80, 0x6b, 0x27, 0xff, 0x40, 0xd1>
{
public:
  /// Indicates whether a cluster is eligible to be joined.
  ///
  /// \param cluster_descriptor   Provides information about the cluster.
  /// \return                     \c true, if the cluster should be joined, or \c false otherwise.
  virtual bool is_eligible(const ICluster_descriptor* cluster_descriptor) = 0;
};

/// A filter used to decide whether a worker node is eligible to be included in a cluster.
class IWorker_node_filter : public mi::base::Interface_declare<0x9af36fa0, 0xbe40, 0x4fe4, 0x89,
                              0x03, 0x37, 0x7e, 0x12, 0xaf, 0xcb, 0xc8>
{
public:
  /// Indicates whether a worker node is eligible to be included in a cluster.
  ///
  /// \param worker_node_descriptor   Provides information about the worker node.
  /// \return                         \c true, if the worker node might be included in the
  ///                                 cluster, or \c false otherwise.
  virtual bool is_eligible(const IWorker_node_descriptor* worker_node_descriptor) = 0;
};

/// The node manager client allows to start or join \neurayAdjectiveName clusters built from worker
/// nodes.
///
/// It should be used in a process running on a client node.
class INode_manager_client : public mi::base::Interface_declare<0xe8feacc5, 0x1f7c, 0x4abc, 0x8a,
                               0x23, 0x50, 0x3c, 0x56, 0xf4, 0xa6, 0x63>
{
public:
  /// Starts the operation of the node manager.
  ///
  /// \param listen_address      The address used to communicate with other node manager
  ///                            instances. Should be a multicast address unless TCP networking is
  ///                            selected. In case of TCP networking, if the address is the local
  ///                            IP address, the host will become the head node which is used to
  ///                            discover the other hosts.
  /// \param tcp                 Indicates whether TCP or UDP should be used.
  /// \param discovery_address   The address of the TCP head node used for host discovery. If this
  ///                            is the same as the listen address, the node will be head node.
  /// \param cluster_interface   The address of the cluster interface for listening.
  /// \return                    0 in case of success, -1 otherwise
  virtual Sint32 start(const char* listen_address, bool tcp = false,
    const char* discovery_address = 0, const char* cluster_interface = 0) = 0;

  /// Shuts down the operation of the node manager.
  ///
  /// \return 0 in case of success, -1 otherwise.
  virtual Sint32 shutdown() = 0;

  /// Returns the listen address used by the node manager.
  ///
  /// \return    The listen address and port.
  virtual const IString* get_listen_address() const = 0;

  /// Sets the multicast base address.
  ///
  /// The node manager reserves a unique multicast address for each cluster that it manages.
  /// These multicast addresses start at the multicast base address and are obtained by
  /// incrementing the last octet, then the second-last octet, and so on.
  ///
  /// \param  base_address       A multicast address to be used as a base when reserving
  ///                            cluster multicast addresses.
  /// \return                    0 in case of success, -1 otherwise
  ///
  /// \see #get_multicast_base_address(),
  ///      #mi::neuraylib::ICluster_descriptor::get_multicast_address()
  virtual Sint32 set_multicast_base_address(const char* base_address) = 0;

  /// Returns the multicast base address.
  ///
  /// The node manager reserves a unique multicast address for each cluster that it manages.
  /// These multicast addresses start at the multicast base address and are obtained by
  /// incrementing the last octet, then the second-last octet, and so on.
  ///
  /// \return The currently set multicast base address.
  ///
  /// \see #set_multicast_base_address(),
  ///      #mi::neuraylib::ICluster_descriptor::get_multicast_address()
  virtual const IString* get_multicast_base_address() const = 0;

  /// Returns the number of worker nodes currently known to the node manager.
  virtual Size get_number_of_worker_nodes() const = 0;

  /// Returns a descriptor for a worker node currently known to the node manager.
  ///
  /// \note The set of worker nodes in the cluster can change at any time. That is, this function
  ///       can return \c NULL even if \p index is smaller than the result of the last call to
  ///       #get_number_of_worker_nodes().
  ///
  /// \param index   The index of the worker node (from 0 to #get_number_of_worker_nodes()-1).
  /// \return        The descriptor for the specified worker node, or \c NULL if \p index is out
  ///                of bounds.
  virtual const IWorker_node_descriptor* get_worker_node(Size index) const = 0;

  /// Adds a callback to be called when a request to shutdown all clients and workers is
  /// received.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_shutdown_node_managers_callback()
  virtual void add_shutdown_node_managers_callback(IShutdown_node_managers_callback* callback) = 0;

  /// Removes a previously added callback to be called when a request to shutdown all clients
  /// and workers is received.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_shutdown_node_managers_callback()
  virtual void remove_shutdown_node_managers_callback(
    IShutdown_node_managers_callback* callback) = 0;

  /// Adds a callback to be called when a request to shutdown a cluster is received.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_shutdown_cluster_callback()
  virtual void add_shutdown_cluster_callback(IShutdown_cluster_callback* callback) = 0;

  /// Removes a previously added callback to be called when a request to shutdown a cluster
  /// is received.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_shutdown_cluster_callback()
  virtual void remove_shutdown_cluster_callback(IShutdown_cluster_callback* callback) = 0;

  /// Adds a callback to be called when a worker process has been fully started.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_worker_process_started_callback()
  virtual void add_worker_process_started_callback(IWorker_process_started_callback* callback) = 0;

  /// Removed a callback to be called when a worker process has been fully started.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_worker_process_started_callback()
  virtual void remove_worker_process_started_callback(
    IWorker_process_started_callback* callback) = 0;

  /// Unconditionally shut down the cluster that cluster_descriptor refers to and release
  /// reserved worker nodes. The caller does not need to be the creator of the cluster.
  ///
  /// \param cluster_descriptor  Descriptor of the cluster that is supposed to be shut down.
  /// \return                    0 in case of success, -1 if cluster_descriptor is unknown.
  virtual Sint32 shutdown_cluster(const ICluster_descriptor* cluster_descriptor) = 0;

  /// Symbolic constant to pass as child_process_timeout to #join_or_create_cluster. If set,
  /// the watchdog mechanism is disabled and only signals when the child process has been
  /// fully started.
  ///
  /// \see #join_or_create_cluster()
  static const Uint32 SIGNAL_STARTUP_ONLY = 0xffffffff;

  /// Joins an existing cluster or creates a new one.
  ///
  /// This function will do the following:
  /// - For all existing clusters:
  ///   - Check if the cluster size matches the minimum and maximum number of requested worker
  ///     nodes.
  ///   - Check if the program name and argument string of the cluster nodes match the values
  ///     given in \p program_name and \p argument_string.
  ///   - Check if calling the \p cluster_filter callback returns \c true.
  ///   - If all above conditions are met, the calling node joins the existing cluster as a new
  ///     client node. An interface describing the existing cluster is returned. The function
  ///     terminates.
  /// - If no matching cluster was found, then for all existing worker nodes:
  ///   - Check if \p worker_node_filter returns \c true, if yes reserve the worker node. Leave
  ///     the loop if the number of reserved worker nodes meets the maximum number of requested
  ///     nodes.
  /// - If at least the minimum number of requested worker nodes have been reserved, create the
  ///   cluster, and an interface describing the newly created cluster is returned.
  /// - Otherwise, the reservation of worker nodes is released, and \c NULL is returned.
  ///
  /// If the flag \p child_process_watchdog is set to \c true, the child process started on worker
  /// nodes will be under closer scrutiny by the node manager. The node manager will substitute
  /// any occurrence of the placeholder %w in the command line used to start the child process
  /// with the string identifier of the named pipe the child process is supposed to write to.
  /// The child process may open the pipe for writing and may write PDUs from the watchdog
  /// protocol to it. The watchdog is activated, once the first PDU has been written to the pipe.
  /// The child process needs to keep writing PDUs in intervals of n seconds from that moment on
  /// or the node manager will give up on the child process and will terminate it.
  ///
  /// \param min_number_of_requested_worker_nodes   The minimum number of worker nodes expected
  ///                                               in the cluster.
  /// \param max_number_of_requested_worker_nodes   The maximum number of worker nodes expected
  ///                                               in the cluster.
  /// \param cluster_filter                         A filter specifying required cluster
  ///                                               properties. If \c NULL, no existing cluster
  ///                                               will be joined.
  /// \param worker_node_filter                     A filter specifying required worker node
  ///                                               properties. If \c NULL, no cluster will be
  ///                                               created. The filter will be used again when
  ///                                               calling
  ///                                               #mi::neuraylib::INode_manager_cluster::grow()
  ///                                               to add nodes to the cluster later on.
  /// \param program_name                           The name of the program to run on the worker
  ///                                               nodes. If \c NULL, a cluster is created
  ///                                               without child processes being forked by the
  ///                                               worker nodes.
  /// \param argument_string                        Arguments to \p program_name. The string may
  ///                                               include the substring \%m which will be
  ///                                               replaced by the multicast address of the
  ///                                               cluster (use \%\% to escape percent signs).
  ///                                               It may also contain the substring \%h which
  ///                                               will be expanded into the head node, a node
  ///                                               marked within the cluster for applications
  ///                                               that need support for it. The \%H substring
  ///                                               will be expanded to 1 on the head node, to
  ///                                               0 on all other hosts. The \%n substring will
  ///                                               be expanded to the number of nodes in the
  ///                                               cluster. If the child_process_timeout
  ///                                               parameter is defined, the command line also
  ///                                               needs to contain the substrings \%w, which
  ///                                               will be substituted with the name of the
  ///                                               named pipe and \%t which will be replaced
  ///                                               with the timeout value. If a substring of
  ///                                               the format \%p:property is included, it will
  ///                                               be expanded to the value for the property
  ///                                               set by the worker. Please note that adding
  ///                                               \%p placeholders dismisses nodes during
  ///                                               recruitment that do not have the assigned
  ///                                               property set.
  ///
  /// \param child_process_timeout                  A parameter indicating whether there should
  ///                                               be a watchdog controlling the child process
  ///                                               on worker nodes. The value specifies the
  ///                                               timeout in seconds after which the node
  ///                                               manager assumes the child process is hanging.
  ///                                               It needs to be >= 5 seconds.
  ///                                               This parameter only has an effect when
  ///                                               creating new clusters. If it is defined, the
  ///                                               \p argument_string parameter needs to contain
  ///                                               the substrings \%w and \%t.
  ///
  /// \param reusable                               A flag indicating whether it should be
  ///                                               allowed to start a different child process
  ///                                               application after having shut down the
  ///                                               application that was run in the cluster
  ///                                               first. Clusters flagged 'reusable' can't
  ///                                               be joined by more than one client. This flag
  ///                                               is only useful if new child processes are
  ///                                               supposed to be started using
  ///                               #mi::neuraylib::INode_manager_cluster::start_worker_program().
  ///
  /// \return                                       An interface to the joined or created cluster,
  ///                                               or \c NULL in case of failure.
  virtual INode_manager_cluster* join_or_create_cluster(Size min_number_of_requested_worker_nodes,
    Size max_number_of_requested_worker_nodes, ICluster_filter* cluster_filter,
    IWorker_node_filter* worker_node_filter, const char* program_name, const char* argument_string,
    bool reusable = false, Uint32 child_process_timeout = 0) = 0;

  /// Returns the number of existing clusters.
  virtual Size get_number_of_clusters() const = 0;

  /// Returns a descriptor for a cluster.
  ///
  /// \note The set of clusters can change at any time. This function can therefore return
  ///       \c NULL even if \p index is smaller than the result of the last call to
  ///       #get_number_of_clusters().
  ///
  /// \param index   The index of the cluster (from 0 to #get_number_of_clusters()-1).
  /// \return        The descriptor for the specified cluster, or \c NULL if \p index
  ///                is out of bounds.
  virtual const ICluster_descriptor* get_cluster(Size index) const = 0;

  /// Shuts down all node manager clients and workers which are currently joined.
  ///
  /// Can be used for example when installing new versions.
  virtual void shutdown_node_managers() = 0;

  /// Set the head node address and subnet qualifier.
  ///
  /// The IP address of the interface can be specified as a sub net using the CIDR
  /// notation a.b.c.d/xx. If there is an interface on the host with an address inside this
  /// range the first match will be used in the address returned by
  /// #mi::neuraylib::IWorker_node_descriptor::get_address() for the worker node descriptor
  /// returned by #mi::neuraylib::INode_manager_cluster::get_head_node. This means that on
  /// a host which has the address 192.168.1.1, specifying the address as 192.168.0.0/16
  /// would return the address 192.168.1.1.
  ///
  /// To have an effect, this needs to be set before creating the cluster.
  virtual void set_head_node_interface(const char* address) = 0;

  /// Get the head node address and subnet qualifier.
  ///
  /// This returns the address set using #set_head_node_interface().
  ///
  /// \return        The head node interface address and subnet qualifier.
  virtual const IString* get_head_node_interface() = 0;
};

/// A filter used to decide if a command string to start a child process is eligible for
/// execution.
class IChild_process_resolver : public mi::base::Interface_declare<0xbd1ab5cb, 0x2794, 0x4cd8, 0x99,
                                  0xa9, 0x30, 0x36, 0x32, 0x8a, 0xca, 0xff>
{
public:
  /// Indicates whether a command string to start a child process should be executed by the worker
  /// node.
  ///
  /// This can be used to enforce security policies. In case of success, in addition, it allows to
  /// adapt the incoming program name to the local file system.
  ///
  /// \param program_name         The name of the program to be executed as child process.
  /// \param program_arguments    The arguments supposed to be passed to the program.
  /// \return                     If \p program_name (in combination with \p program_arguments) is
  ///                             eligible to be run then \p program_name is returned.
  ///                             (The returned value may include the full pathname and may be
  ///                             modified based on the requirements of the local file system
  ///                             of the worker node) Otherwise, \c NULL is returned.
  virtual const IString* resolve_process(
    const char* program_name, const char* program_arguments) = 0;
};

/// The node manager worker class allows to set properties and announce them to other nodes.
///
/// It also allows a client node to start child processes. It should be used in a process running on
/// the worker nodes.
class INode_manager_worker : public mi::base::Interface_declare<0xeb232bd5, 0x0abf, 0x4872, 0xab,
                               0x18, 0x92, 0x49, 0x31, 0x36, 0xf9, 0x91>
{
public:
  /// Starts the operation of the node manager.
  ///
  /// For the optional cluster_interface parameter, the address can also be specified as a sub
  /// net using the CIDR notation a.b.c.d/xx. If there is an interface on the host with an
  /// address inside this range the first match will be used. This is useful for example when
  /// configuring several hosts. This means that on a host which has the address 192.168.1.1,
  /// specifying the address as 192.168.0.0/16:10000 would make the host bind to the
  /// 192.168.1.1 address on port 10000.
  ///
  /// \param listen_address      The address used to communicate with other node manager
  ///                            instances. Should be a multicast address unless TCP networking is
  ///                            selected.
  /// \param tcp                 Indicates whether TCP or UDP should be used.
  /// \param discovery_address   The address of the TCP head node used for host discovery. If this
  ///                            is the same as the listen address, the node will be head node.
  /// \param cluster_interface   The address of the cluster interface for listening.
  ///
  /// \return                    0 in case of success, -1 otherwise
  virtual Sint32 start(const char* listen_address = 0, bool tcp = false,
    const char* discovery_address = 0, const char* cluster_interface = 0) = 0;

  /// Shuts down the operation of the node manager.
  ///
  /// \return 0 in case of success, -1 otherwise.
  virtual Sint32 shutdown() = 0;

  /// Returns the listen address used by the node manager.
  ///
  /// \return    The listen address and port.
  virtual const IString* get_listen_address() const = 0;

  /// Sets the multicast base address.
  ///
  /// The node manager reserves a unique multicast address for each cluster that it manages.
  /// These multicast addresses start at the multicast base address and are obtained by
  /// incrementing the last octet, then the second-last octet, and so on.
  ///
  /// \param  base_address       A multicast address to be used as a base when reserving
  ///                            cluster multicast addresses.
  /// \return                    0 in case of success, -1 otherwise
  ///
  /// \see #get_multicast_base_address(),
  ///      #mi::neuraylib::ICluster_descriptor::get_multicast_address()
  virtual Sint32 set_multicast_base_address(const char* base_address) = 0;

  /// Returns the multicast base address.
  ///
  /// The node manager reserves a unique multicast address for each cluster that it manages.
  /// These multicast addresses start at the multicast base address and are obtained by
  /// incrementing the last octet, then the second-last octet, and so on.
  ///
  /// \return The currently set multicast base address.
  ///
  /// \see #set_multicast_base_address(),
  ///      #mi::neuraylib::ICluster_descriptor::get_multicast_address()
  virtual const IString* get_multicast_base_address() const = 0;

  /// Sets a property of a worker node.
  ///
  /// This change will be forwarded immediately to all clients.
  ///
  /// \see #get_property(), #remove_property()
  ///
  /// \param name    The name of the property.
  /// \param value   The value to set for the property.
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointers).
  virtual Sint32 set_property(const char* name, const char* value) = 0;

  /// Returns a property of a worker node.
  ///
  /// \see #set_property(), #remove_property()
  ///
  /// \param name    The name of the property to get.
  /// \return        A string representing the value of the property, or \c NULL if there is no
  ///                property with the given name.
  virtual const IString* get_property(const char* name) const = 0;

  /// Removes a property of a worker node.
  ///
  /// This change will be forwarded immediately to all clients.
  ///
  /// \see #set_property(), #get_property()
  ///
  /// \param name    The name of the property to remove.
  /// \return
  ///                -  0: Success.
  ///                - -1: Invalid parameters (\c NULL pointer).
  ///                - -2: There is no property with the given name.
  virtual Sint32 remove_property(const char* name) = 0;

  /// Sets the child process resolver.
  ///
  /// \see #get_child_process_resolver()
  ///
  /// \param child_process_resolver   The new child process resolver. The value \c NULL can be
  ///                                 used to remove the current child process resolver (which
  ///                                 effectively is the same as a child process resolver
  ///                                 instance that returns its first argument unchanged).
  virtual void set_child_process_resolver(IChild_process_resolver* child_process_resolver) = 0;

  /// Returns the child process resolver.
  ///
  /// \see #set_child_process_resolver()
  ///
  /// \return The child process resolver.
  virtual IChild_process_resolver* get_child_process_resolver() const = 0;

  /// Adds a callback to be called when a request to shutdown all clients and workers is received.
  ///
  /// \param callback   The callback to be added.
  ///
  /// \see #remove_shutdown_node_managers_callback()
  virtual void add_shutdown_node_managers_callback(IShutdown_node_managers_callback* callback) = 0;

  /// Removes a previously added callback to be called when a request to shutdown all clients
  /// and workers is received.
  ///
  /// \param callback   The callback to be removed.
  ///
  /// \see #add_shutdown_node_managers_callback()
  virtual void remove_shutdown_node_managers_callback(
    IShutdown_node_managers_callback* callback) = 0;
};

/// Factory to create node manager client and worker instances.
class INode_manager_factory : public mi::base::Interface_declare<0xd54aaa9c, 0x4798, 0x4405, 0xa4,
                                0x58, 0xd8, 0x63, 0x44, 0xb4, 0xb1, 0xdd>
{
public:
  /// Creates a node manager client instance.
  virtual INode_manager_client* create_client() = 0;

  /// Creates a node manager worker instance.
  virtual INode_manager_worker* create_worker() = 0;
};

/*@}*/ // end group mi_neuray_node_manager

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_INODE_MANAGER_H
