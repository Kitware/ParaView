/*=========================================================================

  Program:   ParaView
  Module:    vtkType.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Platform independent data types of fixed size.
// .SECTION Description
// Typedefs for cross-platform fixed-size integer and floating-point
// types are provided.

/*
  TODO:
   - Define mapping from VTK_TYPE_* value to type:
       template <int> vtkTypeFromValue;
*/

#ifndef __vtkType_h
#define __vtkType_h

#include "vtkSystemIncludes.h"

//----------------------------------------------------------------------------
/* Select an 8-bit integer type.  */
#if VTK_SIZEOF_CHAR == 1
typedef unsigned char vtkTypeUInt8;
typedef signed char   vtkTypeInt8;
# define VTK_TYPE_FORMAT_UINT8 "hhu"
# define VTK_TYPE_FORMAT_INT8 "hhd"
# define VTK_TYPE_UINT8 VTK_UNSIGNED_CHAR
# if VTK_TYPE_CHAR_IS_SIGNED
#  define VTK_TYPE_INT8 VTK_CHAR
# else
#  define VTK_TYPE_INT8 VTK_SIGNED_CHAR
# endif
#else
# error "No native data type can represent an 8-bit integer."
#endif

/* Select a 16-bit integer type.  */
#if VTK_SIZEOF_SHORT == 2
typedef unsigned short vtkTypeUInt16;
typedef signed short   vtkTypeInt16;
# define VTK_TYPE_FORMAT_UINT16 "hu"
# define VTK_TYPE_FORMAT_INT16 "hd"
# define VTK_TYPE_UINT16 VTK_UNSIGNED_SHORT
# define VTK_TYPE_INT16 VTK_SHORT
#elif VTK_SIZEOF_INT == 2
typedef unsigned int vtkTypeUInt16;
typedef signed int   vtkTypeInt16;
# define VTK_TYPE_FORMAT_UINT16 "u"
# define VTK_TYPE_FORMAT_INT16 "d"
# define VTK_TYPE_UINT16 VTK_UNSIGNED_INT
# define VTK_TYPE_INT16 VTK_INT
#elif VTK_SIZEOF_LONG == 2
typedef unsigned long vtkTypeUInt16;
typedef signed long   vtkTypeInt16;
# define VTK_TYPE_FORMAT_UINT16 "lu"
# define VTK_TYPE_FORMAT_INT16 "ld"
# define VTK_TYPE_UINT16 VTK_UNSIGNED_LONG
# define VTK_TYPE_INT16 VTK_LONG
#else
# error "No native data type can represent a 16-bit integer."
#endif

/* Select a 32-bit integer type.  */
#if VTK_SIZEOF_SHORT == 4
typedef unsigned short vtkTypeUInt32;
typedef signed short   vtkTypeInt32;
# define VTK_TYPE_FORMAT_UINT32 "hu"
# define VTK_TYPE_FORMAT_INT32 "hd"
# define VTK_TYPE_UINT32 VTK_UNSIGNED_SHORT
# define VTK_TYPE_INT32 VTK_SHORT
#elif VTK_SIZEOF_INT == 4
typedef unsigned int vtkTypeUInt32;
typedef signed int   vtkTypeInt32;
# define VTK_TYPE_FORMAT_UINT32 "u"
# define VTK_TYPE_FORMAT_INT32 "d"
# define VTK_TYPE_UINT32 VTK_UNSIGNED_INT
# define VTK_TYPE_INT32 VTK_INT
#elif VTK_SIZEOF_LONG == 4
typedef unsigned long vtkTypeUInt32;
typedef signed long   vtkTypeInt32;
# define VTK_TYPE_FORMAT_UINT32 "lu"
# define VTK_TYPE_FORMAT_INT32 "ld"
# define VTK_TYPE_UINT32 VTK_UNSIGNED_LONG
# define VTK_TYPE_INT32 VTK_LONG
#else
# error "No native data type can represent a 32-bit integer."
#endif

/* Select a 64-bit integer type.  */
#if VTK_SIZEOF_SHORT == 8
typedef unsigned short vtkTypeUInt64;
typedef signed short   vtkTypeInt64;
# define VTK_TYPE_FORMAT_UINT64 "hu"
# define VTK_TYPE_FORMAT_INT64 "hd"
# define VTK_TYPE_UINT64 VTK_UNSIGNED_SHORT
# define VTK_TYPE_INT64 VTK_SHORT
#elif VTK_SIZEOF_INT == 8
typedef unsigned int vtkTypeUInt64;
typedef signed int   vtkTypeInt64;
# define VTK_TYPE_FORMAT_UINT64 "u"
# define VTK_TYPE_FORMAT_INT64 "d"
# define VTK_TYPE_UINT64 VTK_UNSIGNED_INT
# define VTK_TYPE_INT64 VTK_INT
#elif VTK_SIZEOF_LONG == 8
typedef unsigned long vtkTypeUInt64;
typedef signed long   vtkTypeInt64;
# define VTK_TYPE_FORMAT_UINT64 "lu"
# define VTK_TYPE_FORMAT_INT64 "ld"
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG
# define VTK_TYPE_INT64 VTK_LONG
#elif defined(VTK_TYPE_USE_LONG_LONG) && VTK_SIZEOF_LONG_LONG == 8
typedef unsigned long long vtkTypeUInt64;
typedef signed long long   vtkTypeInt64;
# define VTK_TYPE_FORMAT_UINT64 "llu"
# define VTK_TYPE_FORMAT_INT64 "lld"
# define VTK_TYPE_UINT64 VTK_UNSIGNED_LONG_LONG
# define VTK_TYPE_INT64 VTK_LONG_LONG
#elif defined(VTK_TYPE_USE___INT64) && VTK_SIZEOF___INT64 == 8
typedef unsigned __int64 vtkTypeUInt64;
typedef signed __int64   vtkTypeInt64;
# define VTK_TYPE_FORMAT_UINT64 "I64u"
# define VTK_TYPE_FORMAT_INT64 "I64d"
# define VTK_TYPE_UINT64 VTK_UNSIGNED___INT64
# define VTK_TYPE_INT64 VTK___INT64
#else
# error "No native data type can represent a 64-bit integer."
#endif

/* Select a 32-bit floating point type.  */
#if VTK_SIZEOF_FLOAT == 4
typedef float vtkTypeFloat32;
# define VTK_TYPE_FORMAT_FLOAT32 "f"
# define VTK_TYPE_FLOAT32 VTK_FLOAT
#elif VTK_SIZEOF_DOUBLE == 4
typedef double vtkTypeFloat32;
# define VTK_TYPE_FORMAT_FLOAT32 "lf"
# define VTK_TYPE_FLOAT32 VTK_DOUBLE
#else
# error "No native data type can represent a 32-bit floating point value."
#endif

/* Select a 64-bit floating point type.  */
#if VTK_SIZEOF_FLOAT == 8
typedef float vtkTypeFloat64;
# define VTK_TYPE_FORMAT_FLOAT64 "f"
# define VTK_TYPE_FLOAT64 VTK_FLOAT
#elif VTK_SIZEOF_DOUBLE == 8
typedef double vtkTypeFloat64;
# define VTK_TYPE_FORMAT_FLOAT64 "lf"
# define VTK_TYPE_FLOAT64 VTK_DOUBLE
#else
# error "No native data type can represent a 64-bit floating point value."
#endif

#endif
