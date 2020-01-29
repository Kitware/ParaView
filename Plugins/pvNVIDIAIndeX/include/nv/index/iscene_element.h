/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interface class representing scene elements in the scene description.

#ifndef NVIDIA_INDEX_ISCENE_ELEMENT_H
#define NVIDIA_INDEX_ISCENE_ELEMENT_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv {

namespace index {

/// The interface class represents scene elements. Implementations of the interface
/// class represent, for example, shapes and attributes in the scene.
/// An IScene_element is a part of the scene description.
///
/// \ingroup nv_index_scene_description
///
class IScene_element :
    public mi::base::Interface_declare<0x62a00ed6,0x8688,0x4edb,0x9c,0x03,0x36,0xcc,0xb4,0xf1,0x49,0x6f,
        mi::neuraylib::IElement>
{
public:
    /// Enable or disable a scene element for inclusion in rendering.
    /// Every scene element, as part of the hierarchical scene
    /// description, can be individually included (enabled) in the
    /// scene to be rendered or excluded (disabled) from the scene
    /// to be rendered.  An excluded scene element will not be
    /// rendered by any renderer but remains part of scene description
    /// and can be queried and processed by means of computing tasks.
    ///
    /// \param[in] enable   Enable (true) or disable (false) the scene element
    ///                     in the scene description.
    virtual void set_enabled(bool enable) = 0;

    /// Retrieve if a scene element is enabled or disabled from rendering.
    ///
    /// \return             Returns \c true if the rendering is enabled and \c false
    ///                     otherwise
    virtual bool get_enabled() const = 0;

    /// Add metadata to a scene element. Each scene element can store
    /// additional user-defined metadata. Metadata may include, for
    /// example, a string representing the scene element's name or
    /// domain-specific attributes. A class that represents metadata
    /// has to be a database element. The scene element refers to this
    /// database element by means of a tag.
    ///
    /// \param[in] tag      The tag that refers to the user-defined metadata associated
    ///                     with the scene element.
    ///
    virtual void set_meta_data(mi::neuraylib::Tag_struct tag) = 0;
    
    /// Retrieve the scene element's reference to user-defined metadata.
    ///
    /// \return             Returns the tag that refers to the user-defined metadata
    ///                     associated with the scene element
    ///
    virtual mi::neuraylib::Tag_struct get_meta_data() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISCENE_ELEMENT_H
