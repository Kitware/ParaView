/*=========================================================================

  Program:   ParaView
  Module:    vtkPVProcessModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVProcessModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// This super class assumes the application is running all in one process
// with no MPI.

#ifndef __vtkPVProcessModule_h
#define __vtkPVProcessModule_h

#include "vtkProcessModule.h"

#include "vtkClientServerID.h" // Needed for UniqueID ...

class vtkPolyData;
class vtkKWLoadSaveDialog;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVInformation;
class vtkPVPart;
class vtkPVPartDisplay;
class vtkSource;
class vtkStringList;
class vtkCallbackCommand;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkSocketController;
class vtkKWApplication;

class VTK_EXPORT vtkPVProcessModule : public vtkProcessModule
{
public:
  static vtkPVProcessModule* New();
  vtkTypeRevisionMacro(vtkPVProcessModule, vtkProcessModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ParaView.cxx (main) calls this method to setup the processes.
  // It currently creates the application, but I will try to pass
  // the application as an argument.
  virtual int Start(int argc, char **argv);
  
  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // Access to the subclass PVApplication store in the superclass
  // as a generic vtkKWApplication.
  vtkPVApplication *GetPVApplication();

  // Description:
  // Set the application instance for this class.
  virtual void SetApplication (vtkKWApplication* arg);
  
  // Description:
  // Get a directory listing for the given directory.  Returns 1 for
  // success, and 0 for failure (when the directory does not exist).
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, int save);
  
  // Description:
  // Get an object from an int id.  This is only useful in
  // when in client mode and calling this from tcl where vtkClientServerID
  // is not wrapped.
  virtual vtkObjectBase* GetObjectFromIntID(unsigned int);
  //BTX
  // Description:
  // Return the vtk object associated with the given id for the client.
  // If the id is for an object on the server then 0 is returned.
  virtual vtkObjectBase* GetObjectFromID(vtkClientServerID);
  //ETX

  // Description:
  // Load a ClientServer wrapper module dynamically in the server
  // processes.  Returns 1 if all server nodes loaded the module and 0
  // otherwise.  The second argument may be used to specify a directory
  // in which to look for the module.
  virtual int LoadModule(const char* name, const char* directory);

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  virtual int LoadModuleInternal(const char* name, const char* directory);
  vtkClientServerID GetMPIMToNSocketConnectionID() { return this->MPIMToNSocketConnectionID;}

  // Description:
  // Initialize/Finalize the process module's
  // vtkClientServerInterpreter.
  virtual void InitializeInterpreter();
  virtual void FinalizeInterpreter();

  // Description:
  // This is a socket controller used to communicate
  // between the client and process 0 of the server.
  vtkSocketController* GetSocketController() { return 0; }

  vtkClientServerID GetApplicationID();

  // Description:
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();

  // Description:
  // Set the local progress
  void SetLocalProgress(const char* filter, int progress);
protected:
  vtkPVProcessModule();
  ~vtkPVProcessModule();

  vtkClientServerID MPIMToNSocketConnectionID;
  vtkKWApplication* Application;

  int ProgressEnabled;
private:
  vtkPVProcessModule(const vtkPVProcessModule&); // Not implemented
  void operator=(const vtkPVProcessModule&); // Not implemented
};

#endif
