/*=========================================================================

  Module:    vtkKWWidgets.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkKWWidgets_h
#define __vtkKWWidgets_h

#include "vtkKWWidgetsConfigure.h"

#if defined(_WIN32) && defined(KWWidgets_BUILD_SHARED_LIBS)
# if defined(KWWidgets_EXPORTS)
#  define KWWidgets_EXPORT __declspec( dllexport )
# else
#  define KWWidgets_EXPORT __declspec( dllimport )
# endif
#else
# define KWWidgets_EXPORT
#endif

#ifdef __cplusplus
#define KWWidgets_EXTERN extern "C"
#else
#define KWWidgets_EXTERN extern
#endif

#endif
