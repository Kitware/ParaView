/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief  Interface class for implementing user-defined snapping algorithms.

#ifndef NVIDIA_INDEX_ISNAPPING_ALGORITHM_H
#define NVIDIA_INDEX_ISNAPPING_ALGORITHM_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/idistributed_data_access.h>

namespace nv
{
namespace index
{
/// @ingroup nv_index_data_computing

/// Interface class that enables the implementation of user-defined snapping algorithms.
/// The 'picking' operation determines the intersection point between a ray cast from a
/// camera position through pixel coordinate with a slice in the scene. The returned
/// vertex usually extends a heightfield dataset by a new point of support.
///
/// A snapping algorithm allows for a user-defined computing of a new pick position,
/// for instance, based on the volume amplitude values below and above the original
/// pick location. The interface class \c IHeightfield_interaction provides means to
/// register a user-defined snapping algorithm when invoking the manual pick operation.
///
class ISnapping_algorithm : public mi::base::Interface_declare<0xda250d90, 0x49f6, 0x4804, 0x9b,
                              0x83, 0x9f, 0x97, 0x86, 0xcc, 0xc4, 0x1b>
{
public:
  /// The interface method allows implementing a user-defined algorithm
  /// that adjusts the manual pick results based on the amplitude values
  /// of the volume associated with the slice picked against.
  ///
  /// \param[in]  pick                The original manual pick position that is given in the
  ///                                 slice's IJK space and results from the ray intersecting
  ///                                 with a slice in the scene.
  /// \param[in]  data_access         The volume data access provides means to access the
  ///                                 volume amplitude values required for computing the
  ///                                 snapped pick position. Usually the new pick position is based
  ///                                 on the volume amplitude values below and above the
  ///                                 ray/slice intersection point.
  /// \param[in]  dice_transaction    The DiCE transaction that the snapping algorithm performs in.
  /// \param[out] result              Returns the resulting new pick position in the slice's IJK
  ///                                 space.
  ///
  /// \return                         Returns 'true' if the pick was valid or 'false' otherwise.
  ///                                 If a valid pick has been computed, then the returned
  ///                                 new pick position will be passed as a new vertex
  ///                                 to the heightfield.
  ///
  virtual bool adjust_pick(const mi::math::Vector_struct<mi::Float32, 3>& pick,
    nv::index::IRegular_volume_data_access* data_access,
    mi::neuraylib::IDice_transaction* dice_transaction,
    mi::math::Vector_struct<mi::Float32, 3>& result) const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ISNAPPING_ALGORITHM_H
