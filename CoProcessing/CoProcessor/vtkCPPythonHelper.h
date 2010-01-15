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
// .NAME vtkCPPythonHelper - Singleton class for python interpretor.
// .SECTION Description
// Singleton class for python interpretor.

#ifndef vtkCPPythonHelper_h
#define vtkCPPythonHelper_h

#include "vtkCPPipeline.h"

class vtkCPProcessModulePythonHelper;
class vtkPVMain;
class vtkPVPythonOptions;
class vtkPVPythonInterpretor;

class VTK_EXPORT vtkCPPythonHelper : public vtkObject
{
public:
  static vtkCPPythonHelper* New();
  vtkTypeRevisionMacro(vtkCPPythonHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the interpretor that has been setup.
  vtkPVPythonInterpretor* GetPythonInterpretor();

protected:
  vtkCPPythonHelper();
  virtual ~vtkCPPythonHelper();

private:
  vtkCPPythonHelper(const vtkCPPythonHelper&); // Not implemented
  void operator=(const vtkCPPythonHelper&); // Not implemented

  vtkCPProcessModulePythonHelper* ProcessModuleHelper;
  vtkPVMain* PVMain;
  vtkPVPythonOptions* PythonOptions;

  // Description:
  // The singleton instance of the class.
  static vtkCPPythonHelper* Instance;
};



#endif
