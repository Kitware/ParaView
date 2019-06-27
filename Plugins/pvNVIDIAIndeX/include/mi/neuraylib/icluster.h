/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Additional clusters.

#ifndef MI_NEURAYLIB_ICLUSTER_H
#define MI_NEURAYLIB_ICLUSTER_H

#include <mi/base/interface_declare.h>
#include <mi/neuraylib/ineuray.h>

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#undef Status
#endif // _XLIB_H_ || _X11_XLIB_H_

namespace mi {

namespace neuraylib {

class IHost_properties;

/** \addtogroup mi_neuray_cluster
@{
*/

/// This class represents an additional cluster to be used for storing data, communicating, and
/// executing jobs.
///
/// This interface is similar to #mi::neuraylib::INeuray in the sense that it exposes its
/// functionality via several API components. In contrast to #mi::neuraylib::INeuray it offers only
/// a small set of API components that are relevant for the functionality of the additional cluster,
/// e.g., networking and scheduling configuration, the cluster-specific database and distributed
/// cache. Instances of this interface can be created via the API component
/// #mi::neuraylib::ICluster_factory.
///
/// \note
///   Each cluster has its own database and distributed cache. The database from which the scope
///   and transaction was obtained controls which cluster executes submitted jobs.
///
/// \note
///   Log messages are never forwarded via an additional cluster (from the view point of the
///   sending host). This might only happen via the main cluster. Although hosts may receive log
///   messages from other hosts via an additional cluster (from the view point of the receiving
///   host) in case this cluster is the main cluster for the sending host.
class ICluster : public
    mi::base::Interface_declare<0x4511782c,0x83cb,0x4d92,0xbd,0xb8,0x4d,0x41,0xad,0x4f,0xd2,0xeb>
{
public:
    /// Returns an API component belonging to this cluster.
    ///
    /// This interface supports only a subset of the available API components, namely those which
    /// are available per cluster. Currently, the supported API components are:
    /// - #mi::neuraylib::INetwork_configuration: the networking settings for this cluster are
    ///   set here.
    /// - #mi::neuraylib::IScheduling_configuration: this can be used to influence work delegation
    ///   and thread pool size.
    /// - #mi::neuraylib::IDatabase: the cluster-specific database.
    /// - #mi::neuraylib::IDistributed_cache: the cluster-specific distributed cache.
    ///
    /// \param uuid        The interface ID of the corresponding interface.
    /// \return            A pointer to the API component or \c NULL if the API component is not
    ///                    supported or currently not available.
    virtual base::IInterface* get_api_component( const base::Uuid& uuid) const = 0;

    /// Returns an API component belonging to this cluster.
    ///
    /// This interface supports only a subset of the available API components, namely those which
    /// are available per cluster. Currently, the supported API components are:
    /// - #mi::neuraylib::INetwork_configuration: the networking settings for this cluster are
    ///   set here.
    /// - #mi::neuraylib::IScheduling_configuration: this can be used to influence work delegation
    ///   and thread pool size.
    /// - #mi::neuraylib::IDatabase: the cluster-specific database.
    /// - #mi::neuraylib::IDistributed_cache: the cluster-specific distributed cache.
    ///
    /// \tparam T          The type of the API components to be queried.
    /// \return            A pointer to the API component or \c NULL if the API component is not
    ///                    supported or currently not available.
    template<class T>
    T* get_api_component() const
    {
        base::IInterface* ptr_iinterface = get_api_component( typename T::IID());
        if ( !ptr_iinterface)
            return 0;
        T* ptr_T = static_cast<T*>( ptr_iinterface->get_interface( typename T::IID()));
        ptr_iinterface->release();
        return ptr_T;
    }

    /// Joins the cluster.
    ///
    /// This method should be called after all parameters for the cluster have been configured. It
    /// will then join the cluster according to the given settings.
    ///
    /// \param blocking    Indicates whether the startup should be done in blocking mode. If
    ///                    \c true the method will not return before all initialization was done.
    ///                    If \c false the method will return immediately and the startup is done
    ///                    in a separate thread. The status of the startup sequence can be checked
    ///                    via #get_status().
    /// \return
    ///                    -  0: Success
    ///                    - -1: Unspecified failure.
    virtual Sint32 join( bool blocking = true) = 0;

    /// Leaves the cluster.
    virtual Sint32 leave( bool blocking = true) = 0;

    /// Returns the status of this cluster.
    ///
    /// \return            The status
    virtual neuraylib::INeuray::Status get_status() const = 0;

    /// Returns the host properties for this host.
    ////
    /// The host properties returned by this method are specific for this cluster. The host
    /// properties handled in #mi::neuraylib::IGeneral_configuration apply only to the main cluster.
    ///
    /// Host properties are only available after the additional cluster has been started.
    ///
    /// \return            The host properties for this host, or \c NULL on error.
    virtual const IHost_properties* get_host_properties() const = 0;

    /// Sets a host property for this host.
    ///
    /// The host properties set by this method are specific for this cluster. The host
    /// properties handled in #mi::neuraylib::IGeneral_configuration apply only to the main cluster.
    ///
    /// The change will be propagated to all other hosts in the cluster.
    /// The method only works if networking support is available.
    ///
    /// \param key         The key for the property.
    /// \param value       The property value.
    /// \return            0, in case of success, -1 in case of failure.
    virtual Sint32 set_host_property( const char* key, const char* value) = 0;
};

/*@}*/ // end group mi_neuray_cluster

} // namespace neuraylib

} // namespace mi

// X11/Xlib.h defines Status to int
#if defined(_XLIB_H_) || defined(_X11_XLIB_H_)
#define Status int
#endif // _XLIB_H_ || _X11_XLIB_H_

#endif // MI_NEURAYLIB_ICLUSTER_H
