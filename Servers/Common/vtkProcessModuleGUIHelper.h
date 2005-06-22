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

class VTK_EXPORT vtkProcessModuleGUIHelper : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkProcessModuleGUIHelper,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description: 
  // run main gui loop from process module
  virtual int RunGUIStart(int argc, char **argv, int numServerProcs, int myId) = 0; 

  // Description:
  // Open a connection dialog GUI.
  virtual int OpenConnectionDialog(int* start) = 0;
  
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

protected:
  vtkProcessModuleGUIHelper();

  vtkProcessModule* ProcessModule;
  
private:
  vtkProcessModuleGUIHelper(const vtkProcessModuleGUIHelper&); // Not implemented
  void operator=(const vtkProcessModuleGUIHelper&); // Not implemented
};

#endif
