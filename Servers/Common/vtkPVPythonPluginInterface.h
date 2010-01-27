/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonPluginInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonPluginInterface
// .SECTION Description
// vtkPVPythonPluginInterface defines the interface required by ParaView plugins
// that add python modules to ParaView.

#ifndef __vtkPVPythonPluginInterface_h
#define __vtkPVPythonPluginInterface_h

#include "vtkObject.h"
#include <vtkstd/vector> // STL Header
#include <vtkstd/string> // STL Header

class VTK_EXPORT vtkPVPythonPluginInterface
{
public:
  virtual ~vtkPVPythonPluginInterface();

  virtual void GetPythonSourceList(vtkstd::vector<vtkstd::string>& modules,
    vtkstd::vector<vtkstd::string>& sources,
    vtkstd::vector<int> &package_flags) = 0;
};

#endif

