/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerModule.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVClientServerModule
// .SECTION Description
// A class to encapaulate all of the process initialization,
// distributed data model and duplication  of the pipeline.
// Filters and compositers will still need a controller, 
// but every thing else should be handled here.  This class 
// sets up the default MPI processes with the user interface
// running on process 0.  I plan to make an alternative module
// for client server mode, where the client running the UI 
// is not in the MPI group but links to the MPI group through 
// a socket connection.

#ifndef __vtkPVClientServerModule_h
#define __vtkPVClientServerModule_h

#include "vtkPVProcessModule.h"

class vtkKWRemoteExecute;
class vtkMapper;
class vtkMapper;
class vtkMultiProcessController;
class vtkPVApplication;
class vtkPVPart;
class vtkSocketController;
class vtkSource;

class VTK_EXPORT vtkPVClientServerModule : public vtkPVProcessModule
{
public:
  static vtkPVClientServerModule* New();
  vtkTypeRevisionMacro(vtkPVClientServerModule,vtkPVProcessModule);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This starts the whole application.
  // This method initializes the MPI controller, then passes control
  // onto the init method.
  virtual int Start(int argc, char **argv);

  // Description:  
  // Start calls this method to continue initialization.
  // This method initializes the sockets and then calls
  // vtkPVApplication::Start(argc, argv);
  void Initialize();

  // Description:
  // This breaks rmi loops and cleans up processes.`                
  virtual void Exit();

  // Description:
  // Get the Partition piece. -1 means no partition assigned to this process.
  virtual int GetPartitionId();

  // Description:
  // Get the number of processes participating in sharing the data.
  virtual int GetNumberOfPartitions();
  
  // Description:
  // Flag that differentiates between clinet and server programs.
  vtkGetMacro(ClientMode, int);

  // Description:
  // This is a socket controller used to communicate
  // between the client and process 0 of the server.
  vtkGetObjectMacro(SocketController, vtkSocketController);

  //BTX
  // Description:
  // Module dependant method for collecting data information from all procs.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  //ETX
  virtual void GatherInformationInternal(const char* infoClassName,
                                         vtkObject* object);

  // Description:
  // Get a directory listing for the given directory.  This
  // implementation will always give a listing on the server side.
  virtual int GetDirectoryListing(const char* dir, vtkStringList* dirs,
                                  vtkStringList* files, int save);
  
  // Description:
  // Get a file selection dialog instance.
  virtual vtkKWLoadSaveDialog* NewLoadSaveDialog();
  
//BTX
  enum 
    {
    SINGLE_PROCESS_MODE = 0,
    MPI_MODE
    };
//ETX
  
  static void ErrorCallback(vtkObject *caller, unsigned long eid, void *clientdata, void *calldata);

  // Description:
  // Process a client server message on the server.
  void ProcessMessage(unsigned char* arg, size_t len);
  
  // Description:
  // Send current ClientServerStream data to the client
  virtual void SendStreamToClient();
  
  // Description:
  // Send current ClientServerStream data to the server
  virtual void SendStreamToServer();
  
  // Send the current vtkClientServerStream contents to the server
  // root node.  Also reset the vtkClientServerStream object.
  virtual void SendStreamToServerRoot();

  // Description:
  // Send current ClientServerStream data to the server and the client.
  virtual void SendStreamToClientAndServer();

  // Description:
  // Send current ClientServerStream data to the server root and the client.
  virtual void SendStreamToClientAndServerRoot();

  //BTX
  // Description:
  // Return a message containing the result of the last SendMessages call.
  // In client/server mode this causes a round trip to the server.
  virtual const vtkClientServerStream& GetLastServerResult();

  // Description:
  // Return a message containing the result of the last call made on
  // the client.
  virtual const vtkClientServerStream& GetLastClientResult();
  friend void vtkPVClientServerLastResultRMI(  void *, void* , int ,int );
  //ETX

  // Description:
  // Used internally.  Do not call.  Use LoadModule instead.
  int LoadModuleInternal(const char* name);
protected:
  vtkPVClientServerModule();
  ~vtkPVClientServerModule();

  // Description:
  // Send the last client server result to the client called from an RMI
  void SendLastClientServerResult();

  void SendStreamToServerInternal();
  void SendStreamToServerRootInternal();
  void Connect();

  int NumberOfServerProcesses;
  int ClientMode;
  vtkSocketController* SocketController;

  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

  vtkSetStringMacro(Hostname);
  vtkSetStringMacro(Username);
  char* Hostname;
  char* Username;
  int Port;
  int MultiProcessMode;
  int NumberOfProcesses;
  
  vtkClientServerStream* LastServerResultStream;
  
  vtkKWRemoteExecute* RemoteExecution;
private:  
  vtkPVClientServerModule(const vtkPVClientServerModule&); // Not implemented
  void operator=(const vtkPVClientServerModule&); // Not implemented
};

#endif
