/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVClientServerModule.h"
#include "vtkPVServerInformation.h"
#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFloatArray.h"
#include "vtkInstantiator.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkShortArray.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkString.h"
#include "vtkStringList.h"
#include "vtkStringList.h"
#include "vtkToolkits.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkCallbackCommand.h"
#include "vtkKWRemoteExecute.h"
#include "vtkPVOptions.h"
#ifndef _WIN32
#include <unistd.h>
#endif
#include <vtkstd/string>
#include "vtkProcessModuleGUIHelper.h"
#include "vtkPVProgressHandler.h"

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"
#include "vtkMPIMToNSocketConnectionPortInformation.h"


#define VTK_PV_CLIENTSERVER_RMI_TAG          938531
#define VTK_PV_CLIENTSERVER_ROOT_RMI_TAG     938532

#define VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG    397529

#define VTK_PV_ROOT_RESULT_LENGTH_TAG        838487
#define VTK_PV_ROOT_RESULT_TAG               838488
#define VTK_PV_CLIENT_SERVER_LAST_RESULT_TAG 838490

#define VTK_PV_DATA_OBJECT_TAG               923857


//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVClientServerLastResultRMI(void *localArg, void* ,
                                    int vtkNotUsed(remoteArgLength),
                                    int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  self->SendLastClientServerResult();
}


void vtkPVClientServerMPIRMI(void *localArg, void *remoteArg,
                                int remoteArgLength,
                                int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  self->ProcessMessage((unsigned char*)remoteArg, remoteArgLength);
  // do something with result here??
}

//----------------------------------------------------------------------------
// This RMI is used for
void vtkPVClientServerSocketRMI(void *localArg, void *remoteArg,
                                int remoteArgLength,
                                int remoteProcessId)
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  // Do not execute the RMI if Enabled flag is set to false. This
  // flag is set at start up if the client does not have the right
  // credentials
  if (self->GetEnabled())
    {
    vtkMultiProcessController* controler = self->GetController();
    for(int i = 1; i < controler->GetNumberOfProcesses(); ++i)
      {
      controler->TriggerRMI(
        i, remoteArg, remoteArgLength, VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG);
      }
    vtkPVClientServerMPIRMI(
      localArg, remoteArg, remoteArgLength, remoteProcessId);
    }
}


//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVClientServerRootRMI(void *localArg, void *remoteArg,
                              int remoteArgLength,
                              int remoteProcessId)
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  // Do not execute the RMI if Enabled flag is set to false. This
  // flag is set at start up if the client does not have the right
  // credentials
  if (self->GetEnabled())
    {
    vtkPVClientServerMPIRMI(
      localArg, remoteArg, remoteArgLength, remoteProcessId);
    }
}

//----------------------------------------------------------------------------
void vtkPVSendStreamToClientServerNodeRMI(void *localArg, void *remoteArg,
                                          int remoteArgLength,
                                          int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule* self =
    reinterpret_cast<vtkPVClientServerModule*>(localArg);
  self->GetInterpreter()
    ->ProcessStream(reinterpret_cast<unsigned char*>(remoteArg),
                    remoteArgLength);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVClientServerModule);
vtkCxxRevisionMacro(vtkPVClientServerModule, "1.19");


//----------------------------------------------------------------------------
vtkPVClientServerModule::vtkPVClientServerModule()
{
  this->LastServerResultStream = new vtkClientServerStream;
  this->Controller = NULL;
  this->SocketController = NULL;
  this->RenderServerSocket = NULL;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;

  this->MultiProcessMode = vtkPVClientServerModule::SINGLE_PROCESS_MODE;
  this->NumberOfProcesses = 2;
  this->GatherRenderServer = 0;
  this->RemoteExecution = vtkKWRemoteExecute::New();

  this->Enabled = 1;
}

//----------------------------------------------------------------------------
vtkPVClientServerModule::~vtkPVClientServerModule()
{
  delete this->LastServerResultStream;
  if (this->Controller)
    {
    this->Controller->Delete();
    this->Controller = NULL;
    }
  if (this->SocketController)
    {
    this->SocketController->Delete();
    this->SocketController = NULL;
    }
  if (this->RenderServerSocket)
    {
    this->RenderServerSocket->Delete();
    this->RenderServerSocket = NULL;
    }

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;

  this->RemoteExecution->Delete();
}


//----------------------------------------------------------------------------
vtkSocketController* vtkPVClientServerModule::GetRenderServerSocketController()
{
  if(!this->RenderServerSocket)
    {
    return this->SocketController;
    }
  return this->RenderServerSocket;
}

  



//----------------------------------------------------------------------------
// Each server process starts with this method.  One process is designated as
// "master" to handle communication.  The other processes are slaves.
void vtkPVClientServerInit(vtkMultiProcessController *, void *arg )
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule*)arg;
  self->Initialize();
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::ErrorCallback(vtkObject *vtkNotUsed(caller),
  unsigned long vtkNotUsed(eid), void *vtkNotUsed(clientdata), void *calldata)
{
  cout << (char*)calldata << endl;
}

//----------------------------------------------------------------------------
// This method is a bit long, we should probably break it up
// to simplify it. !!!!!
void vtkPVClientServerModule::Initialize()
{
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int id;

  this->Connect();
  if (this->ReturnValue)
    { // Could not connect.
    return;
    } 

  if (this->Options->GetClientMode())
    {  
    if(!this->GUIHelper)
      {
      vtkErrorMacro("GUIHelper must be set, for vtkPVProcessModule to be able to run a gui.");
      return;
      }
  
    // The client sends the connect id to data server
    int cid = this->Options->GetConnectID();
    this->SocketController->Send(&cid, 1, 1, 8843);
    int dsmatch = 1, rsmatch = 1;
    // Check if it matched
    this->SocketController->Receive(&dsmatch, 1, 1, 8843);
    if (!dsmatch)
      {
      vtkErrorMacro("Connect ID mismatch. Data server was expecting another ID. "
                    "The server will exit.");
      }

    // Send the client version
    int version;
    version = PARAVIEW_VERSION_MAJOR;
    this->SocketController->Send(&version, 1, 1, 8843);
    version = PARAVIEW_VERSION_MINOR;
    this->SocketController->Send(&version, 1, 1, 8843);
    version = PARAVIEW_VERSION_PATCH;
    this->SocketController->Send(&version, 1, 1, 8843);

    // Check if versions matched
    int tmpmatch;
    this->SocketController->Receive(&tmpmatch, 1, 1, 8843);
    if (dsmatch && !tmpmatch)
      {
      vtkErrorMacro("Client and data server versions do not match. "
                    "Please make sure that you are using the right "
                    "client version.");
      dsmatch = 0;
      }
    
    
    // Receive as the hand shake the number of processes available.
    int numServerProcs = 0;
    this->SocketController->Receive(&numServerProcs, 1, 1, 8843);
    this->NumberOfServerProcesses = numServerProcs;
   
    if(this->Options->GetRenderServerMode())
      { 
      // The client sends the connect id to data server
      cid = this->Options->GetConnectID();
      this->RenderServerSocket->Send(&cid, 1, 1, 8843);
      // Check if it matched
      this->RenderServerSocket->Receive(&rsmatch, 1, 1, 8843);
      if (!rsmatch)
        {
        vtkErrorMacro("Connect ID mismatch. Data server was expecting "
                      "another ID. The server will exit.");
        }
      // Send the client version
      version = PARAVIEW_VERSION_MAJOR;
      this->RenderServerSocket->Send(&version, 1, 1, 8843);
      version = PARAVIEW_VERSION_MINOR;
      this->RenderServerSocket->Send(&version, 1, 1, 8843);
      version = PARAVIEW_VERSION_PATCH;
      this->RenderServerSocket->Send(&version, 1, 1, 8843);

      // Check if versions matched
      this->RenderServerSocket->Receive(&tmpmatch, 1, 1, 8843);
      if (!tmpmatch)
        {
        vtkErrorMacro("Client and render server versions do not match. "
                      "Please make sure that you are using the right "
                      "client version.");
        rsmatch = 0;
        }
      
      this->RenderServerSocket->Receive(&numServerProcs, 1, 1, 8843);
      this->NumberOfRenderServerProcesses = numServerProcs;
      }

    // Continue only if both ids match. Otherwise, the servers
    // will exit anyway.
    if (dsmatch && rsmatch)
      {
      // attempt to initialize render server connection to data server
      this->InitializeRenderServer();
      // Juggle the compositing flag to let server in on the decision
      // whether to allow compositing / rendering on the server.
      // This might better be handled in the render module initialize method.
      // Find out if the server supports compositing.
      vtkPVServerInformation* serverInfo = vtkPVServerInformation::New();
      this->GatherInformation(serverInfo, this->GetProcessModuleID());
      this->ServerInformation->AddInformation(serverInfo);
      serverInfo->Delete();
      serverInfo = NULL;
      
      this->ReturnValue = this->GUIHelper->
        RunGUIStart(this->ArgumentCount, this->Arguments, numServerProcs, myId);
      }
    else
      {
      this->GUIHelper->ExitApplication();
      }
    cout << "Exit Client\n";
    cout.flush();
    }
  else if (myId == 0)
    { // process 0 of Server
    int connectID;
    // Receive the connect id from client
    this->SocketController->Receive(&connectID, 1, 1, 8843);
    int match = 1;
    if ( (this->Options->GetConnectID() != 0) && (connectID != this->Options->GetConnectID()) )
      {
      // If the ids do not match, disable all rmis by setting
      // this->Enabled to 0.
      match = 0;
      vtkErrorMacro("Connection ID mismatch.");
      this->Enabled = 0;
      }
    // Tell the client the result of id check
    this->SocketController->Send(&match, 1, 1, 8843);

    // Do not send the actual number of procs. if the
    // security check failed.
    if (!match)
      {
      numProcs = 0;
      }

    // Receive the client version
    int majorVersion, minorVersion, patchVersion;
    this->SocketController->Receive(&majorVersion, 1, 1, 8843);
    this->SocketController->Receive(&minorVersion, 1, 1, 8843);
    this->SocketController->Receive(&patchVersion, 1, 1, 8843);

    if ( (majorVersion != PARAVIEW_VERSION_MAJOR) ||
         (minorVersion != PARAVIEW_VERSION_MINOR) )
      {
      match = 0;
      vtkErrorMacro("Version mismatch.");
      this->Enabled = 0;
      }
    // Tell the client the result of version check
    this->SocketController->Send(&match, 1, 1, 8843);

    // send the number of server processes as a handshake. 
    this->SocketController->Send(&numProcs, 1, 1, 8843);
    
    //
    this->SocketController->AddRMI(vtkPVClientServerLastResultRMI, (void *)(this),
                                   VTK_PV_CLIENT_SERVER_LAST_RESULT_TAG);
    // for SendMessages
    this->SocketController->AddRMI(vtkPVClientServerSocketRMI, (void *)(this),
                                   VTK_PV_CLIENTSERVER_RMI_TAG);
    this->SocketController->AddRMI(vtkPVClientServerRootRMI, (void *)(this),
                                   VTK_PV_CLIENTSERVER_ROOT_RMI_TAG);
    
    this->Controller->CreateOutputWindow();
    this->SocketController->ProcessRMIs();
    if(this->Options->GetRenderServerMode())
      {
      cout << "Exit Render Server.\n";
      cout.flush();
      }
    else
      {
      cout << "Exit Data Server.\n";
      cout.flush();
      }
    
    
    // Exiting.  Relay the break RMI to otehr processes.
    for (id = 1; id < numProcs; ++id)
      {
      this->Controller->TriggerRMI(id, vtkMultiProcessController::BREAK_RMI_TAG);
      }

    this->SocketController->CloseConnection();
    }
  else
    { // Sattelite processes of server.
    this->Controller->CreateOutputWindow();

    this->Controller->AddRMI(vtkPVSendStreamToClientServerNodeRMI, this,
                             VTK_PV_SLAVE_CLIENTSERVER_RMI_TAG);

    // Process rmis until the application exits.
    this->Controller->ProcessRMIs();    
    // Now we are exiting.
    }  
}


//----------------------------------------------------------------------------
int vtkPVClientServerModule::ShouldWaitForConnection()
{  
  // if client mode then return reverse connection
  if(this->Options->GetClientMode())
    {
    // if in client mode, it should not wait for a connection
    // unless reverse is 1, so just return reverse connection value
    return this->Options->GetReverseConnection();
    }
  // if server mode, then by default wait for the connection
  // so return not getreverseconnection
  return !this->Options->GetReverseConnection();
  
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::OpenConnectionDialog(int* start)
{ 
  if(!this->GUIHelper)
    {
      vtkErrorMacro("GUIHelper must be set, for OpenConnectionDialog to work.");
      return 0;
    }
  return this->GUIHelper->OpenConnectionDialog(start);
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::StartRemoteParaView(vtkSocketCommunicator* comm)
{ 
  char numbuffer[100];
  vtkstd::string runcommand = "eval ${PARAVIEW_SETUP_SCRIPT} ; ";
  // Add mpi
  if ( this->MultiProcessMode == vtkPVClientServerModule::MPI_MODE )
    {
    sprintf(numbuffer, "%d", this->NumberOfProcesses);
    runcommand += "mpirun -np ";
    runcommand += numbuffer;
    runcommand += " ";
    }
  runcommand += "eval ${PARAVIEW_EXECUTABLE} --server --port=";
  sprintf(numbuffer, "%d", this->Options->GetPort());
  runcommand += numbuffer;
  this->RemoteExecution->SetRemoteHost(this->Options->GetHostName());
  if ( this->Options->GetUsername() && this->Options->GetUsername()[0] )
    {
    this->RemoteExecution->SetSSHUser(this->Options->GetUsername());
    }
  else
    {
    this->RemoteExecution->SetSSHUser(0);
    }
  this->RemoteExecution->RunRemoteCommand(runcommand.c_str());
  int cc;
  const int max_try = 10;
  for ( cc = 0; cc < max_try; cc ++ )
    {
#ifdef _WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    if ( this->RemoteExecution->GetResult() != vtkKWRemoteExecute::RUNNING )
      {
      cc = max_try;
      break;
      }
    if (comm->ConnectTo(this->Options->GetHostName(), this->Options->GetPort()))
      {
      break;
      }
    }
  if ( cc < max_try )
    {
    return 1;
    }
  return 0;
}

  

//----------------------------------------------------------------------------
void vtkPVClientServerModule::ConnectToRemote()
{
  // according to the cvs logs this stops a memory leak
  vtkSocketController* dummy = vtkSocketController::New();
  dummy->Initialize();
  dummy->Delete();
  
  // create a socket communicator
  vtkSocketCommunicator *comm = vtkSocketCommunicator::New();
  vtkSocketCommunicator *commRenderServer = vtkSocketCommunicator::New();
  
  // Get the host name from the command line arguments
  vtkCallbackCommand* cb = vtkCallbackCommand::New();
  cb->SetCallback(vtkPVClientServerModule::ErrorCallback);
  cb->SetClientData(this);
  comm->AddObserver(vtkCommand::ErrorEvent, cb);
  cb->Delete();

  // Get the port from the command line arguments
  // Establish connection
  int start = 0;
  if ( this->Options->GetAlwaysSSH() )
    {
    start = 1;
    }
  int port = this->Options->GetPort();
  if(this->Options->GetRenderServerMode() && !this->Options->GetClientMode())
    {
    port = this->Options->GetRenderServerPort();
    }
  cout << "Connect to " << this->Options->GetHostName() << ":" << port << endl;
  while (!comm->ConnectTo(this->Options->GetHostName(), port))
    {  
    // Do not bother trying to start the client if reverse connection is specified.
    // only try the ConnectTo once if it is a server in reverse mode
    if ( ! this->Options->GetClientMode())
      {  
      // This is the "reverse-connection" server condition.  
      // For now just fail if connection is not found.
      vtkErrorMacro("Server error: Could not connect to the client. " 
                    << this->Options->GetHostName() << " " << port);
      comm->Delete();
      commRenderServer->Delete();
      if(this->GUIHelper)
        {
        this->GUIHelper->ExitApplication();
        }
      else
        {
        vtkErrorMacro("No GUIHelper set, can not exit application");
        }
      this->ReturnValue = 1;
      return;
      }
    if ( start)
      {
      start = 0;
      if(this->StartRemoteParaView(comm))
        {
        // if a remote paraview was successfuly started
        // the connection would be made in StartRemoteParaView
        // so break from the connect while loop 
        break;
        }
      continue;
      }
    if (this->Options->GetClientMode())
      {
      if(!this->OpenConnectionDialog(&start))
        {
        // if the user canceled then just quit
        vtkErrorMacro("Client error: Could not connect to the server.");
        comm->Delete();
        commRenderServer->Delete();
        if(this->GUIHelper)
          {
          this->GUIHelper->ExitApplication();
          }  
        else
          {
          vtkErrorMacro("No GUIHelper set, can not exit application");
          }
        this->ReturnValue = 1;
        return;
        }
      }
    }
  
  if(this->Options->GetClientMode() && this->Options->GetRenderServerMode())
    {
    cout << "Connect to " << this->Options->GetRenderServerHostName() << ":" 
         << this->Options->GetRenderServerPort() << endl;
    if(commRenderServer->ConnectTo(this->Options->GetRenderServerHostName(), this->Options->GetRenderServerPort()))
      {
      this->RenderServerSocket = vtkSocketController::New();
      this->RenderServerSocket->SetCommunicator(commRenderServer);
      }
    else
      {
      vtkErrorMacro("Could not connect to render server on host: " << this->Options->GetRenderServerHostName() 
                    << " Port: " << this->Options->GetRenderServerPort()); 
      comm->Delete();
      commRenderServer->Delete();
      if(this->GUIHelper)
        {
        this->GUIHelper->ExitApplication();
        }  
      else
        {
        vtkErrorMacro("No GUIHelper set, can not exit application");
        }
      this->ReturnValue = 1;
      return;
      }
    }

  // if you make it this far, then the connection
  // was made.
  this->SocketController = vtkSocketController::New();
  this->SocketController->SetCommunicator(comm);
  this->ProgressHandler->SetSocketController(this->SocketController);
  comm->Delete();
  commRenderServer->Delete();
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::SetupWaitForConnection()
{
  int needTwoSockets = 0;
  if(this->Options->GetRenderServerMode() && this->Options->GetClientMode())
    {
    needTwoSockets = 1;
    this->RenderServerSocket = vtkSocketController::New();
    }
  
  this->SocketController = vtkSocketController::New();
  this->SocketController->Initialize();
  this->ProgressHandler->SetSocketController(this->SocketController);
  vtkSocketCommunicator* comm = vtkSocketCommunicator::New();
  vtkSocketCommunicator* comm2 = 0;
  int sock2 = 0;
  if(needTwoSockets)
    {
    comm2 = vtkSocketCommunicator::New();
    cout << "Listen on port: " << this->Options->GetRenderServerPort() << endl;
    sock2 = comm2->OpenSocket(this->Options->GetRenderServerPort());
    }
  int port= this->Options->GetPort();
  if((!needTwoSockets && this->Options->GetRenderServerMode()) ||
     (!this->Options->GetClientMode() && this->Options->GetRenderServerMode()))
    {
    port = this->Options->GetRenderServerPort();
    }
  cout << "Listen on port: " << port << endl;
  int sock = comm->OpenSocket(port);
  if ( this->Options->GetClientMode() )
    {
    cout << "Waiting for server..." << endl;
    }
  else
    {
    if( this->Options->GetRenderServerMode() )
      {
      cout << "RenderServer: ";
      }
    cout << "Waiting for client..." << endl;
    }

  // Establish connection
  if (!comm->WaitForConnectionOnSocket(sock))
    {
    vtkErrorMacro("Wait timed out or could not initialize socket.");
    comm->Delete();
    this->ReturnValue = 1;
    return;
    }
  cout << "connected to port " << port << "\n";
    // Establish connection
  if (comm2 && !comm2->WaitForConnectionOnSocket(sock2))
    {
    vtkErrorMacro("Wait timed out or could not initialize render server socket.");
    comm->Delete();
    this->ReturnValue = 1;
    return;
    }
  if(comm2)
    {
    cout << "connected to port " << this->Options->GetRenderServerPort() << "\n";
    }
  
  if ( this->Options->GetClientMode() )
    {
    cout << "Server connected." << endl;
    }
  else
    {
    cout << "Client connected." << endl;
    }
  this->SocketController->SetCommunicator(comm);
  if(comm2)
    {
    this->RenderServerSocket->SetCommunicator(comm2);
    comm2->Delete();
    comm2 = 0;
    }
  comm->Delete();
  comm = 0;
}



//----------------------------------------------------------------------------
void vtkPVClientServerModule::Connect()
{
  int myId = this->Controller->GetLocalProcessId();
 
#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif


  // Do not try to connect sockets on MPI processes other than root.
  if (myId > 0)
    {
    return;
    }

  if ( this->ShouldWaitForConnection())
    {
    this->SetupWaitForConnection();
    }
  else
    {
    this->ConnectToRemote();
    }
}


void vtkPVClientServerModule::InitializeRenderServer()
{
  // if this is not client and using render server, then exit
  if(!(this->Options->GetClientMode() && this->Options->GetRenderServerMode()))
    {
    return;
    }
  int connectingServer;
  int waitingServer;
  int numberOfRenderNodes = 0;
  if(this->Options->GetRenderServerMode() == 1)
    {
    connectingServer = vtkProcessModule::DATA_SERVER;
    waitingServer = vtkProcessModule::RENDER_SERVER;
    }
  else
    {
    waitingServer = vtkProcessModule::DATA_SERVER;
    connectingServer = vtkProcessModule::RENDER_SERVER;
    }
  // Create a vtkMPIMToNSocketConnection object on both the 
  // servers.  This object holds the vtkSocketCommunicator object
  // for each machine and makes the connections
  vtkClientServerID id = this->NewStreamObject("vtkMPIMToNSocketConnection");
  this->MPIMToNSocketConnectionID = id;
  this->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);

  vtkMPIMToNSocketConnectionPortInformation* info 
    = vtkMPIMToNSocketConnectionPortInformation::New();
  // if the data server is going to wait for the render server
  // then we have to tell the data server how many connections to make
  // if the render server is waiting, it already knows how many to make
  if(this->Options->GetRenderServerMode() == 2)
    {
      // get the number of processes on the render server
      this->GatherInformationRenderServer(info, id);
      // Set the number of connections on the server to be equal to
      // the number of connections on the render server
      numberOfRenderNodes = info->GetNumberOfConnections();
      this->GetStream() 
        << vtkClientServerStream::Invoke << id 
        << "SetNumberOfConnections" << numberOfRenderNodes
        << vtkClientServerStream::End;
      this->SendStream(vtkProcessModule::DATA_SERVER);
    }
  
  // now initilaize the waiting server and have it set up the connections
  this->GetStream()  
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID() << "GetRenderNodePort" << vtkClientServerStream::End;
  this->GetStream()
    << vtkClientServerStream::Invoke << id << "SetPortNumber"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  this->GetStream()  
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID() << "GetMachinesFileName" << vtkClientServerStream::End;
  this->GetStream()
    << vtkClientServerStream::Invoke << id << "SetMachinesFileName"
    << vtkClientServerStream::LastResult
    << vtkClientServerStream::End;
  this->GetStream() 
    << vtkClientServerStream::Invoke << id << "SetupWaitForConnection"
    << vtkClientServerStream::End;
  this->SendStream(waitingServer);

  // Get the information about the connection after the call to SetupWaitForConnection
  if(this->Options->GetRenderServerMode() == 1)
    {
      this->GatherInformationRenderServer(info, id);
      numberOfRenderNodes = info->GetNumberOfConnections();
    }
  else
    {
      this->GatherInformation(info, id);
    }
   // let the connecting server know how many render nodes 
  this->GetStream() 
    << vtkClientServerStream::Invoke << id 
    << "SetNumberOfConnections" << numberOfRenderNodes
    << vtkClientServerStream::End;

  // set up host/port information for the connecting
  // server so it will know what machines to connect to
  for(int i=0; i < numberOfRenderNodes; ++i)
    {
    this->GetStream() 
      << vtkClientServerStream::Invoke << id 
      << "SetPortInformation" 
      << static_cast<unsigned int>(i)
      << info->GetProcessPort(i)
      << info->GetProcessHostName(i)
      << vtkClientServerStream::End;
    } 
  this->SendStream(connectingServer);
  // all should be ready now to wait and connect

  // tell the waiting server to wait for the connections
  this->GetStream() 
    << vtkClientServerStream::Invoke << id << "WaitForConnection"
    << vtkClientServerStream::End;
  this->SendStream(waitingServer);

  // tell the connecting server to make the connections
  this->GetStream() 
    << vtkClientServerStream::Invoke << id << "Connect"
    << vtkClientServerStream::End;
  this->SendStream(connectingServer);
  info->Delete();
}

//----------------------------------------------------------------------------
// same as the MPI start.
int vtkPVClientServerModule::Start(int argc, char **argv)
{
  // First we initialize the mpi controller.
  // We are assuming that the client has been started with one process
  // and is linked with MPI.
  this->ArgumentCount = argc;
  this->Arguments = argv;
#ifdef VTK_USE_MPI
  this->Controller = vtkMPIController::New();
  vtkMultiProcessController::SetGlobalController(this->Controller);
  this->Controller->Initialize(&argc, &argv, 1);
  this->Controller->SetSingleMethod(vtkPVClientServerInit, (void *)(this));
  this->Controller->SingleMethodExecute();
  this->Controller->Finalize(1);
#else
  this->Controller = vtkDummyController::New();
  // This would be simpler if vtkDummyController::SingleMethodExecute
  // did its job correctly.
  vtkMultiProcessController::SetGlobalController(this->Controller);
  vtkPVClientServerInit(this->Controller, (void*)this);
#endif

  return this->ReturnValue;
}




//----------------------------------------------------------------------------
// Only called by the client.
void vtkPVClientServerModule::Exit()
{
  if ( ! this->Options->GetClientMode())
    {
    vtkErrorMacro("Not expecting server to call Exit.");
    return;
    }

  if (this->MPIMToNSocketConnectionID.ID)
    {    
    this->DeleteStreamObject(this->MPIMToNSocketConnectionID);
    this->SendStream(vtkProcessModule::RENDER_SERVER|vtkProcessModule::DATA_SERVER);
    this->MPIMToNSocketConnectionID.ID = 0;
    }
 

  // If we are being called because a connection was not established,
  // we don't need to cleanup the connection.
  if(this->SocketController)
    {
    this->SocketController->TriggerRMI(
      1, vtkMultiProcessController::BREAK_RMI_TAG);
#ifdef _WIN32
    // if you start client server mode with mpi the server
    // never gets the break rmi unless this sleep is here.
    Sleep(1000);
#else
    sleep(1);
#endif
    }
  if(this->RenderServerSocket)
    {
    this->RenderServerSocket->TriggerRMI(
      1, vtkMultiProcessController::BREAK_RMI_TAG);
#ifdef _WIN32
    // if you start client server mode with mpi the server
    // never gets the break rmi unless this sleep is here.
    Sleep(1000);
#else
    sleep(1);
#endif
    }
  // Break RMI for MPI controller is in Init method.
}





//----------------------------------------------------------------------------
// I do not think this method is really necessary.
// Eliminate it if possible. !!!!!!!!
int vtkPVClientServerModule::GetPartitionId()
{
  if (this->Options->GetClientMode())
    {
    return -1;
    }
  if (this->Controller)
    {
    return this->Controller->GetLocalProcessId();
    }
  return 0;
}

//----------------------------------------------------------------------------
// This is used to determine which filters are available.
int vtkPVClientServerModule::GetNumberOfPartitions()
{
  if (this->Options->GetClientMode())
    {
    return this->NumberOfServerProcesses;
    }

  if (this->Controller)
    {
    return this->Controller->GetNumberOfProcesses();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::GatherInformation(vtkPVInformation* info,
                                                vtkClientServerID id)
{
  // Just a simple way of passing the information object to the next method.
  this->TemporaryInformation = info;

  // Gather on the server.
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "GatherInformationInternal" << info->GetClassName() << id
    << vtkClientServerStream::End;
  this->SendStream(vtkProcessModule::DATA_SERVER);

  // Gather on the client.
  this->GatherInformationInternal(NULL, NULL);
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::GatherInformationRenderServer(vtkPVInformation* info,
                                                            vtkClientServerID id)
{
  // Just a simple way of passing the information object to the next method.
  this->TemporaryInformation = info;

  // Gather on the server.
  this->GetStream()
    << vtkClientServerStream::Invoke
    << this->GetProcessModuleID()
    << "GatherInformationInternal" << info->GetClassName() << id
    << vtkClientServerStream::End;
  this->SendStream(vtkProcessModule::RENDER_SERVER);
  this->GatherRenderServer = 1;
  // Gather on the client.
  this->GatherInformationInternal(NULL, NULL);
  this->GatherRenderServer = 0; 
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
// This method is broadcast to all processes.
void
vtkPVClientServerModule::GatherInformationInternal(const char* infoClassName,
                                                   vtkObject* object)
{
  vtkClientServerStream css;
  
  if(this->Options->GetClientMode())
    {
    vtkSocketController* controller = this->SocketController;
    if(this->GatherRenderServer)
      {
      controller = this->RenderServerSocket;
      }
    
    // Client just receives information from the server.
    int length;
    controller->Receive(&length, 1, 1, 398798);
    if (length < 0)
      { // I got this condition when the server aborted.
      vtkErrorMacro("Could not gather information.");
      return;
      }
    unsigned char* data = new unsigned char[length];
    controller->Receive(data, length, 1, 398799);
    css.SetData(data, length);
    this->TemporaryInformation->CopyFromStream(&css);
    delete [] data;
    return;
    }

  // We are one of the server nodes.
  int myId = this->Controller->GetLocalProcessId();
  if(!object)
    {
    vtkErrorMacro("Deci id must be wrong.");
    return;
    }

  // Create a temporary information object to hold the input object's
  // information.
  vtkObject* o = vtkInstantiator::CreateInstance(infoClassName);
  if(!o)
    {
    vtkErrorMacro("Could not instantiate object " << infoClassName);
    return;
    }
  vtkPVInformation* tempInfo1 = vtkPVInformation::SafeDownCast(o);
  o = 0;

  // Nodes other than 0 just send their information.
  if(myId != 0)
    {
    if(tempInfo1->GetRootOnly())
      {
      // Root-only and we are not the root.  Do nothing.
      tempInfo1->Delete();
      return;
      }
    tempInfo1->CopyFromObject(object);
    tempInfo1->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = static_cast<int>(length);
    this->Controller->Send(&len, 1, 0, 498798);
    this->Controller->Send(const_cast<unsigned char*>(data),
                           length, 0, 498799);
    tempInfo1->Delete();
    return;
    }

  // This is node 0.  First get our own information.
  tempInfo1->CopyFromObject(object);

  if(!tempInfo1->GetRootOnly())
    {
    // Create another temporary information object in which to
    // receive information from other nodes.
    o = vtkInstantiator::CreateInstance(infoClassName);
    vtkPVInformation* tempInfo2 = vtkPVInformation::SafeDownCast(o);
    o = NULL;

    // Merge information from other nodes.
    int numProcs = this->Controller->GetNumberOfProcesses();
    int idx;
    for (idx = 1; idx < numProcs; ++idx)
      {
      int length;
      this->Controller->Receive(&length, 1, idx, 498798);
      unsigned char* data = new unsigned char[length];
      this->Controller->Receive(data, length, idx, 498799);
      css.SetData(data, length);
      tempInfo2->CopyFromStream(&css);
      tempInfo1->AddInformation(tempInfo2);
      delete [] data;
      }
    tempInfo2->Delete();
    }

  // Send final information to client over socket connection.
  size_t length;
  const unsigned char* data;
  tempInfo1->CopyToStream(&css);
  css.GetData(&data, &length);
  int len = static_cast<int>(length);
  this->SocketController->Send(&len, 1, 1, 398798);
  this->SocketController->Send(const_cast<unsigned char*>(data),
                               length, 1, 398799);
  tempInfo1->Delete();
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
  os << indent << "SocketController: " << this->SocketController << endl;;
  os << indent << "RenderServerSocket: " << this->RenderServerSocket << endl;;
  os << indent << "NumberOfProcesses: " << this->NumberOfProcesses << endl;
  os << indent << "MultiProcessMode: " << this->MultiProcessMode << endl;
  os << indent << "NumberOfServerProcesses: " << this->NumberOfServerProcesses << endl;
  os << indent << "Enabled: " << this->Enabled << endl;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::GetDirectoryListing(const char* dir,
                                                 vtkStringList* dirs,
                                                 vtkStringList* files,
                                                 int save)
{
  if(this->Options->GetClientMode())
    {
    return this->Superclass::GetDirectoryListing(dir, dirs, files, save);
    }
  else
    {
    dirs->RemoveAllItems();
    files->RemoveAllItems();
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::ProcessMessage(unsigned char* msg, size_t len)
{
  this->Interpreter->ProcessStream(msg, len);
}

  
//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVClientServerModule::GetLastDataServerResult()
{ 
  if(!this->Options->GetClientMode())
    {
    vtkErrorMacro("GetLastDataServerResult() should not be called on the server.");
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule::GetLastDataServerResult() should not be called on the server."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }
  int length;
  this->SocketController->TriggerRMI(1, "", VTK_PV_CLIENT_SERVER_LAST_RESULT_TAG);
  this->SocketController->Receive(&length, 1, 1, VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if(length <= 0)
    {
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule::GetLastDataServerResult() received no data from the server."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }
  unsigned char* result = new unsigned char[length];
  this->SocketController->Receive((char*)result, length, 1, VTK_PV_ROOT_RESULT_TAG);
  this->LastServerResultStream->SetData(result, length);
  delete [] result;
  return *this->LastServerResultStream;
}

  
//----------------------------------------------------------------------------
const vtkClientServerStream& vtkPVClientServerModule::GetLastRenderServerResult()
{
  if(!this->Options->GetClientMode())
    {
    vtkErrorMacro("GetLastRenderServerResult() should not be called on the server.");
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule::GetLastRenderServerResult() should not be called on the server."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }
  int length;
  this->RenderServerSocket->TriggerRMI(1, "", VTK_PV_CLIENT_SERVER_LAST_RESULT_TAG);
  this->RenderServerSocket->Receive(&length, 1, 1, VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if(length <= 0)
    {
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule::GetLastRenderServerResult() received no data from the server."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }
  unsigned char* result = new unsigned char[length];
  this->RenderServerSocket->Receive((char*)result, length, 1, VTK_PV_ROOT_RESULT_TAG);
  this->LastServerResultStream->SetData(result, length);
  delete [] result;
  return *this->LastServerResultStream;
}

  
vtkTypeUInt32 vtkPVClientServerModule::CreateSendFlag(vtkTypeUInt32 servers)
{  
  vtkTypeUInt32 sendflag = 0;  

  // for RenderServer mode keep the bit vector the same
  // because all servers are different processes
  if(this->Options->GetRenderServerMode())
    {
    return servers;
    }
  // for data server only mode convert all render server calls
  // into data server calls
  if(servers & CLIENT)
    {
    sendflag |= CLIENT;
    }
  if(servers & RENDER_SERVER)
    {
    sendflag |= DATA_SERVER;
    }
  if(servers & RENDER_SERVER_ROOT)
    {
    sendflag |= DATA_SERVER_ROOT;
    }
  if(servers & DATA_SERVER)
    {
    sendflag |= DATA_SERVER;
    }
  if(servers & DATA_SERVER_ROOT)
    {
    sendflag |= DATA_SERVER_ROOT;
    }
  return sendflag;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::SendStreamToClient(vtkClientServerStream& stream)
{ 
  if(!this->Options->GetClientMode())
    {
    vtkErrorMacro("Attempt to call SendStreamToClient on server node.");
    return -1;
    }

  // Just process the stream locally.
  this->Interpreter->ProcessStream(stream);
  return 0;
}


//----------------------------------------------------------------------------
int vtkPVClientServerModule::SendStreamToDataServer(vtkClientServerStream& stream)
{ 
  if(!this->Options->GetClientMode())
    {
    vtkErrorMacro("Attempt to call SendStreamToDataServer on server node.");
    return -1;
    }
  if (stream.GetNumberOfMessages() < 1)
    {
    return 1;
    }
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  this->SocketController->TriggerRMI(1, (void*)(data), len,
                                     VTK_PV_CLIENTSERVER_RMI_TAG);
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::SendStreamToDataServerRoot(vtkClientServerStream& stream)
{
  if(!this->Options->GetClientMode())
    {
    vtkErrorMacro("Attempt to call SendStreamToDataServerRoot on server node.");
    return -1;
    }
  if (stream.GetNumberOfMessages() < 1)
    {
    return 0;
    }
  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  this->SocketController->TriggerRMI(1, (void*)(data), len,
                                     VTK_PV_CLIENTSERVER_ROOT_RMI_TAG);
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::SendStreamToRenderServer(vtkClientServerStream& stream)
{  
  if (stream.GetNumberOfMessages() < 1)
    {
    return 0;
    }
  if(!this->Options->GetRenderServerMode())
    {
    vtkErrorMacro("Attempt to call SendStreamToRenderServer when not in renderserver mode.");
    return -1;
    }

  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  this->RenderServerSocket->TriggerRMI(1, (void*)(data), len,
                                       VTK_PV_CLIENTSERVER_RMI_TAG);
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::SendStreamToRenderServerRoot(vtkClientServerStream& stream)
{
  if (stream.GetNumberOfMessages() < 1)
    {
    return 0;
    }
  if(!this->Options->GetRenderServerMode())
    {
    vtkErrorMacro("Attempt to call SendStreamToRenderServerRoot when not in renderserver mode.");
    return -1;
    }

  const unsigned char* data;
  size_t len;
  stream.GetData(&data, &len);
  this->RenderServerSocket->TriggerRMI(1, (void*)(data), len,
                                       VTK_PV_CLIENTSERVER_ROOT_RMI_TAG);
  return 0;
}


//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendLastClientServerResult()
{
  const unsigned char* data;
  size_t length = 0;
  this->Interpreter->GetLastResult().GetData(&data, &length);
  int len = static_cast<int>(length);
  this->GetSocketController()->Send(&len, 1, 1,
                                    VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if(length > 0)
    {
    this->GetSocketController()->Send((char*)(data), length, 1,
                                      VTK_PV_ROOT_RESULT_TAG);
    }
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::LoadModuleInternal(const char* name,
                                                const char* directory)
{
  // Try to load the module on the local process.
  int localResult = this->Superclass::LoadModuleInternal(name, directory);

  // Make sure we have a communicator.
#ifdef VTK_USE_MPI
  vtkMPICommunicator* communicator = vtkMPICommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if(!communicator)
    {
    return 0;
    }

  // Gather load results to process 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int myid = this->Controller->GetLocalProcessId();
  int* results = new int[numProcs];
  communicator->Gather(&localResult, results, numProcs, 0);

  int globalResult = 1;
  if(myid == 0)
    {
    for(int i=0; i < numProcs; ++i)
      {
      if(!results[i])
        {
        globalResult = 0;
        }
      }
    }

  delete [] results;

  return globalResult;
#else
  return localResult;
#endif
}

//----------------------------------------------------------------------------
// This method leaks memory.  It is a quick and dirty way to set different 
// DISPLAY environment variables on the render server.  I think the string 
// cannot be deleted until paraview exits.  The var should have the form:
// "DISPLAY=amber1"
void vtkPVClientServerModule::SetProcessEnvironmentVariable(int processId,
                                                            const char* var)
{
  vtkMultiProcessController* controller = this->GetController();
  if (controller && controller->GetLocalProcessId() == processId)
    {
    this->Superclass::SetProcessEnvironmentVariable(processId, var);
    }
}
