/*=========================================================================

  Program:   ParaView
  Module:    vtkCPProcessModulePythonHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef __vtkCPProcessModulePythonHelper_h
#define __vtkCPProcessModulePythonHelper_h

#include "vtkPVProcessModulePythonHelper.h"
#include "CPWin32Header.h" // For windows import/export of shared libraries

class vtkPVProcessModule;
class vtkPVPythonInterpretor;
class vtkSMApplication;

/// @ingroup CoProcessing
/// A class that can be used to provide GUI elements to the vtkProcessModule
/// without forcing the process modules to link to a GUI.
class COPROCESSING_EXPORT vtkCPProcessModulePythonHelper : public vtkPVProcessModulePythonHelper
{
public: 
  static vtkCPProcessModulePythonHelper* New();
  vtkTypeRevisionMacro(vtkCPProcessModulePythonHelper,vtkPVProcessModulePythonHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  /// Run main gui loop from process module.
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId); 

  vtkGetMacro(Interpretor, vtkPVPythonInterpretor*);

protected:
  vtkCPProcessModulePythonHelper();
  virtual ~vtkCPProcessModulePythonHelper();

private:
  vtkCPProcessModulePythonHelper(const vtkCPProcessModulePythonHelper&); // Not implemented
  void operator=(const vtkCPProcessModulePythonHelper&); // Not implemented

  vtkPVPythonInterpretor* Interpretor;
};

#endif
