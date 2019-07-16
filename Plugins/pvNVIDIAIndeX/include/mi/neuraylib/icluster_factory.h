/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component to create additional clusters.

#ifndef MI_NEURAYLIB_ICLUSTER_FACTORY_H
#define MI_NEURAYLIB_ICLUSTER_FACTORY_H

#include <mi/base/interface_declare.h>

namespace mi {

namespace neuraylib {

class ICluster;

/** \defgroup mi_neuray_cluster Additional clusters
    \ingroup mi_neuray
    
    This module provides functionality to join additional clusters.
*/

/** \addtogroup mi_neuray_cluster
@{
*/

/// This API component is used to create additional clusters.
///
/// \see #mi::neuraylib::INeuray, #mi::neuraylib::IPlugin_api
class ICluster_factory : public
    mi::base::Interface_declare<0xfe3f4dd1,0xcec3,0x4576,0x82,0x0b,0xa7,0x66,0xe8,0x3e,0x4b,0x54>
{
public:
    /// Creates a new cluster instance.
    ///
    /// The cluster is initially not started or joined. The application can use the functionality in
    /// #mi::neuraylib::ICluster for configuration and then start the cluster.
    ///
    /// \return A new cluster instance,  or \c NULL if no cluster could be created.
    virtual ICluster* create_cluster() = 0;
};

/*@}*/ // end group mi_neuray_cluster

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_ICLUSTER_FACTORY_H
