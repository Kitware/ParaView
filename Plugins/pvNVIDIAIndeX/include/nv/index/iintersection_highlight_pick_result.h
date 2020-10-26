/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Intersection-highlight specific pick results returned by the
///        NVIDIA IndeX library when querying a scene's contents using the pick operation.

#ifndef NVIDIA_INDEX_IINTERSECTION_HIGHLIGHT_PICK_RESULT_H
#define NVIDIA_INDEX_IINTERSECTION_HIGHLIGHT_PICK_RESULT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{

/// Interface class that returns the pick result specific to the intersection highlighting.
///
/// The interface class sub-classes from \c IScene_pick_result to provide additional
/// query results that are specific to highlighting of an intersection between a shape class,
/// such as triangle mesh or a heightfield, and a plane (\c IPlane).
///
/// \c IIntersection_highlight_pick_result represents a query result in addition to an
/// \c IPlane_pick_result when an intersection highlight was hit during a pick operation on a
/// plane. Instances of \c IIntersection_highlight_pick_result are always returned right before
/// the actual plane pick results. Multiple \c IIntersection_highlight_pick_result instances
/// are returned by pick operations intersecting multiple overlapping intersection highlights
/// on the same plane. The ordering of the \c IIntersection_highlight_pick_result instances is
/// top-most to bottom-most intersection highlight as defined by the order of the
/// \c IIntersection_highlighting attributes given through the scene description.
/// 
/// \ingroup nv_scene_queries
///
class IIntersection_highlight_pick_result :
    public mi::base::Interface_declare<0xec5d55c0,0x3bb0,0x41f6,0x93,0xd2,0x2a,0x4e,0xa4,0x55,0xb6,0x6a,
                                        nv::index::IScene_pick_result>
{
public:
    /// Returns the reference to the shape used for intersection highlighting.
    ///
    /// The intersection highlights on a plane are caused by an intersection of
    /// a shape in the scene and the plane. The pick query provides the reference
    /// to the shape if a pick operation hits the intersection highlight.
    /// The reference represents an additional query result. The pick position,
    /// color, reference to the plane, etc. are provided by the super class' 
    /// interface methods.
    ///
    /// \return     Returns the reference to the shape that intersects with the
    ///             plane and causes an intersection highlight on the plane's 
    ///             surface.
    ///
    virtual mi::neuraylib::Tag_struct get_intersection_shape() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IINTERSECTION_HIGHLIGHT_PICK_RESULT_H
