/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/bbox.h
/// \brief An axis-aligned N-dimensional bounding box class template of fixed dimension with
///        supporting functions.
///
/// See \ref mi_math_bbox.

#ifndef MI_MATH_BBOX_H
#define MI_MATH_BBOX_H

#include <mi/base/types.h>
#include <mi/math/assert.h>
#include <mi/math/function.h>
#include <mi/math/vector.h>
#include <mi/math/matrix.h>

namespace mi {

namespace math {

/** \defgroup mi_math_bbox Bounding Box Class
    \ingroup mi_math

    An axis-aligned N-dimensional bounding box class template of fixed dimension with supporting
    functions.

    \par Include File:
    <tt> \#include <mi/math/bbox.h></tt>

    @{
*/

//------- POD struct that provides storage for bbox elements ----------

/** Storage class for an axis-aligned N-dimensional bounding box class template of fixed dimension.

    Use the #mi::math::Bbox template in your programs and this storage class only if you need a POD
    type, for example for parameter passing.

    A bounding box is represented by two #mi::math::Vector_struct vectors representing the
    elementwise minimal box corner, \c min, and the elementwise largest box corner, \c max.
*/
template <typename T, Size DIM>
struct Bbox_struct
{
    Vector_struct<T,DIM> min;  ///< Elementwise minimal bounding box corner
    Vector_struct<T,DIM> max;  ///< Elementwise maximal bounding box corner
};


/** Axis-aligned N-dimensional bounding box class template of fixed dimension.

    A bounding box is represented by two #mi::math::Vector vectors representing the elementwise
    minimal box corner, \c min, and the elementwise largest box corner, \c max.

    An instantiation of the bounding box class template is a model of the STL container concept. It
    provides random access to its two vectors and corresponding random access iterators.

    The template parameters have the following requirements:
      - \b T: an arithmetic type supporting <tt>+ - * / == != < > <= >= </tt>.
      - \b DIM: a value > 0 of type #mi::Size that defines the fixed dimension of the vectors used
                to represent the bounding box.

   \see
        The underlying POD type #mi::math::Bbox_struct.

   \see
        For the free functions and operators available for bounding boxes see \ref mi_math_bbox.
*/
template <typename T, Size DIM>
class Bbox
{
public:
    typedef math::Vector<T,DIM> Vector;         ///< Corresponding vector type.
    typedef Bbox_struct<T,DIM>  Pod_type;       ///< POD class corresponding to this bounding box.
    typedef Vector         value_type;          ///< Coordinate type.
    typedef Size           size_type;           ///< Size type, unsigned.
    typedef Difference     difference_type;     ///< Difference type, signed.
    typedef Vector *       pointer;             ///< Mutable pointer to vector.
    typedef const Vector * const_pointer;       ///< Const pointer to vector.
    typedef Vector &       reference;           ///< Mutable reference to vector.
    typedef const Vector & const_reference;     ///< Const reference to vector.

    static const Size DIMENSION = DIM;          ///< Constant dimension of the vectors.
    static const Size SIZE      = 2;            ///< Constant size of the bounding box.

    /// Constant size of the bounding box.
    static inline Size size()     { return SIZE; }

    /// Constant maximum size of the bounding box.
    static inline Size max_size() { return SIZE; }

    /// Enum type used to tag a special constructor that does not initialize the
    /// elements of the constructed bounding box.
    enum Uninitialized_tag {
        /// Enum value used to call a special constructor that does not initialize
        /// the elements of the constructed bounding box.
        UNINITIALIZED_TAG
    };

    Vector min;  ///< Elementwise minimal bounding box corner
    Vector max;  ///< Elementwise maximal bounding box corner

    /// Reinitializes this bounding box to the empty space.
    /// The vector \c min is set elementwise to #mi::base::numeric_traits<T>::max()
    /// and the vector \c max is set elementwise to
    /// #mi::base::numeric_traits<T>::negative_max(). This initialization allows to
    /// insert points and other bounding boxes; a cleared bounding box will
    /// take the value of the first inserted point or bound box.
    inline void clear()
    {
        for( Size i = 0; i < DIM; i++) {
            min[i] =  base::numeric_traits<T>::max MI_PREVENT_MACRO_EXPAND ();
            max[i] =  base::numeric_traits<T>::negative_max();
        }
    }

    /// Bounding box initialized to the empty space, see also the clear function.
    /// The vector \c min is set elementwise to #mi::base::numeric_traits<T>::max()
    /// and the vector \c max is set elementwise to
    /// #mi::base::numeric_traits<T>::negative_max(). This initialization allows to
    /// insert points and other bounding boxes; a cleared bounding box will
    /// take the value of the first inserted point or bound box.
    inline Bbox() { clear(); }

    /// Bounding box with its elements not initialized.
    inline explicit Bbox( Uninitialized_tag) { }

    /// Bounding box initialized from corresponding POD type.
    inline Bbox( const Bbox_struct<T,DIM>& bbox_struct )
    {
        min = bbox_struct.min;
        max = bbox_struct.max;
    }

    /// Bounding box initialized to a single \c point.
    inline explicit Bbox(
        const Vector& point) : min(point), max(point) ///< point.
    {
    }

    /// Bounding box initialized to the new extreme corner vectors, \c nmin and \c nmax.
    inline Bbox(
        const Vector& nmin,     ///< \c min corner vector
        const Vector& nmax)     ///< \c max corner vector
  : min(nmin), max(nmax)
    {
    }

    /// 1D bounding box (interval) initialized to the new extreme corner vectors, \c (min_x) and
    /// \c (max_x).
    ///
    /// \pre <tt>DIM == 1</tt>
    inline Bbox(
        T min_x,                ///< x-coordinate of \c min corner vector
        T max_x)                ///< x coordinate of \c max corner vector
    {
        mi_static_assert(DIM == 1);
        min = Vector( min_x);
        max = Vector( max_x);
    }

    /// 2D bounding box (interval) initialized to the new extreme corner vectors, \c (min_x,min_y)
    /// and \c (max_x,max_y).
    ///
    /// \pre <tt>DIM == 2</tt>
    inline Bbox(
        T min_x,                ///< x-coordinate of \c min corner vector
        T min_y,                ///< y-coordinate of \c min corner vector
        T max_x,                ///< x coordinate of \c max corner vector
        T max_y)                ///< y coordinate of \c max corner vector
    {
        mi_static_assert(DIM == 2);
        min = Vector( min_x, min_y);
        max = Vector( max_x, max_y);
    }

    /// 3D bounding box (interval) initialized to the new extreme corner vectors,
    /// \c (min_x,min_y,min_z) and \c (max_x,max_y,max_z).
    ///
    /// \pre <tt>DIM == 3</tt>
    inline Bbox(
        T min_x,                ///< x-coordinate of \c min corner vector
        T min_y,                ///< y-coordinate of \c min corner vector
        T min_z,                ///< z-coordinate of \c min corner vector
        T max_x,                ///< x coordinate of \c max corner vector
        T max_y,                ///< y coordinate of \c max corner vector
        T max_z)                ///< z coordinate of \c max corner vector
    {
        mi_static_assert(DIM == 3);
        min = Vector( min_x, min_y, min_z);
        max = Vector( max_x, max_y, max_z);
    }

    /// Constructs a bounding box from a range [\p first, \p last) of items.
    ///
    /// The value type of \c InputIterator can be either #Vector to insert points, or it can be
    /// #Bbox to insert bounding boxes.
    ///
    /// \param first   first element of the sequence to insert
    /// \param last    past-the-end position of sequence to insert
    template <typename InputIterator>
    Bbox(
        InputIterator first,
        InputIterator last);

    /// Template constructor that allows explicit conversions from other bounding boxes with
    /// assignment compatible element value type.
    template <typename T2>
    inline explicit Bbox( const Bbox<T2,DIM>& other)
    {
        min = Vector( other.min);
        max = Vector( other.max);
    }

    /// Template constructor that allows explicit conversions from other POD type with
    /// assignment compatible element value type.
    template <typename T2>
    inline explicit Bbox( const Bbox_struct<T2,DIM>& other)
    {
        min = Vector( other.min);
        max = Vector( other.max);
    }

    /// Assignment.
    inline Bbox& operator=( const Bbox& other)
    {
        min = other.min;
        max = other.max;
        return *this;
    }

    /// Assignment from corresponding POD type.
    inline Bbox& operator=( const Bbox_struct<T,DIM>& other)
    {
        min = other.min;
        max = other.max;
        return *this;
    }

    /// Conversion to corresponding POD type.
    inline operator Bbox_struct<T,DIM>() const
    {
        Bbox_struct<T,DIM> result;
        result.min = min;
        result.max = max;
        return result;
    }

    /// Returns the pointer to the first vector, \c min.
    inline Vector* begin() { return &min; }

    /// Returns the pointer to the first vector, \c min.
    inline const Vector* begin() const { return &min; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range [\c min,\c max].
    inline Vector* end() { return begin() + SIZE; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range [\c min,\c max].
    inline const Vector* end() const { return begin() + SIZE; }

    /// Returns the vector \c min for \c i==0, and the vector \c max for \c i==1.
    inline Vector& operator[]( Size i)
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        return begin()[i];
    }

    /// Returns the vector \c min for \c i==0, and the vector \c max for \c i==1.
    inline const Vector& operator[]( Size i) const
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        return begin()[i];
    }

    /// Returns \c true if the box is empty.
    ///
    /// For example, the box is empty after the default constructor or the #clear() method call.
    inline bool empty() const
    {
        for( Size i = 0; i < DIM; i++) {
            if( min[i] !=  base::numeric_traits<T>::max MI_PREVENT_MACRO_EXPAND())
                return false;
            if( max[i] !=  base::numeric_traits<T>::negative_max())
                return false;
        }
        return true;
    }

    /// Returns the rank of the bounding box.
    ///
    /// \return   0 if the bounding box is a point or empty,
    ///           1 if it is an axis-aligned line,
    ///           2 if it is an axis-aligned plane, and
    ///           3 if it has a volume.
    inline Size rank() const
    {
        Size rank = 0;
        for( Size i = 0; i < DIM; i++)
            rank += (min[i] < max[i]);
        return rank;
    }

    /// Returns \c true the bounding box is a single point.
    inline bool is_point() const { return min == max; }

    /// Returns \c true the bounding box is an axis-aligned line.
    ///
    /// \return \c true if #rank() returns 1
    inline bool is_line() const { return rank() == 1u; }

    /// Returns \c true the bounding box is an axis-aligned plane.
    ///
    /// \return \c true if #rank() returns 2
    inline bool is_plane() const { return rank() == 2u; }

    /// Returns \c true the bounding box has a volume.
    ///
    /// \return \c true if #rank() returns 3
    inline bool is_volume() const { return rank() == 3u; }

    /// Returns \c true if the point is inside or on the boundary of the bounding box.
    inline bool contains( const Vector& vec) const
    {
        for( Size i = 0; i < DIM; i++) {
            if( vec[i] < min[i] || vec[i] > max[i])
                return false;
        }
        return true;
    }

    /// Returns \c true if this bounding box and the \c other bounding box intersect in their
    /// interiors or on their boundaries.
    inline bool intersects( const Bbox& other) const
    {
        for( Size i = 0; i < DIM; i++) {
            if( min[i] > (other.max)[i] || max[i] < (other.min)[i])
                return false;
        }
        return true;
    }

    /// Assigns the union of this bounding box and the \c other bounding box to this bounding box.
    inline void insert( const Bbox& other)
    {
        min = elementwise_min( min, (other.min));
        max = elementwise_max( max, (other.max));
    }

    /// Assigns the union of this bounding box and the \c point to this bounding box.
    inline void insert( const Vector& point)
    {
        min = elementwise_min( min, point);
        max = elementwise_max( max, point);
    }

    /// Inserts a range [\p first,\p last) of items into this bounding box.
    ///
    /// The value type of \c InputIterator can be either #Vector to insert points, or it can be
    /// #Bbox to insert bounding boxes.
    ///
    /// \param first   first element of the sequence to insert
    /// \param last    past-the-end position of sequence to insert
    template <typename InputIterator>
    void insert(
        InputIterator first,
        InputIterator last);


    /// Returns the translation of this bounding box by vectors that are inside the scaled bounding
    /// box of vectors, i.e., \c t*vbox.
    ///
    /// \pre this bounding box and \c vbox are not empty
    ///
    /// \param vbox   vector bounding box to add
    /// \param t      scale parameter. A negative scale inverts \c vbox.
    inline Bbox add_motionbox(
        const Bbox& vbox,
        T           t) const
    {
        mi_math_assert_msg( ! empty(),      "precondition");
        mi_math_assert_msg( ! vbox.empty(), "precondition");
        return t < 0 ? Bbox(min + (vbox.max) * t, max + (vbox.min) * t) //-V547 PVS
                     : Bbox(min + (vbox.min) * t, max + (vbox.max) * t);
    }

    /// Assigns the union of this bounding box and the \c other bounding box to this bounding box.
    ///
    /// Makes the bounding box compatible with the \c std::back_inserter function, which allows you
    /// to use STL functions, such as \c std::copy to compute the union of a sequence of bounding
    /// boxes.
    inline void push_back( const Bbox& other)
    {
        return insert( other);
    }

    /// Robustly grows the bounding box by a value computed automatically from the bounding box
    /// dimensions and location in space.
    ///
    /// If a bounding box is far away from the origin, just enlarging the bounding box by
    /// \p eps * (largest box extent) may result in cancellation. To avoid cancellation problems,
    /// this method computes the value for enlarging the box by computing coordinatewise the sum of
    /// the absolute values of the min and max coordinates and the bounding box extent. It takes
    /// then the maximum of all these sums, multiplies it by \p eps, adds it to \c bbox.max and
    /// subtracts it from \c bbox.min, enlarging the bounding box by an equal amount on all sides.
    void robust_grow( T eps = T(1.0e-5f)); ///< grow factor

    /// Returns the volume of the bounding box.
    inline T volume() const
    {
        Vector diag = max - min;
        T vol = base::max MI_PREVENT_MACRO_EXPAND ( T(0), diag[0]);
        for( Size i = 1; i < DIM; i++)
            vol *= base::max MI_PREVENT_MACRO_EXPAND ( T(0), diag[i]);
        return vol;
    }

    /// Returns the length of the diagonal.
    ///
    /// \pre the bounding box is not empty
    inline T diagonal_length() const
    {
        mi_math_assert_msg( !empty(), "precondition");
        Vector diag = max - min;
        return length( diag);
    }

    /// Returns the index of the dimension in which the bounding box has its largest extent, i.e.,
    /// 0=x, 1=y, 2=z.
    inline Size largest_extent_index() const
    {
        Vector diag = max - min;
        T maxval = diag[0];
        Size  maxidx = 0;
        for( Size i = 1; i < DIM; i++) {
            if (maxval < diag[i]) {
                maxval = diag[i];
                maxidx = i;
            }
        }
        return maxidx;
    }

    /// Returns the center point of the bounding box.
    inline Vector center() const { return Vector((max + min) * 0.5);}

    /// Returns the size of the bounding box.
    inline Vector extent() const { return max - min; }

};

//------ Operator declarations for Bbox ---------------------------------------

/// Returns a bounding box that is the \p bbox increased by a constant \p value at each face, i.e.,
/// \p value is added to \c bbox.max and subtracted from \c bbox.min.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>  operator+( const Bbox<T,DIM>& bbox, T value)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    Bbox<T,DIM> result( Bbox<T,DIM>::UNINITIALIZED_TAG);
    for( Size i = 0; i < DIM; i++) {
        (result.min)[i] = (bbox.min)[i] - value;
        (result.max)[i] = (bbox.max)[i] + value;
    }
    return result;
}

/// Returns a bounding box that is the \c bbox shrunk by a constant \c value at each face, i.e.,
/// \p value is subtracted from \c bbox.max and added to \c bbox.min.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>  operator-( const Bbox<T,DIM>& bbox, T value)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    Bbox<T,DIM> result( Bbox<T,DIM>::UNINITIALIZED_TAG);
    for( Size i = 0; i < DIM; i++) {
        (result.min)[i] = (bbox.min)[i] + value;
        (result.max)[i] = (bbox.max)[i] - value;
    }
    return result;
}

/// Returns a bounding box that is a version of \p bbox scaled by \p factor, i.e., \c bbox.max and
/// \c bbox.min are multiplied by \p factor.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>  operator*( const Bbox<T,DIM>& bbox, T factor)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    Bbox<T,DIM> result( Bbox<T,DIM>::UNINITIALIZED_TAG);
    for( Size i = 0; i < DIM; i++) {
        (result.min)[i] = (bbox.min)[i] * factor;
        (result.max)[i] = (bbox.max)[i] * factor;
    }
    return result;
}

/// Returns a bounding box that is a version of \p bbox divided by \p divisor, i.e., \c bbox.max and
/// \c bbox.min are divided by \p divisor.
///
/// \pre \c bbox is not empty and \p divisor is not zero
template <typename T, Size DIM>
inline Bbox<T,DIM>  operator/( const Bbox<T,DIM>& bbox, T divisor)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    mi_math_assert_msg( divisor != T(0), "precondition");
    Bbox<T,DIM> result( Bbox<T,DIM>::UNINITIALIZED_TAG);
    for( Size i = 0; i < DIM; i++) {
        (result.min)[i] = (bbox.min)[i] / divisor;
        (result.max)[i] = (bbox.max)[i] / divisor;
    }
    return result;
}

/// Increases \p bbox by a constant \p value at each face, i.e., \p value is added to \c bbox.max
/// and subtracted from \c bbox.min.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>& operator+=( Bbox<T,DIM>& bbox, T value)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    for( Size i = 0; i < DIM; i++) {
        (bbox.min)[i] -= value;
        (bbox.max)[i] += value;
    }
    return bbox;
}

/// Shrinks \c bbox by a constant \c value at each face, i.e., \p value is subtracted from
/// \c bbox.max and added to \c bbox.min.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>& operator-=( Bbox<T,DIM>& bbox, T value)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    for( Size i = 0; i < DIM; i++) {
        (bbox.min)[i] += value;
        (bbox.max)[i] -= value;
    }
    return bbox;
}

/// Scales \p bbox by \p factor, i.e., \c bbox.max and \c bbox.min are multiplied by \p factor.
///
/// \pre \c bbox is not empty
template <typename T, Size DIM>
inline Bbox<T,DIM>& operator*=( Bbox<T,DIM>& bbox, T factor)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    for( Size i = 0; i < DIM; i++) {
        (bbox.min)[i] *= factor;
        (bbox.max)[i] *= factor;
    }
    return bbox;
}

/// Divide \p bbox by \p divisor, i.e., \c bbox.max and \c bbox.min are divided by \p divisor.
///
/// \pre \c bbox is not empty and \p divisor is not zero
template <typename T, Size DIM>
inline Bbox<T,DIM>& operator/=( Bbox<T,DIM>& bbox, T divisor)
{
    mi_math_assert_msg( !bbox.empty(), "precondition");
    mi_math_assert_msg( divisor != T(0), "precondition");
    for( Size i = 0; i < DIM; i++) {
        (bbox.min)[i] /= divisor;
        (bbox.max)[i] /= divisor;
    }
    return bbox;
}

/// Returns \c true if \c lhs is elementwise equal to \c rhs.
template <typename T, Size DIM>
inline bool  operator==(const Bbox<T,DIM>& lhs, const Bbox<T,DIM>& rhs)
{
    return (lhs.min) == (rhs.min) && (lhs.max) == (rhs.max);
}

/// Returns \c true if \c lhs is elementwise not equal to \c rhs.
template <typename T, Size DIM>
inline bool  operator!=(const Bbox<T,DIM>& lhs, const Bbox<T,DIM>& rhs)
{
    return (lhs.min) != (rhs.min) || (lhs.max) != (rhs.max);
}

/// Returns \c true if \c lhs is lexicographically less than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size DIM>
inline bool operator< ( const Bbox<T,DIM> & lhs, const Bbox<T,DIM> & rhs)
{
    for( Size i(0u); i < DIM; ++i) {
        if( (lhs.min)[i] < (rhs.min)[i])
            return true;
        if( (lhs.min)[i] > (rhs.min)[i])
            return false;
    }
    return lexicographically_less( lhs.max, rhs.max);
}

/// Returns \c true if \c lhs is lexicographically less than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size DIM>
inline bool operator<=( const Bbox<T,DIM>& lhs, const Bbox<T,DIM>& rhs)
{
    for( Size i(0u); i < DIM; ++i) {
        if( (lhs.min)[i] < (rhs.min)[i])
            return true;
        if( (lhs.min)[i] > (rhs.min)[i])
            return false;
    }
    return lexicographically_less_or_equal( lhs.max, rhs.max);
}

/// Returns \c true if \c lhs is lexicographically greater than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size DIM>
inline bool operator>( const Bbox<T,DIM> & lhs, const Bbox<T,DIM> & rhs)
{
    for( Size i(0u); i < DIM; ++i) {
        if( (lhs.min)[i] > (rhs.min)[i])
            return true;
        if( (lhs.min)[i] < (rhs.min)[i])
            return false;
    }
    return lexicographically_greater( lhs.max, rhs.max);
}

/// Returns \c true if \c lhs is lexicographically greater than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
template <typename T, Size DIM>
inline bool operator>=( const Bbox<T,DIM>& lhs, const Bbox<T,DIM>& rhs)
{
    for( Size i(0u); i < DIM; ++i) {
        if( (lhs.min)[i] > (rhs.min)[i])
            return true;
        if( (lhs.min)[i] < (rhs.min)[i])
            return false;
    }
    return lexicographically_greater_or_equal( lhs.max, rhs.max);
}


//------ Free Functions for Bbox ----------------------------------------------


/// Returns the linear interpolation between \p bbox1 and \c bbox2, i.e., it returns
/// <tt>(1-t) * bbox1 + t * bbox2</tt>.
///
/// \pre \p bbox1 and \p bbox2 are not empty.
template <typename T, Size DIM>
inline Bbox<T,DIM> lerp(
    const Bbox<T,DIM> &bbox1,  ///< one bounding box
    const Bbox<T,DIM> &bbox2,  ///< second bounding box
    T          t)              ///< interpolation parameter in [0,1]
{
    mi_math_assert_msg( !bbox1.empty(), "precondition");
    mi_math_assert_msg( !bbox2.empty(), "precondition");
    T t2 = T(1) - t;
    return Bbox<T,DIM>( (bbox1.min)*t2 + (bbox2.min)*t, (bbox1.max)*t2 + (bbox2.max)*t);
}


/// Clip \c bbox1 at \c bbox2 and return the result. I.e., the resulting bbox is the intersection of
/// \c bbox1 with \c bbox2.
template <typename T, Size DIM>
inline Bbox<T,DIM> clip(
    const Bbox<T,DIM> &bbox1,   ///< first bounding box
    const Bbox<T,DIM> &bbox2)   ///< second bounding box
{
    Bbox<T,DIM> result( Bbox<T,DIM>::UNINITIALIZED_TAG);
    for( Size i = 0; i < DIM; i++) {
        result.min[i] = base::max MI_PREVENT_MACRO_EXPAND (bbox1.min[i], bbox2.min[i]);
        result.max[i] = base::min MI_PREVENT_MACRO_EXPAND (bbox1.max[i], bbox2.max[i]);
        if (result.max[i] < result.min[i]) { // the bbox is empty
            result.clear();
            return result;
        }
    }

    return result;
}

/// Returns the 3D bounding box transformed by a matrix.
///
/// The transformation (including the translation) is applied to the eight bounding box corners
/// (interpreted as points) and a new axis aligned bounding box is computed for these transformed
/// corners.
///
/// \note The transformed bounding box is likely to be a more pessimistic approximation of a
/// geometry that was approximated by the original bounding box. Transforming the approximated
/// geometry and computing a new bounding box gives usually a tighter bounding box.
///
/// \param mat    4x4 transformation matrix
/// \param bbox   the bounding box to transform
template <typename TT, typename T>
Bbox<T,3> transform_point( const Matrix<TT,4,4>& mat, const Bbox<T,3>& bbox);


/// Returns the 3D bounding box transformed by a matrix.
///
/// The transformation (excluding the translation) is applied to the eight bounding box corners
/// (interpreted as vectors) and a new axis aligned bounding box is computed for these transformed
/// corners.
///
/// \param mat    4x4 transformation matrix
/// \param bbox   the bounding box to transform
template <typename TT, typename T>
Bbox<T,3> transform_vector( const Matrix<TT,4,4>& mat, const Bbox<T,3>& bbox);



//------ Definitions of non-inline function -----------------------------------

#ifndef MI_FOR_DOXYGEN_ONLY

template <typename T, Size DIM>
template <typename InputIterator>
void Bbox<T,DIM>::insert( InputIterator first, InputIterator last)
{
    for( ; first != last; ++first)
        insert( *first);
}

template <typename T, Size DIM>
template <typename InputIterator>
Bbox<T,DIM>::Bbox( InputIterator first, InputIterator last)
{
    clear();
    insert( first, last);
}

template <typename T, Size DIM>
void Bbox<T, DIM>::robust_grow( T eps)
{
    // Just enlarging the bounding box by epsilon * (largest box extent) is not sufficient, since
    // there may be cancellation errors if the box is far away from the origin. Hence we take into
    // account the distance of the box from the origin: the further the box is away, the larger we
    // have to make it. We just add the two contributions. If we are very far away, then distance
    // will dominate. For very large boxes, the extent will dominate. We neglect exact weight
    // factors. We are just a bit less generous with the epsilon to compensate for the extra stuff
    // we added. If we have objects that in some direction are several orders of magnitude larger
    // than in the others, this method will not be perfect.
    //
    // compute size heuristic for each dimension
    Vector dist;
    for( Size i = 0; i < DIM; i++)
        dist[i] = base::abs(min[i]) + base::abs(max[i])
                + base::abs(max[i] - min[i]);
    // compute the grow factor
    T max_dist = T(0);
    for( Size i = 0; i < DIM; i++)
        max_dist = base::max MI_PREVENT_MACRO_EXPAND (max_dist, dist[i]);
    const T margin = max_dist * eps;
    // grow the bounding box
    *this += margin;
}

#endif // MI_FOR_DOXYGEN_ONLY

template <typename TT, typename T>
Bbox<T,3> transform_point( const Matrix<TT,4,4>& mat, const Bbox<T,3>& bbox)
{
    if( bbox.empty())
        return bbox;

    // Transforms all eight corner points, and finds then the bounding box of these eight points.
    // The corners are computed in an optimized manner which factorizes computations.
    //
    // The transformation is decomposed into the transformation of (min,1) and the transformation of
    // bounding box difference vectors (max.x-min.x,0,0,0),(0, max.y-min.y,0,0),(0,0,max.z-min.z,0).
    // The transformation of max is the sum of all 4 transformed vectors. The division by the
    // homogeneous w is deferred to the end.
    Vector<T,3> corners[8];
    corners[0] = transform_vector( mat, bbox.min);
    corners[0].x += T(mat.wx);
    corners[0].y += T(mat.wy);
    corners[0].z += T(mat.wz);

    // difference vectors between min and max
    Vector<T,3> dx = transform_vector( mat, Vector<T,3>( (bbox.max).x - (bbox.min).x, 0, 0));
    Vector<T,3> dy = transform_vector( mat, Vector<T,3>( 0, (bbox.max).y - (bbox.min).y, 0));
    Vector<T,3> dz = transform_vector( mat, Vector<T,3>( 0, 0, (bbox.max).z - (bbox.min).z));

    corners[1] = corners[0] + dz;                       // vertex 001
    corners[2] = corners[0] + dy;                       // vertex 010
    corners[3] = corners[0] + dy + dz;                  // vertex 011
    corners[4] = corners[0] + dx;                       // vertex 100
    corners[5] = corners[0] + dx + dz;                  // vertex 101
    corners[6] = corners[0] + dx + dy;                  // vertex 110
    corners[7] = corners[0] + dx + dy + dz;             // vertex 110

    // compute the w-coordinates separately. This is done to avoid copying from 4D to 3D vectors.
    // Again, the computation is decomposed.
    //
    // w-coordinate for difference vectors between min and max
    T wx = T( mat.xw * ((bbox.max).x - (bbox.min).x));
    T wy = T( mat.yw * ((bbox.max).y - (bbox.min).y));
    T wz = T( mat.zw * ((bbox.max).z - (bbox.min).z));
    // w-coordinate for corners
    T w[8];
    w[0] = T(mat.xw * (bbox.min).x + mat.yw * (bbox.min).y + mat.zw * (bbox.min).z + mat.ww);
    w[1] = w[0] + wz;                                   // vertex 001
    w[2] = w[0] + wy;                                   // vertex 010
    w[3] = w[0] + wy + wz;                              // vertex 011
    w[4] = w[0] + wx;                                   // vertex 100
    w[5] = w[0] + wx + wz;                              // vertex 101
    w[6] = w[0] + wx + wy;                              // vertex 110
    w[7] = w[0] + wx + wy + wz;                         // vertex 111

    // divide the other coordinates (x,y,z) by w to obtain 3D coordinates
    for( unsigned int i=0; i<8; ++i) {
        if( w[i]!=0 && w[i]!=1) {
            T inv = T(1) / w[i];
            corners[i].x *= inv;
            corners[i].y *= inv;
            corners[i].z *= inv;
        }
    }
    Bbox<T,3> result( corners, corners+8);
    return result;
}

template <typename TT, typename T>
Bbox<T,3> transform_vector( const Matrix<TT,4,4>& mat, const Bbox<T,3>& bbox)
{
    // See transform_point() above for implementation notes.
    if( bbox.empty())
        return bbox;

    Vector<T,3> corners[8];
    corners[0] = transform_vector( mat, (bbox.min));

    Vector<T,3> dx = transform_vector( mat, Vector<T,3>( (bbox.max).x - (bbox.min).x, 0, 0));
    Vector<T,3> dy = transform_vector( mat, Vector<T,3>( 0, (bbox.max).y - (bbox.min).y, 0));
    Vector<T,3> dz = transform_vector( mat, Vector<T,3>( 0, 0, (bbox.max).z - (bbox.min).z));

    corners[1] = corners[0] + dz;
    corners[2] = corners[0] + dy;
    corners[3] = corners[0] + dy + dz;
    corners[4] = corners[0] + dx;
    corners[5] = corners[0] + dx + dz;
    corners[6] = corners[0] + dx + dy;
    corners[7] = corners[0] + dx + dy + dz;

    Bbox<T,3> result( corners, corners+8);
    return result;
}

/*@}*/ // end group mi_math_bbox

} // namespace math

} // namespace mi

#endif // MI_MATH_BBOX_H
