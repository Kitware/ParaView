/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModulePythonHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProcessModulePythonHelper
// .SECTION Description
// A class that can be used to provide GUI elements to the vtkProcessModule
// without forcing the process modules to link to a GUI.

#ifndef __vtkPVProcessModulePythonHelper_h
#define __vtkPVProcessModulePythonHelper_h

#include "vtkProcessModuleGUIHelper.h"

class vtkPVProcessModule;
class vtkSMApplication;

class VTK_EXPORT vtkPVProcessModulePythonHelper : public vtkProcessModuleGUIHelper
{
public: 
  static vtkPVProcessModulePythonHelper* New();
  vtkTypeMacro(vtkPVProcessModulePythonHelper,vtkProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description: 
  // run main gui loop from process module
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId); 

  // Description:
  // Open a connection dialog GUI.
  virtual int OpenConnectionDialog(int*) { return 1; }
  
  // Description:
  // Handle progress links.
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();
  virtual void SetLocalProgress(const char*, int);

  // Description:
  // Exit the application
  virtual void ExitApplication();

  // Description:
  // When set to true, the python interpretor's console won't be shown, instead
  // we simply run the script provided as argument and exit. Set to false by
  // default.
  vtkSetMacro(DisableConsole, bool);
  vtkGetMacro(DisableConsole, bool);

protected:
  vtkPVProcessModulePythonHelper();
  virtual ~vtkPVProcessModulePythonHelper();

  vtkSMApplication* SMApplication;

  bool DisableConsole;

private:

  vtkPVProcessModulePythonHelper(const vtkPVProcessModulePythonHelper&); // Not implemented
  void operator=(const vtkPVProcessModulePythonHelper&); // Not implemented
};

#endif
