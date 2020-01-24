/***************************************************************************************************
 * Copyright 2020 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/spectrum.h
/// \brief %Spectrum class with floating point elements and operations.
///
/// See \ref mi_math_spectrum.

#ifndef MI_MATH_SPECTRUM_H
#define MI_MATH_SPECTRUM_H

#include <mi/base/types.h>
#include <mi/math/assert.h>
#include <mi/math/function.h>
#include <mi/math/vector.h>
#include <mi/math/color.h>

namespace mi {

namespace math {

/** \defgroup mi_math_spectrum Spectrum Class
    \ingroup mi_math

    %Spectrum class with floating point elements and operations.

    \par Include File:
    <tt> \#include <mi/math/spectrum.h></tt>

    @{
*/

//------ Spectrum Class ---------------------------------------------------------

/** %Spectrum with floating point elements and operations.

    \note
    This class will most likely change in the future, since currently it uses only three components.
    Therefore it behaves almost identical to the Color class, apart from the missing alpha
    component. Future implementations will probably contain a lot more bands or might have a
    completely different internal structure.

    The spectrum class is a model of the STL container concept. It provides random access to its
    elements and corresponding random access iterators.

    \see
        For the free functions and operators available for spectra see \ref mi_math_spectrum.

    \par Include File:
    <tt> \#include <mi/math/spectrum.h></tt>
*/
class Spectrum : public Spectrum_struct
{
public:
    typedef Spectrum_struct   Pod_type;         ///< POD class corresponding to this spectrum.
    typedef Spectrum_struct   storage_type;     ///< Storage class used by this spectrum.
    typedef Float32           value_type;       ///< Element type.
    typedef Size              size_type;        ///< Size type, unsigned.
    typedef Difference        difference_type;  ///< Difference type, signed.
    typedef Float32 *         pointer;          ///< Mutable pointer to element.
    typedef const Float32 *   const_pointer;    ///< Const pointer to element.
    typedef Float32 &         reference;        ///< Mutable reference to element.
    typedef const Float32 &   const_reference;  ///< Const reference to element.

    static const Size SIZE      = 3;            ///< Constant size of the spectrum.

     /// Constant size of the spectrum.
    static inline Size size()     { return SIZE; }

     /// Constant maximum size of the spectrum.
    static inline Size max_size() { return SIZE; }

    /// Returns the pointer to the first spectrum element.
    inline Float32*        begin()       { return &c[0]; }

    /// Returns the pointer to the first spectrum element.
    inline const Float32*  begin() const { return &c[0]; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all spectrum elements.
    inline Float32*        end()         { return begin() + SIZE; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all spectrum elements.
    inline const Float32*  end() const   { return begin() + SIZE; }

    /// The default constructor leaves the spectrum elements uninitialized.
    inline Spectrum()
    {
#if defined(DEBUG) || (defined(_MSC_VER) && _MSC_VER <= 1310)
        // In debug mode, default-constructed spectra are initialized with signaling NaNs or, if not
        // applicable, with a maximum value to increase the chances of diagnosing incorrect use of
        // an uninitialized spectrum.
        //
        // When compiling with Visual C++ 7.1 or earlier, this code is enabled in all variants to
        // work around a very obscure compiler bug that causes the compiler to crash.
        typedef mi::base::numeric_traits<Float32> Traits;
        Float32 v = (Traits::has_signaling_NaN)
            ? Traits::signaling_NaN() : Traits::max MI_PREVENT_MACRO_EXPAND ();
        for( Size i = 0; i < SIZE; ++i)
            c[i] = v;
#endif
    }

    /// Constructor from underlying storage type.
    inline Spectrum( const Spectrum_struct& s)
    {
        for( Size i = 0; i < SIZE; ++i)
            c[i] = s.c[i];
    }


    /// Constructor initializes all vector elements to the value \p s.
    inline explicit Spectrum( const Float32 s)
    {
        for( Size i = 0; i < SIZE; ++i)
            c[i] = s;
    }

    /// Constructor initializes (r,g,b) from (\p nr,\p ng,\p nb).
    inline Spectrum( Float32 nr, Float32 ng, Float32 nb)
    {
        c[0] = nr;
        c[1] = ng;
        c[2] = nb;
    }

    /// Conversion from %Vector<Float32,3>.
    inline explicit Spectrum( const Vector<Float32,3>& v3)
    {
        c[0] = v3[0];
        c[1] = v3[1];
        c[2] = v3[2];
    }

    /// Conversion from %Vector<Float32,4>.
    inline explicit Spectrum( const Vector<Float32,4>& v4)
    {
        c[0] = v4[0];
        c[1] = v4[1];
        c[2] = v4[2];
    }

    /// Conversion from %Color.
    inline explicit Spectrum( const Color& col)
    {
        c[0] = col.r;
        c[1] = col.g;
        c[2] = col.b;
    }

    /// Conversion to %Vector<Float32,3>.
    inline Vector<Float32,3> to_vector3() const
    {
        Vector<Float32,3> result;
        result[0] = c[0];
        result[1] = c[1];
        result[2] = c[2];
        return result;
    }

    /// Conversion to %Vector<Float32,4>.
    inline Vector<Float32,4> to_vector4() const
    {
        Vector<Float32,4> result;
        result[0] = c[0];
        result[1] = c[1];
        result[2] = c[2];
        result[3] = 1.0;
        return result;
    }

    /// Accesses the \c i-th spectrum element, <tt>0 <= i < 3</tt>.
    inline const Float32& operator[]( Size i) const
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        return c[i];
    }

    /// Accesses the \c i-th spectrum element, <tt>0 <= i < 3</tt>.
    inline Float32& operator[]( Size i)
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        return c[i];
    }


    /// Returns the \c i-th spectrum element, <tt>0 <= i < 3</tt>.
    inline Float32 get( Size i) const
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        return c[i];
    }

    /// Sets the \c i-th spectrum element to \p value, <tt>0 <= i < 3</tt>.
    inline void set( Size i, Float32 value)
    {
        mi_math_assert_msg( i < SIZE, "precondition");
        c[i] = value;
    }

    /// Returns \c true if the spectrum is black ignoring the alpha value.
    inline bool is_black() const
    {
        for( Size i = 0; i < SIZE; ++i)
            if( c[i] != 0.0f)
                return false;
        return true;
    }

    /// Returns the intensity of the RGB components, equally weighted.
    inline Float32 linear_intensity() const
    {
        Float32 sum = 0.f;
        for( Size i = 0; i < SIZE; ++i)
            sum += c[i];
        return sum / Float32( SIZE);
    }
};

//------ Free comparison operators ==, !=, <, <=, >, >= for spectra  ------------

/// Returns \c true if \c lhs is elementwise equal to \c rhs.
inline bool  operator==( const Spectrum& lhs, const Spectrum& rhs)
{
    return is_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is elementwise not equal to \c rhs.
inline bool  operator!=( const Spectrum& lhs, const Spectrum& rhs)
{
    return is_not_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator<( const Spectrum& lhs, const Spectrum& rhs)
{
    return lexicographically_less( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator<=( const Spectrum& lhs, const Spectrum& rhs)
{
    return lexicographically_less_or_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator>( const Spectrum& lhs, const Spectrum& rhs)
{
    return lexicographically_greater( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator>=( const Spectrum& lhs, const Spectrum& rhs)
{
    return lexicographically_greater_or_equal( lhs, rhs);
}



//------ Free operators +=, -=, *=, /=, +, -, *, and / for spectra --------------

/// Adds \p rhs elementwise to \p lhs and returns the modified \p lhs.
inline Spectrum& operator+=( Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    lhs[0] += rhs[0];
    lhs[1] += rhs[1];
    lhs[2] += rhs[2];
    return lhs;
}

/// Subtracts \p rhs elementwise from \p lhs and returns the modified \p lhs.
inline Spectrum& operator-=( Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    lhs[0] -= rhs[0];
    lhs[1] -= rhs[1];
    lhs[2] -= rhs[2];
    return lhs;
}

/// Multiplies \p rhs elementwise with \p lhs and returns the modified \p lhs.
inline Spectrum& operator*=( Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    lhs[0] *= rhs[0];
    lhs[1] *= rhs[1];
    lhs[2] *= rhs[2];
    return lhs;
}

/// Divides \p lhs elementwise by \p rhs and returns the modified \p lhs.
inline Spectrum& operator/=( Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    lhs[0] /= rhs[0];
    lhs[1] /= rhs[1];
    lhs[2] /= rhs[2];
    return lhs;
}

/// Adds \p lhs and \p rhs elementwise and returns the new result.
inline Spectrum operator+( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2]);
}

/// Subtracts \p rhs elementwise from \p lhs and returns the new result.
inline Spectrum operator-( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]);
}

/// Multiplies \p rhs elementwise with \p lhs and returns the new result.
inline Spectrum operator*( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( lhs[0] * rhs[0], lhs[1] * rhs[1], lhs[2] * rhs[2]);
}

/// Divides \p rhs elementwise by \p lhs and returns the new result.
inline Spectrum operator/( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( lhs[0] / rhs[0], lhs[1] / rhs[1], lhs[2] / rhs[2]);
}

/// Negates the spectrum \p c elementwise and returns the new result.
inline Spectrum operator-( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( -c[0], -c[1], -c[2]);
}



//------ Free operator *=, /=, *, and / definitions for scalars ---------------

/// Multiplies the spectrum \p c elementwise with the scalar \p s and returns the modified spectrum
/// \p c.
inline Spectrum& operator*=( Spectrum& c, Float32 s)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    c[0] *= s;
    c[1] *= s;
    c[2] *= s;
    return c;
}

/// Divides the spectrum \p c elementwise by the scalar \p s and returns the modified spectrum \p c.
inline Spectrum& operator/=( Spectrum& c, Float32 s)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    const Float32 f = 1.0f / s;
    c[0] *= f;
    c[1] *= f;
    c[2] *= f;
    return c;
}

/// Multiplies the spectrum \p c elementwise with the scalar \p s and returns the new result.
inline Spectrum operator*( const Spectrum& c, Float32 s)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( c[0] * s, c[1] * s, c[2] * s);
}

/// Multiplies the spectrum \p c elementwise with the scalar \p s and returns the new result.
inline Spectrum operator*( Float32 s, const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( s * c[0], s * c[1], s* c[2]);
}

/// Divides the spectrum \p c elementwise by the scalar \p s and returns the new result.
inline Spectrum operator/( const Spectrum& c, Float32 s)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    Float32 f = 1.0f / s;
    return Spectrum( c[0] * f, c[1] * f, c[2] * f);
}


//------ Function Overloads for Spectrum Algorithms ------------------------------


/// Returns a spectrum with the elementwise absolute values of the spectrum \p c.
inline Spectrum abs( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( abs( c[0]), abs( c[1]), abs( c[2]));
}

/// Returns a spectrum with the elementwise arc cosine of the spectrum \p c.
inline Spectrum acos( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( acos( c[0]), acos( c[1]), acos( c[2]));
}

/// Returns \c true if all elements of \c c are not equal to zero.
inline bool all( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return (c[0] != 0.0f) && (c[1] != 0.0f) && (c[2] != 0.0f);
}

/// Returns \c true if any element of \c c is not equal to zero.
inline bool any( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return (c[0] != 0.0f) || (c[1] != 0.0f) || (c[2] != 0.0f);
}

/// Returns a spectrum with the elementwise arc sine of the spectrum \p c.
inline Spectrum asin( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( asin( c[0]), asin( c[1]), asin( c[2]));
}

/// Returns a spectrum with the elementwise arc tangent of the spectrum \p c.
inline Spectrum atan( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( atan( c[0]), atan( c[1]), atan( c[2]));
}

/// Returns a spectrum with the elementwise arc tangent of the spectrum \p c / \p d.
///
/// The signs of the elements of \p c and \p d are used to determine the quadrant of the results.
inline Spectrum atan2( const Spectrum& c, const Spectrum& d)
{
    mi_math_assert_msg( c.size() == 3 && d.size() == 3, "precondition");
    return Spectrum( atan2( c[0], d[0]), atan2( c[1], d[1]), atan2( c[2], d[2]));
}

/// Returns a spectrum with the elementwise smallest integral value that is not less than the
/// element in spectrum \p c.
inline Spectrum ceil( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( ceil( c[0]), ceil( c[1]), ceil( c[2]));
}

/// Returns the spectrum \p c elementwise clamped to the range [\p low, \p high].
inline Spectrum clamp( const Spectrum& c, const Spectrum& low, const Spectrum& high)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    mi_math_assert_msg( low.size() == 3, "precondition");
    mi_math_assert_msg( high.size() == 3, "precondition");
    return Spectrum( clamp( c[0], low[0], high[0]),
                     clamp( c[1], low[1], high[1]),
                     clamp( c[2], low[2], high[2]));
}

/// Returns the spectrum \p c elementwise clamped to the range [\p low, \p high].
inline Spectrum clamp( const Spectrum& c, const Spectrum& low, Float32 high)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    mi_math_assert_msg( low.size() == 3, "precondition");
    return Spectrum( clamp( c[0], low[0], high),
                     clamp( c[1], low[1], high),
                     clamp( c[2], low[2], high));
}

/// Returns the spectrum \p c elementwise clamped to the range [\p low, \p high].
inline Spectrum clamp( const Spectrum& c, Float32 low, const Spectrum& high)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    mi_math_assert_msg( high.size() == 3, "precondition");
    return Spectrum( clamp( c[0], low, high[0]),
                     clamp( c[1], low, high[1]),
                     clamp( c[2], low, high[2]));
}

/// Returns the spectrum \p c elementwise clamped to the range [\p low, \p high].
inline Spectrum clamp( const Spectrum& c, Float32 low, Float32 high)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( clamp( c[0], low, high),
                     clamp( c[1], low, high),
                     clamp( c[2], low, high));
}

/// Returns a spectrum with the elementwise cosine of the spectrum \p c.
inline Spectrum cos( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( cos( c[0]), cos( c[1]), cos( c[2]));
}

/// Converts elementwise radians in \p c to degrees.
inline Spectrum degrees( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( degrees( c[0]), degrees( c[1]), degrees( c[2]));
}

/// Returns elementwise max for each element in spectrum \p lhs that is less than the corresponding
/// element in spectrum \p rhs.
inline Spectrum elementwise_max( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( base::max MI_PREVENT_MACRO_EXPAND ( lhs[0], rhs[0]),
                     base::max MI_PREVENT_MACRO_EXPAND ( lhs[1], rhs[1]),
                     base::max MI_PREVENT_MACRO_EXPAND ( lhs[2], rhs[2]));
}

/// Returns elementwise min for each element in spectrum \p lhs that is less than the corresponding
/// element in spectrum \p rhs.
inline Spectrum elementwise_min( const Spectrum& lhs, const Spectrum& rhs)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return Spectrum( base::min MI_PREVENT_MACRO_EXPAND ( lhs[0], rhs[0]),
                     base::min MI_PREVENT_MACRO_EXPAND ( lhs[1], rhs[1]),
                     base::min MI_PREVENT_MACRO_EXPAND ( lhs[2], rhs[2]));
}

/// Returns a spectrum with elementwise \c e to the power of the element in the spectrum \p c.
inline Spectrum exp( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( exp( c[0]), exp( c[1]), exp( c[2]));
}

/// Returns a spectrum with elementwise \c 2 to the power of the element in the spectrum \p c.
inline Spectrum exp2( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( exp2( c[0]), exp2( c[1]), exp2( c[2]));
}

/// Returns a spectrum with the elementwise largest integral value that is not greater than the
/// element in spectrum \p c.
inline Spectrum floor( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( floor( c[0]), floor( c[1]), floor( c[2]));
}

/// Returns elementwise \p a modulo \p b, in other words, the remainder of a/b.
///
/// The elementwise result has the same sign as \p a.
inline Spectrum fmod( const Spectrum& a, const Spectrum& b)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( b.size() == 3, "precondition");
    return Spectrum( fmod( a[0], b[0]), fmod( a[1], b[1]), fmod( a[2], b[2]));
}

/// Returns elementwise \p a modulo \p b, in other words, the remainder of a/b.
///
/// The elementwise result has the same sign as \p a.
inline Spectrum fmod( const Spectrum& a, Float32 b)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    return Spectrum( fmod( a[0], b), fmod( a[1], b), fmod( a[2], b));
}

/// Returns a spectrum with the elementwise positive fractional part of the spectrum \p c.
inline Spectrum frac( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( frac( c[0]), frac( c[1]), frac( c[2]));
}

/// Returns a gamma corrected spectrum.
///
/// Gamma factors are used to correct for non-linear input and output devices; for example, monitors
/// typically have gamma factors between 1.7 and 2.2, meaning that one-half of the peak voltage does
/// not give one half of the brightness. This is corrected for by raising the spectrum components to
/// the gamma exponent. Gamma factors greater than 1 make an image brighter; less than 1 make it
/// darker. The inverse of \c gamma_correction(factor) is \c gamma_correction(1.0/factor).
inline Spectrum gamma_correction(
    const Spectrum& spectrum,    ///< spectrum to be corrected
    Float32 gamma_factor)        ///< gamma factor, must be greater than zero.
{
    mi_math_assert_msg( spectrum.size() == 3, "precondition");
    mi_math_assert( gamma_factor > 0);
    const Float32 f = Float32(1.0) / gamma_factor;
    return Spectrum( fast_pow( spectrum[0], f),
                     fast_pow( spectrum[1], f),
                     fast_pow( spectrum[2], f));
}

/// Compares the two given values elementwise for equality within the given epsilon.
inline bool is_approx_equal(
    const Spectrum& lhs,
    const Spectrum& rhs,
    Float32         e)
{
    mi_math_assert_msg( lhs.size() == 3, "precondition");
    mi_math_assert_msg( rhs.size() == 3, "precondition");
    return is_approx_equal( lhs[0], rhs[0], e)
        && is_approx_equal( lhs[1], rhs[1], e)
        && is_approx_equal( lhs[2], rhs[2], e);
}

/// Returns the elementwise linear interpolation between \p c1 and \c c2, i.e., it returns
/// <tt>(1-t) * c1 + t * c2</tt>.
inline Spectrum lerp(
    const Spectrum& c1,  ///< one spectrum
    const Spectrum& c2,  ///< second spectrum
    const Spectrum& t)   ///< interpolation parameter in [0,1]
{
    mi_math_assert_msg( c1.size() == 3, "precondition");
    mi_math_assert_msg( c2.size() == 3, "precondition");
    mi_math_assert_msg( t.size() == 3, "precondition");
    return Spectrum( lerp( c1[0], c2[0], t[0]),
                     lerp( c1[1], c2[1], t[1]),
                     lerp( c1[2], c2[2], t[2]));
}

/// Returns the linear interpolation between \p c1 and \c c2, i.e., it returns
/// <tt>(1-t) * c1 + t * c2</tt>.
inline Spectrum lerp(
    const Spectrum& c1,  ///< one spectrum
    const Spectrum& c2,  ///< second spectrum
    Float32         t)   ///< interpolation parameter in [0,1]
{
    mi_math_assert_msg( c1.size() == 3, "precondition");
    mi_math_assert_msg( c2.size() == 3, "precondition");
    // equivalent to: return c1 * (Float32(1)-t) + c2 * t;
    return Spectrum( lerp( c1[0], c2[0], t),
                     lerp( c1[1], c2[1], t),
                     lerp( c1[2], c2[2], t));
}

/// Returns a spectrum with elementwise natural logarithm of the spectrum \p c.
inline Spectrum log( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( log( c[0]), log( c[1]), log( c[2]));
}

/// Returns a spectrum with elementwise %base 2 logarithm of the spectrum \p c.
inline Spectrum log2 MI_PREVENT_MACRO_EXPAND ( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( log2 MI_PREVENT_MACRO_EXPAND (c[0]),
                     log2 MI_PREVENT_MACRO_EXPAND (c[1]),
                     log2 MI_PREVENT_MACRO_EXPAND (c[2]));
}

/// Returns a spectrum with elementwise %base 10 logarithm of the spectrum \p c.
inline Spectrum log10( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( log10( c[0]), log10( c[1]), log10( c[2]));
}

/// Returns the elementwise fractional part of \p c and stores the elementwise integral part of \p c
/// in \p i.
///
/// Both parts have elementwise the same sign as \p c.
inline Spectrum modf( const Spectrum& c, Spectrum& i)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    mi_math_assert_msg( i.size() == 3, "precondition");
    return Spectrum( modf( c[0], i[0]), modf( c[1], i[1]), modf( c[2], i[2]));
}

/// Returns the spectrum \p a  elementwise to the power of \p b.
inline Spectrum pow( const Spectrum& a,  const Spectrum& b)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( b.size() == 3, "precondition");
    return Spectrum( pow( a[0], b[0]), pow( a[1], b[1]), pow( a[2], b[2]));
}

/// Returns the spectrum \p a  elementwise to the power of \p b.
inline Spectrum pow( const Spectrum& a,  Float32 b)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    return Spectrum( pow( a[0], b), pow( a[1], b), pow( a[2], b));
}

/// Converts elementwise degrees in \p c to radians.
inline Spectrum radians( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( radians( c[0]), radians( c[1]), radians( c[2]));
}

/// Returns a spectrum with the elements of spectrum \p c  rounded to nearest integers.
inline Spectrum round( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( round( c[0]), round( c[1]), round( c[2]));
}

/// Returns the reciprocal of the square root of each element of \p c.
inline Spectrum rsqrt( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( rsqrt( c[0]), rsqrt( c[1]), rsqrt( c[2]));
}

/// Returns the spectrum \p c clamped elementwise to the range [0,1].
inline Spectrum saturate( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( saturate( c[0]), saturate( c[1]), saturate( c[2]));
}

/// Returns the elementwise sign of spectrum \p c.
inline Spectrum sign( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( sign( c[0]), sign( c[1]), sign( c[2]));
}

/// Returns a spectrum with the elementwise sine of the spectrum \p c.
inline Spectrum sin( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( sin( c[0]), sin( c[1]), sin( c[2]));
}

/// Computes elementwise the sine \p s and cosine \p c of angles \p a simultaneously.
///
/// The angles \p a are specified in radians.
inline void sincos( const Spectrum& a, Spectrum& s, Spectrum& c)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( s.size() == 3, "precondition");
    mi_math_assert_msg( c.size() == 3, "precondition");
    sincos( a[0], s[0], c[0]);
    sincos( a[1], s[1], c[1]);
    sincos( a[2], s[2], c[2]);
}

/// Returns 0 if \p c is less than \p a and 1 if \p c is greater than \p b in an elementwise
/// fashion.
///
/// A smooth curve is applied in-between so that the return values vary continuously from 0 to 1 as
/// elements in \p c vary from \p a to \p b.
inline Spectrum smoothstep( const Spectrum& a, const Spectrum& b, const Spectrum& c)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( b.size() == 3, "precondition");
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( smoothstep( a[0], b[0], c[0]),
                     smoothstep( a[1], b[1], c[1]),
                     smoothstep( a[2], b[2], c[2]));
}

/// Returns 0 if \p c is less than \p a and 1 if \p c is greater than \p b in an elementwise
/// fashion.
///
/// A smooth curve is applied in-between so that the return values vary continuously from 0 to 1 as
/// \p x varies from \p a to \p b.
inline Spectrum smoothstep( const Spectrum& a, const Spectrum& b, Float32 x)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( b.size() == 3, "precondition");
    return Spectrum( smoothstep( a[0], b[0], x),
                     smoothstep( a[1], b[1], x),
                     smoothstep( a[2], b[2], x));
}

/// Returns the square root of each element of \p c.
inline Spectrum sqrt( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( sqrt( c[0]), sqrt( c[1]), sqrt( c[2]));
}

/// Returns elementwise 0 if \p c is less than \p a and 1 otherwise.
inline Spectrum step( const Spectrum& a, const Spectrum& c)
{
    mi_math_assert_msg( a.size() == 3, "precondition");
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( step( a[0], c[0]), step( a[1], c[1]), step( a[1], c[2]));
}

/// Returns a spectrum with the elementwise tangent of the spectrum \p c.
inline Spectrum tan( const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return Spectrum( tan( c[0]), tan( c[1]), tan( c[2]));
}

/// Indicates whether all components of the spectrum are finite.
inline bool isfinite MI_PREVENT_MACRO_EXPAND (const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return isfinite MI_PREVENT_MACRO_EXPAND (c[0])
        && isfinite MI_PREVENT_MACRO_EXPAND (c[1])
        && isfinite MI_PREVENT_MACRO_EXPAND (c[2]);
}

/// Indicates whether any component of the spectrum is infinite.
inline bool isinfinite MI_PREVENT_MACRO_EXPAND (const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return isinfinite MI_PREVENT_MACRO_EXPAND (c[0])
        || isinfinite MI_PREVENT_MACRO_EXPAND (c[1])
        || isinfinite MI_PREVENT_MACRO_EXPAND (c[2]);
}

/// Indicates whether any component of the spectrum is "not a number".
inline bool isnan MI_PREVENT_MACRO_EXPAND (const Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    return isnan MI_PREVENT_MACRO_EXPAND (c[0])
        || isnan MI_PREVENT_MACRO_EXPAND (c[1])
        || isnan MI_PREVENT_MACRO_EXPAND (c[2]);
}

/// Encodes a spectrum into RGBE representation.
inline void to_rgbe( const Spectrum& c, Uint32& rgbe)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    to_rgbe( &c[0], rgbe);
}

/// Encodes a spectrum into RGBE representation.
inline void to_rgbe( const Spectrum& c, Uint8 rgbe[4])
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    to_rgbe( &c[0], rgbe);
}

/// Decodes a color from RGBE representation.
inline void from_rgbe( const Uint8 rgbe[4], Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    from_rgbe( rgbe, &c[0]);
}

/// Decodes a color from RGBE representation.
inline void from_rgbe( const Uint32 rgbe, Spectrum& c)
{
    mi_math_assert_msg( c.size() == 3, "precondition");
    from_rgbe( rgbe, &c[0]);
}

/*@}*/ // end group mi_math_spectrum

} // namespace math

} // namespace mi

#endif // MI_MATH_SPECTRUM_H
