/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component to interact with the cluster manager

#ifndef MI_NEURAYLIB_ICLUSTER_MANAGER_CONFIGURATION_H
#define MI_NEURAYLIB_ICLUSTER_MANAGER_CONFIGURATION_H

#include <mi/base/interface_declare.h>

namespace mi
{

namespace neuraylib
{

/// \defgroup mi_neuray_cluster_manager Cluster manager
/// \ingroup mi_neuray
///
/// This module allows to connect to the cluster manager and to reserve nodes in a cluster.
///
/// A cluster manager is a service that allows to manage nodes in a cluster. The cluster manager
/// allows users to reserve nodes of the cluster and to start software that has been installed on
/// the cluster. After using the software the user releases the reservation and the nodes are
/// returned to the pool and are made available for other users.
///
/// See the API component #mi::neuraylib::ICluster_manager_configuration for a starting point.

/** \addtogroup mi_neuray_cluster_manager
@{
*/

/// Abstract interface for giving notifications about errors and status changes.
///
/// \see #mi::neuraylib::ICluster_manager_connection::set_cluster_notification_callback()
class ICluster_notification_callback : public base::Interface_declare<0x1b9bfd07, 0x9693, 0x4e3f,
                                         0x83, 0xb1, 0x85, 0x3b, 0x91, 0x83, 0xd2, 0x66>
{
public:
  /// This callback will be called when an error occurred, like the connection broke down.
  virtual void error_callback() = 0;

  /// This callback will be called when some node in the pool changed status, e.g., a cluster was
  /// reserved or released.
  virtual void status_change_callback() = 0;

  /// This callback will be called when the user's cluster changed. It will be called
  /// after a cluster reservation succeeded and after the cluster got released.
  virtual void cluster_change_callback() = 0;

  /// This callback will be called when the cluster is fully started and connected.
  ///
  /// \param is_ready   True, if the cluster is up and ready to be connected
  virtual void cluster_ready_callback(bool is_ready) = 0;
};

/// Provides information about a node which is part of a node pool or reserved cluster.
///
/// \see #mi::neuraylib::ICluster_manager_cluster::get_cluster_node(),
///      #mi::neuraylib::ICluster_pool_information::get_node()
class ICluster_manager_node : public base::Interface_declare<0x6b1cbc68, 0x3af1, 0x4a56, 0x83, 0x68,
                                0x00, 0x13, 0xd4, 0xec, 0xc2, 0xfa>
{
public:
  /// Returns the node name of the node.
  virtual const char* get_node_name() = 0;

  /// Returns the IP address of the node.
  virtual const char* get_ip_address() = 0;

  /// Returns the name of a user which has currently reserved the node, or \c NULL if the node is
  /// free.
  virtual const char* get_user_name() = 0;

  /// Returns the number of CPU cores installed in the node.
  virtual Size get_cpu_count() = 0;

  /// Returns the current CPU load on the node in percent.
  virtual Float32 get_cpu_load() = 0;

  /// Returns the total amount of main memory in bytes on the node.
  virtual Size get_total_main_memory() = 0;

  /// Returns the amount of free main memory in bytes on the node.
  virtual Size get_free_main_memory() = 0;

  /// Returns the number of GPUs installed in the node.
  virtual Size get_gpu_count() = 0;

  /// Returns the current GPU load on the node in percent.
  ///
  /// This is an average over all installed GPUs.
  virtual Float32 get_gpu_load() = 0;

  /// Returns the total amount of GPU memory (per GPU).
  ///
  /// The assumption is that all GPUs have the same amount of memory.
  virtual Size get_total_gpu_memory() = 0;

  /// Returns the amount of free GPU memory (per GPU).
  ///
  /// This is an average over all installed GPUs. The assumption is that all GPUs have the same
  /// amount of memory and GPU memory usage is equal on all GPUs.
  virtual Size get_free_gpu_memory() = 0;
};

/// Represents a cluster which was reserved through the cluster manager.
///
/// \see #mi::neuraylib::ICluster_manager_connection::reserve_cluster()
class ICluster_manager_cluster : public base::Interface_declare<0x5c88e256, 0x6d61, 0x4d76, 0x91,
                                   0x13, 0xe0, 0x37, 0x9f, 0x4a, 0xfd, 0x07>
{
public:
  /// Returns the name of the head node of the cluster.
  virtual const char* get_head_node() = 0;

  /// Returns the ID of software package which was started on the cluster, or \c NULL if no
  /// software was started.
  virtual const char* get_software_package_id() = 0;

  /// Returns the Bridge protocol version of the cluster.
  ///
  /// This is for display purposes only. Use #is_compatible() to check whether it is compatible
  /// with the client's Bridge protocol version.
  virtual const char* get_bridge_protocol_version() = 0;

  /// Returns an authentication token which has to be provided by the Bridge client when making
  /// the connection.
  ///
  /// The token was generated by the cluster manager and has been passed to the software started
  /// on the server side.
  virtual const char* get_authentication_token() = 0;

  /// Indicates whether server's Bridge protocol version is compatible with the client's Bridge
  /// protocol version.
  virtual bool is_compatible() = 0;

  /// Returns the number of nodes in the cluster.
  ///
  /// \see #get_cluster_node()
  virtual Size get_number_of_cluster_nodes() = 0;

  /// Returns a node which is part of the cluster.
  ///
  /// \see #get_number_of_cluster_nodes()
  ///
  /// \param index   The index of the cluster node.
  /// \return        A description of the \p index-th cluster node, or \c NULL if \p index is out
  ///                of bounds.
  virtual ICluster_manager_node* get_cluster_node(Size index) = 0;

  /// Indicates if the cluster can be closed by the application through this API.
  virtual bool can_be_closed() = 0;

  /// Returns the address of the head node of the cluster.
  virtual const char* get_head_node_address() = 0;

  /// Returns a descriptor of the head node of the cluster.
  virtual ICluster_manager_node* get_head_node_descriptor() = 0;

  /// Grows the cluster by one node.
  ///
  /// \see #shrink_cluster
  ///
  /// \param[out] errors   An optional pointer to an #mi::Sint32 to which an error code
  ///                      will be written. The error codes have the following meaning:
  ///                      -  0: Success.
  ///                      - -1: No nodes were available.
  ///                      - -2: No cluster is running.
  ///                      - -3: The connection to the cluster manager broke down.
  ///                      - -4: The request timed out. Please check if the cluster
  ///                            manager is working properly.
  /// \return              The decriptor of the node that has been added to the cluster, or
  ///                      \c NULL if no node was available to be added.
  virtual ICluster_manager_node* grow_cluster(Sint32* errors) = 0;

  /// Shrinks the cluster by one node.
  ///
  /// Shrinking the cluster if there's only one node remaining is not allowed. Removing the
  /// head node of the cluster is not prevented but should be avoided when calling this.
  /// There's no built-in way after the cluster has been started to tell a head node process
  /// that is has been selected as the new head node.
  ///
  /// \see #grow_cluster
  ///
  /// \param node    The descriptor of the node that is to be removed from the cluster.
  /// \return
  ///                -  0: Success.
  ///                -  1: Success. The removed node was the head node.
  ///                - -1: Node not removed because it's the last node in the cluster.
  ///                - -2: No cluster is running.
  ///                - -3: The connection to the cluster manager broke down.
  ///                - -4: The request timed out. Please check if the cluster
  ///                      manager is working properly.
  ///                - -5: The server failed to remove the node from the cluster.
  ///
  virtual Sint32 shrink_cluster(ICluster_manager_node* node) = 0;
};

/// Represents a pool of nodes managed though the cluster manager.
///
/// \see #mi::neuraylib::ICluster_manager_connection::get_cluster_pool_information()
class ICluster_pool_information : public base::Interface_declare<0xd444f7cc, 0xf8ff, 0x4a79, 0xa0,
                                    0xe5, 0x51, 0x02, 0xc6, 0x66, 0x37, 0x94>
{
public:
  /// Returns the number of nodes in the pool.
  ///
  /// \see #get_node()
  virtual Size get_number_of_nodes() const = 0;

  /// Returns a node which is part of the pool.
  ///
  /// \see #get_number_of_nodes()
  ///
  /// \param index    The index of the node.
  /// \return         A description of the \p index-th pool node, or \c NULL if \p index is out
  ///                 of bounds.
  virtual ICluster_manager_node* get_node(Size index) const = 0;
};

/// Represents a software package installed on the pool of nodes.
///
/// \see #mi::neuraylib::ICluster_manager_connection::get_software_package(),
///      #mi::neuraylib::ICluster_manager_connection::get_compatible_software_package()
class ISoftware_package : public base::Interface_declare<0xa7a3a95c, 0x3db8, 0x4fef, 0xb9, 0x10,
                            0x56, 0x68, 0xd7, 0xa2, 0xac, 0x5>
{
public:
  /// Returns the ID of the software package.
  virtual const char* get_id() const = 0;

  /// Returns the description of the software package.
  virtual const char* get_description() const = 0;

  /// Returns the bridge protocol version
  virtual const char* get_bridge_protocol_version() const = 0;
};

/// Represents a connection to a cluster manager.
///
/// The connection allows inquiring information about the cluster manager and the nodes in the pool.
/// Furthermore, it allows reserving a cluster of nodes.
///
/// \see #mi::neuraylib::ICluster_manager_configuration::connect()
class ICluster_manager_connection : public base::Interface_declare<0x0a54aeb4, 0xb2e9, 0x4893, 0x9f,
                                      0x93, 0x30, 0xa4, 0x61, 0x1b, 0xea, 0x2a>
{
public:
  /// Returns the address of the cluster manager to which this connection connects to.
  virtual const char* get_address() const = 0;

  /// Returns the number of compatible software packages.
  ///
  /// An installed software package is compatible if its Bridge version is compatible with
  /// the client's Bridge version.
  ///
  /// \see #get_compatible_software_package()
  virtual Size get_number_of_compatible_software_packages() const = 0;

  /// Returns a compatible software package.
  ///
  /// An installed software package is compatible if its Bridge version is compatible with
  /// the client's Bridge version.
  ///
  /// \see #get_number_of_compatible_software_packages()
  ///
  /// \param index   The index of the software package.
  /// \return        The \p index-th software package or \c NULL if \p index is out of bounds.
  virtual const ISoftware_package* get_compatible_software_package(Size index) const = 0;

  /// Returns the number of software packages.
  ///
  /// \see #get_software_package()
  virtual Size get_number_of_software_packages() const = 0;

  /// Returns a software package.
  ///
  /// \see #get_number_of_software_packages()
  ///
  /// \param index   The index of the software package.
  /// \return        The \p index-th software package or \c NULL if \p index is out of bounds.
  virtual const ISoftware_package* get_software_package(Size index) const = 0;

  /// Returns information about the pool.
  ///
  /// This will ask the cluster manager for the information and block until the information was
  /// received.
  virtual const ICluster_pool_information* get_cluster_pool_information() const = 0;

  /// Reserves a cluster of nodes from the pool of free nodes.
  ///
  /// \note Currently only one cluster may be reserved at a time.
  ///       This method must not be called from within a callback.
  ///
  /// \param requested_nodes       The number of nodes requested for the cluster. This is the
  ///                              maximum number of nodes. The returned cluster may contain fewer
  ///                              nodes.
  /// \param software_package_id   The ID of the software package which should be started. The
  ///                              value \c NULL can be used to specify that no software package
  ///                              should be started as part of the reservation process.
  /// \param[out] errors           An optional pointer to an #mi::Sint32 to which an error code
  ///                              will be written. The error codes have the following meaning:
  ///                              -  0: Success.
  ///                              - -1: No nodes were available.
  ///                              - -2: There was already running cluster. Release that first.
  ///                              - -3: The connection to the cluster manager broke down.
  ///                              - -4: The selected software package is not available
  ///                              - -5: The request timed out. Please check if the cluster
  ///                                    manager is working properly.
  /// \return                      The reserved cluster or \c NULL in case of failure.
  ///
  /// \note Releasing the returned interface will \em not release the reserved cluster. Use
  ///       #release_cluster() to release the reserved cluster.
  virtual ICluster_manager_cluster* reserve_cluster(
    Size requested_nodes, const char* software_package_id, Sint32* errors = 0) = 0;

  /// Releases the reserved cluster for this user.
  ///
  /// \note This method must not be called from within a callback.
  ///
  /// \return
  ///                         -  0: Success.
  ///                         - -1: Cluster could not be released/was immediately re-established.
  ///                         - -2: No cluster is running at the moment.
  virtual Sint32 release_cluster() = 0;

  /// Returns the reserved cluster for this user, or \c NULL if there is none.
  ///
  /// \note This method also returns the cluster if it was reserved by other means than via
  ///       #reserve_cluster(), e.g., via a GUI.
  virtual ICluster_manager_cluster* get_cluster() = 0;

  /// Sets a callback to be called in case of certain events.
  ///
  /// If there was a previously installed callback, it will be released.
  ///
  /// \note This method must not be called from within such a callback.
  ///
  /// \param callback   The new callback object.
  virtual void set_cluster_notification_callback(ICluster_notification_callback* callback) = 0;

  /// Checks whether an user exists on the cluster manager and whether the password matches.
  ///
  /// This can be used to share an authentication database between a cluster manager and a
  /// connected application.
  ///
  /// \param user_name            User name of the user to be checked.
  /// \param password             Password of the user to be checked.
  /// \param[out] is_admin        An optional pointer to a bool to which the information is
  ///                             written whether an authenticated user is an admin. If the user
  ///                             could not be authenticated, then the content is undefined.
  /// \param[out] errors          An optional pointer to an #mi::Sint32 to which an error code
  ///                             will be written. The error codes have the following meaning:
  ///                             -  0: Success.
  ///                             - -3: The connection to the cluster manager broke down.
  ///                             - -5: The request timed out. Please check if the cluster
  ///                                   manager is working properly.
  /// \return                     \c true, if the user could be authenticated, or \c false
  ///                             otherwise.
  virtual bool authenticate_user(
    const char* user_name, const char* password, bool* is_admin = 0, Sint32* errors = 0) = 0;

  /// Sets if a reserved cluster should be auto-released, if this connection is closed / lost
  /// for example because of a network outage or a crash of the client process. If set, then
  /// any reserved cluster will be released by the VCA manager when the connection is closed from
  /// its point of view. The default is that auto-release is not enabled.
  ///
  /// \param auto_release_enabled True, if auto-release should be enabled, false otherwise.
  virtual void set_auto_release_cluster(bool auto_release_enabled) = 0;

  /// Return if auto-release is enabled or not.
  ///
  /// \return                     True, if auto-release is enabled, false otherwise.
  virtual bool get_auto_release_cluster() = 0;

  /// Get the name of the VCA to which this connections connects to.
  ///
  /// \return The name of the VCA or VCA pool.
  virtual const char* get_vca_name() const = 0;
};

/// An API component which can be used to create a connection to a cluster manager.
class ICluster_manager_configuration : public base::Interface_declare<0x6ac7506d, 0x3604, 0x49ad,
                                         0xb9, 0xc3, 0x6f, 0xb3, 0xf5, 0x0c, 0xa3, 0x0f>
{
public:
  /// Creates a connection to a cluster manager.
  ///
  /// \param address      The address of the cluster manager including the port. The address can
  ///                     be prefixed by \c "wss://" to encrypt the connection using the SSL
  ///                     protocol. If no prefix is given, the default prefix \c "ws://" is used,
  ///                     resulting in an unencrypted connection.
  /// \param user_name    The user name to be used for logging into the cluster manager.
  /// \param password     The password to be used for logging into the cluster manager.
  /// \param[out] errors  An optional pointer to an #mi::Sint32 to which an error code will be
  ///                     written. The error codes have the following meaning:
  ///                     -  0: Success.
  ///                     - -1: The connection failed. Please check the server address.
  ///                     - -2: The authentication failed. Please check user name and password.
  ///                     - -5: The request timed out. Please check if the cluster manager is
  ///                           working properly.
  /// \return             The cluster manager connection, or \c NULL in case of failures.
  virtual ICluster_manager_connection* connect(
    const char* address, const char* user_name, const char* password, Sint32* errors = 0) = 0;
};

/*@}*/ // end group mi_neuray_cluster_manager

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ICLUSTER_MANAGER_CONFIGURATION_H
