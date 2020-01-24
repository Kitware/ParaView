/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface for a convenience scene manipulations.

#ifndef NVIDIA_INDEX_ISCENE_CONVENIENCE_MANIPULATION_H
#define NVIDIA_INDEX_ISCENE_CONVENIENCE_MANIPULATION_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>
#include <mi/base/uuid.h>

namespace nv
{
namespace index
{

/// An interface class that provides a convenient way to change commonly used scene manipulations.
///
/// The interface class allows an application-writer to efficiently manipulate the scene configuration
/// and set up and reduces the efforts to tinker with the hierarchical scene description.
/// Nonetheless, defining the scene description explicitly and managing the scene in the application logic
/// represents the method of choice.
///
/// In addition, the interface class provides access to entities in the scene that otherwise are hidden
/// from the application-writer, such as a default light source defined by NVIDIA IndeX if no light source
/// is set explicitly by an application-writer.
///
/// \note The methods of this class will directly edit scene elements, including the \c IScene.
/// Hence, care must be taken that all edit operations to these elements are closed before calling
/// them, to prevent conflicts.
///
/// \ingroup nv_index_scene_description
///
class IScene_convenience_manipulation :
    public mi::base::Interface_declare<0xc8934dfa,0xc049,0x401e,0x90,0x3a,0x8d,0x18,0x2f,0xfa,0x39,0x81>
{
public:
    /// Enabling and disabling the highlighting of the intersections of all heightfields with a plane.
    /// For convenience, the intersection highlighting between a plane and all heightfields
    /// defined in the scene can be enabled or disabled at once using the following call.
    ///
    /// \param[in] enable               Enable if true, disable if false.
    /// \param[in] tag                  The reference to the plane that shall display the highlights.
    ///                                 If set to the \c mi::neuraylib::NULL_TAG then all planes shall
    ///                                 display all the intersections with all heightfields.
    /// \param[in] dice_transaction     The DiCE transaction to be able to edit the scene description
    ///                                 internally.
    ///
    virtual void set_heightfield_intersection_highlighting(
        bool                                    enable,
        mi::neuraylib::Tag_struct               tag,
        mi::neuraylib::IDice_transaction*       dice_transaction) = 0;

    /// Enabling and disabling the highlighting of the intersections of all triangle meshes with a plane.
    /// For convenience, the intersection highlighting between a plane and all triangle meshes
    /// defined in the scene can be enabled or disabled at once using the following call.
    ///
    /// \param[in] enable           Enable if true, disable if false.
    /// \param[in] tag              The reference to the plane that shall display the highlights.
    ///                             If set to the \c mi::neuraylib::NULL_TAG then all planes shall
    ///                             display all the intersections with all heightfields.
    /// \param[in] dice_transaction The DiCE transaction to be able to edit the scene description
    ///                             internally.
    ///
    virtual void set_trianglemesh_intersection_highlighting(
        bool                                    enable,
        mi::neuraylib::Tag_struct               tag,
        mi::neuraylib::IDice_transaction*       dice_transaction) = 0;

    /// Changing the default properties used for intersection highlighting.
    ///
    /// The visualization technique for intersection highlighting relies on user-defined properties
    /// that specify the appearances of the highlights on the planes' surfaces. The attribute class
    /// \c IIntersection_highlighting enables an application-writer to specify the properties on
    /// a per-intersection basis. The following convenience method specifies the default values for
    /// these properties for all intersection highlights. Individual properties can only be set
    /// by means of an instance of the attribute class (\c IIntersection_highlighting).
    ///
    /// \note This method needs to be called each time set_heightfield_intersection_highlighting()
    /// or set_trianglemesh_intersection_highlighting() have enabled highlighting for some scene
    /// elements.
    ///
    /// \param[in]  color               The RGBA color of the highlights.
    /// \param[in]  width               The width of a highlight always considered in each plane's
    ///                                 local space.
    /// \param[in]  smoothness          The amount of smoothing applied to the intersection highlighting.
    ///                                 The default value 0.0 means no smoothing and 1.0 means maximum
    ///                                 smoothing.
    /// \param[in]  discontinuity_limit Maximum height difference before discontinuity is detected,
    ///                                 or 0.0 (the default) to disable discontinuity detection.
    /// \param[in]  dice_transaction    The DiCE transaction to be able to edit the scene description
    ///                                 internally.
    ///
    virtual void set_default_intersection_highlighting_properties(
        const mi::math::Color_struct&           color,
        mi::Float32                             width,
        mi::Float32                             smoothness,
        mi::Float32                             discontinuity_limit,
        mi::neuraylib::IDice_transaction*       dice_transaction) = 0;

    /// Changing the parameter of the default light source.
    ///
    /// In contrast to many other (lower-level) graphics libraries, NVIDIA IndeX defines a default
    /// light source for visualizing a scene. The purpose of the default light is to avoid common
    /// pitfalls where a scene is unlit and the rendering remains simply black. The use of a default
    /// light source for serious applications is not intended and NVIDIA IndeX logs a warning if
    /// the renderer has to fall back to the default light source.
    /// Nevertheless, the following method allows an application-writer to alter the parameter
    /// of a default light.
    ///
    /// The default light source represents a directional head light source, i.e., is direction
    /// is specified relative to the camera.
    /// The default light source is not exposed through the NVIDIA IndeX library. The following
    /// convenience method allows an application-writer to set the values of the default light
    /// source.
    ///
    /// \todo Not yet implemented.
    ///
    /// \param[in] intensity        The light source's color that the light emits.
    /// \param[in] direction        The direction of the light source.
    /// \param[in] dice_transaction The DiCE transaction to be able to edit the scene description
    ///                             internally.
    ///
    virtual void set_values_of_default_light(
        const mi::math::Color_struct&                   intensity,
        const mi::math::Vector_struct<mi::Float32, 3>&  direction,
        mi::neuraylib::IDice_transaction*               dice_transaction) = 0;

    /// Since the default light source is not exposed through the NVIDIA IndeX library,
    /// the following method queries its values for an application-writer's convenience.
    ///
    /// \todo Not yet implemented.
    ///
    /// \param[out] intensity       The light source's color that the light emits.
    /// \param[out] direction       The direction of the light source.
    /// \param[in] dice_transaction The DiCE transaction to be able to edit the scene description
    ///                             internally.
    ///
    virtual void get_values_of_default_light(
        mi::math::Color_struct&                     intensity,
        mi::math::Vector_struct<mi::Float32, 3>&    direction,
        mi::neuraylib::IDice_transaction*           dice_transaction) const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISCENE_CONVENIENCE_MANIPULATION_H
