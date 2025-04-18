/***************************************************************************************************
 * Copyright 2025 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file
/// \brief Typedefs for types from the math API

#ifndef MI_NEURAYLIB_MATRIX_TYPEDEFS_H
#define MI_NEURAYLIB_MATRIX_TYPEDEFS_H

#include <mi/math/matrix.h>

namespace mi {

/** \addtogroup mi_neuray_compounds
@{
*/

/// 2 x 2 matrix of bool.
///
/// \see #mi::Boolean_2_2_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_2_2 = math::Matrix<bool,2,2>;

/// 2 x 3 matrix of bool.
///
/// \see #mi::Boolean_2_3_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_2_3 = math::Matrix<bool,2,3>;

/// 2 x 4 matrix of bool.
///
/// \see #mi::Boolean_2_4_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_2_4 = math::Matrix<bool,2,4>;

/// 3 x 2 matrix of bool.
///
/// \see #mi::Boolean_3_2_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_3_2 = math::Matrix<bool,3,2>;

/// 3 x 3 matrix of bool.
///
/// \see #mi::Boolean_3_3_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_3_3 = math::Matrix<bool,3,3>;

/// 3 x 4 matrix of bool.
///
/// \see #mi::Boolean_3_4_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_3_4 = math::Matrix<bool,3,4>;

/// 4 x 2 matrix of bool.
///
/// \see #mi::Boolean_4_2_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_4_2 = math::Matrix<bool,4,2>;

/// 4 x 3 matrix of bool.
///
/// \see #mi::Boolean_4_3_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_4_3 = math::Matrix<bool,4,3>;

/// 4 x 4 matrix of bool.
///
/// \see #mi::Boolean_4_4_struct for the corresponding POD type and
///      #mi::math::Matrix for the underlying template class
using Boolean_4_4 = math::Matrix<bool,4,4>;



/// 2 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_2_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_2 = math::Matrix<Sint32,2,2>;

/// 2 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_2_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_3 = math::Matrix<Sint32,2,3>;

/// 2 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_2_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_4 = math::Matrix<Sint32,2,4>;

/// 3 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_3_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_2 = math::Matrix<Sint32,3,2>;

/// 3 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_3_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_3 = math::Matrix<Sint32,3,3>;

/// 3 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_3_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_4 = math::Matrix<Sint32,3,4>;

/// 4 x 2 matrix of %Sint32.
///
/// \see #mi::Sint32_4_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_2 = math::Matrix<Sint32,4,2>;

/// 4 x 3 matrix of %Sint32.
///
/// \see #mi::Sint32_4_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_3 = math::Matrix<Sint32,4,3>;

/// 4 x 4 matrix of %Sint32.
///
/// \see #mi::Sint32_4_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_4 = math::Matrix<Sint32,4,4>;



/// 2 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_2_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_2 = math::Matrix<Uint32,2,2>;

/// 2 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_2_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_3 = math::Matrix<Uint32,2,3>;

/// 2 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_2_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_4 = math::Matrix<Uint32,2,4>;

/// 3 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_3_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_2 = math::Matrix<Uint32,3,2>;

/// 3 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_3_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_3 = math::Matrix<Uint32,3,3>;

/// 3 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_3_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_4 = math::Matrix<Uint32,3,4>;

/// 4 x 2 matrix of %Uint32.
///
/// \see #mi::Uint32_4_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_2 = math::Matrix<Uint32,4,2>;

/// 4 x 3 matrix of %Uint32.
///
/// \see #mi::Uint32_4_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_3 = math::Matrix<Uint32,4,3>;

/// 4 x 4 matrix of %Uint32.
///
/// \see #mi::Uint32_4_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_4 = math::Matrix<Uint32,4,4>;



/// 2 x 2 matrix of %Float32.
///
/// \see #mi::Float32_2_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_2 = math::Matrix<Float32,2,2>;

/// 2 x 3 matrix of %Float32.
///
/// \see #mi::Float32_2_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_3 = math::Matrix<Float32,2,3>;

/// 2 x 4 matrix of %Float32.
///
/// \see #mi::Float32_2_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_4 = math::Matrix<Float32,2,4>;

/// 3 x 2 matrix of %Float32.
///
/// \see #mi::Float32_3_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_2 = math::Matrix<Float32,3,2>;

/// 3 x 3 matrix of %Float32.
///
/// \see #mi::Float32_3_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_3 = math::Matrix<Float32,3,3>;

/// 3 x 4 matrix of %Float32.
///
/// \see #mi::Float32_3_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_4 = math::Matrix<Float32,3,4>;

/// 4 x 2 matrix of %Float32.
///
/// \see #mi::Float32_4_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_2 = math::Matrix<Float32,4,2>;

/// 4 x 3 matrix of %Float32.
///
/// \see #mi::Float32_4_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_3 = math::Matrix<Float32,4,3>;

/// 4 x 4 matrix of %Float32.
///
/// \see #mi::Float32_4_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_4 = math::Matrix<Float32,4,4>;



/// 2 x 2 matrix of %Float64.
///
/// \see #mi::Float64_2_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_2 = math::Matrix<Float64,2,2>;

/// 2 x 3 matrix of %Float64.
///
/// \see #mi::Float64_2_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_3 = math::Matrix<Float64,2,3>;

/// 2 x 4 matrix of %Float64.
///
/// \see #mi::Float64_2_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_4 = math::Matrix<Float64,2,4>;

/// 3 x 2 matrix of %Float64.
///
/// \see #mi::Float64_3_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_2 = math::Matrix<Float64,3,2>;

/// 3 x 3 matrix of %Float64.
///
/// \see #mi::Float64_3_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_3 = math::Matrix<Float64,3,3>;

/// 3 x 4 matrix of %Float64.
///
/// \see #mi::Float64_3_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_4 = math::Matrix<Float64,3,4>;

/// 4 x 2 matrix of %Float64.
///
/// \see #mi::Float64_4_2_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_2 = math::Matrix<Float64,4,2>;

/// 4 x 3 matrix of %Float64.
///
/// \see #mi::Float64_4_3_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_3 = math::Matrix<Float64,4,3>;

/// 4 x 4 matrix of %Float64.
///
/// \see #mi::Float64_4_4_struct for the corresponding POD type,
///      #mi::math::Matrix for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_4 = math::Matrix<Float64,4,4>;



/// 2 x 2 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_2_2_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_2_2_struct = math::Matrix_struct<bool,2,2>;

/// 2 x 3 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_2_3_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_2_3_struct = math::Matrix_struct<bool,2,3>;

/// 2 x 4 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_2_4_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_2_4_struct = math::Matrix_struct<bool,2,4>;

/// 3 x 2 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_3_2_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_3_2_struct = math::Matrix_struct<bool,3,2>;

/// 3 x 3 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_3_3_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_3_3_struct = math::Matrix_struct<bool,3,3>;

/// 3 x 4 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_3_4_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_3_4_struct = math::Matrix_struct<bool,3,4>;

/// 4 x 2 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_4_2_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_4_2_struct = math::Matrix_struct<bool,4,2>;

/// 4 x 3 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_4_3_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_4_3_struct = math::Matrix_struct<bool,4,3>;

/// 4 x 4 matrix of bool (underlying POD type).
///
/// \see #mi::Boolean_4_4_struct for the corresponding non-POD type and
///      #mi::math::Matrix_struct for the underlying template class
using Boolean_4_4_struct = math::Matrix_struct<bool,4,4>;



/// 2 x 2 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_2_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_2_struct = math::Matrix_struct<Sint32,2,2>;

/// 2 x 3 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_2_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_3_struct = math::Matrix_struct<Sint32,2,3>;

/// 2 x 4 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_2_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_2_4_struct = math::Matrix_struct<Sint32,2,4>;

/// 3 x 2 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_3_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_2_struct = math::Matrix_struct<Sint32,3,2>;

/// 3 x 3 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_3_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_3_struct = math::Matrix_struct<Sint32,3,3>;

/// 3 x 4 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_3_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_3_4_struct = math::Matrix_struct<Sint32,3,4>;

/// 4 x 2 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_4_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_2_struct = math::Matrix_struct<Sint32,4,2>;

/// 4 x 3 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_4_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_3_struct = math::Matrix_struct<Sint32,4,3>;

/// 4 x 4 matrix of %Sint32 (underlying POD type).
///
/// \see #mi::Sint32_4_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Sint32 for the type of the matrix components
using Sint32_4_4_struct = math::Matrix_struct<Sint32,4,4>;



/// 2 x 2 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_2_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_2_struct = math::Matrix_struct<Uint32,2,2>;

/// 2 x 3 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_2_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_3_struct = math::Matrix_struct<Uint32,2,3>;

/// 2 x 4 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_2_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_2_4_struct = math::Matrix_struct<Uint32,2,4>;

/// 3 x 2 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_3_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_2_struct = math::Matrix_struct<Uint32,3,2>;

/// 3 x 3 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_3_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_3_struct = math::Matrix_struct<Uint32,3,3>;

/// 3 x 4 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_3_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_3_4_struct = math::Matrix_struct<Uint32,3,4>;

/// 4 x 2 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_4_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_2_struct = math::Matrix_struct<Uint32,4,2>;

/// 4 x 3 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_4_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_3_struct = math::Matrix_struct<Uint32,4,3>;

/// 4 x 4 matrix of %Uint32 (underlying POD type).
///
/// \see #mi::Uint32_4_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Uint32 for the type of the matrix components
using Uint32_4_4_struct = math::Matrix_struct<Uint32,4,4>;



/// 2 x 2 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_2_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_2_struct = math::Matrix_struct<Float32,2,2>;

/// 2 x 3 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_2_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_3_struct = math::Matrix_struct<Float32,2,3>;

/// 2 x 4 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_2_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_2_4_struct = math::Matrix_struct<Float32,2,4>;

/// 3 x 2 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_3_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_2_struct = math::Matrix_struct<Float32,3,2>;

/// 3 x 3 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_3_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_3_struct = math::Matrix_struct<Float32,3,3>;

/// 3 x 4 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_3_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_3_4_struct = math::Matrix_struct<Float32,3,4>;

/// 4 x 2 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_4_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_2_struct = math::Matrix_struct<Float32,4,2>;

/// 4 x 3 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_4_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_3_struct = math::Matrix_struct<Float32,4,3>;

/// 4 x 4 matrix of %Float32 (underlying POD type).
///
/// \see #mi::Float32_4_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float32 for the type of the matrix components
using Float32_4_4_struct = math::Matrix_struct<Float32,4,4>;



/// 2 x 2 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_2_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_2_struct = math::Matrix_struct<Float64,2,2>;

/// 2 x 3 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_2_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_3_struct = math::Matrix_struct<Float64,2,3>;

/// 2 x 4 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_2_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_2_4_struct = math::Matrix_struct<Float64,2,4>;

/// 3 x 2 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_3_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_2_struct = math::Matrix_struct<Float64,3,2>;

/// 3 x 3 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_3_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_3_struct = math::Matrix_struct<Float64,3,3>;

/// 3 x 4 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_3_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_3_4_struct = math::Matrix_struct<Float64,3,4>;

/// 4 x 2 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_4_2_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_2_struct = math::Matrix_struct<Float64,4,2>;

/// 4 x 3 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_4_3_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_3_struct = math::Matrix_struct<Float64,4,3>;

/// 4 x 4 matrix of %Float64 (underlying POD type).
///
/// \see #mi::Float64_4_4_struct for the corresponding non-POD type,
///      #mi::math::Matrix_struct for the underlying template class, and
///      #mi::Float64 for the type of the matrix components
using Float64_4_4_struct = math::Matrix_struct<Float64,4,4>;

/**@}*/ // end group mi_neuray_compounds

} // namespace mi

#endif // MI_NEURAYLIB_MATRIX_TYPEDEFS_H
