// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkInSituPythonConduitHelper
 *
 * A helper class to get the conduit node stored during the execution of one of
 * the catalyst*  calls as a python object.
 */

#ifndef vtkInSituPythonConduitHelper_h
#define vtkInSituPythonConduitHelper_h

#if VTK_MODULE_ENABLE_VTK_WrappingPythonCore
#include "vtkPython.h" // must be first
#else
// if we are being wrapped hide the forward declaration of PyObject to avoid
// duplicate definition error in Python wrappings
#if !defined(VTK_WRAPPING_CXX)
struct PyObject;
#endif
#endif

#include "vtkObject.h"
#include "vtkPVInSituModule.h" // For windows import/export of shared libraries

class VTKPVINSITU_EXPORT vtkInSituPythonConduitHelper : public vtkObject
{
public:
  vtkTypeMacro(vtkInSituPythonConduitHelper, vtkObject);

  static PyObject* GetCatalystParameters();

protected:
  vtkInSituPythonConduitHelper() = default;
  ~vtkInSituPythonConduitHelper() override = default;

private:
  vtkInSituPythonConduitHelper(const vtkInSituPythonConduitHelper&) = delete;
  void operator=(const vtkInSituPythonConduitHelper&) = delete;
};

#endif
