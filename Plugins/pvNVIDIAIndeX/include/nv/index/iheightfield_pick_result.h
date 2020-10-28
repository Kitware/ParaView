/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Path specific pick results returned by the NVIDIA IndeX library when querying a scene's contents using the pick operation.

#ifndef NVIDIA_INDEX_IHEIGHTFIELD_PICK_RESULT_H
#define NVIDIA_INDEX_IHEIGHTFIELD_PICK_RESULT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_query_results.h>

namespace nv
{
namespace index
{

/// Interface class that returns the heightfield specific result of a pick operation/query.
/// The interface class sub classes from \c IScene_pick_result to provide additional 
/// intersection results specific to \c IHeightfield scene elements.
///
/// \ingroup nv_scene_queries
///
class IHeightfield_pick_result :
    public mi::base::Interface_declare<0x5634e199,0xb768,0x4512,0xba,0xe9,0xd7,0x70,0x50,0xa3,0x6c,0xc9,
                                        nv::index::IScene_pick_result>
{
public:
    /// A heightfield can also serve as a canvas for displaying compute results.
    /// The method indicates if computing is enabled for this heightfield. If so,
    /// the computed values (a color or a color index value) are provided with
    /// this pick result.
    ///
    /// \return     Returns if a computed value is available. 
    ///
    virtual bool is_computing_enabled() const = 0;

    /// Returns the color value of the color value computed for the intersection point.
    ///
    /// \return     The computed color at the intersection point.
    ///
    virtual const mi::math::Color_struct& get_computed_color() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IHEIGHTFIELD_PICK_RESULT_H
