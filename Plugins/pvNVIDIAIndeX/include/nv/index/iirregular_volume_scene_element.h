/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element for irregular volumes.

#ifndef NVIDIA_INDEX_IIRREGULAR_VOLUME_SCENE_ELEMENT_H
#define NVIDIA_INDEX_IIRREGULAR_VOLUME_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>

namespace nv
{
namespace index
{

/// Interface for irregular volume scene elements.
///
/// \ingroup nv_index_scene_description_distributed_data
///
class IIrregular_volume_scene_element :
    public  mi::base::Interface_declare<0xf844b38d,0xc764,0x4f55,0xae,0xda,0xa9,0x41,0x1f,0xeb,0x3,0x6b,
                                        nv::index::IDistributed_data>
{
public:
    /// Returns the length of the longest edge in the irregular volume mesh.
    ///
    /// \note       This information is required for correct visual results. It sets an important
    ///             bound for the renderer to consider for setting up internal resources.
    ///
    /// \returns    The length of the longest edge in the irregular volume mesh.
    ///
    virtual mi::Float32 get_max_mesh_edge_length() const = 0;
};

}}

#endif // NVIDIA_INDEX_IIRREGULAR_VOLUME_SCENE_ELEMENT_H
