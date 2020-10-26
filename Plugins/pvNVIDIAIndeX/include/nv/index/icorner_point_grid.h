/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// Scene element representing corner-point grids.

#ifndef NVIDIA_INDEX_ICORNER_POINT_GRID_H
#define NVIDIA_INDEX_ICORNER_POINT_GRID_H

#include <nv/index/idistributed_data.h>

namespace nv {
namespace index {

/// Scene element representing corner-point grids.
///
/// \ingroup nv_index_scene_description_distributed_data
///
class ICorner_point_grid :
    public  mi::base::Interface_declare<0xd350be27,0x9904,0x49b7,0x98,0x77,0xa7,0x15,0xf1,0x50,0xc4,0x29,
                                        nv::index::IDistributed_data>
{
public:
    /// Returns the dimensions of the underlying layered grid data.
    ///
    /// The x-y dimensions give the grid resolution, and the z dimensions gives the 
    /// number of layers in the dataset.
    ///
    virtual mi::math::Vector_struct<mi::Uint32, 3>  get_grid_dimensions() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ICORNER_POINT_GRID_H
