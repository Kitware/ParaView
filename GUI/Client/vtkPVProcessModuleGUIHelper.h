/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModuleGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProcessModuleGUIHelper
// .SECTION Description
// A class that can be used to provide GUI elements to the vtkProcessModule
// without forcing the process modules to link to a GUI.

#ifndef __vtkPVProcessModuleGUIHelper_h
#define __vtkPVProcessModuleGUIHelper_h

#include "vtkProcessModuleGUIHelper.h"

class vtkPVApplication;
class vtkPVProcessModule;

class VTK_EXPORT vtkPVProcessModuleGUIHelper : public vtkProcessModuleGUIHelper
{
public: 
  static vtkPVProcessModuleGUIHelper* New();
  vtkTypeRevisionMacro(vtkPVProcessModuleGUIHelper,vtkProcessModuleGUIHelper);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description: 
  // run main gui loop from process module
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId); 

  // Description:
  // Open a connection dialog GUI.
  virtual int OpenConnectionDialog(int* start);
  
  // Description:
  // Handle progress links.
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();
  virtual void SetLocalProgress(const char* filter, int progress);

  // Description:
  // Exit the application
  virtual void ExitApplication();
  // Description:
  // Set the Application pointer
  virtual void SetPVApplication(vtkPVApplication*);
  vtkPVApplication* GetPVApplication() 
    {
      return this->PVApplication;
    }
  
  // Set the Process module pointer
  virtual void SetPVProcessModule(vtkPVProcessModule*);
protected:
  vtkPVProcessModuleGUIHelper();
  virtual ~vtkPVProcessModuleGUIHelper();
private:
  vtkPVApplication* PVApplication;
  vtkPVProcessModule* PVProcessModule;
  vtkPVProcessModuleGUIHelper(const vtkPVProcessModuleGUIHelper&); // Not implemented
  void operator=(const vtkPVProcessModuleGUIHelper&); // Not implemented
};

#endif
