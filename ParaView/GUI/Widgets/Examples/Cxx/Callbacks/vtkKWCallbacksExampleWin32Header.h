/*=========================================================================

  Module:    vtkKWCallbacksExampleWin32Header.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkKWCallbacksExampleWin32Header_h
#define __vtkKWCallbacksExampleWin32Header_h

#include "vtkKWCallbacksExampleConfigure.h"

#if defined(_WIN32) && defined(KWCallbacksExample_BUILD_SHARED_LIBS)
# if defined(KWCallbacksExampleLib_EXPORTS)
#  define KWCallbacksExample_EXPORT __declspec( dllexport )
# else
#  define KWCallbacksExample_EXPORT __declspec( dllimport )
# endif
#else
# define KWCallbacksExample_EXPORT
#endif

#endif
