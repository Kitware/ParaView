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

#include "vtkCPPipeline.h"
#include "CPWin32Header.h" // For windows import/export of shared libraries

class vtkPVPythonOptions;
class vtkPVPythonInterpretor;

/// @ingroup CoProcessing
/// Singleton class for python interpretor.
class COPROCESSING_EXPORT vtkCPPythonHelper : public vtkObject
{
public:
  static vtkCPPythonHelper* New();
  vtkTypeMacro(vtkCPPythonHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Get the interpretor that has been setup.
  vtkPVPythonInterpretor* GetPythonInterpretor();

protected:
  vtkCPPythonHelper();
  virtual ~vtkCPPythonHelper();

private:
  vtkCPPythonHelper(const vtkCPPythonHelper&); // Not implemented
  void operator=(const vtkCPPythonHelper&); // Not implemented

  vtkPVPythonOptions* PythonOptions;
  vtkPVPythonInterpretor* PythonInterpretor;

  /// The singleton instance of the class.
  static vtkCPPythonHelper* Instance;
};



#endif
