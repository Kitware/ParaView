/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/matrix.h
/// \brief A NxM-dimensional matrix class template of fixed dimensions with supporting
///        functions.
///
/// See \ref mi_math_matrix.

#ifndef MI_MATH_MATRIX_H
#define MI_MATH_MATRIX_H

#include <mi/base/types.h>
#include <mi/math/assert.h>
#include <mi/math/function.h>
#include <mi/math/vector.h>

namespace mi {

namespace math {

/** \defgroup mi_math_matrix Matrix Class
    \ingroup mi_math

    A NxM-dimensional matrix class template of fixed dimensions with supporting functions.

    \par Include File:
    <tt> \#include <mi/math/matrix.h></tt>

    @{
*/

/** \defgroup mi_math_matrix_struct Internal Storage Classes for Matrices
    \ingroup mi_math_matrix

    Storage class for matrix templates with support for \c xx, \c xy, \c xz, etc., members for
    appropriate dimensions.

    Use the #mi::math::Matrix template in your programs and this storage class only if you need a
    POD type, for example for parameter passing.

    \par Include File:
    <tt> \#include <mi/math/matrix.h></tt>

    @{
*/

//------- POD struct that provides storage for matrix elements --------

/** Storage class for a NxM-dimensional matrix class template of fixed dimensions.

    Used as %base class for the #mi::math::Matrix class template.

    Use the #mi::math::Matrix template in your programs and this storage class only if you need a
    POD type, for example for parameter passing.

    This class template provides array-like storage for \c ROW times \c COL many values of a
    (arithmetic) type \c T. It has specializations for \c ROW in [1,4] and \c COL in [1,4]. These
    specializations use data members named \c xx, \c xy, \c xz, etc., according to the dimensions.
    The first letter denotes the row, the second letter the column. These specializations provide
    users of the #mi::math::Matrix class template with the conventional member access to matrix
    elements, such as in the following example, which, for the sake of illustration, defines a
    transformation matrix with a factor two scaling transformation:
    \code
    mi::math::Matrix<mi::Float64,4,4> mat( 0.0);
    mat.xx = 2.0;
    mat.yy = 2.0;
    mat.zz = 2.0;
    mat.ww = 1.0;
    \endcode

    This class template contains only the data and no member functions. The necessary access
    abstraction is encoded in the free function #mi::math::matrix_base_ptr(), which is overloaded
    for the %general case and the various specializations for the small dimensions. It returns a
    pointer to the first element.

    \par Memory layout:
    Matrices are stored in row-major order, which says that the memory layout of a 4x4 matrix
    will be sequentially in memory <tt>xx, xy, xz, xw, yx, yy, ..., wx, wy, wz, ww</tt>.

    \note
    The matrix interpretation as a transformation is done by
    #mi::math::transform_point(const Matrix<T,4,4>&,const Vector<U,4>&) and related functions. The
    convention if vectors are row vectors and multiplied from the left with matrices, or column
    vectors and multiplied from the right with matrices, determines where the different parameters
    of a transformation are actually stored in the matrix.
*/
template <typename T, Size ROW, Size COL>
struct Matrix_struct
{
    T elements[ROW*COL]; ///< %general case matrix elements.
};

/// Specialization for 1x1-matrix.
template <typename T> struct Matrix_struct<T,1,1>
{
    T xx;  ///< xx-element.
};

/// Specialization for 2x1-matrix.
template <typename T> struct Matrix_struct<T,2,1>
{
    T xx;  ///< xx-element.
    T yx;  ///< yx-element.
};

/// Specialization for 3x1-matrix.
template <typename T> struct Matrix_struct<T,3,1>
{
    T xx;  ///< xx-element.
    T yx;  ///< yx-element.
    T zx;  ///< zx-element.
};

/// Specialization for 4x1-matrix.
template <typename T> struct Matrix_struct<T,4,1>
{
    T xx;  ///< xx-element.
    T yx;  ///< yx-element.
    T zx;  ///< zx-element.
    T wx;  ///< wx-element.
};

/// Specialization for 1x2-matrix.
template <typename T> struct Matrix_struct<T,1,2>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
};

/// Specialization for 2x2-matrix.
template <typename T> struct Matrix_struct<T,2,2>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
};

/// Specialization for 3x2-matrix.
template <typename T> struct Matrix_struct<T,3,2>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
};

/// Specialization for 4x2-matrix.
template <typename T> struct Matrix_struct<T,4,2>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
    T wx;  ///< wx-element.
    T wy;  ///< wy-element.
};

/// Specialization for 1x3-matrix.
template <typename T> struct Matrix_struct<T,1,3>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
};

/// Specialization for 2x3-matrix.
template <typename T> struct Matrix_struct<T,2,3>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
};

/// Specialization for 3x3-matrix.
template <typename T> struct Matrix_struct<T,3,3>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
    T zz;  ///< zz-element.
};

/// Specialization for 4x3-matrix.
template <typename T> struct Matrix_struct<T,4,3>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
    T zz;  ///< zz-element.
    T wx;  ///< wx-element.
    T wy;  ///< wy-element.
    T wz;  ///< wz-element.
};

/// Specialization for 1x4-matrix.
template <typename T> struct Matrix_struct<T,1,4>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T xw;  ///< xw-element.
};

/// Specialization for 2x4-matrix.
template <typename T> struct Matrix_struct<T,2,4>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T xw;  ///< xw-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
    T yw;  ///< yw-element.
};

/// Specialization for 3x4-matrix.
template <typename T> struct Matrix_struct<T,3,4>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T xw;  ///< xw-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
    T yw;  ///< yw-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
    T zz;  ///< zz-element.
    T zw;  ///< zw-element.
};

/// Specialization for 4x4-matrix.
template <typename T> struct Matrix_struct<T,4,4>
{
    T xx;  ///< xx-element.
    T xy;  ///< xy-element.
    T xz;  ///< xz-element.
    T xw;  ///< xw-element.
    T yx;  ///< yx-element.
    T yy;  ///< yy-element.
    T yz;  ///< yz-element.
    T yw;  ///< yw-element.
    T zx;  ///< zx-element.
    T zy;  ///< zy-element.
    T zz;  ///< zz-element.
    T zw;  ///< zw-element.
    T wx;  ///< wx-element.
    T wy;  ///< wy-element.
    T wz;  ///< wz-element.
    T ww;  ///< ww-element.
};

//------ Indirect access to matrix storage base ptr to keep Matrix_struct a POD --

// Helper class to determine the matrix struct base pointer of the generic
// #Matrix_struct (\c specialized==\c false).
template <typename T, class Matrix, bool specialized>
struct Matrix_struct_get_base_pointer
{
    static inline T*       get_base_ptr( Matrix& m)       { return m.elements; }
    static inline const T* get_base_ptr( const Matrix& m) { return m.elements; }
};

// Specialization of helper class to determine the matrix struct base pointer
// of a specialized #Matrix_struct (\c specialized==\c true).
template <typename T, class Matrix>
struct Matrix_struct_get_base_pointer<T,Matrix,true>
{
    static inline T*       get_base_ptr( Matrix& m)       { return &m.xx; }
    static inline const T* get_base_ptr( const Matrix& m) { return &m.xx; }
};


/// Returns the %base pointer to the matrix data.
template <typename T, Size ROW, Size COL>
inline T* matrix_base_ptr( Matrix_struct<T,ROW,COL>& mat)
{
    return Matrix_struct_get_base_pointer<T,Matrix_struct<T,ROW,COL>,
               (ROW<=4 && COL<=4)>::get_base_ptr( mat);
}

/// Returns the %base pointer to the matrix data.
template <typename T, Size ROW, Size COL>
inline const T* matrix_base_ptr( const Matrix_struct<T,ROW,COL>& mat)
{
    return Matrix_struct_get_base_pointer<T,Matrix_struct<T,ROW,COL>,
               (ROW<=4 && COL<=4)>::get_base_ptr( mat);
}

/*@}*/ // end group mi_math_matrix_struct


/** NxM-dimensional matrix class template of fixed dimensions.

    This class template provides array-like storage for \c ROW times \c COL many values of a
    (arithmetic) type \c T. Several functions and arithmetic operators support the work with
    matrices.

    The template parameters have the following requirements:

      - \b T: an arithmetic type supporting <tt>+ - * / == != < > <= >= sqrt() </tt>.
      - \b ROW: a value > 0 of type #mi::Size that defines the fixed number of rows in the matrix.
      - \b COL: a value > 0 of type #mi::Size that defines the fixed number of columns in the
                matrix.

    An instantiation of the matrix class template is a model of the STL container concept. It
    provides random access to its elements and corresponding random access iterators.

    Depending on the dimensions \c ROW and \c COL, the #mi::math::Matrix class template offers
    element access through the conventional data members named \c xx, \c xy, \c xz, \c xw, \c yx,
    \c yy, ..., \c wx, \c wy, \c wz, \c ww. The first letter denotes the row, the second letter the
    column. The following example, which, for the sake of illustration, defines a transformation
    matrix with a factor two scaling transformation:
    \code
    mi::math::Matrix<mi::Float64,4,4> mat( 0.0);
    mat.xx = 2.0;
    mat.yy = 2.0;
    mat.zz = 2.0;
    mat.ww = 1.0;
    \endcode

    \par Memory layout:
    Matrices are stored in row-major order, which says that the memory layout of a 4x4 matrix
    will be sequentially in memory <tt>xx, xy, xz, xw, yx, yy, ..., wx, wy, wz, ww</tt>.

    \note
    The matrix interpretation as a transformation is done by
    #mi::math::transform_point(const Matrix<T,4,4>&,const Vector<U,4>&) and related functions. The
    convention if vectors are row vectors and multiplied from the left with matrices, or column
    vectors and multiplied from the right with matrices, determines where the different parameters
    of a transformation are actually stored in the matrix.

    \note
    The matrix iterators access the matrix elements, however, the access \c operator[] accesses row
    vectors, which follows the interpretation of the matrix as a vector of row vectors in accordance
    with its underlying row-major memory layout.

    \see
        For the free functions and operators available for matrices see \ref mi_math_matrix.

    \see
        The underlying POD type #mi::math::Matrix_struct.
*/
template <typename T, Size ROW, Size COL>
class Matrix : public Matrix_struct<T,ROW,COL>
{
public:
    typedef Matrix_struct<T,ROW,COL> Pod_type;     ///< POD class corresponding to this matrix.
    typedef Matrix_struct<T,ROW,COL> storage_type; ///< Storage class used by this matrix.

    typedef T           value_type;                ///< Element type.
    typedef Size        size_type;                 ///< Size type, unsigned.
    typedef Difference  difference_type;           ///< Difference type, signed.
    typedef T *         pointer;                   ///< Mutable pointer to element.
    typedef const T *   const_pointer;             ///< Const pointer to element.
    typedef T &         reference;                 ///< Mutable reference to element.
    typedef const T &   const_reference;           ///< Const reference to element.

    /// Associated row vector of dimension \c COL.
    typedef Vector<T,COL>  Row_vector;

    /// Associated column vector of dimension \c ROW.
    typedef Vector<T,ROW>  Column_vector;

    static const Size ROWS    = ROW;      ///< Constant number of rows of the matrix.
    static const Size COLUMNS = COL;      ///< Constant number of columns of the matrix.
    static const Size SIZE    = ROW*COL;  ///< Constant size of the matrix.

     /// Constant size of the vector.
    static inline Size size()     { return SIZE; }

     /// Constant maximum size of the vector.
    static inline Size max_size() { return SIZE; }

    /// Enum type used to tag a special copy constructor that transposes the
    /// matrix while copying.
    enum Transposed_copy_tag {
        /// Enum value used to call a special copy constructor that transposes the
        /// matrix while copying.
        TRANSPOSED_COPY_TAG
    };

    /// Returns the pointer to the first matrix element.
    inline T * begin() { return mi::math::matrix_base_ptr( *this); }

    /// Returns the pointer to the first matrix element.
    inline T const * begin() const { return mi::math::matrix_base_ptr( *this); }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all \c ROW times \c COL many matrix elements.
    inline T * end() {  return begin() + SIZE; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all \c ROW times \c COL many matrix elements.
    inline T const * end() const { return begin() + SIZE; }

    /// Accesses the \p row-th row vector, <tt>0 <= row < ROW</tt>.
    inline Row_vector & operator[] (Size row)
    {
        mi_math_assert_msg( row < ROW, "precondition");
        return reinterpret_cast<Row_vector&>(begin()[row * COL]);
    }

    /// Accesses the \p row-th row vector, <tt>0 <= row < ROW</tt>.
    inline const Row_vector & operator[] (Size row) const
    {
        mi_math_assert_msg( row < ROW, "precondition");
        return reinterpret_cast<const Row_vector&>(begin()[row * COL]);
    }

    /// The default constructor leaves the vector elements uninitialized.
    inline Matrix() { }

    /// Constructor from underlying storage type.
    inline Matrix( const Matrix_struct<T,ROW,COL>& other)
    {
        for( Size i(0u); i < SIZE; ++i)
            begin()[i] = mi::math::matrix_base_ptr( other)[i];
    }

    /// Constructor initializes all matrix elements to zero and the diagonal elements to \p diag.
    ///
    /// Examples are \c diag==0 to create the null matrix and \c diag==1 to create the unit matrix.
    explicit inline Matrix( T diag) ///< value for diagonal elements.
    {
        for( Size i(0u); i < SIZE; ++i)
            begin()[i] = T(0);
        const Size MIN_DIM = (ROW < COL) ? ROW : COL;
        for( Size k(0u); k < MIN_DIM; ++k)
            begin()[k * COL + k] = diag;
    }

    /** Constructor requires the #mi::math::FROM_ITERATOR tag as first argument and initializes the
        matrix elements with the first \c ROW times \c COL elements from the sequence starting at
        the iterator \p p.

        \c Iterator must be a model of an input iterator. The value type of \c Iterator must be
        assignment compatible with the matrix elements type \c T.

        An example:
        \code
        std::vector<int> data( 42, 42); // 42 elements of value 42
        mi::math::Matrix<mi::Float64,4,4> mat( mi::math::FROM_ITERATOR, data.begin());
        \endcode
    */
    template <typename Iterator>
    inline Matrix( From_iterator_tag, Iterator p)
    {
        for( Size i(0u); i < SIZE; ++i, ++p)
            begin()[i] = *p;
    }

    /** Constructor initializes the matrix elements from an \c array of dimension \c ROW times
        \c COL.

        The value type \c T2 of the \c array must be assignment compatible with the matrix elements
        type \c T.

        An example that initializes a 2x2 unit matrix from an array:
        \code
        int data[4] = { 1, 0, 0, 1};
        mi::math::Matrix<mi::Float64,2,2> mat(data);
        \endcode
    */
    template <typename T2>
    inline explicit Matrix( T2 const (& array)[SIZE])
    {
        for( Size i(0u); i < SIZE; ++i)
            begin()[i] = array[i];
    }

    /// Template constructor that allows explicit conversions from other matrices with assignment
    /// compatible element value type.
    template <typename T2>
    inline explicit Matrix( const Matrix<T2,ROW,COL>& other)
    {
        for( Size i(0u); i < SIZE; ++i)
            begin()[i] = T(other.begin()[i]);
    }

    /// Template constructor that allows explicit conversions from underlying storage type with
    /// assignment compatible element value type.
    template <typename T2>
    inline explicit Matrix( const Matrix_struct<T2,ROW,COL>& other)
    {
        for( Size i(0u); i < SIZE; ++i)
            begin()[i] = T(mi::math::matrix_base_ptr( other)[i]);
    }

    /// Constructor that initializes the matrix with the transpose matrix of \c other.
    inline Matrix(
        Transposed_copy_tag,
        const Matrix<T,COL,ROW>& other)
    {
        for( Size i(0u); i < ROW; ++i)
            for( Size j(0u); j < COL; ++j)
                begin()[i * COL + j] = other.begin()[j * ROW + i];
    }

    /// Template constructor that initializes the matrix with the transpose matrix of \c other that
    /// allows the explicit conversions from other matrices with assignment compatible element value
    /// type.
    template <typename T2>
    inline Matrix(
        Transposed_copy_tag,
        const Matrix<T2,COL,ROW>& other)
    {
        for( Size i(0u); i < ROW; ++i)
            for( Size j(0u); j < COL; ++j)
                begin()[i * COL + j] = T(other.begin()[j * ROW + i]);
    }

    /// Dedicated constructor, for \c ROW==1 only, that initializes matrix from one row vector
    /// \c v0.
    ///
    /// \pre <tt>ROW == 1</tt>
    inline explicit Matrix(
        const Row_vector& v0)
    {
        mi_static_assert( ROW == 1);
        (*this)[0] = v0;
    }

    /// Dedicated constructor, for \c ROW==2 only, that initializes matrix from two row vectors
    /// \c (v0,v1).
    ///
    /// \pre <tt>ROW == 2</tt>
    inline Matrix(
        const Row_vector& v0,
        const Row_vector& v1)
    {
        mi_static_assert( ROW == 2);
        (*this)[0] = v0;
        (*this)[1] = v1;
    }

    /// Dedicated constructor, for \c ROW==3 only, that initializes matrix from three row vectors
    /// \c (v0,v1,v2).
    ///
    /// \pre <tt>ROW == 3</tt>
    inline Matrix(
        const Row_vector& v0,
        const Row_vector& v1,
        const Row_vector& v2)
    {
        mi_static_assert( ROW == 3);
        (*this)[0] = v0;
        (*this)[1] = v1;
        (*this)[2] = v2;
    }

    /// Dedicated constructor, for \c ROW==4 only, that initializes matrix from four row vectors
    /// \c (v0,v1,v2,v3).
    ///
    /// \pre <tt>ROW == 4</tt>
    inline Matrix(
        const Row_vector& v0,
        const Row_vector& v1,
        const Row_vector& v2,
        const Row_vector& v3)
    {
        mi_static_assert( ROW == 4);
        (*this)[0] = v0;
        (*this)[1] = v1;
        (*this)[2] = v2;
        (*this)[3] = v3;
    }

    /// 2-element constructor, must be a 1x2 or 2x1 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1)
    {
        mi_static_assert( SIZE == 2);
        begin()[0]  = m0;  begin()[1]  = m1;
    }

    /// 3-element constructor, must be a 1x3 or 3x1 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1, T m2)
    {
        mi_static_assert( SIZE == 3);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;
    }

    /// 4-element constructor, must be a 1x4, 2x2, or 4x1 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1, T m2, T m3)
    {
        mi_static_assert( SIZE == 4);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
    }

    /// 6-element constructor, must be a 2x3 or 3x2 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1, T m2, T m3, T m4, T m5)
    {
        mi_static_assert( SIZE == 6);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
        begin()[4]  = m4;  begin()[5]  = m5;
    }

    /// 8-element constructor, must be a 2x4 or 4x2 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1, T m2, T m3, T m4, T m5, T m6, T m7)
    {
        mi_static_assert( SIZE == 8);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
        begin()[4]  = m4;  begin()[5]  = m5;  begin()[6]  = m6;  begin()[7]  = m7;
    }

    /// 9-element constructor, must be a 3x3 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix( T m0, T m1, T m2, T m3, T m4, T m5, T m6, T m7, T m8)
    {
        mi_static_assert( SIZE == 9);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
        begin()[4]  = m4;  begin()[5]  = m5;  begin()[6]  = m6;  begin()[7]  = m7;
        begin()[8]  = m8;
    }

    /// 12-element constructor, must be a 3x4 or 4x3 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix(
        T  m0, T  m1, T  m2, T  m3,
        T  m4, T  m5, T  m6, T  m7,
        T  m8, T  m9, T m10, T m11)
    {
        mi_static_assert( SIZE == 12);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
        begin()[4]  = m4;  begin()[5]  = m5;  begin()[6]  = m6;  begin()[7]  = m7;
        begin()[8]  = m8;  begin()[9]  = m9;  begin()[10] = m10; begin()[11] = m11;
    }

    /// 16-element constructor, must be a 4x4 matrix.
    ///
    /// The elements are given in row-major order.
    inline Matrix(
        T  m0, T  m1, T  m2, T  m3,
        T  m4, T  m5, T  m6, T  m7,
        T  m8, T  m9, T m10, T m11,
        T m12, T m13, T m14, T m15)
    {
        mi_static_assert( SIZE == 16);
        begin()[0]  = m0;  begin()[1]  = m1;  begin()[2]  = m2;  begin()[3]  = m3;
        begin()[4]  = m4;  begin()[5]  = m5;  begin()[6]  = m6;  begin()[7]  = m7;
        begin()[8]  = m8;  begin()[9]  = m9;  begin()[10] = m10; begin()[11] = m11;
        begin()[12] = m12; begin()[13] = m13; begin()[14] = m14; begin()[15] = m15;
    }

    /// Assignment.
    inline Matrix& operator=( const Matrix& other)
    {
        Matrix_struct<T,ROW,COL>::operator=( other);
        return *this;
    }

    /// Accesses the (\p row,\p col)-th matrix element.
    ///
    /// \pre <tt>0 <= row < ROW</tt> and <tt>0 <= col < COL</tt>
    inline T& operator()( Size row, Size col)
    {
        mi_math_assert_msg( row < ROW, "precondition");
        mi_math_assert_msg( col < COL, "precondition");
        return begin()[row * COL + col];
    }

    /// Accesses the (\p row, \p col)-th matrix element.
    ///
    /// \pre <tt>0 <= row < ROW</tt> and <tt>0 <= col < COL</tt>
    inline const T& operator()( Size row, Size col) const
    {
        mi_math_assert_msg( row < ROW, "precondition");
        mi_math_assert_msg( col < COL, "precondition");
        return begin()[row * COL + col];
    }

    /// Accesses the \c i-th matrix element, indexed in the order of the row-major memory layout.
    ///
    /// \pre <tt>0 <= i < ROW*COL</tt>
    inline T get( Size i) const
    {
        mi_math_assert_msg( i < ROW*COL, "precondition");
        return begin()[i];
    }

    /// Accesses the (\p row, \p col)-th matrix element.
    ///
    /// \pre <tt>0 <= row < ROW</tt> and <tt>0 <= col < COL</tt>
    inline T get( Size row, Size col) const
    {
        mi_math_assert_msg( row < ROW, "precondition");
        mi_math_assert_msg( col < COL, "precondition");
        return begin()[row * COL + col];
    }

    /// Sets the \p i-th matrix element to \p value, indexed in the order of the row-major memory
    /// layout.
    ///
    /// \pre <tt>0 <= i < ROW*COL</tt>
    inline void set( Size i, T value)
    {
        mi_math_assert_msg( i < ROW*COL, "precondition");
        begin()[i] = value;
    }

    /// Sets the \p i-th matrix element to \p value, indexed in the order of the row-major memory
    /// layout.
    ///
    /// \pre <tt>0 <= row < ROW</tt> and <tt>0 <= col < COL</tt>
    inline void set( Size row, Size col, T value)
    {
        mi_math_assert_msg( row < ROW, "precondition");
        mi_math_assert_msg( col < COL, "precondition");
        begin()[row * COL + col] = value;
    }

    /// Returns the determinant of the upper-left 3x3 sub-matrix.
    ///
    /// \pre (\c ROW==3 or \c ROW==4) and (\c COL==3 or \c COL==4)
    inline T det33() const
    {
        mi_static_assert( (ROW==3 || ROW==4) && (COL==3 || COL==4) );
        return this->xx * this->yy * this->zz
             + this->xy * this->yz * this->zx
             + this->xz * this->yx * this->zy
             - this->xx * this->zy * this->yz
             - this->xy * this->zz * this->yx
             - this->xz * this->zx * this->yy;
    }

    /// Inverts this matrix and returns success or failure.
    ///
    /// The matrix cannot be inverted if it is singular or if it is non-square.
    inline bool invert();

    /// Transposes this matrix by exchanging rows and columns.
    ///
    /// \pre ROW==\c COL
    ///
    /// For transposing non-square matrices see the #mi::math::transpose() function.
    inline void transpose()
    {
        mi_static_assert( ROW==COL);
        for( Size i=0; i < ROW-1; ++i) {
            for( Size j=i+1; j < COL; ++j) {
                T tmp = get(i,j);
                set(i,j, get(j,i));
                set(j,i, tmp);
            }
        }
    }

    /// Adds a relative translation to the matrix (by components).
    ///
    /// The other matrix elements remain unchanged.
    inline void translate( T x, T y, T z)
    {
        this->wx += x;
        this->wy += y;
        this->wz += z;
    }

    /// Adds a relative translation to the matrix (by vector).
    ///
    /// The other matrix elements remain unchanged.
    inline void translate( const Vector<Float32,3>& vector)
    {
        this->wx += T( vector.x);
        this->wy += T( vector.y);
        this->wz += T( vector.z);
    }

    /// Adds a relative translation to the matrix (by vector).
    ///
    /// The other matrix elements remain unchanged.
    inline void translate( const Vector<Float64,3>& vector)
    {
        this->wx += T( vector.x);
        this->wy += T( vector.y);
        this->wz += T( vector.z);
    }

    /// Stores an absolute translation in the matrix (by component).
    ///
    /// The other matrix elements remain unchanged.
    inline void set_translation( T dx, T dy, T dz)
    {
        this->wx = dx;
        this->wy = dy;
        this->wz = dz;
    }

    /// Stores an absolute translation in the matrix (by vector).
    ///
    /// The other matrix elements remain unchanged.
    inline void set_translation( const Vector<Float32,3>& vector)
    {
        this->wx = T( vector.x);
        this->wy = T( vector.y);
        this->wz = T( vector.z);
    }

    /// Stores an absolute translation in the matrix (by vector).
    ///
    /// The other matrix elements remain unchanged.
    inline void set_translation( const Vector<Float64,3>& vector)
    {
        this->wx = T( vector.x);
        this->wy = T( vector.y);
        this->wz = T( vector.z);
    }

    /// Adds a relative rotation to the matrix (Euler angles, by component).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The elements in the last column of
    /// the matrix remain unchanged.
    inline void rotate( T xangle, T yangle, T zangle)
    {
        Matrix<T,4,4> tmp( T( 1));
        tmp.set_rotation( xangle, yangle, zangle);
        (*this) *= tmp;
    }

    /// Adds a relative rotation to the matrix (Euler angles, by vector).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The elements in the last column of
    /// the matrix remain unchanged.
    inline void rotate( const Vector<Float32,3>& angles)
    {
        Matrix<T,4,4> tmp( T( 1));
        tmp.set_rotation( T( angles.x), T( angles.y), T( angles.z));
        (*this) *= tmp;
    }

    /// Adds a relative rotation to the matrix (Euler angles, by vector).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The elements in the last column of
    /// the matrix remain unchanged.
    inline void rotate( const Vector<Float64,3>& angles)
    {
        Matrix<T,4,4> tmp( T( 1));
        tmp.set_rotation( T( angles.x), T( angles.y), T( angles.z));
        (*this) *= tmp;
    }

    /// Stores an absolute rotation in the upper left 3x3 rotation matrix (Euler angles, by
    /// component).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The other matrix elements remain
    /// unchanged.
    inline void set_rotation( T x_angle, T y_angle, T z_angle);

    /// Stores an absolute rotation in the upper left 3x3 rotation matrix (Euler angles, by
    /// vector).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The other matrix elements remain
    /// unchanged.
    inline void set_rotation( const Vector<Float32,3>& angles)
    {
        set_rotation( T( angles.x), T( angles.y), T( angles.z));
    }

    /// Stores an absolute rotation in the upper left 3x3 rotation matrix (Euler angles, by
    /// vector).
    ///
    /// The rotation is given by XYZ Euler angles (in radians). The other matrix elements remain
    /// unchanged.
    inline void set_rotation( const Vector<Float64,3>& angles)
    {
        set_rotation( T( angles.x), T( angles.y), T( angles.z));
    }

    /// Stores an absolute rotation (by axis and angle).
    ///
    /// The computed rotation matrix describes a rotation by the given angle (in radians) around the
    /// given axis. The other matrix elements are set to 0 and 1, respectively.
    ///
    /// \pre \p axis is normalized
    inline void set_rotation( const Vector<Float32,3>& axis, Float64 angle);

    /// Stores an absolute rotation (by axis and angle).
    ///
    /// The computed rotation matrix describes a rotation by the given angle (in radians) around the
    /// given axis. The other matrix elements are set to 0 and 1, respectively.
    ///
    /// \pre \p axis is normalized
    inline void set_rotation( const Vector<Float64,3>& axis, Float64 angle);

    /// Sets a transformation matrix based on a given center, a reference point, and a direction.
    ///
    /// The transformation is computed such that \p position is mapped to the origin, \p target
    /// is mapped to the negative Z-axis, and the \p up direction is mapped to the positive Y-axis.
    inline void lookat(
        const Vector<Float32,3>& position,
        const Vector<Float32,3>& target,
        const Vector<Float32,3>& up);

    /// Sets a transformation matrix based on a given center, a reference point, and a direction.
    ///
    /// The transformation is computed such that the origin is mapped to \p position, the negative
    /// Z-axis is mapped to the \p target direction, and the positive Y-axis is mapped to the \p up
    /// direction.
    inline void lookat(
        const Vector<Float64,3>& position,
        const Vector<Float64,3>& target,
        const Vector<Float64,3>& up);
};


//------ Free comparison operators ==, !=, <, <=, >, >= for matrices -----------

/// Returns \c true if \c lhs is elementwise equal to \c rhs.
template <typename T, Size ROW, Size COL>
inline bool operator==(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return is_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is elementwise not equal to \c rhs.
template <typename T, Size ROW, Size COL>
inline bool operator!=(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return is_not_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size ROW, Size COL>
inline bool operator<(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return lexicographically_less( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size ROW, Size COL>
inline bool operator<=(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return lexicographically_less_or_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size ROW, Size COL>
inline bool operator>(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return lexicographically_greater( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size ROW, Size COL>
inline bool operator>=(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    return lexicographically_greater_or_equal( lhs, rhs);
}

//------ Operator declarations for Matrix -------------------------------------

/// Adds \p rhs elementwise to \p lhs and returns the modified \p lhs.
template <typename T, Size ROW, Size COL>
Matrix<T,ROW,COL>& operator+=( Matrix<T,ROW,COL>& lhs, const Matrix<T,ROW,COL>& rhs);

/// Subtracts \p rhs elementwise from \p lhs and returns the modified \p lhs.
template <typename T, Size ROW, Size COL>
Matrix<T,ROW,COL>& operator-=( Matrix<T,ROW,COL>& lhs, const Matrix<T,ROW,COL>& rhs);

/// Adds \p lhs and \p rhs elementwise and returns the new result.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL> operator+(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    Matrix<T,ROW,COL> temp( lhs);
    temp += rhs;
    return temp;
}

/// Subtracts \p rhs elementwise from \p lhs and returns the new result.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL> operator-(
    const Matrix<T,ROW,COL>& lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    Matrix<T,ROW,COL> temp( lhs);
    temp -= rhs;
    return temp;
}

/// Negates the matrix \p mat elementwise and returns the new result.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL> operator-(
    const Matrix<T,ROW,COL>& mat)
{
    Matrix<T,ROW,COL> temp;
    for( Size i(0u); i < ROW*COL; ++i)
        temp.begin()[i] = -mat.get(i);
    return temp;
}

/// Performs matrix multiplication, \p lhs times \p rhs, assigns it to \p lhs, and
/// returns the modified \p lhs.
///
/// The matrix \p lhs must be of size \p ROW times \p COL, and the matrix \p rhs must be of size \p
/// COL times \p COL. For mixed size matrix multiplications, see the #mi::math::operator*() on
/// matrices.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL>& operator*=(
    Matrix<T,ROW,COL>&       lhs,
    const Matrix<T,COL,COL>& rhs)
{
    // There are more efficient ways of implementing this. Its a default solution. Specializations
    // exist below.
    Matrix<T,ROW,COL> old( lhs);
    for( Size rrow = 0; rrow < ROW; ++rrow) {
        for( Size rcol = 0; rcol < COL; ++rcol) {
            lhs( rrow, rcol) = T(0);
            for( Size k = 0; k < COL; ++k)
                lhs( rrow, rcol) += old( rrow, k) * rhs( k, rcol);
        }
    }
    return lhs;
}

/// Performs matrix multiplication, \p lhs times \p rhs, and returns the new result.
///
/// The matrices can be of different sizes, but the number of columns of \p lhs must be equal to the
/// number of rows of \p rhs. The result matrix has then the size (number of rows of \p lhs) times
/// (number of columns of \p rhs).
template <typename T, Size ROW1, Size COL1, Size ROW2, Size COL2>
inline Matrix<T,ROW1,COL2> operator*(
    const Matrix<T,ROW1,COL1>& lhs,
    const Matrix<T,ROW2,COL2>& rhs)
{
    // There are more efficient ways of implementing this. Its a default solution. Specializations
    // exist below.
    mi_static_assert( COL1 == ROW2);
    Matrix<T,ROW1,COL2> result;
    for( Size rrow = 0; rrow < ROW1; ++rrow) {
        for( Size rcol = 0; rcol < COL2; ++rcol) {
            result( rrow, rcol) = T(0);
            for( Size k = 0; k < COL1; ++k)
                result( rrow, rcol) += lhs( rrow, k) * rhs( k, rcol);
        }
    }
    return result;
}

/// Multiplies the (column) vector \p vec from the right with the matrix \p mat and returns the
/// resulting vector.
///
/// The result vector has the same dimension as the number of rows in the matrix \p mat.
///
/// \pre The vector \p vec must have the same dimension as the number of columns in the matrix \p
/// mat.
template <typename T, Size ROW, Size COL, Size DIM>
inline Vector<T,ROW> operator*(
    const Matrix<T,ROW,COL>& mat,
    const Vector<T,DIM>&     vec)
{
    mi_static_assert( COL == DIM);
    Vector<T,ROW> result;
    for( Size row = 0; row < ROW; ++row) {
        result[row] = T(0);
        for( Size col = 0; col < COL; ++col)
            result[row] += mat( row, col) * vec[col];
    }
    return result;
}

/// Multiplies the (row) vector \p vec from the left with the matrix \p mat and returns the
/// resulting vector.
///
/// The result vector has the same dimension as the number of columns in the matrix \p mat.
///
/// \pre The vector \p vec must have the same dimension as the number of rows in the matrix \p mat.
template <Size DIM, typename T, Size ROW, Size COL>
inline Vector<T,COL> operator*(
    const Vector<T,DIM>&     vec,
    const Matrix<T,ROW,COL>& mat)
{
    mi_static_assert( DIM == ROW);
    Vector<T,COL> result;
    for( Size col = 0; col < COL; ++col) {
        result[col] = T(0);
        for( Size row = 0; row < ROW; ++row)
            result[col] += mat( row, col) * vec[row];
    }
    return result;
}

/// Multiplies the matrix \p mat elementwise with the scalar \p factor and returns the modified
/// matrix \p mat.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL>& operator*=( Matrix<T,ROW,COL>& mat, T factor)
{
    for( Size i=0; i < ROW*COL; ++i)
        mat.begin()[i] *= factor;
    return mat;
}

/// Multiplies the matrix \p mat elementwise with the scalar \p factor and returns the new result.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL> operator*( const Matrix<T,ROW,COL>& mat, T factor)
{
    Matrix<T,ROW,COL> temp( mat);
    temp *= factor;
    return temp;
}

/// Multiplies the matrix \p mat elementwise with the scalar \p factor and returns the new result.
template <typename T, Size ROW, Size COL>
inline Matrix<T,ROW,COL> operator*( T factor, const Matrix<T,ROW,COL>& mat)
{
    Matrix<T,ROW,COL> temp( mat);
    temp *= factor; // * on T should be commutative (IEEE-754)
    return temp;
}


//------ Free Functions for Matrix --------------------------------------------

/// Returns the upper-left sub-matrix of size \c NEW_ROW times \c NEW_COL.
///
/// \pre \c NEW_ROW <= \c ROW and \c NEW_COL <= \c COL.
template <Size NEW_ROW, Size NEW_COL, typename T, Size ROW, Size COL>
inline Matrix<T,NEW_ROW,NEW_COL> sub_matrix(
    const Matrix<T,ROW,COL>& mat)
{
    mi_static_assert( NEW_ROW <= ROW);
    mi_static_assert( NEW_COL <= COL);
    Matrix<T,NEW_ROW,NEW_COL> result;
    for( Size i=0; i < NEW_ROW; ++i)
        for( Size j=0; j < NEW_COL; ++j)
            result( i, j) = mat( i, j);
    return result;
}



/// Returns the transpose of the matrix \p mat by exchanging rows and columns.
template <typename T, Size ROW, Size COL>
inline Matrix<T,COL,ROW> transpose(
    const Matrix<T,ROW,COL>& mat)
{
    Matrix<T,COL,ROW> result;
    for( Size i=0; i < ROW; ++i)
        for( Size j=0; j < COL; ++j)
            result( j, i) = mat( i, j);
    return result;
}

/// Returns a transformed 1D point by applying the full transformation in the 4x4 matrix
/// \c mat on the 1D point \c point, which includes the translation.
///
/// The point \c point is considered to be a row vector, which is multiplied from the left with the
/// matrix \c mat.
template <typename T, typename U>
inline U transform_point(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const U&             point)  ///< point to transform
{
    const T w = T(mat.xw * point + mat.ww);
    if( w == T(0) || w == T(1))
        return U(mat.xx * point + mat.wx);
    else
        return U((mat.xx * point + mat.wx) / w);
}

/// Returns a transformed 2D point by applying the full transformation in the 4x4 matrix
/// \c mat on the 2D point \c point, which includes the translation.
///
/// The point \c point is considered to be a row vector, which is multiplied from the left with the
/// matrix \c mat.
template <typename T, typename U>
inline Vector<U,2> transform_point(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,2>&   point)  ///< point to transform
{
    T w = T(mat.xw * point.x + mat.yw * point.y + mat.ww);
    if( w == T(0) || w == T(1))
        return Vector<U, 2>(
            U(mat.xx * point.x + mat.yx * point.y + mat.wx),
            U(mat.xy * point.x + mat.yy * point.y + mat.wy));
    else {
        w = T(1)/w; // optimization
        return Vector<U, 2>(
            U((mat.xx * point.x + mat.yx * point.y + mat.wx) * w),
            U((mat.xy * point.x + mat.yy * point.y + mat.wy) * w));
    }
}

/// Returns a transformed 3D point by applying the full transformation in the 4x4 matrix
/// \c mat on the 3D point \c point, which includes the translation.
///
/// The point \c point is considered to be a row vector, which is multiplied from the left with the
/// matrix \c mat.
template <typename T, typename U>
inline Vector<U,3> transform_point(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,3>&   point)  ///< point to transform
{
    T w = T(mat.xw * point.x + mat.yw * point.y + mat.zw * point.z + mat.ww);
    if( w == T(0) || w == T(1)) // avoids temporary and division
        return Vector<U,3>(
            U(mat.xx * point.x + mat.yx * point.y + mat.zx * point.z + mat.wx),
            U(mat.xy * point.x + mat.yy * point.y + mat.zy * point.z + mat.wy),
            U(mat.xz * point.x + mat.yz * point.y + mat.zz * point.z + mat.wz));
    else {
        w = T(1)/w; // optimization
        return Vector<U,3>(
            U((mat.xx * point.x + mat.yx * point.y + mat.zx * point.z + mat.wx) * w),
            U((mat.xy * point.x + mat.yy * point.y + mat.zy * point.z + mat.wy) * w),
            U((mat.xz * point.x + mat.yz * point.y + mat.zz * point.z + mat.wz) * w));
    }
}

/// Returns a transformed 4D point by applying the full transformation in the 4x4 matrix
/// \c mat on the 4D point \c point, which includes the translation.
///
/// The point \c point is considered to be a row vector, which is multiplied from the left with the
/// matrix \c mat.
template <typename T, typename U>
inline Vector<U,4> transform_point(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,4>&   point)  ///< point to transform
{
    return Vector<U, 4>(
        U(mat.xx * point.x + mat.yx * point.y + mat.zx * point.z + mat.wx * point.w),
        U(mat.xy * point.x + mat.yy * point.y + mat.zy * point.z + mat.wy * point.w),
        U(mat.xz * point.x + mat.yz * point.y + mat.zz * point.z + mat.wz * point.w),
        U(mat.xw * point.x + mat.yw * point.y + mat.zw * point.z + mat.ww * point.w));
}

/// Returns a transformed 1D vector by applying the 1x1 linear sub-transformation in the
/// 4x4 matrix \c mat on the 1D vector \c vector, which excludes the translation.
///
/// The vector \c vector is considered to be a row vector, which is multiplied from the left with
/// the matrix \c mat.
template <typename T, typename U>
inline U transform_vector(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const U&             vector) ///< vector to transform
{
    return U(mat.xx * vector);
}

/// Returns a transformed 2D vector by applying the 2x2 linear sub-transformation in the
/// 4x4 matrix \c mat on the 2D vector \c vector, which excludes the translation.
///
/// The vector \c vector is considered to be a row vector, which is multiplied from the left with
/// the matrix \c mat.
template <typename T, typename U>
inline Vector<U,2> transform_vector(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,2>&   vector) ///< vector to transform
{
    return Vector<U,2>(
        U(mat.xx * vector.x + mat.yx * vector.y),
        U(mat.xy * vector.x + mat.yy * vector.y));
}

/// Returns a transformed 3D vector by applying the 3x3 linear sub-transformation in the
/// 4x4 matrix \c mat on the 3D vector \c vector, which excludes the translation.
///
/// The vector \c vector is considered to be a row vector, which is multiplied from the left with
/// the matrix \c mat.
template <typename T, typename U>
inline Vector<U,3> transform_vector(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,3>&   vector) ///< vector to transform
{
    return Vector<U,3>(
        U(mat.xx * vector.x + mat.yx * vector.y + mat.zx * vector.z),
        U(mat.xy * vector.x + mat.yy * vector.y + mat.zy * vector.z),
        U(mat.xz * vector.x + mat.yz * vector.y + mat.zz * vector.z));
}

/// Returns an inverse transformed 3D normal vector by applying the 3x3 transposed linear
/// sub-transformation in the 4x4 matrix \c inv_mat on the 3D normal vector \c normal.
///
/// Note that in %general, a normal vector is transformed by the transposed inverse matrix (compared
/// to a point transformation). The inverse is often costly to compute, why one typically keeps the
/// inverse stored and this function operates then on the inverse matrix to properly transform
/// normal vectors. If you need to transform only one normal, you can also consider the
/// #mi::math::transform_normal() function, which includes the inverse computation.
///
/// The normal vector \c normal is considered to be a row vector, which is multiplied from the left
/// with the transposed upper-left 3x3 sub-matrix of \c inv_mat.
template <typename T, typename U>
inline Vector<U,3> transform_normal_inv(
    const Matrix<T,4,4>& inv_mat, ///< inverse 4x4 transformation matrix
    const Vector<U,3>&   normal)  ///< normal vector to transform
{
    return Vector<U,3>(
        U(inv_mat.xx * normal.x + inv_mat.xy * normal.y + inv_mat.xz * normal.z),
        U(inv_mat.yx * normal.x + inv_mat.yy * normal.y + inv_mat.yz * normal.z),
        U(inv_mat.zx * normal.x + inv_mat.zy * normal.y + inv_mat.zz * normal.z));
}

/// Returns a transformed 3D normal vector by applying the 3x3 transposed linear
/// sub-transformation in the inverse of the 4x4 matrix \c mat on the 3D normal vector \c normal.
///
/// Note that in %general, a normal vector is transformed by the transposed inverse matrix (compared
/// to a point transformation) and the inverse is often costly to compute. So, if you wish to
/// compute the inverse once and keep it to transform several normal vectors, you can use the
/// #mi::math::transform_normal_inv() function, which accepts the inverse
/// matrix as argument.
///
/// If the linear sub-matrix cannot be inverted, this function returns the unchanged \p normal.
///
/// The normal vector \c normal is considered to be a row vector, which is multiplied from the
/// left with the transposed upper-left 3x3 sub-matrix of the inverse of \c mat.
template <typename T, typename U>
inline Vector<U,3> transform_normal(
    const Matrix<T,4,4>& mat,    ///< 4x4 transformation matrix
    const Vector<U,3>&   normal) ///< normal vector to transform
{
    Matrix<T,3,3> sub_mat( mat.xx, mat.xy, mat.xz,
                           mat.yx, mat.yy, mat.yz,
                           mat.zx, mat.zy, mat.zz);
    bool inverted = sub_mat.invert();
    if( !inverted)
        return normal;
    return Vector<U,3>(
        U(sub_mat.xx * normal.x + sub_mat.xy * normal.y + sub_mat.xz * normal.z),
        U(sub_mat.yx * normal.x + sub_mat.yy * normal.y + sub_mat.yz * normal.z),
        U(sub_mat.zx * normal.x + sub_mat.zy * normal.y + sub_mat.zz * normal.z));
}


//------ Definitions of non-inline function -----------------------------------

template <typename T, Size ROW, Size COL>
Matrix<T,ROW,COL>& operator+=(
    Matrix<T,ROW,COL>&       lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    for( Size i=0; i < ROW*COL; ++i)
        lhs.begin()[i] += rhs.get(i);
    return lhs;
}

template <typename T, Size ROW, Size COL>
Matrix<T,ROW,COL>& operator-=(
    Matrix<T,ROW,COL>&       lhs,
    const Matrix<T,ROW,COL>& rhs)
{
    for( Size i=0; i < ROW*COL; ++i)
        lhs.begin()[i] -= rhs.get(i);
    return lhs;
}

#ifndef MI_FOR_DOXYGEN_ONLY

template <typename T, Size ROW, Size COL>
inline void Matrix<T,ROW,COL>::set_rotation( T xangle, T yangle, T zangle)
{
    mi_static_assert( COL == 4 && ROW == 4);
    T tsx, tsy, tsz; // sine of [xyz]_angle
    T tcx, tcy, tcz; // cosine of [xyz]_angle
    T tmp;
    const T min_angle = T(0.00024f);

    if( abs( xangle) > min_angle) {
        tsx = sin( xangle);
        tcx = cos( xangle);
    } else {
        tsx = xangle;
        tcx = T(1);
    }
    if( abs( yangle) > min_angle) {
        tsy = sin( yangle);
        tcy = cos( yangle);
    } else {
        tsy = yangle;
        tcy = T(1);
    }
    if( abs(zangle) > min_angle) {
        tsz = sin( zangle);
        tcz = cos( zangle);
    } else {
        tsz = T(zangle);
        tcz = T(1);
    }
    this->xx = tcy * tcz;
    this->xy = tcy * tsz;
    this->xz = -tsy;

    tmp  = tsx * tsy;
    this->yx = tmp * tcz - tcx * tsz;
    this->yy = tmp * tsz + tcx * tcz;
    this->yz = tsx * tcy;

    tmp  = tcx * tsy;
    this->zx = tmp * tcz + tsx * tsz;
    this->zy = tmp * tsz - tsx * tcz;
    this->zz = tcx * tcy;
}

template <typename T, Size ROW, Size COL>
inline void Matrix<T,ROW,COL>::set_rotation( const Vector<Float32,3>& axis_v, Float64 angle)
{
    mi_static_assert( COL == 4 && ROW == 4);
    Vector<T,3> axis( axis_v);
    const T min_angle = T(0.00024f);

    if( abs( T(angle)) < min_angle) {
        T xa = axis.x * T(angle);
        T ya = axis.y * T(angle);
        T za = axis.z * T(angle);

        this->xx = T(1);
        this->xy = za;
        this->xz = -ya;
        this->xw = T(0);

        this->yx = za;
        this->yy = T(1);
        this->yz = xa;
        this->yw = T(0);

        this->zx = -ya;
        this->zy = -xa;
        this->zz = T(1);
        this->zw = T(0);
    } else {
        T s = sin( T(angle));
        T c = cos( T(angle));
        T t = T(1) - c;
        T tmp;

        tmp = t * T(axis.x);
        this->xx  = tmp * T(axis.x) + c;
        this->xy  = tmp * T(axis.y) + s * T(axis.z);
        this->xz  = tmp * T(axis.z) - s * T(axis.y);
        this->xw  = T(0);

        tmp = t * T(axis.y);
        this->yx  = tmp * T(axis.x) - s * T(axis.z);
        this->yy  = tmp * T(axis.y) + c;
        this->yz  = tmp * T(axis.z) + s * T(axis.x);
        this->yw  = T(0);

        tmp = t * T(axis.z);
        this->zx  = tmp * T(axis.x) + s * T(axis.y);
        this->zy  = tmp * T(axis.y) - s * T(axis.x);
        this->zz  = tmp * T(axis.z) + c;
        this->zw  = T(0);
    }
    this->wx = this->wy = this->wz = T(0);
    this->ww = T(1);
}

template <typename T, Size ROW, Size COL>
inline void Matrix<T,ROW,COL>::set_rotation( const Vector<Float64,3>& axis_v, Float64 angle)
{
    mi_static_assert( COL == 4 && ROW == 4);
    Vector<T,3> axis( axis_v);
    const T min_angle = T(0.00024f);

    if( abs(T(angle)) < min_angle) {
        T xa = axis.x * T(angle);
        T ya = axis.y * T(angle);
        T za = axis.z * T(angle);

        this->xx = T(1);
        this->xy = za;
        this->xz = -ya;
        this->xw = T(0);

        this->yx = za;
        this->yy = T(1);
        this->yz = xa;
        this->yw = T(0);

        this->zx = -ya;
        this->zy = -xa;
        this->zz = T(1);
        this->zw = T(0);
    } else {
        T s = sin( T(angle));
        T c = cos( T(angle));
        T t = T(1) - c;
        T tmp;

        tmp = t * T(axis.x);
        this->xx  = tmp * T(axis.x) + c;
        this->xy  = tmp * T(axis.y) + s * T(axis.z);
        this->xz  = tmp * T(axis.z) - s * T(axis.y);
        this->xw  = T(0);

        tmp = t * T(axis.y);
        this->yx  = tmp * T(axis.x) - s * T(axis.z);
        this->yy  = tmp * T(axis.y) + c;
        this->yz  = tmp * T(axis.z) + s * T(axis.x);
        this->yw  = T(0);

        tmp = t * T(axis.z);
        this->zx  = tmp * T(axis.x) + s * T(axis.y);
        this->zy  = tmp * T(axis.y) - s * T(axis.x);
        this->zz  = tmp * T(axis.z) + c;
        this->zw  = T(0);
    }
    this->wx = this->wy = this->wz = T(0);
    this->ww = T(1);
}

template <typename T, Size ROW, Size COL>
inline void Matrix<T,ROW,COL>::lookat(
    const Vector<Float32,3>& position,
    const Vector<Float32,3>& target,
    const Vector<Float32,3>& up)
{
    mi_static_assert( COL == 4 && ROW == 4);
    Vector<Float32,3> xaxis, yaxis, zaxis;

    // Z vector
    zaxis = position - target;
    zaxis.normalize();

    // X vector = up cross Z
    xaxis = cross( up, zaxis);
    xaxis.normalize();

    // Recompute Y = Z cross X
    yaxis = cross( zaxis, xaxis);
    yaxis.normalize();

    // Build rotation matrix
    Matrix<T,4,4> rot(
        T(xaxis.x), T(yaxis.x), T(zaxis.x), T(0),
        T(xaxis.y), T(yaxis.y), T(zaxis.y), T(0),
        T(xaxis.z), T(yaxis.z), T(zaxis.z), T(0),
        T(0),       T(0),       T(0),       T(1));

    // Compute the new position by multiplying the inverse position with the rotation matrix
    Matrix<T,4,4> trans(
        T(1),      T(0),      T(0),      T(0),
        T(0),      T(1),      T(0),      T(0),
        T(0),      T(0),      T(1),      T(0),
        T(-position.x), T(-position.y), T(-position.z), T(1));

    *this = trans * rot;
}

template <typename T, Size ROW, Size COL>
inline void Matrix<T,ROW,COL>::lookat(
    const Vector<Float64,3>& position,
    const Vector<Float64,3>& target,
    const Vector<Float64,3>& up)
{
    mi_static_assert( COL == 4 && ROW == 4);
    Vector<Float64,3> xaxis, yaxis, zaxis;

    // Z vector
    zaxis = position - target;
    zaxis.normalize();

    // X vector = up cross Z
    xaxis = cross( up, zaxis);
    xaxis.normalize();

    // Recompute Y = Z cross X
    yaxis = cross( zaxis, xaxis);
    yaxis.normalize();

    // Build rotation matrix
    Matrix<T,4,4> rot(
        T(xaxis.x), T(yaxis.x), T(zaxis.x), T(0),
        T(xaxis.y), T(yaxis.y), T(zaxis.y), T(0),
        T(xaxis.z), T(yaxis.z), T(zaxis.z), T(0),
        T(0),       T(0),       T(0),       T(1));

    // Compute the new position by multiplying the inverse position with the rotation matrix
    Matrix<T,4,4> trans(
        T(1),      T(0),      T(0),      T(0),
        T(0),      T(1),      T(0),      T(0),
        T(0),      T(0),      T(1),      T(0),
        T(-position.x), T(-position.y), T(-position.z), T(1));

    *this = trans * rot;
}


//------ Definition and helper for matrix inversion ---------------------------

// Matrix inversion class, specialized for symmetric matrices and low dimensions
template <class T, Size ROW, Size COL>
class Matrix_inverter
{
public:
    typedef math::Matrix<T,ROW,COL> Matrix;

    // Inverts the matrix \c M if possible and returns \c true.
    //
    // If the matrix cannot be inverted or if \c ROW != \c COL, this function returns \c false.
    static inline bool invert( Matrix& /* M */) { return false; }
};

// Matrix inversion class, specialization for symmetric matrices
template <class T, Size DIM>
class Matrix_inverter<T,DIM,DIM>
{
public:
    typedef math::Matrix<T,DIM,DIM> Matrix;
    typedef math::Vector<T,DIM>     Vector;
    typedef math::Vector<Size,DIM>  Index_vector;

    // LU decomposition of matrix lu in place.
    //
    // Returns \c false if matrix cannot be decomposed, for example, if it is not invertible, and \c
    // true otherwise. Returns also a row permutation in indx.
    static bool lu_decomposition(
        Matrix&       lu,         // matrix to decompose, modified in place
        Index_vector& indx);      // row permutation info indx[4]

    // Solves the equation lu * x = b for x.  lu and indx are the results of lu_decomposition. The
    // solution is returned in b.
    static void lu_backsubstitution(
        const Matrix&       lu,   // LU decomposed matrix
        const Index_vector& indx, // permutation vector
        Vector&             b);   // right hand side vector b, modified in place

    static bool invert( Matrix& mat);
};

template <class T, Size DIM>
bool  Matrix_inverter<T,DIM,DIM>::lu_decomposition(
    Matrix&       lu,
    Index_vector& indx)
{
    Vector              vv;             // implicit scaling of each row

    for( Size i = 0; i < DIM; i++) {    // get implicit scaling
        T big = T(0);
        for( Size j = 0; j < DIM; j++) {
            T temp = abs(lu.get(i,j));
            if( temp > big)
                big = temp;
        }
        if( big == T(0))
            return false;
        vv[i] = T(1) / big;             // save scaling
    }
    //
    // loop over columns of Crout's method
    //
    Size imax = 0;
    for( Size j = 0; j < DIM; j++) {
        for( Size i = 0; i < j; i++) {
            T sum = lu.get(i,j);
            for( Size k = 0; k < i; k++)
                sum -= lu.get(i,k) * lu.get(k,j);
            lu.set(i, j, sum);
        }
        T big = 0;                      // init for pivot search
        for( Size i = j; i < DIM; i++) {
            T sum = lu.get(i,j);
            for( Size k = 0; k < j; k++)
                sum -= lu.get(i,k) * lu.get(k,j);
            lu.set(i, j, sum);
            T dum = vv[i] * abs(sum);
            if( dum >= big) {           // pivot good?
                big = dum;
                imax = i;
            }
        }
        if( j != imax) {                // interchange rows
            for( Size k = 0; k < DIM; k++) {
                T dum = lu.get(imax,k);
                lu.set(imax, k, lu.get(j,k));
                lu.set(j, k, dum);
            }
            vv[imax] = vv[j];           // interchange scale factor
        }
        indx[j] = imax;
        if( lu.get(j,j) == 0)
            return false;
        if( j != DIM-1) {               // divide by pivot element
            T dum = T(1) / lu.get(j,j);
            for( Size i = j + 1; i < DIM; i++)
                lu.set(i, j, lu.get(i,j) * dum);
        }
    }
    return true;
}

template <class T, Size DIM>
void Matrix_inverter<T,DIM,DIM>::lu_backsubstitution(
    const Matrix&       lu,
    const Index_vector& indx,
    Vector&             b)
{
    // when ii != DIM+1, ii is index of first non-vanishing element of b
    Size ii = DIM+1;
    for( Size i = 0; i < DIM; i++) {
        Size ip = indx[i];
        T sum   = b[ip];
        b[ip]   = b[i];
        if( ii != DIM+1) {
            for( Size j = ii; j < i; j++) {
                sum -= lu.get(i,j) * b[j];
            }
        } else {
            if( sum != T(0)) {          // non-zero element
                ii = i;
            }
        }
        b[i] = sum;
    }
    for( Size i2 = DIM; i2 > 0;) {      // backsubstitution
        --i2;
        T sum = b[i2];
        for( Size j = i2+1; j < DIM; j++)
            sum -= lu.get(i2,j) * b[j];
        b[i2] = sum / lu.get(i2,i2);    // store comp. of sol. vector
    }
}

template <class T, Size DIM>
bool  Matrix_inverter<T,DIM,DIM>::invert( Matrix& mat)
{
    Matrix lu(mat);     // working copy of matrix to invert
    Index_vector indx;  // row permutation info

    // LU decompose, return if it fails
    if( !lu_decomposition(lu, indx))
        return false;

    // solve for each column vector of the I matrix
    for( Size j = 0; j < DIM; ++j) {
        Vector col(T(0)); // TODO: do that directly in the mat matrix
        col[j] = T(1);
        lu_backsubstitution( lu, indx, col);
        for( Size i = 0; i < DIM; ++i) {
            mat.set( i, j, col[i]);
        }
    }
    return true;
}

// Matrix inversion class, specialization for 1x1 matrix
template <class T>
class Matrix_inverter<T,1,1>
{
public:
    typedef math::Matrix<T,1,1>     Matrix;

    static inline bool invert( Matrix& mat)
    {
        T s = mat.get( 0, 0);
        if( s == T(0))
            return false;
        mat.set( 0, 0, T(1) / s);
        return true;
    }
};

// Matrix inversion class, specialization for 2x2 matrix
template <class T>
class Matrix_inverter<T,2,2>
{
public:
    typedef math::Matrix<T,2,2>        Matrix;

    static inline bool invert( Matrix& mat)
    {
        T a = mat.get( 0, 0);
        T b = mat.get( 0, 1);
        T c = mat.get( 1, 0);
        T d = mat.get( 1, 1);
        T det = a*d - b*c;
        if( det == T( 0))
            return false;
        T rdet = T(1) / det;
        mat.set( 0, 0, d * rdet);
        mat.set( 0, 1,-b * rdet);
        mat.set( 1, 0,-c * rdet);
        mat.set( 1, 1, a * rdet);
        return true;
    }
};

template <typename T, Size ROW, Size COL>
inline bool Matrix<T,ROW,COL>::invert()
{
    return Matrix_inverter<T,ROW,COL>::invert( *this);
}



//------ Specializations and (specialized) overloads --------------------------

// Specialization of common matrix multiplication for 4x4 matrices.
template <typename T>
inline Matrix<T,4,4>& operator*=(
    Matrix<T,4,4>&       lhs,
    const Matrix<T,4,4>& rhs)
{
    Matrix<T,4,4> old( lhs);

    lhs.xx = old.xx * rhs.xx + old.xy * rhs.yx + old.xz * rhs.zx + old.xw * rhs.wx;
    lhs.xy = old.xx * rhs.xy + old.xy * rhs.yy + old.xz * rhs.zy + old.xw * rhs.wy;
    lhs.xz = old.xx * rhs.xz + old.xy * rhs.yz + old.xz * rhs.zz + old.xw * rhs.wz;
    lhs.xw = old.xx * rhs.xw + old.xy * rhs.yw + old.xz * rhs.zw + old.xw * rhs.ww;

    lhs.yx = old.yx * rhs.xx + old.yy * rhs.yx + old.yz * rhs.zx + old.yw * rhs.wx;
    lhs.yy = old.yx * rhs.xy + old.yy * rhs.yy + old.yz * rhs.zy + old.yw * rhs.wy;
    lhs.yz = old.yx * rhs.xz + old.yy * rhs.yz + old.yz * rhs.zz + old.yw * rhs.wz;
    lhs.yw = old.yx * rhs.xw + old.yy * rhs.yw + old.yz * rhs.zw + old.yw * rhs.ww;

    lhs.zx = old.zx * rhs.xx + old.zy * rhs.yx + old.zz * rhs.zx + old.zw * rhs.wx;
    lhs.zy = old.zx * rhs.xy + old.zy * rhs.yy + old.zz * rhs.zy + old.zw * rhs.wy;
    lhs.zz = old.zx * rhs.xz + old.zy * rhs.yz + old.zz * rhs.zz + old.zw * rhs.wz;
    lhs.zw = old.zx * rhs.xw + old.zy * rhs.yw + old.zz * rhs.zw + old.zw * rhs.ww;

    lhs.wx = old.wx * rhs.xx + old.wy * rhs.yx + old.wz * rhs.zx + old.ww * rhs.wx;
    lhs.wy = old.wx * rhs.xy + old.wy * rhs.yy + old.wz * rhs.zy + old.ww * rhs.wy;
    lhs.wz = old.wx * rhs.xz + old.wy * rhs.yz + old.wz * rhs.zz + old.ww * rhs.wz;
    lhs.ww = old.wx * rhs.xw + old.wy * rhs.yw + old.wz * rhs.zw + old.ww * rhs.ww;

    return lhs;
}

// Specialization of common matrix multiplication for 4x4 matrices.
template <typename T>
inline Matrix<T,4,4> operator*(
    const Matrix<T,4,4>& lhs,
    const Matrix<T,4,4>& rhs)
{
    Matrix<T,4,4> temp( lhs);
    temp *= rhs;
    return temp;
}

#endif // MI_FOR_DOXYGEN_ONLY

/*@}*/ // end group mi_math_matrix

} // namespace math

} // namespace mi

#endif // MI_MATH_MATRIX_H
