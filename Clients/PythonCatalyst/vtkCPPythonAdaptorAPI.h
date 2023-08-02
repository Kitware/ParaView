// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Call at the start of the simulation. Users can still call
  /// CoProcessorInitialize() without arguments, in which case Python
  /// interpretor will not be initialized and hence unavailable.
  static void CoProcessorInitialize(const char* pythonFileName);

protected:
  vtkCPPythonAdaptorAPI();
  ~vtkCPPythonAdaptorAPI() override;

private:
  vtkCPPythonAdaptorAPI(const vtkCPPythonAdaptorAPI&) = delete;
  void operator=(const vtkCPPythonAdaptorAPI&) = delete;
};

#endif
