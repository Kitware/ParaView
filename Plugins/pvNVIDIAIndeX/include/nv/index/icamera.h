/******************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Generic camera

#ifndef NVIDIA_INDEX_ICAMERA_H
#define NVIDIA_INDEX_ICAMERA_H

#include <mi/dice.h>
#include <mi/base/interface_declare.h>

namespace nv {
namespace index {

// forward declarations
class ICamera_projection;

/// Abstract interface class representing a generic camera.
///
/// Both perspective projection and orthographic projection are
/// supported through the sub-interface classes \c IPerspective_camera
/// and \c IOrthographic_camera.
///
/// The base class contains general attributes allowing the positioning
/// and orientation of the camera in the virtual scene.
///
/// A camera's position and orientation are defined by three
/// parameters. The camera's position is defined in space (the <em>eye
/// point</em>). The camera's viewing direction is defined by the
/// <em>viewing direction</em>). Rotation around the viewing direction
/// as an axis is defined by a vector which represents "straight up"
/// (the <em>up vector</em>).
///
/// Instances of \c ICamera implementations are created by \c IScene::create_camera().
///
/// Note: \c ICamera is not an \c IScene_element.
///
/// \ingroup nv_index_scene_description
///
class ICamera :
    public mi::base::Interface_declare<0x9ec1179e,0x1b9,0x4bc8,0xb0,0x54,0xef,0x79,0x8a,0x17,0x27,0x3f,
                                       mi::neuraylib::IElement>
{
public:
    /// Set the basic camera parameters. A convenienve method for setting
    /// up basic camera parameters.
    ///
    /// \param[in]  eye_point       Camera eye point
    /// \param[in]  view_direction  Viewing direction
    /// \param[in]  up_direction    Up vector
    virtual void set(
        const mi::math::Vector_struct<mi::Float32, 3>& eye_point,
        const mi::math::Vector_struct<mi::Float32, 3>& view_direction,
        const mi::math::Vector_struct<mi::Float32, 3>& up_direction) = 0;

    /// Sets the eye point (the origin of the camera's coordinate system).
    //
    /// \param[in] eye_point Eye point
    virtual void set_eye_point(const mi::math::Vector_struct<mi::Float32, 3>& eye_point) = 0;

    /// Returns the eye point (the origin of the camera's coordinate system).
    /// \return Eye point
    virtual mi::math::Vector_struct<mi::Float32, 3> get_eye_point() const = 0;

    /// Set the view direction.
    ///
    /// \param[in] view_direction  Camera direction vector.
    /// \return true when succeeded.
    virtual bool set_view_direction(const mi::math::Vector_struct<mi::Float32, 3>& view_direction) = 0;

    /// Returns the normalized view direction.
    ///
    /// \return Normalized view direction
    virtual mi::math::Vector_struct<mi::Float32, 3> get_view_direction() const = 0;

    /// Sets the up vector.
    /// \param[in] up_direction  Up vector, will be normalized.
    virtual void set_up_direction(const mi::math::Vector_struct<mi::Float32, 3>& up_direction) = 0;

    /// Returns the normalized up vector.
    /// \return Normalized up vector
    virtual mi::math::Vector_struct<mi::Float32, 3> get_up_direction() const = 0;

    /// Set camera projection center offset.
    ///
    /// Normally the camera viewing plane is centered on the eye point and viewing direction.
    /// For use cases where asymmetric project is needed, like multi-tile rendering or stereographic projection,
    /// we can specify a two-dimensional offset to shift the viewing plane relative to the eye point.
    /// If the offset value is (0,0), the viewing plane is centered, this is the default.
    ///
    /// The offset value range is relative to \c aperture (aperture defines the width of the viewing plane).
    /// If \c aperture is 1, then offset values of 0.5 define half of the viewing plane extent.
    /// A negative offset moves the viewing plane to the left or down relative to the eye point.
    /// For example, in multi-tile rendering, the lower left tile of four would use an offset of (-0.5, -0.5) assuming aperture = 1.
    ///
    ///
    /// The camera parameters for general tiling are derived as follows:
    ///
    /// The task is to subdivide a given camera projection into a number of horizontal and vertical tiles ( \c nb_tiles ).
    /// All tiles have the same format, their resolution is a fraction of the original resolution.
    /// (We assume that original resolution and number of tiles are integer divisible.)
    /// If the tiled images should represent an upscaled version of the original,
    /// then we scale up the original resolution for the sake of this parameter derivation.
    ///
    /// The input camera parameters are the original \c aspect, \c aperture, \c resolution,
    /// for both perspective and orthographic projection.
    /// Note that for square pixels, \c aspect could be derived from \c resolution (aspect = resolution.x / resolution.y).
    ///
    /// The camera parameters for tile projection are: \c aspect, \c resolution, \c aperture and \c base_offset, \c tile_index.
    /// Pseudo code:
    ///
    ///    tiling_aspect       = nb_tiles.x / nb_tiles.y
    ///    tile_aspect         = aspect / tiling_aspect
    ///    tile_aperture       = aperture / nb_tiles.x
    ///    tile_resolution.x   = resolution.x / nb_tiles.x
    ///    tile_resolution.y   = resolution.y / nb_tiles.y
    ///
    ///    base_offset.x = -aperture/2 + tile_aperture/2
    ///    base_offset.y = -aperture/(2*tiling_aspect) + tile_aperture/2)
    ///
    /// For rendering tiles, each tile should use adapted camera parameters as follows:
    ///
    ///    camera.aspect        := tile_aspect
    ///    camera.aperture      := tile_aperture
    ///    (camera.)resolution  := tile_resolution
    ///
    ///    [for xi in nb_tiles.x; for yi in nb_tiles.y]
    ///    camera.center_offset.x := xi * tile_aperture + base_offset.x
    ///    camera.center_offset.y := yi * tile_aperture + base_offset.y
    ///
    //
    /// \param [in] offset  Two-dimensional offset of viewing plane.
    virtual void set_center_offset(const mi::math::Vector_struct<mi::Float32, 2>& offset) = 0;

    /// Get camera projection center offset.
    /// \return Current camera projection center offset, default is (0,0).
    virtual mi::math::Vector_struct<mi::Float32, 2> get_center_offset() const = 0;


    /// \name Advanced camera projection (VR/XR)
    /// @{

    /// Create a new camera projection object.
    /// This object can be connected to an \c ICamera for advanced camera projection like VR/XR setups.
    /// \return The new \c ICamera_projection
    virtual ICamera_projection* create_advanced_camera_projection() const = 0;

    /// Access current advanced camera projection settings.
    /// \return Tag of \c ICamera_projection element.
    virtual mi::neuraylib::Tag_struct get_advanced_camera_projection() const = 0;

    /// Update current advanced camera projection settings.
    /// \param cam_projection  Tag of I\c Camera_projection element.
    virtual void set_advanced_camera_projection(mi::neuraylib::Tag_struct cam_projection) = 0;

    /// Check if advanced camera projection is enabled.
    /// \return True if advanced camera projection is enabled.
    virtual bool is_advanced_camera_projection_enabled() const = 0;

    /// Enable or disable advanced camera projection.
    /// \param[in]  enable  Enable advanced projection.
    virtual void enable_advanced_camera_projection(bool enable) = 0;
    /// @}
};

/// Interface class for a camera implementing the perspective camera model.
///
/// For perspective projection, the <em>field of view</em>
/// of the camera can be controlled by setting the <em>aperture</em>
/// and the <em>focal length</em>. In this simple camera model the
/// aperture describes the width of the viewing plane, while the
/// focal length controls the distance from the viewing plane to the
/// eye point. The horizontal field of view is then defined as the
/// ratio of the aperture to the focal length. The <em>aspect
/// ratio</em> is defined as the ratio of the width to the height of
/// the rendering canvas. Using this ratio, the vertical field of
/// view can also be computed as the ratio of the aperture to the
/// product of the aspect ratio and the focal length.
///
/// Other attributes stored in the IPerspective_camera class include
/// the near and far clipping planes.
///
/// \ingroup nv_index_scene_description
///
class IPerspective_camera :
    public mi::base::Interface_declare<0x1c7386d2,0xbd2d,0x4973,0xa8,0xa8,0xb5,0xfc,0xe9,0x0,0x22,0x5e,
                                       nv::index::ICamera>
{
public:
    /// Assign all camera parameters from the source object to this camera.
    /// \param[in]  other Source camera
    virtual void assign(const IPerspective_camera* other) = 0;

    /// Returns the aperture width.
    /// \return aperture width
    virtual mi::Float64 get_aperture() const = 0;
    /// Sets the aperture width.
    /// \param[in]  aperture_width Aperture width
    virtual void set_aperture(mi::Float64 aperture_width) = 0;

    /// Returns the aspect ratio, defined as width/height.
    ///
    /// \note This does not take the canvas size controlled by \c set_resolution_x() and \c
    /// set_resolution_y() into account.
    ///
    /// \return aspect ratio
    virtual mi::Float64 get_aspect() const = 0;
    /// Sets the aspect ratio.
    /// It is defined a width/height.
    /// \param[in]  aspect_ratio Aspect ratio, must be positive
    virtual void set_aspect(mi::Float64 aspect_ratio) = 0;

    /// Returns the distance to the near clipping plane (\em hither).
    /// \return near clipping plane distance
    virtual mi::Float64 get_clip_min() const = 0;
    /// Sets the distance to the near clipping plane (\em hither).
    /// \param[in]  clip_min Distance
    virtual void set_clip_min(mi::Float64 clip_min) = 0;

    /// Returns the distance to the far clipping plane (\em yon).
    /// \return far clipping plane distance
    virtual mi::Float64 get_clip_max() const = 0;
    /// Sets the distance to the far clipping plane (\em yon).
    /// \param[in]  clip_max Distance
    virtual void set_clip_max(mi::Float64 clip_max) = 0;

    /// Returns the focal length.
    /// \return focal length
    virtual mi::Float64 get_focal() const = 0;
    /// Sets the focal length.
    /// \param[in]  focal_length Focal length
    virtual void set_focal(mi::Float64 focal_length) = 0;

    /// Returns the vertical field of view.
    /// It is calculated from the aperture, aspect ratio and focal length settings.
    ///
    /// \return Field of view, in radians
    virtual mi::Float64 get_fov_y_rad() const = 0;
};

/// Interface class for a camera implementing the orthographic camera model.
///
/// When orthographic projection is used, only the width
/// and height of the viewing plane are relevant. The size of the
/// viewing plane is controlled through the width of the aperture
/// and the aspect ratio.
///
/// Other attributes stored in the IOrthographic_camera class include
/// the near and far clipping planes and the canvas resolution (width
/// and height).
///
/// \ingroup nv_index_scene_description
///
class IOrthographic_camera :
    public mi::base::Interface_declare<0xd7d339b2,0xa894,0x4db8,0xbe,0x67,0xe,0x42,0x50,0x19,0x9e,0xee,
                                       nv::index::ICamera>
{
public:
    /// Assign all camera parameters from the source object to this camera.
    /// \param[in]  other Source camera
    virtual void assign(const IOrthographic_camera* other) = 0;

    /// Returns the aperture width.
    /// \return aperture width
    virtual mi::Float64 get_aperture() const = 0;
    /// Sets the aperture width.
    /// \param[in]  aperture_width Aperture width
    virtual void set_aperture(mi::Float64 aperture_width) = 0;

    /// Returns the aspect ratio, defined as width/height.
    ///
    /// \note This does not take the canvas size controlled by \c set_resolution_x() and \c
    /// set_resolution_y() into account.
    ///
    /// \return aspect ratio
    virtual mi::Float64 get_aspect() const = 0;
    /// Sets the aspect ratio.
    /// It is defined a width/height.
    /// \param[in]  aspect_ratio Aspect ratio, must be positive
    virtual void set_aspect(mi::Float64 aspect_ratio) = 0;

    /// Returns the distance to the near clipping plane (\em hither).
    /// \return near clipping plane distance
    virtual mi::Float64 get_clip_min() const = 0;
    /// Sets the distance to the near clipping plane (\em hither).
    /// \param[in]  clip_min Distance
    virtual void set_clip_min(mi::Float64 clip_min) = 0;

    /// Returns the distance to the far clipping plane (\em yon).
    /// \return far clipping plane distance
    virtual mi::Float64 get_clip_max() const = 0;
    /// Sets the distance to the far clipping plane (\em yon).
    /// \param[in]  clip_max Distance
    virtual void set_clip_max(mi::Float64 clip_max) = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ICAMERA_H
