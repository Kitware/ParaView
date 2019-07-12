/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Hierarchical scene description groups for structuring the scene.

#ifndef NVIDIA_INDEX_ISCENE_GROUP_H
#define NVIDIA_INDEX_ISCENE_GROUP_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>
#include <mi/math/matrix.h>

#include <nv/index/iscene_element.h>

namespace nv
{
namespace index
{

class IScene;

/// @ingroup nv_index_scene_description_group
///
/// A scene group allows structuring a scene in a hierarchical form. Every
/// scene group may contain a list of scene elements referenced by tags. A
/// scene element can be a shape (e.g., a sphere), an attribute (e.g., a
/// material), or other scene groups.  Scene groups that contains other
/// scene groups create a hierarchical scene description.
class IScene_group :
    public mi::base::Interface_declare<0xdf29205a,0x010c,0x42c1,0xbe,0xc5,0xb9,0x5e,0xcd,0x3c,0x76,0x74,
                                       nv::index::IScene_element>
{
public:
    /// Append a scene element at the end of the scene group's element list.
    ///
    /// Certain scene elements may have restrictions on where in the scene
    /// description they may be added, and these rules will be enforced by
    /// this method.
    ///
    /// \param[in] scene_element_tag Tag of the scene element
    /// \param[in] dice_transaction  DiCE transaction
    /// \return true on success, or false when the element could not be added
    virtual bool append(
        mi::neuraylib::Tag_struct           scene_element_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Add a scene element as the new first item of the scene group's
    /// element list.
    ///
    /// Certain scene elements may have restrictions on where in the scene
    /// description they may be added, and these rules will be enforced by
    /// this method.
    ///
    /// \param[in] scene_element_tag Tag of the scene element
    /// \param[in] dice_transaction  DiCE transaction
    /// \return true on success, or false when the element could not be added
    virtual bool prepend(
        mi::neuraylib::Tag_struct           scene_element_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Get the number of scene elements contained in this group.
    ///
    /// \return the number of contained elements
    virtual mi::Uint32 nb_elements() const = 0;

    /// Get the scene element tag at the position \c index of the scene
    /// group's element list.
    ///
    /// \param[in] index The index in the list of scene elements
    /// \return the scene element tag position at \c index, or NULL_TAG when the \c index is
    /// invalid.
    virtual mi::neuraylib::Tag_struct get_scene_element(mi::Uint32 index) const = 0;

    /// Insert a scene element into the scene group's element list at
    /// position \c index.
    ///
    /// Certain scene elements may have restrictions on where in the scene
    /// description they may be added, and these rules will be enforced by
    /// this method.
    ///
    /// \param[in] index The index in the list of scene elements
    /// \param[in] scene_element_tag Tag of the scene element to be inserted
    /// \param[in] dice_transaction  DiCE transaction
    /// \return true on success, or false when the element could not be added.
    virtual bool insert(
        mi::Uint32                          index,
        mi::neuraylib::Tag_struct           scene_element_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Remove the scene element at the position \c index in the element list
    /// from the group.
    ///
    /// \param[in] index The index in the list of scene elements
    /// \param[in] dice_transaction  DiCE transaction
    /// \return true on success, or false when the element was not found
    virtual bool remove(
        mi::Uint32                          index,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Remove all scene elements with the given tag from the group.
    ///
    /// \param[in] scene_element_tag Tag of the scene element to be removed
    /// \param[in] dice_transaction  DiCE transaction
    /// \return true on success, or false when the element was not found
    virtual bool remove(
        mi::neuraylib::Tag_struct           scene_element_tag,
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;

    /// Remove all scene elements from the group.
    ///
    /// \param[in] dice_transaction  DiCE transaction
    ///
    virtual void clear(
        mi::neuraylib::IDice_transaction*   dice_transaction) = 0;
};

/// @ingroup nv_index_scene_description_group
///
/// A <em>transformed scene group</em> is used in the typical manner for
/// structuring the scene but also defines a transformation matrix which
/// will be applied to all of its children.  Transformations are inherited
/// down through the hierarchy represented by nested scene groups.
///
/// Scene elements that represent large scale data, such as volumes or
/// heightfields, may not be added to this type of scene group.
class ITransformed_scene_group :
    public mi::base::Interface_declare<0xf6de2020,0x3fcd,0x4764,0xb3,0xbe,0x3a,0x07,0xf0,0xee,0xd6,0x60,
                                       nv::index::IScene_group>
{
public:
    /// Set the transformation matrix.
    /// \param[in] transform_mat 4x4 transformation matrix
    virtual void set_transform(
        const mi::math::Matrix_struct<mi::Float32, 4, 4>& transform_mat) = 0;

    /// Get the current transformation matrix.
    /// \return current 4x4 transformation matrix
    virtual mi::math::Matrix_struct<mi::Float32, 4, 4> get_transform() const = 0;
};

/// @ingroup nv_index_scene_description_group
///
/// A <em>static scene group</em> is used in the typical manner for
/// structuring the scene but does not maintain its own transformation
/// matrix.
///
/// A static scene group may only be attached to the scene description root
/// or other static scene groups.  It is the only group that allows for a
/// spatial distribution of large scale data.  Scene elements representing
/// large scale data, such as volumes or heightfields, may only be attached to
/// a static scene group.
class IStatic_scene_group :
    public mi::base::Interface_declare<0x11971a80,0x8607,0x4f8b,0xa7,0x77,0x04,0x42,0x6a,0xc2,0xcc,0xda,
                                       nv::index::IScene_group>
{
};

/// @ingroup nv_index_scene_description_group
///
/// A <em>shape scene group</em> (or simply "shape group") aggregates
/// multiple scene elements to create new user-defined geometry
/// objects. The shape group only stores the geometry; its contents are
/// created and modified by an associated IShape_scene_group_manipulator.
class IShape_scene_group :
    public mi::base::Interface_declare<0xab821bda,0x0214,0x4908,0xbb,0x51,0xb4,0xd6,0x76,0x74,0xb9,0xb6,
                                       nv::index::ITransformed_scene_group>
{
public:
    /// Fill the shape group with geometry using the given manipulator. The
    /// shape group will be associated with it.  This association means
    /// that the same manipulator should not be used for several shape
    /// groups, unless it stores no internal state.
    ///
    /// \param[in] manipulator_tag   Tag of an IShape_scene_group_manipulator
    /// \param[in] scene             Scene object
    /// \param[in] dice_transaction  DiCE transaction
    virtual void build(
        mi::neuraylib::Tag_struct         manipulator_tag,
        const IScene*                     scene,
        mi::neuraylib::IDice_transaction* dice_transaction) = 0;

    /// Returns the manipulator that is associated with the shape group
    /// \return Tag of an IShape_scene_group_manipulator
    virtual mi::neuraylib::Tag_struct get_manipulator() const = 0;
};

/// @ingroup nv_index_scene_description_group
///
/// An implementation of IShape_scene_group_manipulator creates
/// user-defined geometry by filling an IShape_scene_group with scene
/// elements. Additional methods may be added to modify the contents of an
/// existing shape group.
class IShape_scene_group_manipulator :
    public mi::base::Interface_declare<0x8ba83935,0xb95b,0x44ce,0x83,0x2a,0x14,0x06,0x52,0x93,0x45,0x14,
                                       mi::neuraylib::IElement>
{
public:
    /// Creates the geometry inside the given shape group.
    ///
    /// \note This method should not be called directly, it will be executed by
    /// IShape_scene_group::build().
    ///
    /// \param[in] shape_group       The shape group that should be filled with geometry
    /// \param[in] scene             Scene object
    /// \param[in] dice_transaction  DiCE transaction
    virtual void build(
        IShape_scene_group*               shape_group,
        const IScene*                     scene,
        mi::neuraylib::IDice_transaction* dice_transaction) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISCENE_GROUP_H
