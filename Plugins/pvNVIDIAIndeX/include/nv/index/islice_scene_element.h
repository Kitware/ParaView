/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Interfaces representing slice scene elements.

#ifndef NVIDIA_INDEX_ISLICE_SCENE_ELEMENT_H
#define NVIDIA_INDEX_ISLICE_SCENE_ELEMENT_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

#include <nv/index/ishape.h>

namespace nv {

namespace index {

/// Interface class representing slice scene elements as part of the
/// scene description, i.e., in \c IScene.
///
/// Slices represent planes through the volume data and
/// display the intersected volume data by means of
/// the assigned colormap that may be different from the one
/// used for rendering the volume.
///
/// The slice interfaces classes \c ISection_scene_element, which
/// represents the section slices and \c IVertical_profile_scene_element,
/// which represents vertical profiles, derive from the base interface.
/// @ingroup nv_index_scene_description_shape
///
class ISlice_scene_element :
    public mi::base::Interface_declare<0xcc8eccf3,0x7a53,0x4b4c,0x80,0x7e,0x47,0x92,0x4d,0x27,0xaa,0x28,
                                       nv::index::IShape>
{
public:
    /// Assign a colormap used for displaying the intersected
    /// volume data.
    ///
    /// \param[in] tag  The unique tag that refers to the colormap
    ///                 database element stored in the distributed
    ///                 database.
    ///
    virtual void assign_colormap(mi::neuraylib::Tag_struct tag) = 0;

    /// Get the colormap assigned to the slice for displaying the
    /// intersected volume data.
    ///
    /// \return The unique tag that refers to the assigned
    ///         colormap database element.
    ///
    virtual mi::neuraylib::Tag_struct assigned_colormap() const = 0;
};

/// The abstract interface class representing section slices in the scene
/// description. Section slices represent axis-aligned planes defined and
/// positioned in a volume's local IJK space.
///
/// A section slice can be an inline section, a cross-line section, or horizontal
/// section positioned arbitrarily along the I, J, or K axes.
/// @ingroup nv_index_scene_description_shape
///
class ISection_scene_element :
    public mi::base::Interface_declare<0xcb0e44c4,0xe8c6,0x4533,0x9b,0xc0,0x0e,0xf7,0xf9,0x2e,0xac,0xba,
                                       nv::index::ISlice_scene_element>
{
public:
    /// The enum defines the section slice's orientation
    /// in a volumes local IJK space.
    enum Slice_orientation
    {
        /// A inline section is oriented parallel to the JK plane
        /// and positioned along the I axis.
        INLINE_SECTION     = 0,
        /// A cross-line section is oriented parallel to the IK plane
        /// and positioned along the J axis.
        CROSS_LINE_SECTION = 1,
        /// A horizontal section is oriented parallel to the IJ plane
        /// and positioned along the K axis.
        HORIZONTAL_SECTION = 2
    };

    /// Get the respective orientation of the section slice scene element.
    /// \return     The section slice's orientation.
    virtual Slice_orientation get_orientation() const = 0;

    /// Get the position of the section slice along the respective axis.
    /// A section slice can be positioned arbitrarily along the
    /// positive axis in the volume's IJK space.
    ///
    /// \return     The section slice position along the positive axis.
    virtual mi::Float32 get_position() const = 0;

    /// Set the position of the section slice along the respective axis.
    /// A section slice can be positioned arbitrarily along the
    /// positive axis in the volume's IJK space.
    ///
    /// \param[in] position     The section slice's position along
    ///                         the respective axis in IJK space.
    virtual void set_position(mi::Float32 position) = 0;
};

/// The abstract interface class representing multi-segment vertical profiles
/// in the scene description. Multi-segment vertical profiles represent connected
/// line segments that are defined in the volume's local IJ space and are
/// extruded along the vertical axis covering the volume's IJK extent.
/// @ingroup nv_index_scene_description_shape
///
class IVertical_profile_scene_element :
    public mi::base::Interface_declare<0xff52984a,0xc189,0x451c,0x80,0x22,0xc2,0xc4,0x8b,0xb6,0x31,0xae,
                                       nv::index::ISlice_scene_element>
{
public:
    /// Get the number of vertices of the vertical profile that define
    /// the connected line segments in the volume's IJ space.
    ///
    /// \return     The number of vertices of the vertical profile.
    virtual mi::Uint32 get_nb_vertices() const = 0;

    /// Append a new vertex defined in the volume's IJ space to
    /// append an additional line segment to the vertical profile.
    ///
    /// \param[in]  vertex  The vertex to append a new line segment.
    virtual void append_vertex(
        const mi::math::Vector_struct<mi::Float32, 2>& vertex) = 0;

    /// Get a vertex of the vertex array that defines the connected line segments
    /// of the vertical profile.
    ///
    /// \param[in]  index   The index of the vertex that shall be accessed
    ///                     needs to be in the range [0, \c get_nb_vertices()-1].
    /// \param[out] vertex  The vertex in IJ space indexed by the \c index.
    virtual void get_vertex(
        mi::Uint32                                  index,
        mi::math::Vector_struct<mi::Float32, 2>&    vertex) const = 0;

    /// Set the vertex of the vertex array that defines the connected line segments
    /// of the vertical profile by overwriting the vertex indexed by \c index.
    ///
    /// \param[in]  index   The index of the vertex that shall be overwritten
    ///                     needs to be in the range [0, \c get_nb_vertices()-1].
    /// \param[in] vertex   The vertex in IJ space that overwrites the former vertex
    ///                     at \c index.
    virtual void set_vertex(
        mi::Uint32                                      index,
        const mi::math::Vector_struct<mi::Float32, 2>&  vertex) = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ISLICE_SCENE_ELEMENT_H
