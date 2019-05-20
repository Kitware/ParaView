/******************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 *****************************************************************************/
/// \file
/// \brief Scene attribute controlling volume filter mode.

#ifndef NVIDIA_INDEX_IVOLUME_FILTER_MODE_H
#define NVIDIA_INDEX_IVOLUME_FILTER_MODE_H

#include <mi/base/interface_declare.h>

#include <nv/index/iattribute.h>

namespace nv
{
namespace index
{

/// @ingroup nv_index_scene_description_attribute

/// Scene attribute controlling volume filter mode.
///
/// The volume filter mode specifies what kind of interpolation of volume samples is performed upon
/// rendering a volume primitive directly (using \c IRegular_volume) or indirectly (e.g. mapped on
/// an \c IPlane).
///
/// \note This attribute takes precedence over the global setting from \c
/// IConfig_settings::set_volume_filter().
///
/// \todo Currently this attribute only affects volume data mapped onto an \c IPlane.
///
class IVolume_filter_mode : public mi::base::Interface_declare<0x7346860a, 0x21c6, 0x4f71, 0xab,
                              0x1d, 0x95, 0xb4, 0xd9, 0x07, 0xf2, 0xd8, nv::index::IAttribute>
{
public:
  /// Filter modes (interpolation) for volume access. With post-classification the volume data
  /// is filtered before applying the colormap, while with pre-classification the colormap is
  /// applied before filtering.
  /// In GPU rendering mode, all the filters below including those tagged as 'software' are
  /// implemented using CUDA. Those filters that are are tagged 'hardware' leverage the
  /// texture hardware, e.g., linear filters, exposed through CUDA to further optimize
  /// the filtering performance.
  enum Filter_mode
  {
    /// Access a single voxel with nearest filtering (a.k.a. point filtering).
    VOLUME_FILTER_NEAREST = 0,
    /// Trilinear interpolation with post-classification, using texture hardware.
    VOLUME_FILTER_TRILINEAR_POST_HW = 1,
    /// Trilinear interpolation with post-classification, using software filtering.
    VOLUME_FILTER_TRILINEAR_POST_SW = 2,
    /// Trilinear interpolation with pre-classification, using software filtering.
    VOLUME_FILTER_TRILINEAR_PRE_SW = 3,
    /// Tricubic Catmull-Rom interpolation with pre-classification, using software filtering.
    VOLUME_FILTER_TRICUBIC_CATMULL = 4,
    /// Tricubic Catmull-Rom interpolation with pre-classification, using texture hardware.
    VOLUME_FILTER_TRICUBIC_CATMULL_POST_HW = 5,
    /// Tricubic B-spline interpolation with pre-classification, using software filtering.
    VOLUME_FILTER_TRICUBIC_BSPLINE = 6,
    /// Tricubic B-spline interpolation with post-classification, using texture hardware.
    VOLUME_FILTER_TRICUBIC_BSPLINE_POST_HW = 7
  };

  /// Sets the filter mode for volume rendering, which is used when sampling volume data
  /// (default: \c VOLUME_FILTER_NEAREST).
  ///
  /// \param[in] mode Volume filter mode
  virtual void set_filter_mode(Filter_mode mode) = 0;

  /// Returns the current volume filter mode.
  ///
  /// \return volume filter mode
  virtual Filter_mode get_filter_mode() const = 0;
};

} // namespace index
} // namespace nv

#endif // NVIDIA_INDEX_ITEXTURE_FILTER_MODE_H
