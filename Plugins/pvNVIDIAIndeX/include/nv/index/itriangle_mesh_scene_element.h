/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene element representing triangle meshes.

#ifndef NVIDIA_INDEX_ITRIANGLE_MESH_SCENE_ELEMENT_H
#define NVIDIA_INDEX_ITRIANGLE_MESH_SCENE_ELEMENT_H

#include <mi/math/bbox.h>
#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data.h>
#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
/// Triangle mesh interface class that represents a large-scale triangle mesh in the scene description. 
///
class ITriangle_mesh_scene_element :
    public  mi::base::Interface_declare<0x8fcb5afd,0x937f,0x40c9,0xb4,0xf6,0xe3,0x52,0x1d,0x9d,0xd4,0xcf,
                                        nv::index::IDistributed_data>
{
};

}}

#endif // NVIDIA_INDEX_ITRIANGLE_MESH_SCENE_ELEMENT_H
