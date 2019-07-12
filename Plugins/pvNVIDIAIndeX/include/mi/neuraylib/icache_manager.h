/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component to interact with the cache manager.

#ifndef MI_NEURAYLIB_ICACHE_MANAGER_H
#define MI_NEURAYLIB_ICACHE_MANAGER_H

#include <mi/base/interface_declare.h>

namespace mi {

class IString;

namespace neuraylib {

/// Represents an instance of the cache manager.
///
/// You cannot directly interact with the cache manager yourself. The purpose of this interface is
/// to allow you to start a cache manager such that it can be used by other components, e.g., the
/// Bridge, see #mi::bridge::IApplication::set_disk_cache() \if IRAY_API , or the Iray Bridge, see
/// #mi::bridge::IIray_bridge_application::set_disk_cache() \endif .
///
/// \see #mi::neuraylib::ICache_manager_factory
class ICache_manager : public
    mi::base::Interface_declare<0x6b063e2d,0x41cf,0x4a48,0x8b,0xdf,0x2e,0x41,0x65,0xfe,0x48,0xb7>
{
public:
    /// Starts the cache manager.
    ///
    /// \param listen_address           The address to listen on.
    /// \param directory                The directory to be used by the cache manager to store its
    ///                                 data.
    /// \return
    ///                                 -  0: Success.
    ///                                 - -1: The cache manager is already running.
    ///                                 - -2: Invalid parameters (\p listen_address or \p directory
    ///                                       is \c NULL).
    ///                                 - -3: Failed to create directory for the disk cache.
    ///                                 - -4: Directory for the disk cache is not writable.
    ///                                 - -5: Failed to open the database.
    ///                                 - -6: Database locked by another instance (from the same or
    ///                                       a different process).
    ///                                 - -7: Failed to upgrade the database.
    ///                                 - -8: Failed to listen on the listen address.
    ///                                 - -9: Failed to establish cluster on the multicast address,
    ///                                       cluster interface, and/or RDMA interface.
    virtual Sint32 start( const char* listen_address, const char* directory) = 0;

    /// Shuts down the cache manager.
    ///
    /// \return
    ///                                 -  0: Success.
    ///                                 - -1: The cache manager is not running.
    virtual Sint32 shutdown() = 0;

    /// Sets the address and authentication information of the HTTP server.
    ///
    /// \param address                  The address on which the HTTP server listens, or \c NULL to
    ///                                 disable authentication.
    /// \param user_name                The user name for authentication with the HTTP server, or
    ////                                \c NULL to disable authentication.
    /// \param password                 The password for authentication with the HTTP server, or
    ///                                 \c NULL to disable authentication.
    /// \return                         -  0: Success.
    ///                                 - -1: The cache manager is already running.
    ///                                 - -2: Invalid address.
    virtual Sint32 set_http_address(
        const char* address, const char* user_name, const char* password) = 0;

    /// Returns the address of the HTTP server, or \c NULL if none is set.
    virtual const IString* get_http_address() const = 0;

    /// Returns the user name for the HTTP server, or \c NULL if none is set.
    virtual const IString* get_http_user_name() const = 0;

    /// Returns the password for the HTTP server, or \c NULL if none is set.
    virtual const IString* get_http_password() const = 0;

    /// Sets the multicast address of the cache manager.
    ///
    /// The multicast address is used to create a cluster of cache managers.
    ///
    /// \see #mi::neuraylib::INetwork_configuration::set_multicast_address() for details about
    /// multicast addresses.
    ///
    /// \param address                  The multicast address to be used, or \c NULL to disable
    ///                                 clustering.
    /// \return                         -  0: Success.
    ///                                 - -1: The cache manager is already running.
    ///                                 - -2: Invalid address.
    virtual Sint32 set_multicast_address( const char* address) = 0;

    /// Returns the multicast address, or \c NULL if none is set.
    virtual const IString* get_multicast_address() const = 0;

    /// Sets the cluster interface used by the cache manager.
    ///
    /// \see #mi::neuraylib::INetwork_configuration::set_cluster_interface() for details about
    /// cluster interfaces.
    ///
    /// \param address                  The cluster interface to be used, or \c NULL to disable
    ///                                 a previous setting.
    /// \return                         -  0: Success.
    ///                                 - -1: The cache manager is already running.
    ///                                 - -2: Invalid address.
    virtual Sint32 set_cluster_interface( const char* address) = 0;

    /// Returns the cluster interface, or \c NULL if none is set.
    virtual const IString* get_cluster_interface() const = 0;

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
    /// If unspecified, an arbitrary RDMA InfiniBand interface will be chosen.
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

    /// Sets the address for the administrative HTTP server.
    ///
    /// \see #mi::neuraylib::IGeneral_configuration::set_admin_http_address() for details about
    /// administrative HTTP servers.
    ///
    /// \param address                  The address to be used, or \c NULL to disable the
    ///                                 administrative HTTP server.
    /// \return                         -  0: Success.
    ///                                 - -1: The cache manager is already running.
    ///                                 - -2: Invalid address.
    virtual Sint32 set_admin_http_address( const char* address) = 0;

    /// Returns the address of the admin HTTP server, or \c NULL if none is set.
    virtual const IString* get_admin_http_address() const = 0;

    /// Sets the memory limits for the garbage collection.
    ///
    /// The cache manager uses a garbage collection scheme to attempt to keep the total amount of
    /// memory, i.e., disk space, below a configurable limit called \c high \c water \c mark. If
    /// that limit is exceeded the garbage collection reduces memory usage usage until another
    /// limit, the \c low \c water \c mark, is reached (or no further memory reduction is
    /// possible).
    ///
    /// \note No guarantee is given that the high water mark is not exceeded. In particular, cache
    ///       elements may be retained which prevents them from cache eviction.
    ///
    /// \note The memory usage here refers to the size of the cache values itself. The meta data
    ///       for managing the cache as well as overhead from the database backend are not
    ///       included here.
    ///
    /// \param low_water                The garbage collection stops when memory usage (in bytes)
    ///                                 drops below this value.
    /// \param high_water               The garbage collection starts when memory usage (in bytes)
    ///                                 exceeds this value. The value 0 disables the garbage
    ///                                 collection.
    /// \return
    ///                                 -  0: Success.
    ///                                 - -1: Invalid parameters (\p low_water is not less than
    ///                                       \p high_water unless \p high_water equals zero).
    virtual Sint32 set_memory_limits( Size low_water, Size high_water) = 0;

    /// Returns the memory limits for the garbage collection.
    virtual void get_memory_limits( Size* low_water, Size* high_water) const = 0;

    /// Enables or disables checking of reference counts.
    ///
    /// If enabled, reference counts of cache elements are checked during startup and adjusted as
    /// necessary. Depending on the cache size this can take some time and delay startup.
    virtual Sint32 set_reference_count_checking( bool enabled) = 0;

    /// Indicates whether checking of reference counts is enabled.
    virtual bool get_reference_count_checking() const = 0;
};

/// This factory allows to create cache manager instances.
class ICache_manager_factory : public
    mi::base::Interface_declare<0xd5908356,0x1686,0x4299,0xae,0x10,0x8c,0x88,0x71,0x43,0x1f,0x04>
{
public:
    /// Creates a new cache manager instance.
    virtual ICache_manager* create_cache_manager() = 0;
};

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ICACHE_MANAGER_H
