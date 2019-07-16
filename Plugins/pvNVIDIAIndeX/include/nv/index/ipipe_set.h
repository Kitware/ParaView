/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/

#ifndef NVIDIA_INDEX_IPIPE_SET_H
#define NVIDIA_INDEX_IPIPE_SET_H

#include <nv/index/idistributed_data.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape

/// Pipe set.
class IPipe_set :
    public mi::base::Interface_declare<0x95073d19,0x97ff,0x4ab2,0xaf,0xbc,0x8a,0x2c,0x4f,0x49,0x29,0xb3,
                                       nv::index::IDistributed_data>
{
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPIPE_SET_H
