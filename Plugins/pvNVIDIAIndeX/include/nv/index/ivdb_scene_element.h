/******************************************************************************
 * Copyright 2021 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for VDB data.

#ifndef NVIDIA_INDEX_IVDB_SCENE_ELEMENT_H
#define NVIDIA_INDEX_IVDB_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>

namespace nv {
namespace index {

/// Interface for VDB scene elements.
///
/// \ingroup nv_index_scene_description_distributed_data
///
class IVDB_scene_element :
    public  mi::base::Interface_declare<0x5c983181,0x8b9,0x4603,0xb9,0xf9,0xc3,0x7d,0xfe,0x3c,0x34,0xe1,
                                        nv::index::IDistributed_data>
{
public:
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IVDB_SCENE_ELEMENT_H
