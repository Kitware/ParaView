/*=========================================================================

  Program:   ParaView
  Module:    vtkTypeFromNative.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTypeFromNative - Get vtkType... typedef from a native C type.
// .SECTION Description
// vtkTypeFromNative is a class template mapping from a native type to
// a type from vtkType.h of the same size and signedness.  Example:
//   vtkTypeFromNative<signed char>::Type is vtkTypeInt8
#ifndef __vtkTypeFromNative_h
#define __vtkTypeFromNative_h

#include "vtkType.h"

// Forward-declare template.  There is no primary template.
template <class T> struct vtkTypeFromNative;

// Choose a method for implementing full template specialization.
// Old SGI compiler does not like template <> syntax for specialization.
#if defined(__sgi) && !defined(__GNUC__)
# if !defined(_COMPILER_VERSION)
#  define VTK_TEMPLATE_SPECIALIZATION
# endif
#endif

#if !defined(VTK_TEMPLATE_SPECIALIZATION)
# define VTK_TEMPLATE_SPECIALIZATION template <>
#endif

#define VTK_TYPE_FROM_NATIVE(input, output) \
  VTK_TEMPLATE_SPECIALIZATION \
  struct vtkTypeFromNative<input> { typedef output Type; }

// Map char types.  Size is always 1.
#if VTK_TYPE_CHAR_IS_SIGNED
VTK_TYPE_FROM_NATIVE(char, vtkTypeInt8);
#else
VTK_TYPE_FROM_NATIVE(char, vtkTypeUInt8);
#endif
VTK_TYPE_FROM_NATIVE(signed char, vtkTypeInt8);
VTK_TYPE_FROM_NATIVE(unsigned char, vtkTypeUInt8);

// Map short types.
#if VTK_SIZEOF_SHORT == 2
VTK_TYPE_FROM_NATIVE(short, vtkTypeInt16);
VTK_TYPE_FROM_NATIVE(unsigned short, vtkTypeUInt16);
#elif VTK_SIZEOF_SHORT == 4
VTK_TYPE_FROM_NATIVE(short, vtkTypeInt32);
VTK_TYPE_FROM_NATIVE(unsigned short, vtkTypeUInt32);
#elif VTK_SIZEOF_SHORT == 8
VTK_TYPE_FROM_NATIVE(short, vtkTypeInt64);
VTK_TYPE_FROM_NATIVE(unsigned short, vtkTypeUInt64);
#endif

// Map int types.
#if VTK_SIZEOF_INT == 2
VTK_TYPE_FROM_NATIVE(int, vtkTypeInt16);
VTK_TYPE_FROM_NATIVE(unsigned int, vtkTypeUInt16);
#elif VTK_SIZEOF_INT == 4
VTK_TYPE_FROM_NATIVE(int, vtkTypeInt32);
VTK_TYPE_FROM_NATIVE(unsigned int, vtkTypeUInt32);
#elif VTK_SIZEOF_INT == 8
VTK_TYPE_FROM_NATIVE(int, vtkTypeInt64);
VTK_TYPE_FROM_NATIVE(unsigned int, vtkTypeUInt64);
#endif

// Map long types.
#if VTK_SIZEOF_LONG == 2
VTK_TYPE_FROM_NATIVE(long, vtkTypeInt16);
VTK_TYPE_FROM_NATIVE(unsigned long, vtkTypeUInt16);
#elif VTK_SIZEOF_LONG == 4
VTK_TYPE_FROM_NATIVE(long, vtkTypeInt32);
VTK_TYPE_FROM_NATIVE(unsigned long, vtkTypeUInt32);
#elif VTK_SIZEOF_LONG == 8
VTK_TYPE_FROM_NATIVE(long, vtkTypeInt64);
VTK_TYPE_FROM_NATIVE(unsigned long, vtkTypeUInt64);
#endif

// Map non-standard types.
#if defined(VTK_TYPE_USE_LONG_LONG)
# if VTK_SIZEOF_LONG_LONG == 8
VTK_TYPE_FROM_NATIVE(long long, vtkTypeInt64);
VTK_TYPE_FROM_NATIVE(unsigned long long, vtkTypeUInt64);
# endif
#endif
#if defined(VTK_TYPE_USE___INT64)
# if VTK_SIZEOF___INT64 == 8
VTK_TYPE_FROM_NATIVE(__int64, vtkTypeInt64);
VTK_TYPE_FROM_NATIVE(unsigned __int64, vtkTypeUInt64);
# endif
#endif

// Map floating-point types.
#if VTK_SIZEOF_FLOAT == 4
VTK_TYPE_FROM_NATIVE(float, vtkTypeFloat32);
#elif VTK_SIZEOF_FLOAT == 8
VTK_TYPE_FROM_NATIVE(float, vtkTypeFloat64);
#endif
#if VTK_SIZEOF_DOUBLE == 4
VTK_TYPE_FROM_NATIVE(double, vtkTypeFloat32);
#elif VTK_SIZEOF_DOUBLE == 8
VTK_TYPE_FROM_NATIVE(double, vtkTypeFloat64);
#endif

#undef VTK_TYPE_FROM_NATIVE

#endif
