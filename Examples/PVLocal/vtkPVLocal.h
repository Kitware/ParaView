/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLocal.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkPVLocal_h
#define __vtkPVLocal_h

#if defined(_WIN32)
# if defined(PVLocal_EXPORTS)
#  define VTK_PVLocal_EXPORT __declspec(dllexport)
# else
#  define VTK_PVLocal_EXPORT __declspec(dllimport)
# endif
#else
# define VTK_PVLocal_EXPORT
#endif

#endif
