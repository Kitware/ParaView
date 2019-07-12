/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Database element for a colormap

#ifndef NVIDIA_INDEX_ICOLORMAP_H
#define NVIDIA_INDEX_ICOLORMAP_H

#include <mi/base/interface_declare.h>
#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute
/// An abstract representation of a colormap for storing in the distributed database.
class IColormap :
    public mi::base::Interface_declare<0x68ba9632,0xe3d6,0x4ced,0xa8,0xd7,0x32,0x0f,0x87,0x33,0xde,0x1b,
                                       nv::index::IAttribute>
{
public:
    /// Returns an entry of the colormap.
    ///
    /// \note This method is not used in the IndeX library to access the colormap array. It
    /// should be used by an application.
    ///
    /// \param[in] idx Index in the colormap
    ///
    /// \return Color at the given index
    virtual mi::math::Color_struct get_color(mi::Uint32 idx) const = 0;

    /// Sets an entry of the colormap.
    ///
    /// \param[in] idx   Index in the colormap
    /// \param[in] color Color to be set
    virtual void set_color(
        mi::Uint32                    idx,
        const mi::math::Color_struct& color) = 0;

    /// Set the colormap values using an array of color values.
    ///
    /// \param[in] colormap_values      Array of color values.
    /// \param[in] nb_entries           Number of color values.
    ///
    virtual void set_colormap(
        const mi::math::Color_struct* colormap_values,
        mi::Uint32                    nb_entries) = 0;

    /// Returns a reference to the colormap array.
    ///
    /// \note This method is used internally by the IndeX library to access the colormap data.
    ///
    /// \return Constant reference to the internal colormap array
    virtual const mi::math::Color_struct* get_colormap() const = 0;

    /// Returns the number of entries in the colormap.
    /// \return number of colormap entries
    virtual mi::Uint32 get_number_of_entries() const = 0;

    /// Returns the range of colormap indices in which color values are non-transparent.
    /// \param[out] min Minimum index with non-transparent color (inclusive).
    /// \param[out] max Maximum index with non-transparent color (inclusive).
    virtual void get_non_transparent_range(
        mi::Uint32& min,
        mi::Uint32& max) const = 0;

    /// Sets the domain of the colormap, describing how source data
    /// (e.g. voxel values) should be mapped to color values.
    ///
    /// The domain is normalized to support different types of
    /// source values: The default domain [0.0, 1.0] corresponds to
    /// raw data values in the range [0, 255] for 8-bit integer
    /// data, [0, 65535] for 16-bit integer data, or [0.0, 1.0] for
    /// normalized float data. For non-normalized float data the
    /// domain should be set to the actual value range found in the
    /// data.
    ///
    /// The domain may also be outside of the actual value range of
    /// the data, or just cover part of the value range. Data values
    /// outside of the specified domain treated according to the
    /// \c Domain_boundary_mode controlled by \c set_domain_boundary_mode().
    ///
    /// Note that \c value_first > \c value_last is valid, and will
    /// effectively flip the colormap. If \c value_first is equal to
    /// \c value_last then the default domain [0.0, 1.0] will be
    /// used.
    ///
    /// \param[in] value_first  Data value (normalized) that should be
    ///                         mapped to the first entry of the colormap.
    /// \param[in] value_last   Data value (normalized) that should be
    ///                         mapped to the last entry of the colormap.
    virtual void set_domain(
        mi::Float32 value_first,
        mi::Float32 value_last) = 0;

    /// Returns the domain of the colormap.
    ///
    /// \param[out] value_first  Data value (normalized) that should be
    ///                          mapped to the first entry of the colormap.
    /// \param[out] value_last   Data value (normalized) that should be
    ///                          mapped to the last entry of the colormap.
    virtual void get_domain(
        mi::Float32& value_first,
        mi::Float32& value_last) const = 0;

    /// Controls how data values that are outside of the colormap's
    /// domain should be treated.
    enum Domain_boundary_mode
    {
        /// Data values outside of the domain will be clamped to the
        /// domain, i.e. will be mapped to either the first or last
        /// entry of the colormap.
        CLAMP_TO_EDGE        = 0,
        /// Data values outside of the domain will be considered
        /// fully transparent and therefore ignored for rendering
        CLAMP_TO_TRANSPARENT = 1
    };

    /// Sets the mode for handling data values outside of the
    /// colormap's domain.
    ///
    /// \param[in] mode Domain boundary mode
    virtual void set_domain_boundary_mode(Domain_boundary_mode mode) = 0;

    /// Returns the mode for handling data values outside of the
    /// colormap's domain.
    ///
    /// \return Current domain boundary mode
    virtual Domain_boundary_mode get_domain_boundary_mode() const = 0;
};

}} // namespace index / nv

#endif // NVIDIA_INDEX_ICOLORMAP_H
