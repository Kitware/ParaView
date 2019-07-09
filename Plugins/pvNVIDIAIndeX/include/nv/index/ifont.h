/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Base class representing the fonts in the scene description.

#ifndef NVIDIA_INDEX_IFONT_H
#define NVIDIA_INDEX_IFONT_H

#include <mi/base/interface_declare.h>
#include <mi/dice.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// A font attribute describes the appearance of a label
/// in the scene description. 
///
class IFont :
        public mi::base::Interface_declare<0xde2a47e9,0xa0b3,0x4b47,0xb2,0xc6,0x8f,0x22,0x6c,0x36,0x02,0x61,
                                           nv::index::IAttribute>
{
public:
    /// Set the font file name.
    ///
    /// \param          font_file_name      Unix style path to the font file.
    /// \return false when the specified font path doesn't found on
    /// the system when this method is called. However the path will
    /// be set.
    ///
    virtual bool set_file_name(const char* font_file_name) = 0;

    /// Returns the font file name.
    ///
    /// \return         Unix style path to the font file.
    ///
    virtual const char* get_file_name() const = 0;
    
    /// Set the font resolution. The resolution of the font defines
    /// the resolution of the bitmap generated for the labels, thus,
    /// influences the visual quality of the labels. The recommended
    /// useful range of this size is in [16, 1024].
    ///
    /// \param       resolution  The font resolution in pixels.
    /// \return true when set is succeeded.
    ///
    virtual bool set_font_resolution(mi::Float32 resolution) = 0;

    /// Get the resolution of the font. The resolution of the font
    /// defines the resolution of the bitmap generated for the
    /// labels, thus, influences the visual quality of the labels.
    ///
    /// \return         The font resolution in pixels.
    ///
    virtual mi::Float32 get_font_resolution() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_IFONT_H
