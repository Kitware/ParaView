/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Results exposed by the NVIDIA IndeX library when querying a scene's contents.

#ifndef NVIDIA_INDEX_ISCENE_VISITOR_H
#define NVIDIA_INDEX_ISCENE_VISITOR_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/base/uuid.h>

namespace nv
{
namespace index
{

/// @ingroup scene_queries

/// Enables user-specific evaluations of the scene representation.
///
/// The scene visitor applies to the root of the scene and will be
/// passed through the entire scene representation. In this way, the
/// visitor visits all scene elements (e.g., scene groups, attributes
/// and shapes) contained in the scene automatically.
/// When visiting a scene element, the \c evaluate method is called.
/// Deriving from the visitor enables the implementation of user-specific
/// evaluations of the scene representation.
/// Common use-cases include the logging of the scene representation or
/// determining the path (\c IScene_path) from the root to some given 
/// scene element.
///
class IScene_visitor
{
public:
    /// The scene representation can be traversed depth-first order only today.
    /// When traversing in depth-first order, each node is visited twice
    /// i.e., in forward or backward directions. 
    ///
    /// Traversing the scene in breadth-first order is not yet supported.
    enum Scene_evaluation_mode
    {
        START_EVALUATION                = 0,
        DEPTH_FIRST_FORWARD_EVALUATION  = 1,
        DEPTH_FIRST_BACKWARD_EVALUATION = 2
    };

    /// When traversing the scene representation each scene element is
    /// visited. When visiting the scene element, the \c evaluate method
    /// is called by the NVIDIA IndeX library to enable application-side
    /// inspections of the scene elements.
    ///
    /// \param scene_element    The tag of the visited scene element.
    ///
    /// \param uuid             The unique identifier that represents the
    ///                         type of the visited class.
    ///
    /// \param evaluation_mode  The scene representation can be traversed
    ///                         depth-first order and a node is evaluated
    ///                         twice in, i.e., in forward or backward
    ///                         directions. The method can handle
    ///                         the evaluation based on either one
    ///                         appropriately.
    ///
    /// \param transaction      The transaction used when traversing the
    ///                         scene.
    ///
    virtual void evaluate(
        mi::neuraylib::Tag                      scene_element,
        const mi::base::Uuid&                   uuid,
        IScene_visitor::Scene_evaluation_mode   evaluation_mode,
        mi::neuraylib::IDice_transaction*       transaction) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISCENE_VISITOR_H
