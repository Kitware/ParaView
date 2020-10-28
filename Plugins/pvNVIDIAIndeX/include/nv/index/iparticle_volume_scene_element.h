/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for particle volumes.

#ifndef NVIDIA_INDEX_IPARTICLE_VOLUME_SCENE_ELEMENT_H
#define NVIDIA_INDEX_IPARTICLE_VOLUME_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>

namespace nv {
namespace index {

/// Interface for particle volume scene elements.
///
/// \ingroup nv_index_scene_description_distributed_data
///
class IParticle_volume_scene_element :
    public  mi::base::Interface_declare<0xf81de023,0xd8a8,0x4fea,0x90,0x62,0x8a,0x1c,0x3d,0x8e,0xbd,0x1c,
                                        nv::index::IDistributed_data>
{
public:
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IPARTICLE_VOLUME_SCENE_ELEMENT_H
