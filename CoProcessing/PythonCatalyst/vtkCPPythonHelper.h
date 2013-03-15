/*=========================================================================

  Program:   ParaView
  Module:    vtkCPPythonHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef vtkCPPythonHelper_h
#define vtkCPPythonHelper_h

#include "vtkObject.h"
#include "vtkPVPythonCatalystModule.h" // For windows import/export of shared libraries
#include "vtkWeakPointer.h" // needed for vtkWeakPointer.

class vtkPVPythonOptions;
class vtkPVPythonInterpretor;

/// @ingroup CoProcessing
/// Singleton class for initializing with python interpretor.
/// The vtkCPPythonHelper instance is created on the first call to
/// vtkCPPythonHelper::New(), subsequent calls return the same instance (but with
/// an increased reference count). When the caller is done with the
/// vtkCPPythonHelper instance, it should simply call Delete() or UnRegister() on
/// it. When the last caller of vtkCPPythonHelper::New() releases the reference,
/// the singleton instance will be cleaned up.
class VTKPVPYTHONCATALYST_EXPORT vtkCPPythonHelper : public vtkObject
{
public:
  static vtkCPPythonHelper* New();
  vtkTypeMacro(vtkCPPythonHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Get the interpretor that has been setup.
  static vtkPVPythonInterpretor* GetPythonInterpretor();

  /// Get access to helper script.
  /// Developer Note: We may want to convert this into a regular Python module.
  static const char* GetPythonHelperScript();

protected:
  vtkCPPythonHelper();
  virtual ~vtkCPPythonHelper();

private:
  vtkCPPythonHelper(const vtkCPPythonHelper&); // Not implemented
  void operator=(const vtkCPPythonHelper&); // Not implemented

  vtkPVPythonOptions* PythonOptions;
  vtkPVPythonInterpretor* PythonInterpretor;

  /// The singleton instance of the class.
  static vtkWeakPointer<vtkCPPythonHelper> Instance;
};



#endif
