/***************************************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Typedefs for types from the math API

#ifndef MI_NEURAYLIB_TYPEDEFS_H
#define MI_NEURAYLIB_TYPEDEFS_H

#include <mi/neuraylib/matrix_typedefs.h>
#include <mi/neuraylib/vector_typedefs.h>

#include <mi/math/bbox.h>
#include <mi/math/color.h>
#include <mi/math/spectrum.h>

namespace mi {

/** \addtogroup mi_neuray_compounds
@{
*/


/// RGBA color class.
///
/// \see #mi::Color_struct for the corresponding POD type and
///      #mi::math::Color for the source of the typedef
using Color = math::Color;

/// RGBA color class (underlying POD type).
///
/// \see #mi::Color for the corresponding non-POD type and
///      #mi::math::Color_struct for the source of the typedef
using Color_struct = math::Color_struct;

using mi::math::Clip_mode;
using mi::math::CLIP_RGB;
using mi::math::CLIP_ALPHA;
using mi::math::CLIP_RAW;



/// Spectrum class.
///
/// \see #mi::Spectrum_struct for the corresponding POD type and
///      #mi::math::Spectrum for the source of the typedef
using Spectrum = math::Spectrum;

/// Spectrum class (underlying POD type).
///
/// \see #mi::Spectrum for the corresponding non-POD type and
///      #mi::math::Spectrum_struct for the source of the typedef
using Spectrum_struct = math::Spectrum_struct;



/// Three-dimensional bounding box.
///
/// \see #mi::Bbox3_struct for the corresponding POD type and
///      #mi::math::Bbox for the underlying template class
using Bbox3 = math::Bbox<Float32,3>;

/// Three-dimensional bounding box (underlying POD type).
///
/// \see #mi::Bbox3 for the corresponding non-POD type and
///      #mi::math::Bbox_struct for the underlying template class
using Bbox3_struct = math::Bbox_struct<Float32,3>;



/**@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_TYPEDEFS_H
