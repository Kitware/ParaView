/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPVStreamingMacros
 *
 * This file simple consolidates arbitrary macros used to debugging/logging for
 * streaming.
*/

#ifndef vtkPVStreamingMacros_h
#define vtkPVStreamingMacros_h

//#ifndef NDEBUG
//// vtkStreamingStatusMacro simply prints out debugging text. We define the macro
//// as empty when NDEBUG is defined (i.e. building in release mode).
//# define vtkStreamingStatusMacro(x) cout << "streaming: " x << endl;
//#else
//# define vtkStreamingStatusMacro(x)
//#endif

#ifndef PV_DEBUG_STREAMING
#define vtkStreamingStatusMacro(x)
#else
#define vtkStreamingStatusMacro(x) cout << "streaming: " x << endl;
#endif

#endif
// VTK-HeaderTest-Exclude: vtkPVStreamingMacros.h
