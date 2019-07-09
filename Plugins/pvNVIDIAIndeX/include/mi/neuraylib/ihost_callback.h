/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Callback interface for notifications about joining or leaving hosts.

#ifndef MI_NEURAYLIB_IHOST_CALLBACK_H
#define MI_NEURAYLIB_IHOST_CALLBACK_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

class IHost_properties;

/** \addtogroup mi_neuray_configuration
@{
*/

/// Abstract interface to report cluster status changes.
///
/// Each of the methods below deals with a certain type of cluster status changes, for example,
/// hosts joining or leaving a cluster.
///
/// Users can implement this interface and register an instance of the implementation class as
/// callback object to be notified in case of cluster status changes (see
/// #mi::neuraylib::INetwork_configuration::register_host_callback()).
///
/// \note Instances of this interface should not be created on the stack, since this might lead
/// to premature destruction of such instances while still being in use by \neurayProductName.
class IHost_callback : public
    mi::base::Interface_declare<0x39163199,0xfd6a,0x4c53,0xa8,0x00,0xdd,0x70,0xc6,0xbc,0x61,0xf3>
{
public:
    /// This function is called when this host established or lost the connection to the cluster.
    ///
    /// It should return as soon as possible because it may block further network operations.
    ///
    /// \param host_id          The host ID of this host. This ID is guaranteed to be unique in a
    ///                         cluster and will never be reused for another host.
    /// \param flag             \c true in case of an established connection, \c false in case of
    ///                         a lost connection.
    virtual void connection_callback(Uint32 host_id, bool flag) = 0;

    /// This function is called when a remote host joined or left the cluster.
    ///
    /// It should return as soon as possible because it may block further network operations.
    ///
    /// \param host_id          The host ID of the host joining or leaving. This ID is guaranteed
    ///                         to be unique in a cluster and will never be reused for another
    ///                         host.
    /// \param flag             \c true in case of a joining host, \c false in case of a leaving
    ///                         host.
    virtual void membership_callback(Uint32 host_id, bool flag) = 0;

    /// This function is called when a remote host communicates its properties.
    ///
    /// It should return as soon as possible because it may block further network operations.
    ///
    /// \param host_id          The host ID of the remote host. This ID is guaranteed to be unique
    ///                         in a cluster and will never be reused for another host.
    /// \param properties       The properties of the remote host, e.g., its address.
    virtual void property_callback(Uint32 host_id, const IHost_properties* properties) = 0;

    /// This function is called whenever the synchronizer host changes.
    ///
    /// It can be used to record the synchronizer ID for display usages. Since it is guaranteed that
    /// there is at any point in time exactly one synchronizer in a working cluster it can also be
    /// used to do certain tasks only on the synchronizer.
    ///
    /// \param host_id          The host ID of the new synchronizer.
    virtual void synchronizer_callback(Uint32 host_id) = 0;

    /// This function is called when the database changes status.
    ///
    /// When the database is fully operational the function will be called first with the status \c
    /// "ok". When the database is recovering from a host failure the function will be called with
    /// the status \c "recovery". When the recovery is finished the function will be called with the
    /// status \c "ok" again.
    ///
    /// Note that the database can switch to recovery mode when a host is started. This happens when
    /// the configured redundancy level could not be reached because simply not enough hosts where
    /// present before.
    ///
    /// \param status           The status of the database.
    virtual void database_status_callback(const char* status) = 0;
};

/*@}*/ // end group mi_neuray_configuration

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IHOST_CALLBACK_H
