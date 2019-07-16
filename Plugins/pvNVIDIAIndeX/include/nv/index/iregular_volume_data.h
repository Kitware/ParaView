/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces for regular volume voxel data returned by \c IRegular_volume_data_access.

#ifndef NVIDIA_INDEX_IREGULAR_VOLUME_DATA_H
#define NVIDIA_INDEX_IREGULAR_VOLUME_DATA_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv {
namespace index {

/// @ingroup nv_index_regular_volume_data
///
/// Base-interface class for all regular volume data interface classes. Regular volume data is
/// returned for example by \c IRegular_volume_data_access instances for distributed regular volume
/// data access or accepted by \c IRegular_volume_compute_task for the user-defined operation on
/// distributed regular volume data. The accessed volume data stored locally. Instances of
/// \c IRegular_volume_data and its sub-classes return the pointer to the volume data arrays. The
/// interface classes do only grant access to the volume data without 'owning' it. The ownership
/// of the volume data remains with the \c IRegular_volume_data_access instance returning a
/// \c IRegular_volume_data interface.
///
class IRegular_volume_data :
    public mi::base::Interface_declare<0x85e9531b,0x8469,0x4441,0xb2,0xf,0x4c,0xb7,0x91,0xe0,0x18,0xb9>
{
public:
    /// Returns the bounding box of the volume data. The bounding box encloses the entire exposed data, 
    /// i.e., a volume brick's border is included as well.
    ///
    /// \return     The bounding box of the exposed volume voxel data.
    ///
    virtual mi::math::Bbox_struct<mi::Sint32, 3> get_bounding_box() const = 0;
};

/// @ingroup nv_index_regular_volume_data
///
/// Intermediate-interface class for all typed data-sample interfaces. According to the actual
/// dataset component type (e.g. Uint8, Uint16, Float32, Rgba8) the data sample value can be retrieved.
///
template<typename T>
class IRegular_volume_data_typed :
    public mi::base::Interface_declare<0xaa57121c,0xc289,0x45c1,0xb1,0xce,0x6c,0x1e,0x24,0x70,0x57,0x92,
                                       IRegular_volume_data>
{
public:
    typedef T Value_type;

public:
    /// Returns a pointer to the regular volume voxel data according to the
    /// specific volume voxel type.
    ///
    /// \returns    Regular volume voxel data.
    ///
    virtual const T* get_voxel_data() const = 0;

    /// Returns a pointer to the mutable regular volume voxel data according to
    /// the specific volume voxel type.
    ///
    /// \returns    Mutable regular volume voxel data.
    ///
    virtual T* get_voxel_data_mutable() = 0;
};

/// @ingroup nv_index_regular_volume_data
///
/// Single-channel 8bit unsigned per voxel volume data.
///
class IRegular_volume_data_uint8 :
    public mi::base::Interface_declare<0x7381d9f7,0xce08,0x4d90,0x8d,0xe2,0x0,0x88,0x9b,0xb,0xe4,0xc0,
                                       IRegular_volume_data_typed<mi::Uint8> >
{
};

/// @ingroup nv_index_regular_volume_data
///
/// Single-channel 16bit unsigned per voxel volume data.
///
class IRegular_volume_data_uint16 :
    public mi::base::Interface_declare<0xd2f17224,0xa8ef,0x43a2,0x9f,0x40,0x5c,0x4c,0xd,0xed,0x57,0xcb,
                                       IRegular_volume_data_typed<mi::Uint16> >
{
};

/// @ingroup nv_index_regular_volume_data
///
/// Single-channel 32bit floating point per voxel volume data.
///
class IRegular_volume_data_float32 :
    public mi::base::Interface_declare<0x626285ff,0x4f80,0x43ac,0x85,0x51,0x7e,0x73,0x7f,0xd5,0x9c,0xb5,
                                       IRegular_volume_data_typed<mi::Float32> >
{
};

/// @ingroup nv_index_regular_volume_data
///
/// Four-channel 8bit per channel unsigned integer per voxel volume data.
///
class IRegular_volume_data_rgba8 :
    public mi::base::Interface_declare<0x3ecb4b7e,0xc9c5,0x476c,0xa7,0xf3,0xed,0x31,0x4b,0x43,0x1,0x68,
                                       IRegular_volume_data_typed<mi::math::Vector_struct<mi::Uint8, 4> > >        
{
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IREGULAR_VOLUME_DATA_H
