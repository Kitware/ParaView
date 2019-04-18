/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief User-defined rendering canvas.

#ifndef NVIDIA_INDEX_IINDEX_CANVAS_H
#define NVIDIA_INDEX_IINDEX_CANVAS_H

#include <mi/dice.h>

namespace nv
{
namespace index
{

/// Enables rendering to a user-defined canvas.
/// Technically, the canvas wraps a frame buffer that contains one or more rendering targets
/// or frame buffer layer. The NVIDIA IndeX library renders its rendering/visualization
/// results into the associated frame buffer layers.
/// Typical use cases include the rendering (blending) of all image tiles (a.k.a. horizontal
/// spans) into an OpenGL framebuffer, into main memory or CUDA buffer for
/// video-streaming, and into an image buffer or file.
///
/// The NVIDIA IndeX library ships the implementation of a main memory buffer and an OpenGL buffer
/// based canvas classes as source code to ease the integration into a given application.
/// @ingroup nv_index_rendering
///
class IIndex_canvas : public mi::base::Interface_declare<0x46c5a5f7, 0x31de, 0x468d, 0x9b, 0x6c,
                        0xb0, 0x35, 0x77, 0xed, 0xfe, 0x61>
{
public:
  enum Frame_buffer_layer_type
  {
    FB_LAYER_UNDEFINED = 0u, ///< Undefined frame buffer.
    FB_LAYER_RGB8 = 1u,      ///< Not supported yet // 8-bit per rgb component.
    FB_LAYER_RGBA8 = 2u,     ///< 8-bit per rgba component.
    FB_LAYER_RGBE = 3u,      ///< Not supported yet // RGBe hdr support.
    FB_LAYER_DEPTH16 = 4u,   ///< Not supported yet // 16-bit depth.
    FB_LAYER_DEPTH32 = 5u,   ///< 32-bit depth in float.
    FB_LAYER_UINT8 = 6u,     ///< Not supported yet // 8-bit unsigned int enabling index encoding.
    FB_LAYER_UINT32 = 7u,    ///< Not supported yet // 32-bit unsigned int enabling index encoding.
    FB_LAYER_FLOAT32 = 8u    ///< Not supported yet // 32-bit float enabling scalar value encoding.
  };

  /// The number of layers hosted by the canvas' frame buffer.
  /// The frame buffer is logically defined by an array of the layers
  /// whereas each layer is accessible by a layer id.
  virtual mi::Uint32 get_nb_frame_buffer_layers() const = 0;

  /// Query the type of the frame buffer layer.
  /// The layers are enumerated in sequential order and each layer is associated to an
  /// implicitly given layer id. The layer id allows querying the type of a layer
  /// contained in the frame buffer.
  ///
  /// \param[in] layer_id         The id of the layer.
  ///
  virtual Frame_buffer_layer_type get_layer_type(mi::Uint32 layer_id) const = 0;

  /// Before rendering, a buffer typically needs to be flushed or front- and back buffers
  /// need to be swapped.
  /// This method is called right before any image tile is rendered into the canvas,
  /// i.e, before any call to \c receive_tile().
  ///
  virtual void prepare() = 0;

  /// This interface method receives multiple tiles for each layer.
  /// The method is called for each tile and each layer between \c prepare()
  /// and \c finish().
  ///
  /// \note This method must be thread safe since the library may
  ///       call this method from different threads simultaneously.
  ///
  /// \param[in] layer_id         The id of the layer.
  /// \param[in] layer_type       The type of the layer.
  /// \param[in] area             The 2D bounding box of the tile covered in the screen space.
  /// \param[in] buffer           The image buffer that contains the rendering inside the tile area.
  ///
  virtual void render_tile(mi::Uint32 layer_id, Frame_buffer_layer_type layer_type,
    const mi::math::Bbox_struct<mi::Uint32, 2>& area, mi::Uint8* buffer) = 0;

  /// This interface method receives multiple tiles.
  /// The method is called for each tile and between \c prepare()
  /// and \c finish().
  ///
  /// \note This method must be thread safe since the library may
  ///       call this method from different threads simultaneously.
  ///
  /// \deprecated
  ///
  /// \param[in] buffer           The image buffer that contains the rendering inside the tile area.
  /// \param[in] buffer_size      The size of the tile's image buffer.
  /// \param[in] area             The 2D bounding box of the tile covered in the screen space.
  ///
  virtual void receive_tile(mi::Uint8* buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& area) = 0;

  /// This interface method receives multiple tiles.
  /// The method is called for each tile and between \c prepare()
  /// and \c finish().
  ///
  /// This method blends the received image over existing image data.
  ///
  /// \note This method must be thread safe since the library may
  ///       call this method from different threads simultaneously.
  ///
  /// \deprecated
  ///
  /// \param[in] buffer           The image buffer that contains the rendering inside the tile area.
  /// \param[in] buffer_size      The size of the tile's image buffer.
  /// \param[in] area             The 2D bounding box of the tile covered in the screen space.
  ///
  virtual void receive_tile_blend(mi::Uint8* buffer, mi::Uint32 buffer_size,
    const mi::math::Bbox_struct<mi::Uint32, 2>& area) = 0;

  /// This method is called after all tiles are rendered, i.e, after all
  /// call to \c receive_tile().
  ///
  virtual void finish() = 0;

  /// The tiles can either be rendered in separate threads or in the main
  /// application thread. In a multi-thread scenario the tile rendering
  /// machinery needs to be capable to be processed using multiple threads.
  /// For instance, OpenGL is single threaded and cannot run (easily) in
  /// multiple threads but a rendering into an image or a frame of a video
  /// stream can benefit from multi-threaded tile rendering.
  ///
  /// \return true when tile rendering is in parallel
  virtual bool is_multi_thread_capable() const = 0;

  /// Returns the resolution of the canvas in pixels.
  ///
  /// \return The resolution of the canvas in pixels.
  ///
  virtual mi::math::Vector_struct<mi::Uint32, 2> get_resolution() const = 0;

  /// Returns the canvas that hosts the rendered pixels.
  ///
  /// \return     The main memory canvas that contains the rendered image.
  ///
  virtual mi::neuraylib::ICanvas* get_canvas() const = 0;
};

/// CUDA-memory based canvas.
/// Using a CUDA-memory based enables NVIDIA IndeX to run rendering and image
/// compositing on the GPU only without ever transferring data from GPUs to main
/// memory.
/// NVIDIA IndeX support the creation of a built-in CUDA canvas that derives
/// from the following interface class.
///
class IIndex_cuda_canvas : public mi::base::Interface_declare<0x631ca66c, 0x5ed7, 0x4275, 0xb7,
                             0xb8, 0x10, 0x23, 0x85, 0xf8, 0x36, 0x55, nv::index::IIndex_canvas>
{
public:
  /// Returns the CUDA memory based canvas for external use, e.g., by a
  /// video encoder as input.
  ///
  /// \return     The low-level CUDA memory canvas.
  ///
  virtual mi::neuraylib::ICanvas_cuda* get_cuda_canvas() const = 0;

  /// The CUDA memory canvas is hosted on a CUDA device.
  /// The CUDA device ID tells the user
  /// by which CUDA device the canvas is managed.
  ///
  /// \return     Returns the device ID.
  ///
  virtual mi::Sint32 get_device_id() const = 0;
};

/// Application managed interface class of a CUDA canvas implemented inside the NVIDIA IndeX
/// library.
///
class Index_cuda_canvas : public mi::base::Interface_implement<nv::index::IIndex_cuda_canvas>
{
public:
  /// Sets the resolution of the internal/built-in canvas.
  ///
  /// \param resolution   The resolution the canvas.
  ///
  virtual void set_resolution(const mi::math::Vector_struct<mi::Uint32, 2>& resolution) = 0;
};

/// Mixin class for implementing the IIndex_canvas interface.
///
/// This mixin class provides a default implementation of some of the pure
/// virtual methods of the IIndex_canvas interface.
///
class Index_canvas : public mi::base::Interface_implement<nv::index::IIndex_canvas>
{
public:
  /// Return number of layers.
  ///
  /// \return The number of layers.
  ///
  virtual mi::Uint32 get_nb_frame_buffer_layers() const { return 0; }

  /// Return the frame buffer layer type
  ///
  /// \return The Frame_buffer_layer_type
  ///
  virtual Frame_buffer_layer_type get_layer_type(mi::Uint32 /*layer_id*/) const
  {
    return IIndex_canvas::FB_LAYER_UNDEFINED;
  }

  /// \implements
  virtual void render_tile(mi::Uint32 /*layer_id*/, Frame_buffer_layer_type layer_type,
    const mi::math::Bbox_struct<mi::Uint32, 2>& area, mi::Uint8* buffer)
  {
    // Ignore the layer id but consider the layer type.

    const mi::Uint32 x_range = area.max.x - area.min.x;
    const mi::Uint32 y_range = area.max.y - area.min.y;
    const mi::Uint32 buffer_size = (x_range * y_range);

    if (layer_type == IIndex_canvas::FB_LAYER_RGB8)
    {
      this->receive_tile(buffer, buffer_size * 3, area);
    }
    else if (layer_type == IIndex_canvas::FB_LAYER_RGBA8)
    {
      this->receive_tile_blend(buffer, buffer_size * 4, area);
    }
  }

  /// \implements
  virtual mi::neuraylib::ICanvas* get_canvas() const { return NULL; }
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_IINDEX_CANVAS_H
