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
class vtkSocketCommunicator;

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
  // Flag to determine if the render server is being used.
  // If this is on and ClientMode is on, then the client
  // will be connecting to both a render and data server.
  // If this flag is on and ClientMode is off, then this is 
  // a render server.
  vtkGetMacro(RenderServerMode, int);

  // Description:
  // This is a socket controller used to communicate
  // between the client and process 0 of the server.
  vtkGetObjectMacro(SocketController, vtkSocketController);

  // Description:
  // Return the socket to the RenderServer, if this is not
  // set, return the SocketController.
  vtkSocketController* GetRenderServerSocketController();

  //BTX
  // Description:
  // Module dependant method for collecting data information from all procs.
  virtual void GatherInformation(vtkPVInformation* info,
                                 vtkClientServerID id);
  virtual void GatherInformationRenderServer(vtkPVInformation* info,
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
  // Send the current ClientServerStream data to different places and 
  // combinations of places.  Possible places are the Client, the 
  // Server (data server), or the RenderServer.  Also the stream
  // can be sent to the root of the render and data servers.
  // Most combinations are possible.
  virtual void SendStreamToClient();
  virtual void SendStreamToServer();
  virtual void SendStreamToRenderServer();
  virtual void SendStreamToServerRoot();
  virtual void SendStreamToRenderServerRoot(); 
  virtual void SendStreamToClientAndServerRoot();
  virtual void SendStreamToRenderServerAndServerRoot();
  virtual void SendStreamToClientAndRenderServerRoot(); 
  virtual void SendStreamToClientAndServer();
  virtual void SendStreamToClientAndRenderServer();
  virtual void SendStreamToRenderServerAndServer();
  virtual void SendStreamToRenderServerClientAndServer();

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
  virtual int LoadModuleInternal(const char* name, const char* directory);
protected:
  vtkPVClientServerModule();
  ~vtkPVClientServerModule();

  // Description:
  // Send the last client server result to the client called from an RMI
  void SendLastClientServerResult();
  // Description:
  // Actually send a stream to the server with the socket connection.
  void SendStreamToServerInternal();
  // Description:
  // Send a stream to the root node of the server
  void SendStreamToServerRootInternal();
  // Description:
  // Actually send a stream to the server with the socket connection.
  void SendStreamToRenderServerInternal();
  // Description:
  // Send a stream to the root node of the server
  void SendStreamToRenderServerRootInternal();
  // Description:
  // Connect to servers or clients, this will either set up a wait
  // loop waiting for a connection, or it will create a 
  void Connect();
  // Description:
  // Connect to a remote server or client already waiting for us.
  void ConnectToRemote();
  // Description:
  // Setup a wait connection that is waiting for a remote process to
  // connect to it.  This can be either the client or the server.
  void SetupWaitForConnection();
  // Description:
  // Return 1 if the connection should wait, and 0 if the connet
  int ShouldWaitForConnection();
  // Description:
  // Start a remote paraview server process.  Return 0 if connection failed.
  int StartRemoteParaView(vtkSocketCommunicator* comm);
  // Description:
  // Open a dialog to enter server information, if the start
  // variable is set to 1 in this function, then a remote paraview
  // should be started with StartRemoteParaView.
  int OpenConnectionDialog(int* start);

  // Description:
  // Create connection between render server and data server
  void InitializeRenderServer();
  
  int NumberOfServerProcesses;
  int ClientMode;
  vtkSocketController* SocketController;
  int RenderServerMode;
  vtkSocketController* RenderServerSocket;
  int NumberOfRenderServerProcesses;
  // To pass arguments through controller single method.
  int    ArgumentCount;
  char** Arguments;
  int    ReturnValue;

  vtkSetStringMacro(Hostname);
  vtkSetStringMacro(Username);
  char* Hostname;
  char* Username;
  vtkSetStringMacro(RenderServerHostName);
  char* RenderServerHostName;
  int RenderServerPort;
  int Port;
  int MultiProcessMode;
  int NumberOfProcesses;
  int GatherRenderServer;
  vtkClientServerStream* LastServerResultStream;
  
  vtkKWRemoteExecute* RemoteExecution;
private:  
  vtkPVClientServerModule(const vtkPVClientServerModule&); // Not implemented
  void operator=(const vtkPVClientServerModule&); // Not implemented
};

#endif
