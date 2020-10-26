/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for networking related settings.

#ifndef MI_NEURAYLIB_INETWORK_CONFIGURATION_H
#define MI_NEURAYLIB_INETWORK_CONFIGURATION_H

#include <mi/base/interface_declare.h>

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#undef Status
#endif // _XLIB_H_ || _X11_XLIB_H_

namespace mi {

class IString;

namespace neuraylib {

class IHost_callback;
class INetwork_statistics;

/** \addtogroup mi_neuray_configuration
@{
*/

/// This interface is used to query and change the networking configuration.
class INetwork_configuration : public
    mi::base::Interface_declare<0xb60d3124,0xd410,0x400b,0xa0,0x1a,0x5c,0x5a,0x23,0x8d,0xcf,0xf2>
{
public:
    /// Constants for the networking mode.
    ///
    /// Each setting represents the method of finding other hosts and whether they communicate over
    /// unicast, multicast, or a combination of both after establishing a cluster.
    ///
    /// \see #set_mode(), #get_mode()
    enum Mode
    {
        /// Networking is switched off.
        MODE_OFF = 0,
        /// Networking is using TCP/IP connections between hosts. Only hosts configured in the host
        /// list can join the network. If a host is not available at startup no further connection
        /// attempts will be made.
        MODE_TCP = 1,
        /// Networking is switched to UDP mode with multicast. Hosts can join dynamically after
        /// being started without being listed in the configuration. Data transmission is done
        /// using high bandwidth unicast and multicast UDP. The multicast address is configured
        /// using #set_multicast_address().
        MODE_UDP = 2,
        /// Networking is using TCP/IP connections between hosts. Hosts can join dynamically after
        /// being started without being listed in the configuration. The method
        /// #set_discovery_address() controls which address to use for finding other hosts.
        MODE_TCP_WITH_DISCOVERY = 3,
        //  Undocumented, for alignment only
        MODE_FORCE_32_BIT = 0xffffffffU
    };

    /// Sets the networking mode.
    ///
    /// This can only be called before \neurayProductName has been started.
    ///
    /// \see #get_mode()
    ///
    /// \param mode                    The desired networking mode.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_mode( Mode mode) = 0;

    /// Returns the configured networking mode.
    ///
    /// Note that networking might not be used even though it was configured, e.g., due to license
    /// restrictions. Make sure to check whether #get_status() returns #CONNECTION_STANDALONE or
    /// #CONNECTION_ESTABLISHED after \neurayProductName has been started.
    ///
    /// \see #set_mode()
    ///
    /// \return                        The configured networking mode.
    virtual Mode get_mode() const = 0;

    /// The different states for the networking connection.
    ///
    /// \see #get_status()
    enum Status
    {
        /// Networking is not enabled (see #set_mode()) or is not available, e.g., due to license
        /// restrictions.
        CONNECTION_STANDALONE = 0,
        /// \NeurayProductName is in the process of starting up
        CONNECTION_STARTING = 1,
        /// All connections are established as configured.
        CONNECTION_ESTABLISHED = 2,
        /// \NeurayProductName is in the process of shutting down
        CONNECTION_SHUTTINGDOWN = 3,
        /// \NeurayProductName has shut down
        CONNECTION_SHUTDOWN = 4,
        /// Networking has failed for some reason
        CONNECTION_FAILURE = 5,
        //  Undocumented, for alignment only
        CONNECTION_FORCE_32_BIT = 0xffffffffU
    };

    /// Returns the status of the networking connection.
    virtual Status get_status() const = 0;

    // Configuration for multicast networking

    /// Sets the %base multicast address to be used for networking.
    ///
    /// This address will be used for UDP mode networking. An IPv4 UDP multicast address is
    /// specified by an address in the range 224.0.0.0 to 239.255.255.255, inclusive, along with
    /// a port. An IPv6 multicast address is specified by an IPv6 address with a high-order
    /// byte ff.
    ///
    /// Note that actually not just one, but several multicast addresses are used. The other
    /// multicast addresses are obtained by incrementing the last octet. Hence, the value
    /// configured here is called the \em %base multicast address.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_multicast_address()
    ///
    /// \param address                 The %base multicast address and port.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_multicast_address( const char* address) = 0;

    /// Returns the %base multicast address.
    ///
    /// \see #set_multicast_address()
    ///
    /// \return                        A numerical representation of the %base multicast address
    ///                                and port.
    virtual const IString* get_multicast_address() const = 0;

    /// Sets the time to live (TTL/hops) of multicast packets.
    ///
    /// The default value is 1.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_multicast_ttl()
    ///
    /// \param ttl                     The new TTL/hop value.
    virtual void set_multicast_ttl( Uint32 ttl) = 0;

    /// Returns the set multicast TTL/hop value.
    ///
    /// \see #set_multicast_ttl()
    ///
    /// \return                        The current TTL/hop value.
    virtual Uint32 get_multicast_ttl() const  = 0;

    /// Sets the interface to be used for outgoing packets.
    ///
    /// If not set, it will be the any address. The string may end with : and a port number to
    /// select which port to listen to for UDP and TCP unicast. If no port is set and unicast only
    /// mode is set, port 10000 will be used.
    ///
    /// The IP address of the interface can also be specified as a sub net using the CIDR
    /// notation a.b.c.d/xx. If there is an interface on the host with an address inside this
    /// range the first match will be used. This is useful for example when configuring several
    /// hosts. This means that on a host which has the address 192.168.1.1, specifying the
    /// address as 192.168.0.0/16:10000 would make the host bind to the 192.168.1.1 address on
    /// port 10000.
    ///
    /// IPv6 addresses need to be surrounded by brackets but can otherwise be used just like its
    /// IPv4 counterparts mentioned above. The address [::] means the ``any IPv6 interface''
    /// address.
    ///
    /// It is also possible to specify an interface name instead of an address. DiCE will
    /// then take the address from the interface.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_cluster_interface()
    ///
    /// \param address                 The interface address and port.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_cluster_interface( const char* address) = 0;

    /// Returns the chosen cluster interface address.
    ///
    /// \see #set_cluster_interface()
    ///
    /// \return                        A numerical representation of the cluster interface
    ///                                address and port.
    virtual const IString* get_cluster_interface() const = 0;

    /// Sets the discovery address to be used for automatic host discovery.
    ///
    /// If it is a multicast address multicast will be used to find the other hosts. If it is
    /// a unicast address the discovery network assumes that it is the address of the master
    /// node which has the list of all other hosts. If that unicast address is the same as
    /// the address of the running host, the discovery network will consider itself the master.
    ///
    /// An IPv4 UDP multicast address is specified by an address in the range 224.0.0.0 to
    /// 239.255.255.255, inclusive, along with a port. An IPv6 multicast address is specified
    /// by an IPv6 address with a high-order byte ff. This will be used when
    /// #MODE_TCP_WITH_DISCOVERY is set. It must include a port, like ip:port.
    /// There is a default address which will be used if none is set, both for IPv4 and IPv6.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// Note that discovery mode will be disabled if any hosts are added using the
    /// add_configured_hosts method.
    ///
    /// \see #get_discovery_address()
    ///
    /// \param address                 The discovery address and port.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_discovery_address( const char* address) = 0;

    /// Returns the discovery address.
    ///
    /// \see #set_discovery_address()
    ///
    /// \return                        A numerical representation of the discovery address and
    ///                                port.
    virtual const IString* get_discovery_address() const = 0;

    /// Sets the discovery identifier.
    ///
    /// The discovery identifier restricts the cluster to hosts that use the same identifier.
    /// This can be used to make sure that only compatible or selected nodes can join the cluster.
    /// In particular, if several clusters exist this identifier can be used to ensure that the
    /// host does not by accident join the wrong cluster. There is a default identifier which will
    /// be used if none is set.
    ///
    /// \note This is an additional check on top of the discovery, it is not an alternative to
    /// for example using different multicast addresses for different clusters. If two hosts with  
    /// different identifiers see each other through the discovery mechanisms then an error will
    /// be issued and the hosts will not be able to join a cluster.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_discovery_identifier()
    ///
    /// \param identifier              The discovery identifier.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_discovery_identifier( const char* identifier) = 0;

    /// Returns the discovery identifier.
    ///
    /// \see #set_discovery_identifier()
    ///
    /// \return                        The discovery identifier.
    virtual const IString* get_discovery_identifier() const = 0;

    // Configuration of redundancy

    /// Sets the redundancy level to be used for storing objects in the database.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_redundancy_level()
    ///
    /// \param level    The redundancy level. This is limited to a compiled in maximum that
    ///                 is currently set to 4. If the given value exceeds the possible
    ///                 maximum the return value will signal an error but the value will
    ///                 be set to the possible maximum. \note This value must be set to
    ///                 the same value on all hosts.
    /// \return         0, in case of success, -1 in case of failure.
    virtual Sint32 set_redundancy_level( Uint32 level) = 0;

    /// Returns the redundancy level.
    ///
    /// \see #set_redundancy_level()
    ///
    /// \return                        The configured redundancy level.
    virtual Uint32 get_redundancy_level() const = 0;

    // Configure and query the list of hosts which can connect with this host

    /// Adds a host to the list of hosts to connect to.
    ///
    /// This is used in the unicast network modes to specify the hosts to connect to.
    /// Note that discovery mode will be disabled if the host list is populated.
    ///
    /// \see #remove_configured_host(), #get_number_of_configured_hosts(), #get_configured_host()
    ///
    /// \param address                 The address of the host.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 add_configured_host( const char* address) = 0;

    /// Removes a host from the list of hosts to connect to.
    ///
    /// This is used in the unicast network modes to specify the hosts to connect to.
    ///
    /// \see #add_configured_host(), #get_number_of_configured_hosts(), #get_configured_host()
    ///
    /// \param address                 The address of the host.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 remove_configured_host( const char* address) = 0;

    /// Returns the current number of configured hosts.
    ///
    /// \see #add_configured_host(), #remove_configured_host(), #get_configured_host()
    virtual Uint32 get_number_of_configured_hosts() const = 0;

    /// Returns the address of a host from the list of configured hosts.
    ///
    /// \see #add_configured_host(), #remove_configured_host(), #get_number_of_configured_hosts()
    ///
    /// \param index                   The index of the host to be returned.
    /// \return                        The address of the host, or \c NULL in case of invalid
    ///                                indices.
    virtual const IString* get_configured_host( Uint32 index) const = 0;

    /// Sets the compression level for network traffic.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_compression_level()
    ///
    /// \param level                   The desired compression level. Valid values are 0 to 9.
    ///                                The value 0 disables compression. The higher the value,
    ///                                the higher the efforts to compress network traffic and
    ///                                the smaller the amount of data transferred across the
    ///                                network.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_compression_level( Uint32 level) = 0;

    /// Returns the compression level for network traffic.
    ///
    /// \see #set_compression_level()
    virtual Sint32 get_compression_level() const = 0;

    // Callbacks for joining / leaving hosts

    /// Registers a callback for cluster changes.
    ///
    /// For example, this callback can be used to get notifications whenever a host joins or leaves
    /// the cluster, or when its properties change.
    ///
    /// \see #unregister_host_callback()
    ///
    /// \param callback                The callback to be registered.
    /// \return
    ///                                -  0: Success.
    ///                                - -1: Invalid parameters (\c NULL pointer).
    virtual Sint32 register_host_callback( IHost_callback* callback) = 0;

    /// Unregisters a callback for cluster changes.
    ///
    /// \see #register_host_callback()
    ///
    /// \param callback                The callback to be unregistered.
    /// \return
    ///                                -  0: Success.
    ///                                - -1: Invalid parameters (\c NULL pointer).
    ///                                - -2: \p callback is not a registered callback.
    virtual Sint32 unregister_host_callback( IHost_callback* callback) = 0;

    /// Returns an interface to inquire network statistics.
    ///
    /// \return                        The network statistics object.
    virtual const INetwork_statistics* get_network_statistics() const = 0;

    /// Enables or disables the usage of RDMA InfiniBand. 
    /// 
    /// The default value is false (disabled).
    /// 
    /// \see #get_use_rdma()
    ///
    /// \param use_rdma   Indicates whether RDMA InfiniBand should be used (if it is available).
    /// \return           0, in case of success, -1 in case of failure.
    virtual Sint32 set_use_rdma( bool use_rdma) = 0;

    /// Indicates whether RDMA InfiniBand is enabled.
    ///
    /// \see #set_use_rdma()
    virtual bool get_use_rdma() const = 0;

    /// Set the RDMA InfiniBand interface to be used.
    ///
    /// The string can be either the interface name or it's address, which can be expressed in
    /// CIDR format as well, similar to #set_cluster_interface. Note that only interface names
    /// that start with the prefix 'ib' are accepted.
    ///
    /// If unspecified, an arbitrary RDMA InfiniBand interface will be chosen.
    ///
    /// This can only be configured before \NeurayProductName has been started.
    ///
    /// \see #get_rdma_interface()
    ///
    /// \param rdma_interface          The RDMA InfiniBand interface to be used, either \c "ib0",
    ///                                \c "ib1", etc. or the address of an InfiniBand interface.
    /// \return                        0, in case of success, -1 in case of failure.
    virtual Sint32 set_rdma_interface( const char* rdma_interface) = 0;

    /// Returns RDMA InfiniBand interface to be used.
    ///
    /// \see #get_rdma_interface()
    ///
    /// \return                        The RDMA interface to be used, or \c NULL if none is found.
    virtual const IString* get_rdma_interface() const = 0;
};

mi_static_assert( sizeof( INetwork_configuration::Status) == sizeof( Uint32));
mi_static_assert( sizeof( INetwork_configuration::Mode) == sizeof( Uint32));

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#define Status int
#endif // _XLIB_H_ || _X11_XLIB_H_

#endif // MI_NEURAYLIB_INETWORK_CONFIGURATION_H
