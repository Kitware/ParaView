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
class vtkProcessModuleGUIHelper;

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

  // Description:
  virtual void SendPrepareProgress();
  virtual void SendCleanupPendingProgress();

  // Description:
  // Set the local progress
  void SetLocalProgress(const char* filter, int progress);

  
  // Description:
  // For loggin from Tcl start and end execute events.  We do not have c
  // pointers to all filters.
  void LogStartEvent(char* str);
  void LogEndEvent(char* str);

  // Description:
  // More timer log access methods.  Static methods are not accessible 
  // from tcl.  We need a timer object on all procs.
  void SetLogBufferLength(int length);
  void ResetLog();
  void SetEnableLog(int flag);

  // Description:
  // Time threshold for event (start-end) when getting the log with indents.
  // We do not have a timer object on all procs.  Statics do not work with Tcl.
  vtkSetMacro(LogThreshold, float);
  vtkGetMacro(LogThreshold, float);

  // Ivars copied from vtkPVApplication in SetProcessModule method
  
  // Description:
  // This is used by the render server only.
  vtkGetStringMacro(MachinesFileName);
  vtkSetStringMacro(MachinesFileName);

  // The the portb*c  for the client/render server socket connection.
  vtkGetMacro(ReverseConnection,int);
  vtkSetMacro(ReverseConnection,int);

  //Description:
  // The port used by the render node.
  vtkGetMacro(RenderNodePort,int);
  vtkSetMacro(RenderNodePort,int);
  
  // Description:
  // Flag that differentiates between clinet and server programs.
  vtkGetMacro(ClientMode, int);
  vtkSetMacro(ClientMode, int);

  // Description:
  // Flag to determine if the render server is being used.
  // If this is on and ClientMode is on, then the client
  // will be connecting to both a render and data server.
  // If this flag is on and ClientMode is off, then this is 
  // a render server.
  vtkGetMacro(RenderServerMode, int);
  vtkSetMacro(RenderServerMode, int);

  vtkGetMacro(ServerMode, int);
  vtkSetMacro(ServerMode, int);
  
  vtkSetClampMacro(AlwaysSSH, int, 0, 1);
  vtkBooleanMacro(AlwaysSSH, int);
  vtkGetMacro(AlwaysSSH, int);  

  // Description:
  // The the port for the client/server socket connection.
  vtkGetMacro(Port,int);
  vtkSetMacro(Port,int);
  vtkGetMacro(RenderServerPort,int);
  vtkSetMacro(RenderServerPort,int);
  vtkSetStringMacro(HostName);
  vtkSetStringMacro(Username);
  vtkSetStringMacro(RenderServerHostName);

  // Description:
  // We need to get the data path for the demo on the server.
  const char* GetDemoPath();

  vtkSetStringMacro(DemoPath);

  // Description:
  // Need to put a global flag that indicates interactive rendering.  All
  // process must be consistent in choosing LODs because of the
  // vtkCollectPolydata filter.  This has to be in vtkPVApplication
  // because we do not create a render module on remote processes.
  void SetGlobalLODFlag(int val);
  static int GetGlobalLODFlag();
  static void SetGlobalLODFlagInternal(int val);
  
  void SetGUIHelper(vtkProcessModuleGUIHelper*);
  vtkSetMacro(ProgressEnabled, int);
  vtkGetMacro(ProgressEnabled, int);
  
  // Description:  
  // This method leaks memory.  It is a quick and dirty way to set different 
  // DISPLAY environment variables on the render server.  I think the string 
  // cannot be deleted until paraview exits.  The var should have the form:
  // "DISPLAY=amber1"
  virtual void SetProcessEnvironmentVariable(int processId, const char* var);

protected:
  vtkPVProcessModule();
  ~vtkPVProcessModule();

  vtkClientServerID MPIMToNSocketConnectionID;

  // Need to put a global flag that indicates interactive rendering.
  // All process must be consistent in choosing LODs because
  // of the vtkCollectPolydata filter.
  static int GlobalLODFlag;
  
  char* MachinesFileName;
  int RenderNodePort;
  int ReverseConnection;
  int ProgressEnabled;
  char* HostName; 
  char* Username;
  char* RenderServerHostName;
  int RenderServerPort;
  int Port;
  int ServerMode;
  int RenderServerMode;
  int ClientMode;
  int AlwaysSSH;
  float LogThreshold;
  char* DemoPath;
  vtkProcessModuleGUIHelper* GUIHelper;
private:
  vtkPVProcessModule(const vtkPVProcessModule&); // Not implemented
  void operator=(const vtkPVProcessModule&); // Not implemented
};

#endif
