/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief  [...]
///
#ifndef NVIDIA_INDEX_IHEIGHT_FIELD_SCENE_ELEMENT_H
#define NVIDIA_INDEX_IHEIGHT_FIELD_SCENE_ELEMENT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>
#include <nv/index/idistributed_data.h>

namespace nv
{
namespace index
{

/// Interface for height field volume scene elements.
/// @ingroup nv_index_scene_description_shape
///
class IHeight_field_scene_element :
    public mi::base::Interface_declare<0x9c51971c,0xf04e,0x4761,0xbb,0x69,0x34,0xbd,0x7f,0xd5,0x8c,0xd1,
                                       nv::index::IDistributed_data>
{
public:
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IHEIGHT_FIELD_SCENE_ELEMENT_H
