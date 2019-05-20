/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// Scene element representing corner-point grids.

#ifndef NVIDIA_INDEX_ICORNER_POINT_GRID_H
#define NVIDIA_INDEX_ICORNER_POINT_GRID_H

#include <nv/index/idistributed_data.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_shape
/// Scene element representing corner-point grids.
///
class ICorner_point_grid : public mi::base::Interface_declare<0xd350be27, 0x9904, 0x49b7, 0x98,
                             0x77, 0xa7, 0x15, 0xf1, 0x50, 0xc4, 0x29, nv::index::IDistributed_data>
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ICORNER_POINT_GRID_H
