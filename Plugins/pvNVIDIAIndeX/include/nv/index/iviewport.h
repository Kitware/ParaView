/******************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Multi-view functionality

#ifndef NVIDIA_INDEX_IVIEWPORT_H
#define NVIDIA_INDEX_IVIEWPORT_H

#include <mi/dice.h>

namespace nv {
namespace index {

/// A viewport defines a two-dimensional area of a canvas where the
/// scene from a given DiCE scope should be rendered. Creating
/// multiple views allows to render the same scene or multiple scenes
/// with modifications side-by-side to one canvas.
///
/// Without the multi-view functionality, NVIDIA IndeX implicitly
/// uses a viewport that covers the entire canvas (\c IIndex_canvas)
/// and uses the global DiCE scope.
///
/// The DiCE scoping mechanism makes it possible to define multiple
/// versions of the same basic scene with different scene elements,
/// while sharing those scene elements that are the same in all
/// scopes. A simple example is setting a different camera for each
/// viewport while keeping the same scene. This is achieved by first
/// calling \c mi::neuraylib::IDice_transaction::localize() on the
/// \c ICamera and then modifying the camera separately for each
/// viewport scope. See the DiCE documentation for more details
/// about the scoping mechanism.
///
/// \note When creating new scene elements in a local scope, take
/// care that they are not accessed from other scopes, where they
/// are not visible and where an invalid tag access error would
/// happen.
///
/// As a best practice, it is advisable to create the basic scene in
/// the global scope, create multiple specialized versions of it in
/// local scopes and then assign the local scopes to viewports.
/// Scene changes that should be visible on all viewports can then
/// simply be made in the global scope. Note that the global scope
/// is not actually rendered in this scenario, only the local scopes
/// that inherit from it.
///
/// To support efficient handling of large distributed data, NVIDIA
/// IndeX makes some restrictions on how multiple scene scopes can
/// be defined:
///
/// - The scene in all viewport scopes must contain the same
///   distributed data scene elements (i.e. \c ISparse_volume_scene_element, \c
///   IRegular_heightfield) and a warning will be printed during
///   rendering when such a scene element is missing from one
///   viewport scope. The scene elements may still be localized and
///   therefore differ between viewports, so it is possible to
///   enable a volume in some of the viewports but disable it in
///   others.
///
/// - Results of \c IDistributed_compute_technique will only be
///   shared between viewports when the compute technique is not
///   localized.
///
/// - When using \c Distributed_compute_technique to create volume
///   data, all viewports must use the same compute technique.
///   Otherwise data may be recomputed constantly.
///
///
/// \ingroup nv_index_rendering
///
class IViewport :
    public mi::base::Interface_declare<0x3dd9ae6f,0x819d,0x4c9c,0xbb,0x72,0x7f,0x5a,0x8c,0x49,0xc7,0x01>
{
public:
    /// Sets the DiCE scope that should be used for this viewport.
    ///
    /// \param[in] scope DiCE scope, this class takes ownership.
    virtual void set_scope(mi::neuraylib::IScope* scope) = 0;

    /// Returns the DiCE scope of this viewport.
    ///
    /// \return DiCE scope.
    virtual mi::neuraylib::IScope* get_scope() const = 0;

    /// Sets the anchor point of the viewport window on the
    /// \c IIndex_canvas.
    ///
    /// \param[in] anchor_position Lower left position of viewport window in pixels
    virtual void set_position(const mi::math::Vector_struct<mi::Sint32, 2>& anchor_position) = 0;

    /// Returns the anchor point of the viewport window on the
    /// \c IIndex_canvas in pixel
    ///
    /// \return Anchor point position
    virtual mi::math::Vector_struct<mi::Sint32, 2> get_position() const = 0;

    /// Sets the window size (resolution).
    ///
    /// \param[in] resolution Window resolution in pixels
    virtual void set_size(const mi::math::Vector_struct<mi::Sint32, 2>& resolution) = 0;

    /// Return the window size (resolution).
    ///
    /// \return Window resolution in pixels
    virtual mi::math::Vector_struct<mi::Sint32, 2> get_size() const = 0;

    /// Enable or disable the viewport for inclusion in rendering.
    ///
    /// \param[in] enable   If false, viewport will be skipped for rendering.
    virtual void set_enabled(bool enable) = 0;

    /// Returns whether the viewport should be rendered.
    ///
    /// \return True if viewport should be rendered, false otherwise.
    virtual bool get_enabled() const = 0;
};


/// Defines a list of viewports that will be rendered onto a canvas.
///
/// This list is passed to the multi-view version of \c
/// IIndex_rendering::render() as well as \c
/// IIndex_scene_query::pick(). The order of the viewports in the
/// list defines the rendering order.
///
/// For testing and debugging the multi-view support, extra \em
/// advisory log output can be enabled.
///
/// \ingroup nv_index_rendering
///
class IViewport_list :
    public mi::base::Interface_declare<0xbd7cbc5a,0x4628,0x47ea,0x94,0x9d,0x64,0x3c,0x34,0xa3,0xc6,0x8e>
{
public:
    /// Returns the number of viewports contained in the list.
    ///
    /// \return Number of viewports
    virtual mi::Size size() const = 0;

    /// Returns the viewport at the given position in the list.
    ///
    /// \param[in] index Position in the list.
    /// \return Selected viewport, or 0 when \c index is invalid
    virtual nv::index::IViewport* get(mi::Size index) const = 0;

    /// Appends a viewport to the end of the list.
    ///
    /// This class takes ownership of the viewport.
    ///
    /// \param[in] viewport Viewport to append, this class takes ownership.
    virtual void append(nv::index::IViewport* viewport) = 0;

    /// Inserts a viewport into the list at the given position.
    ///
    /// This class takes ownership of the viewport.
    ///
    /// \param[in] index    Position in the list.
    /// \param[in] viewport Viewport to insert, this class takes ownership.
    /// \return true on success, false when \c index is invalid.
    virtual bool insert(
        mi::Size              index,
        nv::index::IViewport* viewport) = 0;

    /// Removes the viewport at the given position from the list.
    ///
    /// \param[in] index Position in the list.
    /// \return true on success, or false when \c index is invalid.
    virtual bool remove(mi::Size index) = 0;

    /// Removes all viewports from the list.
    virtual void clear() = 0;
    
    /// Enables or disables the advisory output.
    ///
    /// When the advisory is enabled, extra debug messages related
    /// to multi-view support will be printed to the log.
    ///
    /// The advisory output is disabled by default.
    ///
    /// \param[in] enable   Advisory output state
    virtual void set_advisory_enabled(bool enable) = 0;

    /// Returns whether if the advisory output is enabled.
    ///
    /// \return \c true when advisory is enabled
    virtual bool get_advisory_enabled() const = 0;
};

/// Defines a list of canvases with associated viewports. This allows rendering to multiple
/// canvases, each having one or more viewports.
///
/// \note This list must always contain all canvases. When only a subset of the canvases should be
/// rendered, then it is still necessary to include all of them here, to ensure proper cache
/// handling. However, the viewports in the canvases that should be skipped for rendering can be
/// disabled by calling \c IViewport::set_enable().
///
/// This list is passed to the multi-canvas version of \c IIndex_rendering::render().
///
/// \ingroup nv_index_rendering
///
class ICanvas_viewport_list :
    public mi::base::Interface_declare<0x84643c8a,0xd6cb,0x47d8,0xa1,0x5c,0xff,0x6a,0xc9,0x90,0x4b,0x14>
{
public:
    /// Returns the number of canvases and viewport-lists contained in the list.
    ///
    /// \return Number of canvas/viewport-list pairs stored in the list.
    virtual mi::Size size() const = 0;

    /// Returns the canvas at the given position in the list.
    ///
    /// \param[in] index Position in the list.
    /// \return Selected canvas, or 0 when \c index is invalid
    virtual nv::index::IIndex_canvas* get_canvas(mi::Size index) const = 0;

    /// Returns the viewport-list at the given position in the list.
    ///
    /// \param[in] index Position in the list.
    /// \return Selected viewport-list, or 0 when \c index is invalid
    virtual nv::index::IViewport_list* get_viewport_list(mi::Size index) const = 0;

    /// Appends a canvas/viewport-list pair to the end of the list.
    ///
    /// This class takes ownership of the viewport-list.
    ///
    /// \param[in] canvas   Canvas to append, this class does \em not take ownership.
    /// \param[in] viewport_list Viewport-list to append, this class takes ownership.
    virtual void append(
        nv::index::IIndex_canvas*  canvas,
        nv::index::IViewport_list* viewport_list) = 0;

    /// Inserts a canvas/viewport-list pair into the list at the given position.
    ///
    /// This class takes ownership of the viewport-list.
    ///
    /// \param[in] index    Position in the list.
    /// \param[in] canvas   Canvas to insert, this class does \em not take ownership.
    /// \param[in] viewport_list Viewport-list to append, this class takes ownership.
    /// \return true on success, false when \c index is invalid.
    virtual bool insert(
        mi::Size                   index,
        nv::index::IIndex_canvas*  canvas,
        nv::index::IViewport_list* viewport_list) = 0;

    /// Removes the canvas/viewport-list pair at the given position from the list.
    ///
    /// \param[in] index Position in the list.
    /// \return true on success, or false when \c index is invalid.
    virtual bool remove(mi::Size index) = 0;

    /// Removes all canvas/viewport-list pairs from the list.
    virtual void clear() = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IVIEWPORT_H
