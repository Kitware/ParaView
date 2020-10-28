/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for sparse volumes.

#ifndef NVIDIA_INDEX_ISPARSE_VOLUME_SCENE_ELEMENT_H
#define NVIDIA_INDEX_ISPARSE_VOLUME_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>

namespace nv {
namespace index {

/// Interface for sparse volume scene elements.
///
/// \ingroup nv_index_scene_description_distributed_data
///
class ISparse_volume_scene_element :
    public  mi::base::Interface_declare<0xbb6ad4b7,0xd42c,0x47f7,0xb4,0x46,0xe1,0xb,0x1,0xb,0x76,0x1b,
                                        nv::index::IDistributed_data>
{
public:
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ISPARSE_VOLUME_SCENE_ELEMENT_H
