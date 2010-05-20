/*=========================================================================

  Program:   ParaView
  Module:    vtkDummyProcessModuleHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDummyProcessModuleHelper - Process helper that does nothing
// .SECTION Description
// vtkDummyProcessModuleHelper is a dummy sub-class of vtkProcessModuleGUIHelper
// that does nothing. It can be used by application or modules that
// do not need any extra initialization. It is currently used by the
// python modules.

#ifndef __vtkDummyProcessModuleHelper_h
#define __vtkDummyProcessModuleHelper_h

#include "vtkProcessModuleGUIHelper.h"

class vtkPVProcessModule;
class vtkSMApplication;

class VTK_EXPORT vtkDummyProcessModuleHelper : public vtkProcessModuleGUIHelper
{
public: 
  static vtkDummyProcessModuleHelper* New();
  vtkTypeMacro(vtkDummyProcessModuleHelper,vtkProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description: 
  // Does nothing.
  virtual int RunGUIStart(int , char **, int , int ) { return 0; }

  // Description:
  // Open a connection dialog GUI.
  virtual int OpenConnectionDialog(int*) { return 1; }
  
  // Description:
  // These do nothing.
  virtual void SendPrepareProgress() {}
  virtual void SendCleanupPendingProgress() {}
  virtual void SetLocalProgress(const char*, int) {}

  // Description:
  // Does nothing.
  virtual void ExitApplication() {}

protected:
  vtkDummyProcessModuleHelper();
  virtual ~vtkDummyProcessModuleHelper();

private:
  vtkDummyProcessModuleHelper(const vtkDummyProcessModuleHelper&); // Not implemented
  void operator=(const vtkDummyProcessModuleHelper&); // Not implemented
};

#endif
