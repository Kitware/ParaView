/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Point set specific pick results returned by the NVIDIA IndeX library when querying a scene's contents using the pick operation.

#ifndef NVIDIA_INDEX_IPOINT_SET_PICK_RESULT_H
#define NVIDIA_INDEX_IPOINT_SET_PICK_RESULT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{
/// @ingroup scene_queries

/// Interface class that returns the point set specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional 
/// intersection results specific to \c IPoint_set scene elements.
///
class IPoint_set_pick_result :
    public mi::base::Interface_declare<0x22be608a,0x48da,0x4c6a,0xa7,0xb7,0x5e,0x5c,0xa3,0x66,0x15,0x99,
                                        nv::index::IScene_pick_result>
{
public:
    /// Returns the index that of a point in the set of points.
    /// The point set contains an ordered set of points that can be picked separately.
    /// A returned index i corresponds to the i-th point in the set.
    /// 
    /// \return     Returns the index of the picked point in the set of points.
    ///
    virtual mi::Uint32 get_point() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IPOINT_SET_PICK_RESULT_H
