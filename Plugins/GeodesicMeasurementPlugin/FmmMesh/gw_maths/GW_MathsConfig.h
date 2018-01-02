/*------------------------------------------------------------------------------*/
/**
 *  \file   GW_Config.h
 *  \brief  Main configuration file.
 *  \author Gabriel Peyré
 *  \date   10-26-2002
 */
/*------------------------------------------------------------------------------*/

#ifndef _GW_MATHSCONFIG_H_
#define _GW_MATHSCONFIG_H_


//-------------------------------------------------------------------------
/** \name C++ STL */
//-------------------------------------------------------------------------
//@{
#include <algorithm>
#include <map>
#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <fstream>
#include <complex>
using std::string;
using std::cerr;
using std::cout;
using std::endl;
//@}

//-------------------------------------------------------------------------
/** \name classical ANSI c libraries */
//-------------------------------------------------------------------------
//@{

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
//#include <malloc.h>
#include <memory.h>
//#include <tchar.h>
#include <ios>
#include <stdarg.h>
//#include <crtdbg.h>
//@}
/*
#ifdef GW_DEBUG
    #pragma    comment(lib, "laspack_dbg.lib")
#else
    #pragma    comment(lib, "laspack.lib")
#endif // GW_DEBUG
*/
namespace GW {


//-------------------------------------------------------------------------
/** \name debug & inline directive */
//-------------------------------------------------------------------------
//@{
#ifdef _DEBUG
    #ifndef GW_DEBUG
        #define GW_DEBUG
    #endif // GW_DEBUG
#endif // _DEBUG

#ifndef GW_DEBUG
    #ifndef GW_USE_INLINE
        #define GW_USE_INLINE
    #endif // GW_USE_INLINE
#endif // GW_DEBUG

#ifdef GW_USE_INLINE
    #ifndef GW_INLINE
        #define GW_INLINE inline
    #endif // GWION3D_INLINE
#else
    #ifndef GW_INLINE
        #define GW_INLINE
    #endif // GW_INLINE
#endif // GW_USE_INLINE
//@}

//-------------------------------------------------------------------------
/** \name Name space management */
//-------------------------------------------------------------------------
//@{
#define GW_BEGIN_NAMESPACE        namespace GW {
#define GW_END_NAMESPACE        }
//@}

//-------------------------------------------------------------------------
/** \name Basic types */
//-------------------------------------------------------------------------
//@{
#if defined(__UNIX__) || defined(__unix__) || defined(__APPLE__)
    typedef char                    GW_I8;
    typedef unsigned char           GW_U8;
    typedef short                    GW_I16;
    typedef unsigned short            GW_U16;
    typedef long                    GW_I32;
    typedef unsigned long           GW_U32;
    typedef long long int           GW_I64;
    typedef unsigned long long int  GW_U64;
    typedef float                   GW_Real32;
    typedef double                  GW_Real64;
    typedef bool                    GW_Bool;
    typedef GW_Real64              GW_Float;
#elif defined (_WIN32)
    typedef char                GW_I8;
    typedef unsigned char       GW_U8;
    typedef short                GW_I16;
    typedef unsigned short        GW_U16;
    typedef long                GW_I32;
    typedef unsigned long       GW_U32;
    // typedef __int64             GW_I64;
    // typedef unsigned __int64    GW_U64;
    typedef float               GW_Real32;
    typedef double              GW_Real64;
    typedef bool                GW_Bool;
    typedef GW_Real64            GW_Float;
#elif defined (_PS2_)
    typedef char                GW_I8;
    typedef unsigned char       GW_U8;
    typedef short                GW_I16;
    typedef unsigned short        GW_U16;
    typedef int                 GW_I32;
    typedef unsigned int        GW_U32;
    typedef long long           GW_I64;
    typedef unsigned long long  GW_U64;
    typedef float               GW_Real32;
    typedef double              GW_Real64;
    typedef GW_I8              GW_Bool;
    typedef GW_Real64          GW_Float;
#else
    #error "Unknown architecture !"
#endif

    typedef void*                GW_UserData;

    /** value returned by function to indicates error GW GW_OK */
    typedef GW_I32 GW_RETURN_CODE;

//@}


//-------------------------------------------------------------------------
/** \name numerical macros */
//-------------------------------------------------------------------------
//@{
#undef        MIN
#undef        MAX
#define        MIN(a, b)       ((a) < (b) ? (a) : (b))            //!<    Returns the min value between a and b
#define        MAX(a, b)       ((a) > (b) ? (a) : (b))            //!<    Returns the max value between a and b
#define        MAXMAX(a,b,c)   ((a) > (b) ? MAX (a,c) : MAX (b,c))
#define        GW_MIN(a, b)       MIN(a,b)                        //!<    Returns the min value between a and b
#undef        GW_MAX    // already defined by Windows.h ...
#define        GW_MAX(a, b)       MAX(a,b)                        //!<    Returns the max value between a and b
#define        GW_MAXMAX(a,b,c)   MAXMAX(a,b,c)

#define GW_SCALE_01(x,rMin,rMax) ((x-rMin)/(rMax-rMin))

#define        GW_ABS(a)       ((a) > 0 ? (a) : -(a))            //!<    Returns the absolute value a

#define        SQR(x)            ((x)*(x))                        //!<    Returns x square
#define        CUBE(x)            ((x)*(x)*(x))                    //!<    Returns x cube
#define        GW_SQR(x)        SQR(x)                            //!<    Returns x square
#define        GW_CUBE(x)        CUBE(x)                            //!<    Returns x cube

#define GW_CLAMP_01(x)    if( (x)<0 ) x=0; if( (x)>1 ) x=1
#define GW_CLAMP(x, a,b)    if( (x)<a ) x=a; if( (x)>b ) x=b

#define GW_SWAP(x,y) x^=y; y^=x; x^=y
#define GW_ORDER(x,y) if(x>y){ GW_SWAP(x,y); }
//@}

//-------------------------------------------------------------------------
/** \name generic macros */
//-------------------------------------------------------------------------
//@{
/** a random number in [0-1] */
#define GW_RAND ((GW_Float) (rand()%10000))/10000
/** a random number in [a,b] */
#define GW_RAND_RANGE(a,b) (a)+((b)-(a))*((GW_Float) (rand()%10000))/10000
/** delete a single pointer */
#define GW_DELETE(p) {if (p!=NULL) delete p; p=NULL;}
/** delete an array pointer */
#define GW_DELETEARRAY(p) {if (p!=NULL) delete [] p; p=NULL;}
//@}

//-------------------------------------------------------------------------
/** \name some constants */
//-------------------------------------------------------------------------
//@{
#define GW_True  true
#define GW_False false
/** to make approximate computations (derivation, GW_Float comparaisons ...) */
#define GW_EPSILON 1e-9
/** very big number */
#define GW_INFINITE 1e9
/** The temporary file we use to launch debug test involving stream I/O */
#define GW_TEST_FILE "test.bin"
//@}

//-------------------------------------------------------------------------
/** \name numerical  constants */
//-------------------------------------------------------------------------
//@{
/** pi */
#define GW_PI        3.1415926535897932384626433832795028841971693993751f
/** pi/2 */
#define GW_HALFPI    1.57079632679489661923f
/** 2*pi */
#define GW_TWOPI    6.28318530717958647692f
/** 1/pi */
#define GW_INVPI    0.31830988618379067154f
/** 180/pi */
#define GW_RADTODEG(x)    (x)*57.2957795130823208768f
/** pi/180 */
#define GW_DEGTORAD(x)    (x)*0.01745329251994329577f
/** e */
#define GW_EXP        2.71828182845904523536f
/** 1/log10(2) */
#define GW_ILOG2    3.32192809488736234787f
/** 1/3 */
#define GW_INV3        0.33333333333333333333f
/** 1/6 */
#define GW_INV6        0.16666666666666666666f
/** 1/9 */
#define GW_INV7        0.14285714285714285714f
/** 1/9 */
#define GW_INV9        0.11111111111111111111f
/** 1/255 */
#define GW_INV255    0.00392156862745098039f
/** sqrt(2) */
#define GW_SQRT2    1.41421356237f
//@}

//-------------------------------------------------------------------------
/** \name assertion macros */
//-------------------------------------------------------------------------
//@{
#ifdef GW_DEBUG
    #ifndef GW_ASSERT
        #define GW_ASSERT(expr) _ASSERT(expr)
    #endif
    #ifndef GW_DEBUG_ONLY
        #define GW_DEBUG_ONLY(expr) expr
    #endif
#else
    #ifndef GW_ASSERT
        #define GW_ASSERT(expr)    if(!(expr)) cerr << "Error in file " << __FILE__ << " line " << __LINE__ << "." << endl
    #endif
    #ifndef GW_DEBUG_ONLY
        #define GW_DEBUG_ONLY(expr)
    #endif
#endif // GW_DEBUG
//@}

//-------------------------------------------------------------------------
/** \name GW return codes */
//-------------------------------------------------------------------------
//@{
#define GW_OK                            0
#define GW_ERROR                        -9
#define GW_Invalid_Argument                -10
#define GW_Unknown_Argument                -11
#define GW_Error_Opening_File            -14
#define GW_Error_File_Not_Supported        -15
#define GW_Error_Argument_OutOfRange    -17
#define GW_Error_Buffer_OverFlow        -18
//@}

/** coordinates */
#ifndef _GW_CONFIG_H_
enum GW_Coord_XYZW
{
    X, Y, Z, W
};
#endif

/** header and footer for tests */
inline
void TestClassHeader(const char* class_name, std::ostream &s)
{
    s << "---------------------------------------------------------" << endl;
    s << "Beginning test for class " << class_name << " ..." << endl;
}
inline
void TestClassFooter(const char* class_name, std::ostream &s)
{
    s << "Test successful for class " << class_name << "." << endl;
}


#if 1
/** Traits class. This is the basic traits from which each
    implementation for standard traits (float, double, etc) will use.
    Gives default implementations. */
template<class numT>
struct gw_basic_type_traits
{
    static numT SquareModulus(const numT& val)
    { return val*val; }
    static numT Modulus(const numT& val)
    { return GW_ABS(val); }
    /** A value used for equality test. */
    numT Epsilon()
    { return 1e-9; }
    /** must return a number at random between say [0,1] */
    static numT Random(const numT& min, const numT& max)
    { return GW_RAND_RANGE(min,max); }
    static const char* GetBasicTypeName()
    { return NULL; }
    static GW_Bool IsInvertible(const numT& x)
    { return x!=0; }
};
template <>
struct gw_basic_type_traits<GW_Float>
{
    static GW_Float SquareModulus(const GW_Float& val)
    { return val*val; }
    /** A value used for equality test. */
    static GW_Float Epsilon()
    { return 1e-9; }
    /** must return a number at random between say [0,1] */
    static GW_Float Random(const GW_Float& min, const GW_Float& max)
    { return GW_RAND_RANGE(min,max); }
    static const char* GetBasicTypeName()
    { return NULL; }
};
template <>
struct gw_basic_type_traits<int>
{
    /** A value used for equality test. */
    static double Epsilon()
    { return 1e-5; }
    /** must return a number at random between say [0,1] */
    static int Random(const int& min, const int& max)
    {
#if defined(_MSC_VER)
#pragma warning( disable : 4244 )
#endif
        return (int) GW_RAND_RANGE(min,max+1);
    }
    static const char* GetBasicTypeName()
    { return "int"; }
};
#endif

} // End namespace GW

#ifdef GW_DEBUG
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#endif // _GW_CONFIG_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
