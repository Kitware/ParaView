/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class representing attributes that can be defined in a scene description.

#ifndef NVIDIA_INDEX_IATTRIBUTE_H
#define NVIDIA_INDEX_IATTRIBUTE_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute
/// <em>Attributes</em> are a part of the scene description that provide a
/// common mechanism for parameterized information used in processing.
/// Attributes define the rendered appearance of shapes and large-scale datasets
/// and can be used to store meta-information in the scene description for later
/// evaluation.  Attributes can therefore affect both the rendering as well as
/// the processing and evaluation of data.
///
/// The abstract base class \c IAttribute represents attributes that can be
/// structured in a scene description. Common attributes include, for example,
/// material properties, light sources, and the fonts used to display the
/// contents of a label.
///
/// Once specified in a scene description, attributes affect the rendering of
/// those shapes in the scene that are hierarchically below them in the scene
/// description. (For details on structuring a scene please also refer to \c
/// IScene_group.)
///
/// Generally, the NVIDIA IndeX library considers two different evaluation
/// strategies for attributes:
///
///    - Exclusive: Only one attribute of a certain category (such as a material
///      property) can be active during rendering or data processing.
///
///    - Integrative: Attribute evaluation considers multiple instances of an
///      attribute class.  Multiple attributes of a certain category can
///      therefore be active simultaneously and will all be considered for
///      rendering. For example, multiple light sources specified in the scene
///      should be used in rendering a shape.
///
class IAttribute :
        public mi::base::Interface_declare<0xfb32338f,0xca95,0x49ce,0x80,0xdf,0x9c,0x1e,0x5a,0x25,0x7e,0x0f,
                                           nv::index::IScene_element>
{
public:
    /// Represents the category or the attribute class that a concrete attribute
    /// implements.  For instance, a directional light implements the \c ILight
    /// category and a simple OpenGL-like Phong material implements the \c
    /// IMaterial category.  A category enables the implementation of
    /// specialized attributes for certain classes and can thereby extend the
    /// number of variants for that category.  The evaluation strategies for
    /// attribute classes are determined by the category. For example, point
    /// lights, directional lights (\c IDirectional_light) and directional head
    /// lights (\c IDirectional_headlight) implement the \c ILight category and
    /// can all be active at a time.  In contrast, a Cook-Torrance surface
    /// material and an OpenGL-like Phong material implement the \c IMaterial
    /// category but only one instance of the material attributes is active when
    /// shading a shapes surface.
    ///
    /// \return     Returns the UUID of the base attribute category.
    ///
    virtual mi::base::Uuid get_attribute_class() const = 0;

    /// The attributes that implement a category or an attribute class are
    /// evaluated in exclusive or integrative fashion (for example, multiple
    /// light source affect the shading of surfaces while only one material
    /// defines the surface's reflectance properties). To control the attribute
    /// evaluation, an attribute class (that defines a category for derived
    /// attributes) must define whether one or multiple instances of the
    /// attribute can be active at the same time when evaluating a shape or
    /// dataset.
    ///
    /// \return     Returns \c true if multiple instances of an attribute that
    ///             influences a shape's appearance may be active
    ///             simultaneously. Returns \c false if only one attribute can
    ///             be active at a time.
    ///
    virtual bool multiple_active_instances() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IATTRIBUTE_H
