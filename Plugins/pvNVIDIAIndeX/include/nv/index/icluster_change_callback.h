/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Cluster-change callbacks.

#ifndef MI_NVIDIA_INDEX_ICLUSTER_CHANGE_CALLBACK_H
#define MI_NVIDIA_INDEX_ICLUSTER_CHANGE_CALLBACK_H

#include <mi/base/types.h>
#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv
{
namespace index
{

/// The interface class allows implementing user-defined callbacks
/// issued whenever the cluster topology has changed.
///
/// @ingroup nv_index_utilities
class ICluster_change_callback :
    public mi::base::Interface_declare<0x38c98da4,0xe5dc,0x47c4,0x97,0x45,0xdc,0x7f,0x36,0x77,0x60,0xd4>
{
public:
    /// The updates to the cluster topology will be reported to the callback.
    ///
    /// \param[in] machine_id   The unique id of the machine that left or joined
    ///                         the cluster.
    ///
    /// \param[in] machine_name The name of the machine that left or joined
    ///                         the cluster.
    ///
    /// \param[in] change       The flag that notifies a joining or leaving
    ///                         event.
    ///
    virtual void cluster_change(mi::Uint32 machine_id, const char* machine_name, bool change) = 0;
};

}} // namespace index / nv

#endif // MI_NVIDIA_INDEX_ICLUSTER_CHANGE_CALLBACK_H
