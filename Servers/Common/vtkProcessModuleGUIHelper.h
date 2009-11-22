/*=========================================================================

  Program:   ParaView
  Module:    vtkProcessModuleGUIHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProcessModuleGUIHelper
// .SECTION Description
// A class that can be used to provide GUI elements to the vtkProcessModule
// without forcing the process modules to link to a GUI.

#ifndef __vtkProcessModuleGUIHelper_h
#define __vtkProcessModuleGUIHelper_h

#include "vtkObject.h"

class vtkProcessModule;
class vtkPVOptions;

class VTK_EXPORT vtkProcessModuleGUIHelper : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkProcessModuleGUIHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Does some set up and then calls this->ProcessModule->Start(),
  // which would eventually lead on 
  // vtkProcessModuleGUIHelper::RunGUIStart() on the client.
  // This is the method that should be called to start the client
  // event loop.
  virtual int Run(vtkPVOptions* options);
  
  // Description: 
  // run main gui loop from process module
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId) = 0; 

  // Description:
  // Open a connection dialog GUI.
  // OBSOLETE. This is no longer applicable was used in 2.*, but not since 3.0.
  // We may just want to get rid of it.
  virtual int OpenConnectionDialog(int* vtkNotUsed(start)){ return 0; }
  
  // Description:
  // Handle progress links.
  virtual void SendPrepareProgress() = 0;
  virtual void SendCleanupPendingProgress() = 0;
  virtual void SetLocalProgress(const char* filter, int progress) = 0;
  
  // Description:
  // Exit the application
  virtual void ExitApplication() = 0;

  // Set the Process module pointer
  virtual void SetProcessModule(vtkProcessModule*);

  // Description:
  // Popup dialog. The result will contain result when the user answers.
  //BTX
  virtual void PopupDialog(const char* title, const char* text)
    {
    (void) title;
    (void) text;
    }
  virtual int UpdatePopup() { return 1; }
  virtual void ClosePopup() { }
  //ETX

protected:
  vtkProcessModuleGUIHelper();

  vtkProcessModule* ProcessModule;
  
private:
  vtkProcessModuleGUIHelper(const vtkProcessModuleGUIHelper&); // Not implemented
  void operator=(const vtkProcessModuleGUIHelper&); // Not implemented
};

#endif
