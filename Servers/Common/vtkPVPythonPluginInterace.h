/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPythonPluginInterace.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonPluginInterace
// .SECTION Description
// vtkPVPythonPluginInterace defines the interface required by ParaView plugins
// that add python modules to ParaView.

#ifndef __vtkPVPythonPluginInterace_h
#define __vtkPVPythonPluginInterace_h

#include "vtkObject.h"

class VTK_EXPORT vtkPVPythonPluginInterace
{
public:
  virtual void GetPythonSourceList(vtkstd::vector<vtkstd::string>& modules,
    vtkstd::vector<vtkstd::string>& sources,
    vtkstd::vector<int> &package_flags) = 0;
};

#endif

