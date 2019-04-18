/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Plane specific pick results returned by the NVIDIA IndeX library when querying a scene's
/// contents using the pick operation.

#ifndef NVIDIA_INDEX_IPLANE_PICK_RESULT_H
#define NVIDIA_INDEX_IPLANE_PICK_RESULT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{

/// @ingroup scene_queries

/// Interface class that returns the plane specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional
/// intersection results specific to \c IPlane scene elements.
///
class IPlane_pick_result
  : public mi::base::Interface_declare<0x0008694c, 0x2dc5, 0x4e07, 0xb5, 0x4c, 0xd7, 0x09, 0xb7,
      0x29, 0x87, 0x5e, nv::index::IScene_pick_result>
{
public:
  /// Returns the texture color value at the intersection point. This value is either
  /// a computed texture color through an \c IDistributed_compute_technique or a
  /// volume sampled and colormapped through a \c IRegular_volume_texture.
  ///
  /// \return     Returns the texture color at the intersection point.
  ///
  virtual const mi::math::Color_struct& get_texture_color() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_IPLANE_PICK_RESULT_H
