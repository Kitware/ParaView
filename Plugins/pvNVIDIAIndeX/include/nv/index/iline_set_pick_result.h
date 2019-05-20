/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Line set specific pick results returned by the NVIDIA IndeX library when querying a
/// scene's contents using the pick operation.

#ifndef NVIDIA_INDEX_ILINE_SET_PICK_RESULT_H
#define NVIDIA_INDEX_ILINE_SET_PICK_RESULT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{
/// @ingroup scene_queries

/// Interface class that returns the line set specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional
/// intersection results specific to \c ILine_set scene elements.
///
class ILine_set_pick_result
  : public mi::base::Interface_declare<0x42feaea6, 0xf3b0, 0x4427, 0x8a, 0xef, 0xfa, 0x0f, 0x8a,
      0x5f, 0xd0, 0x4a, nv::index::IScene_pick_result>
{
public:
  /// Returns the index that of a line segment in the set of lines.
  /// The line set contains an ordered set of lines that can be picked separately.
  /// A returned index i corresponds to the i-th segment in the set.
  ///
  /// \return     Returns the index of the picked point in the set of points.
  ///
  virtual mi::Uint32 get_segment() const = 0;
};
}
} // namespace index / nv

#endif // NVIDIA_INDEX_ILINE_SET_PICK_RESULT_H
