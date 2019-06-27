/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief API component for various image-related functions.

#ifndef MI_NEURAYLIB_IIMAGE_API_H
#define MI_NEURAYLIB_IIMAGE_API_H

#include <mi/base/interface_declare.h>

namespace mi {

class IArray;

namespace neuraylib {

class IBuffer;
class ICanvas;
class IReader;
class ITile;

/** 
\if MDL_SDK_API
    \defgroup mi_neuray_mdl_sdk_misc Miscellaneous Interfaces
    \ingroup mi_neuray

    \brief Various utility classes.
\endif
*/

/** \if IRAY_API \addtogroup mi_neuray_rendering
    \elseif MDL_SDK_API \addtogroup mi_neuray_mdl_sdk_misc
    \elseif DICE_API \addtogroup mi_neuray_rtmp
    \endif
@{
*/

/// This interface provides various utilities related to canvases and buffers.
///
/// Note that #create_buffer_from_canvas() and #create_canvas_from_buffer() encode and decode pixel
/// data to/from memory buffers. \if IRAY_API To import images from disk use
/// #mi::neuraylib::IImport_api::import_canvas(). To export images to disk use
/// #mi::neuraylib::IExport_api::export_canvas(). \endif
class IImage_api : public
    mi::base::Interface_declare<0x4c25a4f0,0x2bac,0x4ce6,0xb0,0xab,0x4d,0x94,0xbf,0xfd,0x97,0xa5>
{
public:
    /// \name Factory methods for canvases and tiles
    //@{

    /// Creates a canvas with given pixel type, width, height, and layers.
    ///
    /// This factory function allows to create instances of the abstract interface
    /// #mi::neuraylib::ICanvas based on an internal default implementation. However, you are not
    /// obligated to use this factory function and the internal default implementation. It is
    /// absolutely fine to use your own (correct) implementation of the #mi::neuraylib::ICanvas
    /// interface.
    ///
    /// \param pixel_type   The desired pixel type. See \ref mi_neuray_types for a list of
    ///                     supported pixel types.
    /// \param width        The desired width.
    /// \param height       The desired height.
    /// \param tile_width   The desired tile width. The special value 0 represents the canvas
    ///                     width.
    /// \param tile_height  The desired tile height. The special value 0 represents the canvas
    ///                     height.
    /// \param layers       The desired number of layers (depth). Must be 6 for cubemaps.
    /// \param is_cubemap   Flag that indicates whether this canvas represents a cubemap.
    /// \param gamma        The desired gamma value. The special value 0.0 represents the default
    ///                     gamma which is 1.0 for HDR pixel types and 2.2 for LDR pixel types.
    /// \return             The requested canvas, or \c NULL in case of invalid pixel type, width,
    ///                     height, layers, or cubemap flag.
    virtual ICanvas* create_canvas(
        const char* pixel_type,
        Uint32 width,
        Uint32 height,
        Uint32 tile_width = 0,
        Uint32 tile_height = 0,
        Uint32 layers = 1,
        bool is_cubemap = false,
        Float32 gamma = 0.0f) const = 0;

    /// Creates a tile with given pixel type, width, and height.
    ///
    /// This factory function allows to create instances of the abstract interface
    /// #mi::neuraylib::ITile based on an internal default implementation. However, you are not
    /// obligated to use this factory function and the internal default implementation. It is
    /// absolutely fine to use your own (correct) implementation of the #mi::neuraylib::ITile
    /// interface.
    ///
    /// \param pixel_type   The desired pixel type. See \ref mi_neuray_types for a list of supported
    ///                     pixel types.
    /// \param width        The desired width.
    /// \param height       The desired height.
    /// \return             The requested tile, or \c NULL in case of invalid pixel type, width, or
    ///                     height.
    virtual ITile* create_tile(
        const char* pixel_type,
        Uint32 width,
        Uint32 height) const = 0;

    //@}
    /// \name Conversion between canvases and raw memory buffers
    //@{

    /// Reads raw pixel data from a canvas.
    ///
    /// Reads a rectangular area of pixels from a canvas (possibly spanning multiple tiles),
    /// converts the pixel type if needed, and writes the pixel data to buffer in memory.
    /// Management of the buffer memory is the responsibility of the caller.
    ///
    /// \param width               The width of the rectangular pixel area.
    /// \param height              The height of the rectangular pixel area.
    /// \param canvas              The canvas to read the pixel data from.
    /// \param canvas_x            The x-coordinate of the lower-left corner of the rectangle.
    /// \param canvas_y            The y-coordinate of the lower-left corner of the rectangle.
    /// \param canvas_layer        The layer of the canvas that holds the rectangular area.
    /// \param buffer              The buffer to write the pixel data to.
    /// \param buffer_topdown      Indicates whether the buffer stores the rows in top-down order.
    /// \param buffer_pixel_type   The pixel type of the buffer. See \ref mi_neuray_types for a
    ///                            list of supported pixel types.
    /// \param buffer_padding      The padding between subsequent rows of the buffer in bytes.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters (\c NULL pointer).
    ///                            - -2: \p width or \p height is zero.
    ///                            - -3: Invalid pixel type of the buffer.
    ///                            - -4: The rectangular area [\p canvas_x, \p canvas_x + \p width)
    ///                                  x [\p canvas_y, \p canvas_y + \p height) exceeds the size
    ///                                  of the canvas, or \p canvas_layer is invalid.
    virtual Sint32 read_raw_pixels(
        Uint32 width,
        Uint32 height,
        const ICanvas* canvas,
        Uint32 canvas_x,
        Uint32 canvas_y,
        Uint32 canvas_layer,
        void* buffer,
        bool buffer_topdown,
        const char* buffer_pixel_type,
        Uint32 buffer_padding = 0) const = 0;

    /// Writes raw pixel data to a canvas.
    ///
    /// Reads a rectangular area of pixels from a buffer in memory, converts the pixel type if
    /// needed, and writes the pixel data to a canvas (possibly spanning multiple tiles).
    /// Management of the buffer memory is the responsibility of the caller.
    ///
    /// \param width               The width of the rectangular pixel area.
    /// \param height              The height of the rectangular pixel area.
    /// \param canvas              The canvas to write the pixel data to.
    /// \param canvas_x            The x-coordinate of the lower-left corner of the rectangle.
    /// \param canvas_y            The y-coordinate of the lower-left corner of the rectangle.
    /// \param canvas_layer        The layer of the canvas that holds the rectangular area.
    /// \param buffer              The buffer to read the pixel data from.
    /// \param buffer_topdown      Indicates whether the buffer stores the rows in top-down order.
    /// \param buffer_pixel_type   The pixel type of the buffer. See \ref mi_neuray_types for a
    ///                            list of supported pixel types.
    /// \param buffer_padding      The padding between subsequent rows of the buffer in bytes.
    /// \return
    ///                            -  0: Success.
    ///                            - -1: Invalid parameters (\c NULL pointer).
    ///                            - -2: \p width or \p height is zero.
    ///                            - -3: Invalid pixel type of the buffer.
    ///                            - -4: The rectangular area [\p canvas_x, \p canvas_x + \p width)
    ///                                  x [\p canvas_y, \p canvas_y + \p height) exceeds the size
    ///                                  of the canvas, or \p canvas_layer is invalid.
    virtual Sint32 write_raw_pixels(
        Uint32 width,
        Uint32 height,
        ICanvas* canvas,
        Uint32 canvas_x,
        Uint32 canvas_y,
        Uint32 canvas_layer,
        const void* buffer,
        bool buffer_topdown,
        const char* buffer_pixel_type,
        Uint32 buffer_padding = 0) const = 0;

    //@}
    /// \name Conversion between canvases and encoded images
    //@{

    /// Encodes the pixel data of a canvas into a memory buffer.
    ///
    /// \param canvas        The canvas whose contents are to be used.
    /// \param image_format  The desired image format of the image, e.g., \c "jpg". Note that
    ///                      support for a given image format requires an image plugin capable of
    ///                      handling that format.
    /// \param pixel_type    The desired pixel type. See \ref mi_neuray_types for a list of
    ///                      supported pixel types. Not every image plugin supports every pixel
    ///                      type. If the requested pixel type is not supported, the argument is
    ///                      ignored and one of the supported formats is chosen instead.
    /// \param quality       The compression quality is an integer in the range from 0 to 100, where
    ///                      0 is the lowest quality, and 100 is the highest quality.
    /// \return              The created buffer, or \c NULL in case of failure.
    virtual IBuffer* create_buffer_from_canvas(
        const ICanvas* canvas,
        const char* image_format,
        const char* pixel_type,
        const char* quality) const = 0;

    /// Decodes the pixel data of a memory buffer into a canvas.
    ///
    /// \param buffer        The buffer that holds the encoded pixel data.
    /// \param image_format  The image format of the buffer, e.g., \c "jpg". Note that
    ///                      support for a given image format requires an image plugin capable of
    ///                      handling that format.
    /// \return              The canvas with the decoded pixel data, or \c NULL in case of failure.
    virtual ICanvas* create_canvas_from_buffer(
        const IBuffer* buffer,
        const char* image_format) const = 0;

    /// Indicates whether a particular image format is supported for decoding.
    ///
    /// Support for a given image format requires an image plugin capable of handling that format.
    /// This method allows to check whether such a plugin has been loaded for a particular format.
    ///
    /// Decoding is used when the image is converted into a canvas from a \if DICE_API memory
    /// buffer. \else memory buffer or a file \endif. Note that even if this method returns \c true,
    /// #create_canvas_from_buffer() \if IRAY_API or
    /// #mi::neuraylib::IImport_api::import_canvas() \endif can still fail for a particular image if
    /// that image uses an unsupported feature.
    ///
    /// \param image_format   The image format in question, e.g., \c "jpg".
    /// \param reader         An optional reader \if IRAY_API used by
    ///                       #mi::neuraylib::IImage_plugin::test(). \endif
    /// \return               \c true if the image format is supported, \c false otherwise
    virtual bool supports_format_for_decoding(
        const char* image_format, IReader* reader = 0) const = 0;

    /// Indicates whether a particular image format is supported for encoding.
    ///
    /// Support for a given image format requires an image plugin capable of handling that format.
    /// This method allows to check whether such a plugin has been loaded for a particular format.
    ///
    /// Encoding is used when the image is converted from a canvas into a \if DICE_API memory
    /// buffer. \else memory buffer or a file. \endif. Note that even if this method returns
    /// \c true, #create_buffer_from_canvas() \if IRAY_API or
    /// #mi::neuraylib::IExport_api::export_canvas \endif can still fail if the given canvas
    /// uses an unsupported feature, e.g., multiple layers.
    ///
    /// \param image_format   The image format in question, e.g., \c "jpg".
    /// \return               \c true if the image format is supported, \c false otherwise
    virtual bool supports_format_for_encoding( const char* image_format) const = 0;

    //@}
    /// \name Utility methods for canvases
    //@{

    /// Converts a canvas to a different pixel type.
    ///
    /// \note This method creates a copy if the passed-in canvas already has the desired pixel type.
    /// (It cannot return the passed-in canvas since this would require a const cast.) If
    /// performance is critical, you should compare pixel types yourself and skip the method call if
    /// pixel type conversion is not needed.)
    ///
    /// The conversion converts a given pixel as follows:
    ///
    /// - Floating-point values are linearly mapped to integers as follows: 0.0f is mapped to 0 and
    ///   1.0f is mapped to 255 or 65535, respectively. Note that the pixel type \c "Sint8" is
    ///   treated as the corresponding unsigned integer type \c "Uint8" here. Floating-point values
    ///   are clamped to [0.0f, 1.0f] beforehand. The reverse conversion uses the corresponding
    ///   inverse mapping.
    /// - Single-channel formats are converted to grey-scale RGB formats by duplicating the value
    ///   in each channel.
    /// - RGB formats are converted to single-channel formats by mixing the RGB channels with
    ///   weights 0.27f for red, 0.67f for green, and 0.06f for blue.
    /// - If an alpha channel is added, the values are set to 1.0f, 255, or 65535 respectively.
    /// - The pixel type \c "Float32<4>" is treated in the same way as \c "Color", \c "Float32<3>"
    ///   in the same way as \c "Rgb_fp", and \c "Sint32" in the same way as \c "Rgba".
    /// - The pixel type \c "Rgbe" is converted via \c "Rgb_fp". Similarly, \c "Rgbea" is converted
    ///   via \c "Color".
    /// - \c "Float32<2>" is converted to single-channel formats by averaging the two channels. If
    ///   \c "Float32<2>" is converted to three- or four-channel formats, the blue channel is set to
    ///   0.0f, or 0, respectively. Conversion of single-channel formats to \c "Float32<2>"
    ///   duplicates the channel. Conversion of three- or four-channel formats to \c "Float32<2>"
    ///   drops the third and fourth channel.
    ///
    /// \param canvas       The canvas to convert (or to copy).
    /// \param pixel_type   The desired pixel type. See \ref mi_neuray_types for a list of supported
    ///                     pixel types. If this pixel type is the same as the pixel type of \p
    ///                     canvas, then a copy of the canvas is returned.
    /// \return             A canvas with the requested pixel type, or \c NULL in case of errors
    ///                     (\p canvas is \c NULL, or \p pixel_type is not valid).
    virtual ICanvas* convert( const ICanvas* canvas, const char* pixel_type) const = 0;

    /// Sets the gamma value of a canvas and adjusts the pixel data accordingly.
    ///
    /// \note Gamma adjustments are always done in pixel type "Color" or "Rgb_fp". If necessary,
    ///       the pixel data is converted forth and back automatically (which needs temporary
    ///       buffers).
    ///
    /// \param canvas           The canvas whose pixel data is to be adjusted.
    /// \param new_gamma        The new gamma value.
    virtual void adjust_gamma( ICanvas* canvas, Float32 new_gamma) const = 0;

    //@}
    /// \name Utility methods for pixel type characteristics
    //@{

    /// Returns the number of components per pixel type.
    ///
    /// For example, for the pixel type "Color" the method returns 4 because it consists of four
    /// components R, G, B, and A. Returns 0 in case of invalid pixel types.
    ///
    /// \see #get_bytes_per_component()
    virtual Uint32 get_components_per_pixel( const char* pixel_type) const = 0;

    /// Returns the number of bytes used per pixel component.
    ///
    /// For example, for the pixel type "Color" the method returns 4 because its components are of
    /// type #mi::Float32 which needs 4 bytes. Returns 0 in case of invalid pixel types.
    ///
    /// \see #get_components_per_pixel()
    virtual Uint32 get_bytes_per_component( const char* pixel_type) const = 0;

    /// Creates mipmaps from the given canvas.
    ///
    /// \note The base level (the canvas that is passed in) is not included in the returned 
    /// canvas array.
    ///
    /// \param canvas           The canvas to create the mipmaps from.
    /// \param gamma_override   If this parameter is different from zero, it is used instead of the
    ///                         canvas gamma during mipmap creation.
    /// \return                 An array of type #mi::IPointer containing pointers to
    ///                         the mipmaps of type #mi::neuraylib::ICanvas.
    ///                         If no mipmaps could be created, NULL is returned.
    virtual IArray* create_mipmaps(
        const ICanvas* canvas, Float32 gamma_override=0.0f) const = 0;

    //@}
};

/*@}*/ // end group mi_neuray_rendering / mi_neuray_rtmp

} // namespace neuraylib

} // namespace mi

#endif // MI_NEURAYLIB_IIMAGE_API_H
