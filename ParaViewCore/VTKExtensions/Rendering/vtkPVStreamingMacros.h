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
// .NAME vtkPVStreamingMacros
// .SECTION Description
// This file simple consolidates arbitrary macros used to debugging/logging for
// streaming.

#ifndef __vtkPVStreamingMacros_h
#define __vtkPVStreamingMacros_h

#define vtkStreamingStatusMacro(x)\
  cout << "streaming: " x << endl;

#endif
