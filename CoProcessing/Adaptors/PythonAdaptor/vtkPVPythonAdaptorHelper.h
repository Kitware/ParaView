/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile vtkPVPythonAdaptorHelper.h,v $

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVPythonAdapterHelper
// .SECTION Description
// A class that can be used to provide GUI elements to the vtkProcessModule
// without forcing the process modules to link to a GUI.

#ifndef __vtkPVPythonAdapterHelper_h
#define __vtkPVPythonAdapterHelper_h

#include "vtkProcessModuleGUIHelper.h"

#if defined(VTK_BUILD_SHARED_LIBS)
#if defined(PythonAdaptor_EXPORTS)
#define PYTHONADAPTOR_EXPORT VTK_ABI_EXPORT
#else
#define PYTHONADAPTOR_EXPORT VTK_ABI_IMPORT
#endif
#endif

class vtkPVProcessModule;
class vtkPVPythonInterpretor;
class vtkSMApplication;

class PYTHONADAPTOR_EXPORT vtkPVPythonAdapterHelper : public vtkProcessModuleGUIHelper
{
public: 
  static vtkPVPythonAdapterHelper* New();
  vtkTypeRevisionMacro(vtkPVPythonAdapterHelper,vtkProcessModuleGUIHelper);
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

  vtkGetObjectMacro(Interpretor, vtkPVPythonInterpretor);
  
protected:
  vtkPVPythonAdapterHelper();
  virtual ~vtkPVPythonAdapterHelper();

  vtkSMApplication* SMApplication;

  bool DisableConsole;

  vtkPVPythonInterpretor* Interpretor;

private:

  vtkPVPythonAdapterHelper(const vtkPVPythonAdapterHelper&); // Not implemented
  void operator=(const vtkPVPythonAdapterHelper&); // Not implemented
};

#endif
