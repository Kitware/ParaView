/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAltIOHeader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkAltIOHeader_h
#define __vtkAltIOHeader_h

#if defined(_WIN32)
# if defined(vtkAltIO_EXPORTS)
#  define VTK_ALTIO_EXPORT __declspec(dllexport)
# else
#  define VTK_ALTIO_EXPORT __declspec(dllimport)
# endif
#else
# define VTK_ALTIO_EXPORT
#endif

#endif
