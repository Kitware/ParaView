/***************************************************************************************************
 * Copyright 2019 NVIDIA Corporation. All rights reserved.
 **************************************************************************************************/
/// \file mi/math/color.h
/// \brief Standard RGBA color class with floating point elements and operations.
///
/// See \ref mi_math_color.

#ifndef MI_MATH_COLOR_H
#define MI_MATH_COLOR_H

#include <mi/base/types.h>
#include <mi/math/assert.h>
#include <mi/math/function.h>
#include <mi/math/vector.h>

namespace mi {

namespace math {

/** \defgroup mi_math_color Color Class
    \ingroup mi_math

    Standard RGBA color class with floating point elements and operations.

    \par Include File:
    <tt> \#include <mi/math/color.h></tt>

    @{
*/

// Color and Spectrum can be converted into each other. To avoid cyclic dependencies among the
// headers, the Spectrum_struct class is already defined here.

//------- POD struct that provides storage for spectrum elements --------

/** \ingroup mi_math_spectrum

    Generic storage class template for a %Spectrum representation storing three floating point
    elements.

    Used as %base class for the #mi::math::Spectrum class. Implementations should not rely on the
    specific implementation of Spectrum_struct, but instead use the more %general access operations
    defined in the derived Spectrum class.
*/
struct Spectrum_struct
{
    /// Three color bands.
    Float32 c[3];
};

//------ Color Class ---------------------------------------------------------

/// Supported clipping modes
///
/// \sa #mi::Color::clip() function.
enum Clip_mode {
    CLIP_RGB,   ///< First clip RGB to [0,1], then clip A to [max(R,G,B),1].
    CLIP_ALPHA, ///< First clip A to [0,1], then clip RGB to [0,A].
    CLIP_RAW    ///< Clip RGB and A to [0,1].
};

/** Standard RGBA color class with floating point elements and operations.

    This class provides array-like storage for the four RGBA elements of type #mi::Float32.
    Several functions and arithmetic operators support the work with colors.

    The color class is a model of the STL container concept. It provides random access to its
    elements and corresponding random access iterators.

    \see
        For the free functions and operators available for colors see \ref mi_math_color.

    \see
        The underlying POD type #mi::math::Color_struct.

    \par Include File:
    <tt> \#include <mi/math/color.h></tt>
*/
class Color : public Color_struct //-V690 PVS
{
public:
    typedef Color_struct      Pod_type;         ///< POD class corresponding to this color.
    typedef Color_struct      storage_type;     ///< Storage class used by this color.
    typedef Float32           value_type;       ///< Element type.
    typedef Size              size_type;        ///< Size type, unsigned.
    typedef Difference        difference_type;  ///< Difference type, signed.
    typedef Float32 *         pointer;          ///< Mutable pointer to element.
    typedef const Float32 *   const_pointer;    ///< Const pointer to element.
    typedef Float32 &         reference;        ///< Mutable reference to element.
    typedef const Float32 &   const_reference;  ///< Const reference to element.

    static const Size SIZE      = 4;            ///< Constant size of the color.

     /// Constant size of the color.
    static inline Size size()     { return SIZE; }

     /// Constant maximum size of the color.
    static inline Size max_size() { return SIZE; }

    /// Returns the pointer to the first color element.
    inline Float32*        begin()       { return &r; }

    /// Returns the pointer to the first color element.
    inline const Float32*  begin() const { return &r; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all color elements.
    inline Float32*        end()         { return begin() + SIZE; }

    /// Returns the past-the-end pointer.
    ///
    /// The range [begin(),end()) forms the range over all color elements.
    inline const Float32*  end() const   { return begin() + SIZE; }

    /// The default constructor leaves the color elements uninitialized.
    inline Color()
    {
#if defined(DEBUG) || (defined(_MSC_VER) && _MSC_VER <= 1310)
        // In debug mode, default-constructed colors are initialized with signaling NaNs or, if not
        // applicable, with a maximum value to increase the chances of diagnosing incorrect use of
        // an uninitialized color.
        //
        // When compiling with Visual C++ 7.1 or earlier, this code is enabled in all variants to
        // work around a very obscure compiler bug that causes the compiler to crash.
        typedef mi::base::numeric_traits<Float32> Traits;
        Float32 v = (Traits::has_signaling_NaN)
            ? Traits::signaling_NaN() : Traits::max MI_PREVENT_MACRO_EXPAND ();
        r = v;
        g = v;
        b = v;
        a = v;
#endif
    }

    /// Constructor from underlying storage type.
    inline Color( const Color_struct& c)
    {
        r = c.r;
        g = c.g;
        b = c.b;
        a = c.a;
    }


    /// Constructor initializes all vector elements to the value \p s.
    inline explicit Color( const Float32 s)
    {
        r = s;
        g = s;
        b = s;
        a = s;
    }

    /// Constructor initializes (r,g,b,a) from (\p nr,\p ng,\p nb,\p na).
    inline Color( Float32 nr, Float32 ng, Float32 nb, Float32 na = 1.0)
    {
        r = nr;
        g = ng;
        b = nb;
        a = na;
    }

    /** Constructor initializes the color elements from a 4-dimensional \c array.

        The value type \c T of the \c array must be assignment compatible with the #mi::Float32
        type of the vector elements.

        An example defining a red color:
        \code
        int data[4] = { 1, 0, 0, 1};
        mi::math::Color color( data);
        \endcode
    */
    template <typename T>
    inline explicit Color( T array[4])
    {
        r = array[0];
        g = array[1];
        b = array[2];
        a = array[3];
    }

    /// Constructor initializes (r,g,b,a) from (\p v.x, \p v.y, \p v.z, \p v.w) of a compatible
    /// 4D vector \p v.
    inline explicit Color( const Vector<Float32,4>& v)
    {
        r = v.x;
        g = v.y;
        b = v.z;
        a = v.w;
    }

    /// Conversion from %Spectrum.
    inline explicit Color( const Spectrum_struct& s)
    {
        r = s.c[0];
        g = s.c[1];
        b = s.c[2];
        a = 1.0f;
    }

    /// Assignment operator.
    inline Color& operator=( const Color& c)
    {
        Color_struct::operator=( c);
        return *this;
    }

    /// Assignment operator from compatible 4D vector, setting (r,g,b,a) to
    /// (\p v.x, \p v.y, \p v.z, \p v.w).
    inline Color& operator=( const Vector<Float32,4>& v)
    {
        r = v.x;
        g = v.y;
        b = v.z;
        a = v.w;
        return *this;
    }

    /// Accesses the \c i-th color element, <tt>0 <= i < 4</tt>.
    inline const Float32& operator[]( Size i) const
    {
        mi_math_assert_msg( i < 4, "precondition");
        return (&r)[i];
    }

    /// Accesses the \c i-th color element, <tt>0 <= i < 4</tt>.
    inline Float32& operator[]( Size i)
    {
        mi_math_assert_msg( i < 4, "precondition");
        return (&r)[i];
    }


    /// Returns the \c i-th color element, <tt>0 <= i < 4</tt>.
    inline Float32 get( Size i) const
    {
        mi_math_assert_msg( i < 4, "precondition");
        return (&r)[i];
    }

    /// Sets the \c i-th color element to \p value, <tt>0 <= i < 4</tt>.
    inline void set( Size i, Float32 value)
    {
        mi_math_assert_msg( i < 4, "precondition");
       (&r)[i] = value;
    }

    /// Returns \c true if the color is black ignoring the alpha value.
    inline bool is_black() const
    {
        return (r == 0.0f) && (g == 0.0f) && (b == 0.0f);
    }

    /// Returns the intensity of the RGB components, equally weighted.
    inline Float32 linear_intensity() const
    {
        return (r + g + b) * Float32(1.0 / 3.0);
    }

    /// Returns the intensity of the RGB components, weighted according to the NTSC standard.
    ///
    /// Components are weighted to match the subjective color brightness perceived by the human eye;
    /// green appears brighter than blue.
    inline Float32 ntsc_intensity() const
    {
        return r * 0.299f + g * 0.587f + b * 0.114f;
    }

    /// Returns the intensity of the RGB components, weighted according to the CIE standard.
    ///
    /// Components are weighted to match the subjective color brightness perceived by the human eye;
    /// green appears brighter than blue.
    inline Float32 cie_intensity() const
    {
        return r * 0.212671f + g * 0.715160f + b * 0.072169f;
    }

    /** Returns this color clipped into the [0,1] range, according to the
        \c Clip_mode \p mode, and fades overbright colors to white if
        \p desaturate is \c true.
    */
    inline Color clip( Clip_mode mode = CLIP_RGB, bool desaturate = false) const;


    /** Returns this color clipped to the range [0,\p maxval] using color desaturation.

       This function tries to maintain the apparent brightness. It recognizes the brightness values
       of the different colors according to the NTSC standard. If possible, colors are adjusted by
       desaturating towards the brightness value.

       For an explanation of color clipping using desaturation, see pp. 125-128 and pp. 251-252 of
       "Illumination and Color in Computer Generated Imagery" by Roy Hall.
    */
    inline Color desaturate( Float32 maxval = 1.0f) const;
};

//------ Free comparison operators ==, !=, <, <=, >, >= for colors  ------------

/// Returns \c true if \c lhs is elementwise equal to \c rhs.
inline bool  operator==( const Color& lhs, const Color& rhs)
{
    return is_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is elementwise not equal to \c rhs.
inline bool  operator!=( const Color& lhs, const Color& rhs)
{
    return is_not_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator<( const Color& lhs, const Color& rhs)
{
    return lexicographically_less( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically less than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator<=( const Color& lhs, const Color& rhs)
{
    return lexicographically_less_or_equal( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator>( const Color& lhs, const Color& rhs)
{
    return lexicographically_greater( lhs, rhs);
}

/// Returns \c true if \c lhs is lexicographically greater than or equal to \c rhs.
///
/// \see   \ref mi_def_lexicographic_order
inline bool operator>=( const Color& lhs, const Color& rhs)
{
    return lexicographically_greater_or_equal( lhs, rhs);
}



//------ Free operators +=, -=, *=, /=, +, -, *, and / for colors --------------

/// Adds \p rhs elementwise to \p lhs and returns the modified \p lhs.
inline Color& operator+=( Color& lhs, const Color& rhs)
{
    lhs.r += rhs.r;
    lhs.g += rhs.g;
    lhs.b += rhs.b;
    lhs.a += rhs.a;
    return lhs;
}

/// Subtracts \p rhs elementwise from \p lhs and returns the modified \p lhs.
inline Color& operator-=( Color& lhs, const Color& rhs)
{
    lhs.r -= rhs.r;
    lhs.g -= rhs.g;
    lhs.b -= rhs.b;
    lhs.a -= rhs.a;
    return lhs;
}

/// Multiplies \p rhs elementwise with \p lhs and returns the modified \p lhs.
inline Color& operator*=( Color& lhs, const Color& rhs)
{
    lhs.r *= rhs.r;
    lhs.g *= rhs.g;
    lhs.b *= rhs.b;
    lhs.a *= rhs.a;
    return lhs;
}

/// Divides \p lhs elementwise by \p rhs and returns the modified \p lhs.
inline Color& operator/=( Color& lhs, const Color& rhs)
{
    lhs.r /= rhs.r;
    lhs.g /= rhs.g;
    lhs.b /= rhs.b;
    lhs.a /= rhs.a;
    return lhs;
}

/// Adds \p lhs and \p rhs elementwise and returns the new result.
inline Color operator+( const Color& lhs, const Color& rhs)
{
    return Color( lhs.r + rhs.r, lhs.g + rhs.g, lhs.b + rhs.b, lhs.a + rhs.a);
}

/// Subtracts \p rhs elementwise from \p lhs and returns the new result.
inline Color operator-( const Color& lhs, const Color& rhs)
{
    return Color( lhs.r - rhs.r, lhs.g - rhs.g, lhs.b - rhs.b, lhs.a - rhs.a);
}

/// Multiplies \p rhs elementwise with \p lhs and returns the new result.
inline Color operator*( const Color& lhs, const Color& rhs)
{
    return Color( lhs.r * rhs.r, lhs.g * rhs.g, lhs.b * rhs.b, lhs.a * rhs.a);
}

/// Divides \p rhs elementwise by \p lhs and returns the new result.
inline Color operator/( const Color& lhs, const Color& rhs)
{
    return Color( lhs.r / rhs.r, lhs.g / rhs.g, lhs.b / rhs.b, lhs.a / rhs.a);
}

/// Negates the color \p c elementwise and returns the new result.
inline Color operator-( const Color& c)
{
    return Color( -c.r, -c.g, -c.b, -c.a);
}



//------ Free operator *=, /=, *, and / definitions for scalars ---------------

/// Multiplies the color \p c elementwise with the scalar \p s and returns the modified color \p c.
inline Color& operator*=( Color& c, Float32 s)
{
    c.r *= s;
    c.g *= s;
    c.b *= s;
    c.a *= s;
    return c;
}

/// Divides the color \p c elementwise by the scalar \p s and returns the modified color \p c.
inline Color& operator/=( Color& c, Float32 s)
{
    const Float32 f = 1.0f / s;
    c.r *= f;
    c.g *= f;
    c.b *= f;
    c.a *= f;
    return c;
}

/// Multiplies the color \p c elementwise with the scalar \p s and returns the new result.
inline Color operator*( const Color& c, Float32 s)
{
    return Color( c.r * s, c.g * s, c.b * s, c.a * s);
}

/// Multiplies the color \p c elementwise with the scalar \p s and returns
/// the new result.
inline Color operator*( Float32 s, const Color& c)
{
    return Color( s * c.r, s * c.g, s* c.b, s * c.a);
}

/// Divides the color \p c elementwise by the scalar \p s and returns the new result.
inline Color operator/( const Color& c, Float32 s)
{
    const Float32 f = 1.0f / s;
    return Color( c.r * f, c.g * f, c.b * f, c.a * f);
}


//------ Function Overloads for Color Algorithms ------------------------------


/// Returns a color with the elementwise absolute values of the color \p c.
inline Color abs( const Color& c)
{
    return Color( abs( c.r), abs( c.g), abs( c.b), abs( c.a));
}

/// Returns a color with the elementwise arc cosine of the color \p c.
inline Color acos( const Color& c)
{
    return Color( acos( c.r), acos( c.g), acos( c.b), acos( c.a));
}

/// Returns \c true if all elements of \c c are not equal to zero.
inline bool all( const Color& c)
{
    return (c.r != 0.0f) && (c.g != 0.0f) && (c.b != 0.0f) && (c.a != 0.0f);
}

/// Returns \c true if any element of \c c is not equal to zero.
inline bool any( const Color& c)
{
    return (c.r != 0.0f) || (c.g != 0.0f) || (c.b != 0.0f) || (c.a != 0.0f);
}

/// Returns a color with the elementwise arc sine of the color \p c.
inline Color asin( const Color& c)
{
    return Color( asin( c.r), asin( c.g), asin( c.b), asin( c.a));
}

/// Returns a color with the elementwise arc tangent of the color \p c.
inline Color atan( const Color& c)
{
    return Color( atan( c.r), atan( c.g), atan( c.b), atan( c.a));
}

/// Returns a color with the elementwise arc tangent of the color \p c / \p d.
///
/// The signs of the elements of \p c and \p d are used to determine the quadrant of the results.
inline Color atan2( const Color& c, const Color& d)
{
    return Color( atan2( c.r, d.r), atan2( c.g, d.g), atan2( c.b, d.b), atan2( c.a, d.a));
}

/// Returns a color with the elementwise smallest integral value that is not less than the element
/// in color \p c.
inline Color ceil( const Color& c)
{
    return Color( ceil( c.r), ceil( c.g), ceil( c.b), ceil( c.a));
}

/// Returns the color \p c elementwise clamped to the range [\p low, \p high].
inline Color clamp( const Color& c, const Color& low, const Color& high)
{
    return Color( clamp( c.r, low.r, high.r),
                  clamp( c.g, low.g, high.g),
                  clamp( c.b, low.b, high.b),
                  clamp( c.a, low.a, high.a));
}

/// Returns the color \p c elementwise clamped to the range [\p low, \p high].
inline Color clamp( const Color& c, const Color& low, Float32 high)
{
    return Color( clamp( c.r, low.r, high),
                  clamp( c.g, low.g, high),
                  clamp( c.b, low.b, high),
                  clamp( c.a, low.a, high));
}

/// Returns the color \p c elementwise clamped to the range [\p low, \p high].
inline Color clamp( const Color& c, Float32 low, const Color& high)
{
    return Color( clamp( c.r, low, high.r),
                  clamp( c.g, low, high.g),
                  clamp( c.b, low, high.b),
                  clamp( c.a, low, high.a));
}

/// Returns the color \p c elementwise clamped to the range [\p low, \p high].
inline Color clamp( const Color& c, Float32 low, Float32 high)
{
    return Color( clamp( c.r, low, high),
                  clamp( c.g, low, high),
                  clamp( c.b, low, high),
                  clamp( c.a, low, high));
}

/// Returns a color with the elementwise cosine of the color \p c.
inline Color cos( const Color& c)
{
    return Color( cos( c.r), cos( c.g), cos( c.b), cos( c.a));
}

/// Converts elementwise radians in \p c to degrees.
inline Color degrees( const Color& c)
{
    return Color( degrees( c.r), degrees( c.g), degrees( c.b), degrees( c.a));
}

/// Returns elementwise max for each element in color \p lhs that is less than the corresponding
/// element in color \p rhs.
inline Color elementwise_max( const Color& lhs, const Color& rhs)
{
    return Color( base::max MI_PREVENT_MACRO_EXPAND ( lhs.r, rhs.r),
                  base::max MI_PREVENT_MACRO_EXPAND ( lhs.g, rhs.g),
                  base::max MI_PREVENT_MACRO_EXPAND ( lhs.b, rhs.b),
                  base::max MI_PREVENT_MACRO_EXPAND ( lhs.a, rhs.a));
}

/// Returns elementwise min for each element in color \p lhs that is less than the corresponding
/// element in color \p rhs.
inline Color elementwise_min( const Color& lhs, const Color& rhs)
{
    return Color( base::min MI_PREVENT_MACRO_EXPAND ( lhs.r, rhs.r),
                  base::min MI_PREVENT_MACRO_EXPAND ( lhs.g, rhs.g),
                  base::min MI_PREVENT_MACRO_EXPAND ( lhs.b, rhs.b),
                  base::min MI_PREVENT_MACRO_EXPAND ( lhs.a, rhs.a));
}

/// Returns a color with elementwise \c e to the power of the element in the color \p c.
inline Color exp( const Color& c)
{
    return Color( exp( c.r), exp( c.g), exp( c.b), exp( c.a));
}

/// Returns a color with elementwise \c 2 to the power of the element in the color \p c.
inline Color exp2( const Color& c)
{
    return Color( exp2( c.r), exp2( c.g), exp2( c.b), exp2( c.a));
}

/// Returns a color with the elementwise largest integral value that is not greater than the element
/// in color \p c.
inline Color floor( const Color& c)
{
    return Color( floor( c.r), floor( c.g), floor( c.b), floor( c.a));
}

/// Returns elementwise \p a modulo \p b, in other words, the remainder of a/b.
///
/// The elementwise result has the same sign as \p a.
inline Color fmod( const Color& a, const Color& b)
{
    return Color( fmod( a.r, b.r), fmod( a.g, b.g), fmod( a.b, b.b), fmod( a.a, b.a));
}

/// Returns elementwise \p a modulo \p b, in other words, the remainder of a/b.
///
/// The elementwise result has the same sign as \p a.
inline Color fmod( const Color& a, Float32 b)
{
    return Color( fmod( a.r, b), fmod( a.g, b), fmod( a.b, b), fmod( a.a, b));
}

/// Returns a color with the elementwise positive fractional part of the color \p c.
inline Color frac( const Color& c)
{
    return Color( frac( c.r), frac( c.g), frac( c.b), frac( c.a));
}

/// Returns a gamma corrected color.
///
/// Gamma factors are used to correct for non-linear input and output devices; for example, monitors
/// typically have gamma factors between 1.7 and 2.2, meaning that one-half of the peak voltage does
/// not give one half of the brightness. This is corrected for by raising the color components to
/// the gamma exponent. Gamma factors greater than 1 make an image brighter; less than 1 make it
/// darker. The inverse of \c gamma_correction(factor) is \c gamma_correction(1.0/factor).
inline Color gamma_correction(
    const Color& color,     ///< color to be corrected
    Float32 gamma_factor)   ///< gamma factor, must be greater than zero.
{
    mi_math_assert( gamma_factor > 0);
    const Float32 f = Float32(1.0) / gamma_factor;
    return Color( fast_pow( color.r, f),
                  fast_pow( color.g, f),
                  fast_pow( color.b, f),
                  color.a);
}

/// Compares the two given values elementwise for equality within the given epsilon.
inline bool is_approx_equal(
    const Color& lhs,
    const Color& rhs,
    Float32      e)
{
    return is_approx_equal( lhs.r, rhs.r, e)
        && is_approx_equal( lhs.g, rhs.g, e)
        && is_approx_equal( lhs.b, rhs.b, e)
        && is_approx_equal( lhs.a, rhs.a, e);
}

/// Returns the elementwise linear interpolation between \p c1 and \c c2, i.e., it returns
/// <tt>(1-t) * c1 + t * c2</tt>.
inline Color lerp(
    const Color& c1,  ///< one color
    const Color& c2,  ///< second color
    const Color& t)   ///< interpolation parameter in [0,1]
{
    return Color( lerp( c1.r, c2.r, t.r),
                  lerp( c1.g, c2.g, t.g),
                  lerp( c1.b, c2.b, t.b),
                  lerp( c1.a, c2.a, t.a));
}

/// Returns the linear interpolation between \p c1 and \c c2, i.e., it returns
/// <tt>(1-t) * c1 + t * c2</tt>.
inline Color lerp(
    const Color& c1,  ///< one color
    const Color& c2,  ///< second color
    Float32      t)   ///< interpolation parameter in [0,1]
{
    // equivalent to: return c1 * (Float32(1)-t) + c2 * t;
    return Color( lerp( c1.r, c2.r, t),
                  lerp( c1.g, c2.g, t),
                  lerp( c1.b, c2.b, t),
                  lerp( c1.a, c2.a, t));
}

/// Returns a color with elementwise natural logarithm of the color \p c.
inline Color log( const Color& c)
{
    return Color( log( c.r), log( c.g), log( c.b), log( c.a));
}

/// Returns a color with elementwise %base 2 logarithm of the color \p c.
inline Color log2 MI_PREVENT_MACRO_EXPAND ( const Color& c)
{
    return Color( log2 MI_PREVENT_MACRO_EXPAND (c.r),
                  log2 MI_PREVENT_MACRO_EXPAND (c.g),
                  log2 MI_PREVENT_MACRO_EXPAND (c.b),
                  log2 MI_PREVENT_MACRO_EXPAND (c.a));
}

/// Returns a color with elementwise %base 10 logarithm of the color \p c.
inline Color log10( const Color& c)
{
    return Color( log10( c.r), log10( c.g), log10( c.b), log10( c.a));
}

/// Returns the elementwise fractional part of \p c and stores the elementwise integral part of \p c
/// in \p i.
///
/// Both parts have elementwise the same sign as \p c.
inline Color modf( const Color& c, Color& i)
{
    return Color( modf( c.r, i.r), modf( c.g, i.g), modf( c.b, i.b), modf( c.a, i.a));
}

/// Returns the color \p a  elementwise to the power of \p b.
inline Color pow( const Color& a,  const Color& b)
{
    return Color( pow( a.r, b.r), pow( a.g, b.g), pow( a.b, b.b), pow( a.a, b.a));
}

/// Returns the color \p a  elementwise to the power of \p b.
inline Color pow( const Color& a,  Float32 b)
{
    return Color( pow( a.r, b), pow( a.g, b), pow( a.b, b), pow( a.a, b));
}

/// Converts elementwise degrees in \p c to radians.
inline Color radians( const Color& c)
{
    return Color( radians( c.r), radians( c.g), radians( c.b), radians( c.a));
}

/// Returns a color with the elements of color \p c  rounded to nearest integers.
inline Color round( const Color& c)
{
    return Color( round( c.r), round( c.g), round( c.b), round( c.a));
}

/// Returns the reciprocal of the square root of each element of \p c.
inline Color rsqrt( const Color& c)
{
    return Color( rsqrt( c.r), rsqrt( c.g), rsqrt( c.b), rsqrt( c.a));
}

/// Returns the color \p c clamped elementwise to the range [0,1].
inline Color saturate( const Color& c)
{
    return Color( saturate( c.r), saturate( c.g), saturate( c.b), saturate( c.a));
}

/// Returns the elementwise sign of color \p c.
inline Color sign( const Color& c)
{
    return Color( sign( c.r), sign( c.g), sign( c.b), sign( c.a));
}

/// Returns a color with the elementwise sine of the color \p c.
inline Color sin( const Color& c)
{
    return Color( sin( c.r), sin( c.g), sin( c.b), sin( c.a));
}

/// Computes elementwise the sine \p s and cosine \p c of angles \p a simultaneously.
///
/// The angles \p a are specified in radians.
inline void sincos( const Color& a, Color& s, Color& c)
{
    sincos( a.r, s.r, c.r);
    sincos( a.g, s.g, c.g);
    sincos( a.b, s.b, c.b);
    sincos( a.a, s.a, c.a);
}

/// Returns 0 if \p c is less than \p a and 1 if \p c is greater than \p b in an elementwise
/// fashion.
///
/// A smooth curve is applied in-between so that the return values vary continuously from 0 to 1 as
/// elements in \p c vary from \p a to \p b.
inline Color smoothstep( const Color& a, const Color& b, const Color& c)
{
    return Color( smoothstep( a.r, b.r, c.r),
                  smoothstep( a.g, b.g, c.g),
                  smoothstep( a.b, b.b, c.b),
                  smoothstep( a.a, b.a, c.a));
}

/// Returns 0 if \p c is less than \p a and 1 if \p c is greater than \p b in an elementwise
/// fashion.
///
/// A smooth curve is applied in-between so that the return values vary continuously from 0 to 1 as
/// \p x varies from \p a to \p b.
inline Color smoothstep( const Color& a, const Color& b, Float32 x)
{
    return Color( smoothstep( a.r, b.r, x),
                  smoothstep( a.g, b.g, x),
                  smoothstep( a.b, b.b, x),
                  smoothstep( a.a, b.a, x));
}

/// Returns the square root of each element of \p c.
inline Color sqrt( const Color& c)
{
    return Color( sqrt( c.r), sqrt( c.g), sqrt( c.b), sqrt( c.a));
}

/// Returns elementwise 0 if \p c is less than \p a and 1 otherwise.
inline Color step( const Color& a, const Color& c)
{
    return Color( step( a.r, c.r), step( a.g, c.g), step( a.g, c.b), step( a.a, c.a));
}

/// Returns a color with the elementwise tangent of the color \p c.
inline Color tan( const Color& c)
{
    return Color( tan( c.r), tan( c.g), tan( c.b), tan( c.a));
}

/// Indicates whether all components of the color are finite.
inline bool isfinite MI_PREVENT_MACRO_EXPAND (const Color& c)
{
    return isfinite MI_PREVENT_MACRO_EXPAND (c.r)
        && isfinite MI_PREVENT_MACRO_EXPAND (c.g)
        && isfinite MI_PREVENT_MACRO_EXPAND (c.b)
        && isfinite MI_PREVENT_MACRO_EXPAND (c.a);
}

/// Indicates whether any component of the color is infinite.
inline bool isinfinite MI_PREVENT_MACRO_EXPAND (const Color& c)
{
    return isinfinite MI_PREVENT_MACRO_EXPAND (c.r)
        || isinfinite MI_PREVENT_MACRO_EXPAND (c.g)
        || isinfinite MI_PREVENT_MACRO_EXPAND (c.b)
        || isinfinite MI_PREVENT_MACRO_EXPAND (c.a);
}

/// Indicates whether any component of the color is "not a number".
inline bool isnan MI_PREVENT_MACRO_EXPAND (const Color& c)
{
    return isnan MI_PREVENT_MACRO_EXPAND (c.r)
        || isnan MI_PREVENT_MACRO_EXPAND (c.g)
        || isnan MI_PREVENT_MACRO_EXPAND (c.b)
        || isnan MI_PREVENT_MACRO_EXPAND (c.a);
}

/// Encodes a color into RGBE representation.
///
/// \note The alpha value is not part of the RGBE representation.
inline void to_rgbe( const Color& color, Uint32& rgbe)
{
    to_rgbe( &color.r, rgbe);
}

/// Encodes a color into RGBE representation.
///
/// \note The alpha value is not part of the RGBE representation.
inline void to_rgbe( const Color& color, Uint8 rgbe[4])
{
    to_rgbe( &color.r, rgbe);
}

/// Decodes a color from RGBE representation.
///
/// \note The alpha value is set to 1.0.
inline void from_rgbe( const Uint8 rgbe[4], Color& color)
{
    from_rgbe( rgbe, &color.r);
    color.a = 1.0f;
}

/// Decodes a color from RGBE representation.
///
/// \note The alpha value is set to 1.0.
inline void from_rgbe( const Uint32 rgbe, Color& color)
{
    from_rgbe( rgbe, &color.r);
    color.a = 1.0f;
}

//------ Definitions of member functions --------------------------------------

#ifndef MI_FOR_DOXYGEN_ONLY

inline Color Color::clip(
    Clip_mode  mode,
    bool       desaturate) const
{
    Float32 max_val = 1.0f;
    Color col = *this;
    if( col.a < 0.0f)
        col.a = 0.0f;
    if( mode == CLIP_RGB) {
        if( col.a < col.r)  col.a = col.r;
        if( col.a < col.g)  col.a = col.g;
        if( col.a < col.b)  col.a = col.b;
    }
    if( col.a > 1.0f)
        col.a = 1.0f;
    if( mode == CLIP_ALPHA)
        max_val = col.a;
    if( desaturate)
        return col.desaturate(max_val);
    return Color( math::clamp( col.r, 0.0f, max_val),
                  math::clamp( col.g, 0.0f, max_val),
                  math::clamp( col.b, 0.0f, max_val),
                  col.a);
}

inline Color Color::desaturate( Float32 maxval) const
{
    // We compute a new color based on s with the vector formula c(s) = (N + s(I-N)) c0 where N is
    // the 3 by 3 matrix with the [1,3] vector b with the NTSC values as its rows, and c0 is the
    // original color. All c(s) have the same brightness, b*c0, as the original color. It can be
    // algebraically shown that the hue of the c(s) is the same as for c0. Hue can be expressed with
    // the formula h(c) = (I-A)c, where A is a 3 by 3 matrix with all 1/3 values. Essentially,
    // h(c(s)) == h(c0), since A*N == N

    Float32 t; // temp for saturation calc

    Float32 axis = ntsc_intensity();
    if( axis < 0) // negative: black, exit.
        return Color( 0, 0, 0, a);
    if( axis > maxval) // too bright: all white, exit.
        return Color( maxval, maxval, maxval, a);

    Float32 drds = r - axis;                // calculate color axis and
    Float32 dgds = g - axis;                // dcol/dsat. sat==1 at the
    Float32 dbds = b - axis;                // outset.

    Float32 sat = 1.0f;                     // initial saturation
    bool  clip = false;                     // outside range, desaturate

    if( r > maxval) {                       // red > maxval?
        clip = true;
        t = (maxval - axis) / drds;
        if( t < sat) sat = t;
    } else if( r < 0) {                     // red < 0?
        clip = true;
        t = -axis / drds;
        if( t < sat) sat = t;
    }
    if( g > maxval) {                       // green > maxval?
        clip = true;
        t = (maxval - axis) / dgds;
        if( t < sat) sat = t;
    } else if( g < 0) {                     // green < 0?
        clip = true;
        t = -axis / dgds;
        if( t < sat) sat = t;
    }
    if( b > maxval) {                       // blue > maxval?
        clip = true;
        t = (maxval - axis) / dbds;
        if( t < sat) sat = t;
    } else if( b < 0) {                     // blue < 0?
        clip = true;
        t = -axis / dbds;
        if( t < sat) sat = t;
    }
    if( clip) {
        // negative solutions should not be possible
        mi_math_assert( sat >= 0);
        // clamp to avoid numerical imprecision
        return Color( math::clamp( axis + drds * sat, 0.0f, maxval),
                      math::clamp( axis + dgds * sat, 0.0f, maxval),
                      math::clamp( axis + dbds * sat, 0.0f, maxval),
                      a);
    }
    mi_math_assert( r >= 0 && r <= maxval);
    mi_math_assert( g >= 0 && g <= maxval);
    mi_math_assert( b >= 0 && b <= maxval);
    return *this;
}

#endif // MI_FOR_DOXYGEN_ONLY

/*@}*/ // end group mi_math_color

} // namespace math

} // namespace mi

#endif // MI_MATH_COLOR_H
