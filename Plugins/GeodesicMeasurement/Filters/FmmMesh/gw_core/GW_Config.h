/*------------------------------------------------------------------------------
 *  \file   GW_Config.h
 *  \brief  Main configuration file
------------------------------------------------------------------------------*/

#ifndef _GW_CONFIG_H_
#define _GW_CONFIG_H_

#include "stdafx.h"
#include <float.h>
#include <stdio.h>
#include "vtkFmmMeshConfig.h"

namespace GW {

#define GW_VERSION 100

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
    //typedef __int64             GW_I64;
    //typedef unsigned __int64    GW_U64;
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
#undef GW_RAND
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
/** space coords */
enum GW_Coord_XYZW { X, Y, Z, W };
/** color coords */
enum GW_Coord_RVBA { R, V, B, A };
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
    #define GW_ASSERT(expr) _ASSERT(expr)
    #define GW_DEBUG_ONLY(expr) expr
#else
    #ifndef GW_ASSERT
    #define GW_ASSERT(expr)    if(!(expr)) cerr << "Error in file " << __FILE__ << " line " << __LINE__ << "." << endl
  #endif
    #define GW_DEBUG_ONLY(expr)
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

//-------------------------------------------------------------------------
/** \name The different kind of transform we support for the moment. */
//-------------------------------------------------------------------------
//@{
enum T_WaveletTransformType
{
    T_IntegralDataChunk
};
//@}

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_Float */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<GW_Float> T_FloatVector;
typedef T_FloatVector::iterator IT_FloatVector;
typedef T_FloatVector::reverse_iterator RIT_FloatVector;
typedef T_FloatVector::const_iterator CIT_FloatVector;
typedef T_FloatVector::const_reverse_iterator CRIT_FloatVector;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_Float */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, GW_Float> T_FloatMap;
typedef T_FloatMap::iterator IT_FloatMap;
typedef T_FloatMap::reverse_iterator RIT_FloatMap;
typedef T_FloatMap::const_iterator CIT_FloatMap;
typedef T_FloatMap::const_reverse_iterator CRIT_FloatMap;
//@}

/*------------------------------------------------------------------------------*/
/** \name a list of string */
/*------------------------------------------------------------------------------*/
//@{
typedef std::list<std::string> T_StringList;
typedef T_StringList::iterator IT_StringList;
typedef T_StringList::reverse_iterator RIT_StringList;
typedef T_StringList::const_iterator CIT_StringList;
typedef T_StringList::const_reverse_iterator CRIT_StringList;
//@}

/*------------------------------------------------------------------------------*/
/** \name a vector of string */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<std::string> T_StringVector;
typedef T_StringVector::iterator IT_StringVector;
typedef T_StringVector::reverse_iterator RIT_StringVector;
typedef T_StringVector::const_iterator CIT_StringVector;
typedef T_StringVector::const_reverse_iterator CRIT_StringVector;
//@}

/*------------------------------------------------------------------------------*/
/** \name a map of GW_U32 */
/*------------------------------------------------------------------------------*/
//@{
typedef std::map<GW_U32, GW_U32> T_U32Map;
typedef T_U32Map::iterator IT_U32Map;
typedef T_U32Map::reverse_iterator RIT_U32Map;
typedef T_U32Map::const_iterator CIT_U32Map;
typedef T_U32Map::const_reverse_iterator CRIT_U32Map;
//@}


/*------------------------------------------------------------------------------*/
/** \name a vector of GW_U32 */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<GW_U32> T_U32Vector;
typedef T_U32Vector::iterator IT_U32Vector;
typedef T_U32Vector::reverse_iterator RIT_U32Vector;
typedef T_U32Vector::const_iterator CIT_U32Vector;
typedef T_U32Vector::const_reverse_iterator CRIT_U32Vector;
//@}



//-------------------------------------------------------------------------
/** \name Tests for maths operations. */
//-------------------------------------------------------------------------
//@{
inline
void TestMathBit()
{
#if 0
    GW_U32 bit = _clearfp();
    if( bit>1 )
    {
//        GW_ASSERT(GW_False);
        if( (bit&_SW_UNDERFLOW)==_SW_UNDERFLOW )
            cerr << "Math operation problem : underflow." << endl;
        if( (bit&_SW_OVERFLOW)==_SW_OVERFLOW )
            cerr << "Math operation problem : overflow." << endl;
        if( (bit&_SW_ZERODIVIDE)==_SW_ZERODIVIDE )
            cerr << "Math operation problem : zero divide." << endl;
        if( (bit&_SW_INVALID)==_SW_INVALID )
            cerr << "Math operation problem : invalid." << endl;
    }
#endif
}

#ifdef GW_DEBUG
    #define GW_CHECK_MATHSBIT() TestMathBit()
#else
    #define GW_CHECK_MATHSBIT()
#endif // GW_DEBUG
//@}


/** callback that informs on the evolution of an algorithm */
typedef void (*T_EvolutionCallback)( GW_Float rPerCent, const char* message );

/*------------------------------------------------------------------------------*/
/** \name a vector of GW_NodeMap */
/*------------------------------------------------------------------------------*/
//@{
typedef std::vector<class GW_NodeMap*> T_NodeMapVector;
typedef T_NodeMapVector::iterator IT_NodeMapVector;
typedef T_NodeMapVector::reverse_iterator RIT_NodeMapVector;
typedef T_NodeMapVector::const_iterator CIT_NodeMapVector;
typedef T_NodeMapVector::const_reverse_iterator CRIT_NodeMapVector;
//@}

//-------------------------------------------------------------------------
/** \name output direction. */
//-------------------------------------------------------------------------
//@{
extern string gw_alinea;
extern string gw_endl;
extern FILE* GW_OutputStream;


void GW_OutputComment( const char* str );
//@}


} // End namespace GW

#ifdef GW_DEBUG
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

#endif // _GW_CONFIG_H_


///////////////////////////////////////////////////////////////////////////////
//  Copyright (C) 2009 Gabriel Peyre
