/*=========================================================================

  Program:   ParaView
  Module:    vtkTestingProcessModuleGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkTestingProcessModuleGUIHelper_h
#define __vtkTestingProcessModuleGUIHelper_h

#include "vtkProcessModuleGUIHelper.h"
class vtkSMApplication;

class VTK_EXPORT vtkTestingProcessModuleGUIHelper : public vtkProcessModuleGUIHelper
{
public: 
  static vtkTestingProcessModuleGUIHelper* New();
  vtkTypeMacro(vtkTestingProcessModuleGUIHelper,vtkProcessModuleGUIHelper);
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
  
protected:
  vtkTestingProcessModuleGUIHelper();
  virtual ~vtkTestingProcessModuleGUIHelper();

  vtkSMApplication* SMApplication;
  int ShowProgress;
  vtkSetStringMacro(Filter);
  char* Filter;
  int CurrentProgress;

  void CloseCurrentProgress();
private:

  vtkTestingProcessModuleGUIHelper(const vtkTestingProcessModuleGUIHelper&); // Not implemented
  void operator=(const vtkTestingProcessModuleGUIHelper&); // Not implemented
};

#endif

