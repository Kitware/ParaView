/******************************************************************************
 * Copyright 2023 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Distributed subsets of VDB datasets.

#ifndef NVIDIA_INDEX_IVDB_SUBSET_H
#define NVIDIA_INDEX_IVDB_SUBSET_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/math/vector.h>

#include <nv/index/idistributed_data_subset.h>

namespace nv {
namespace index {

/// Forward declarations
class IVDB_subset;
class IVDB_subset_device;

/// Distributed data storage class for VDB subsets.
///
/// Data access (e.g., import, editing) for VDB data associated with \c IVDB_scene_element 
/// instances is performed through instances of this subset class. A subset of a VBD instance is defined
/// by all data bricks/nodes intersecting a rectangular subregion of the entire scene/dataset.
///
/// \ingroup nv_index_data_subsets
///
class IVDB_subset :
    public mi::base::Interface_declare<0xe0555782,0x2353,0x4d3b,0xaf,0x9d,0xa4,0xbb,0xbc,0x10,0xa9,0xec,
                                       IDistributed_data_subset>
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// experimental API - subject to change! /////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
public:
    /// Representing the NanoVDB version.
    struct NVDB_version {
        mi::Uint64 magic;           ///<! Magic number.
        mi::Uint32 major;           ///<! Major of NanoVDB version.
        mi::Uint32 minor;           ///<! Minor of NanoVDB version.
    };

    struct NVDB_grid_storage {
        mi::Size    size;
        void*       data;
    };

public:
    /// Generating a grid storage.
    ///
    /// \param[in] attribute_index      The attribute index to generate the volume storage for.
    /// \param[in] grid_memory_size     The memory size of the grid.
    /// \param[in] nvdb_version         The NanoVDB version.
    /// 
    /// \return                         Returns \c true if the grid generation was successful, otherwise \c false.
    /// 
    virtual bool                    generate_grid_storage(
                                        mi::Uint32      attribute_index,
                                        mi::Size        grid_memory_size,
                                        NVDB_version    nvdb_version) = 0;

    /// Accessing the internal grid storage.
    ///
    /// \param[in] attribute_index      The attribute index to return the volume storage for.
    ///
    /// \return     Returns the internal grid data storage.
    /// 
    virtual NVDB_grid_storage       get_grid_storage(
                                        mi::Uint32      attribute_index) const = 0;

    /// Accessing the device VDB subset for direct access to the VDB device resources
    ///
    /// \return     Returns the device-handled seubset for direct access to the VDB device resources,
    ///             will return \c NULL if data is not stored on device (yet).
    /// 
    virtual IVDB_subset_device*     get_device_subset() const = 0;
};

/// Distributed data storage class for VDB subsets hosted on a GPU device.
///
/// Data access (e.g., compute and editing) for device VDB data associated with \c IVDB_scene_element 
/// instances is performed through instances of this subset class. A subset of a VBD instance is defined
/// by all data bricks/nodes intersecting a rectangular subregion of the entire scene/dataset.
///
/// \ingroup nv_index_data_subsets
///
class IVDB_subset_device :
    public mi::base::Interface_declare<0xee4883a1,0xf5d3,0x4a42,0xaa,0x81,0x37,0xdd,0xe3,0x6c,0x8e,0xc8,
                                       IDistributed_data_subset_device>
{
public:
    typedef IVDB_subset::NVDB_version   NVDB_version;

public:
    virtual NVDB_version        nvdb_version() const = 0;

    virtual void*               grid_buffer(
                                    mi::Uint32      attribute_index) = 0;

    virtual mi::Size            grid_buffer_size(
                                    mi::Uint32      attribute_index) const = 0;
    virtual bool                grid_buffer_resize(
                                    mi::Uint32      attribute_index,
                                    const mi::Size  new_size) = 0;

    // #todo take ownership?
    // - set grid_data_size to ~0x00000000u for unknown size.
    virtual bool                adopt_grid_buffer(
                                    mi::Uint32  attribute_index,
                                    void*       grid_data,
                                    mi::Size    grid_data_size) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IVDB_SUBSET_H
