/*=========================================================================

  Module:    vtkKWMyApplicationWin32Header.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkKWMyApplicationWin32Header_h
#define __vtkKWMyApplicationWin32Header_h

#include "vtkKWMyApplicationConfigure.h"

#if defined(_WIN32) && defined(KWMyApplication_BUILD_SHARED_LIBS)
# if defined(KWCallbacksExampleLib_EXPORTS) // *has* to match the lib name
#  define KWMyApplication_EXPORT __declspec( dllexport )
# else
#  define KWMyApplication_EXPORT __declspec( dllimport )
# endif
#else
# define KWMyApplication_EXPORT
#endif

#endif
