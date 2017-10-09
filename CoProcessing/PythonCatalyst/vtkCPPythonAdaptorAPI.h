/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonAdaptorAPI.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPythonAdaptorAPI_h
#define vtkCPPythonAdaptorAPI_h

#include "vtkCPAdaptorAPI.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries

/// Similar to vtkCPAdaptorAPI provides the implementation for API exposed to
/// typical adaptor, such as C, Fortran, except that is adds the ability to
/// initialize the coprocessor with Python capabilities.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonAdaptorAPI : public vtkCPAdaptorAPI
{
public:
  vtkTypeMacro(vtkCPPythonAdaptorAPI, vtkCPAdaptorAPI);

  /// Call at the start of the simulation. Users can still call
  /// CoProcessorInitialize() without arguments, in which case Python
  /// interpretor will not be initialized and hence unavailable.
  static void CoProcessorInitialize(const char* pythonFileName);

protected:
  vtkCPPythonAdaptorAPI();
  ~vtkCPPythonAdaptorAPI();

private:
  vtkCPPythonAdaptorAPI(const vtkCPPythonAdaptorAPI&) = delete;
  void operator=(const vtkCPPythonAdaptorAPI&) = delete;
};

#endif
// VTK-HeaderTest-Exclude: vtkCPPythonAdaptorAPI.h
