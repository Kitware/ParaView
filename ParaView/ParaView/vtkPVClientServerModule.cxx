/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClientServerModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVClientServerModule.h"

#include "vtkCharArray.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkDummyController.h"
#include "vtkFloatArray.h"
#include "vtkInstantiator.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWMessageDialog.h"
#include "vtkLongArray.h"
#include "vtkMapper.h"
#include "vtkMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPVApplication.h"
#include "vtkPVConfig.h"
#include "vtkPVInformation.h"
#include "vtkPVServerFileDialog.h"
#include "vtkPVWindow.h"
#include "vtkShortArray.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"
#include "vtkSource.h"
#include "vtkString.h"
#include "vtkString.h"
#include "vtkStringList.h"
#include "vtkStringList.h"
#include "vtkTclUtil.h"
#include "vtkTclUtil.h"
#include "vtkToolkits.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"

#include "vtkCallbackCommand.h"
#include "vtkKWRemoteExecute.h"
#include "vtkPVConnectDialog.h"
#ifndef _WIN32
#include <unistd.h>
#endif
#include <vtkstd/string>

#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkPVPart.h"
#include "vtkPVPartDisplay.h"

#include "vtkClientServerStream.h"
#include "vtkClientServerInterpreter.h"

int vtkStringListCommand(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);

#define VTK_PV_CLIENTSERVER_RMI_TAG          938531
#define VTK_PV_CLIENTSERVER_ROOT_RMI_TAG     938532

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
  vtkMultiProcessController* controler = self->GetController();
  for(int i = 1; i < controler->GetNumberOfProcesses(); ++i)
    {
    controler->TriggerRMI(i, remoteArg, remoteArgLength, VTK_PV_CLIENTSERVER_RMI_TAG);
    }
  vtkPVClientServerMPIRMI(localArg, remoteArg, remoteArgLength, remoteProcessId);
}


//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVClientServerRootRMI(void *localArg, void *remoteArg,
                              int remoteArgLength,
                              int remoteProcessId)
{
  vtkPVClientServerMPIRMI(localArg, remoteArg, remoteArgLength, remoteProcessId);
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVClientServerModule);
vtkCxxRevisionMacro(vtkPVClientServerModule, "1.51");

int vtkPVClientServerModuleCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVClientServerModule::vtkPVClientServerModule()
{
  this->LastServerResultStream = new vtkClientServerStream;
  this->Controller = NULL;
  this->SocketController = NULL;
  this->ClientMode = 1;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;

  this->Hostname = 0;
  this->Username = 0;
  this->Port = 0;
  this->MultiProcessMode = vtkPVClientServerModule::SINGLE_PROCESS_MODE;
  this->NumberOfProcesses = 2;

  this->RemoteExecution = vtkKWRemoteExecute::New();
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

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;

  this->SetHostname(0);
  this->SetUsername(0);

  this->RemoteExecution->Delete();
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
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int id;

  this->Connect();
  if (this->ReturnValue)
    { // Could not connect.
    return;
    } 

  if (this->ClientMode)
    {
    this->Interpreter->SetLogFile("pvClient.out");

    // Receive as the hand shake the number of processes available.
    int numServerProcs = 0;
    this->SocketController->Receive(&numServerProcs, 1, 1, 8843);
    this->NumberOfServerProcesses = numServerProcs;
   
    // Start the application (UI). 
    // For SGI pipe option.
    pvApp->SetNumberOfPipes(numServerProcs);
    
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
    pvApp->SetupTrapsForSignals(myId);   
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

    if (pvApp->GetStartGUI())
      {
      pvApp->Script("wm withdraw .");
      pvApp->Start(this->ArgumentCount,this->Arguments);
      }
    else
      {
      pvApp->Exit();
      }

    // Exiting:  CLean up.
    this->ReturnValue = pvApp->GetExitStatus();
    }
  else if (myId == 0)
    { // process 0 of Server
    this->Interpreter->SetLogFile("pvServer.out");

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
    
    // Exiting.  Relay the break RMI to otehr processes.
    for (id = 1; id < numProcs; ++id)
      {
      this->Controller->TriggerRMI(id, vtkMultiProcessController::BREAK_RMI_TAG);
      }
    }
  else
    { // Sattelite processes of server.
    this->Controller->CreateOutputWindow();
    // Process rmis until the application exits.
    this->Controller->ProcessRMIs();    
    // Now we are exiting.
    }
}


//----------------------------------------------------------------------------
void vtkPVClientServerModule::Connect()
{
  int myId = this->Controller->GetLocalProcessId();
  vtkPVApplication *pvApp = this->GetPVApplication();
  int waitForConnection;
 
#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  this->ClientMode = pvApp->GetClientMode();

  // Do not try to connect sockets on MPI processes other than root.
  if (myId > 0)
    {
    return;
    }

  // I want to be able to switch which process waits, and which connects.
  // Just a hard way of doing an exclusive or.
  if (this->ClientMode)
    {
    if (pvApp->GetReverseConnection())
      {
      waitForConnection = 1;
      }
    else
      {
      waitForConnection = 0;
      }
    }
  else
    {
    if (pvApp->GetReverseConnection())
      {
      waitForConnection = 0;
      }
    else
      {
      waitForConnection = 1;
      }
    }

  if (waitForConnection == 0)
    {
    vtkSocketController* dummy = vtkSocketController::New();
    dummy->Initialize();
    dummy->Delete();
    vtkSocketCommunicator *comm = vtkSocketCommunicator::New();

    // Get the host name from the command line arguments
    this->SetHostname(pvApp->GetHostName());
    this->SetUsername(pvApp->GetUsername());
    vtkCallbackCommand* cb = vtkCallbackCommand::New();
    cb->SetCallback(vtkPVClientServerModule::ErrorCallback);
    cb->SetClientData(this);
    comm->AddObserver(vtkCommand::ErrorEvent, cb);
    cb->Delete();

    // Get the port from the command line arguments
    this->Port = pvApp->GetPort();
    // Establish connection
    int start = 0;
    if ( pvApp->GetAlwaysSSH() )
      {
      start = 1;
      }
    while (!comm->ConnectTo(this->Hostname, this->Port))
      {  // Do not bother trying to start the client if reverse connection is specified.
      if ( ! this->ClientMode)
        {  // all the following stuff is for starting the server automatically.
        // This is the "reverse-connection" condition.  
        // For now just fail if connection is not found.
        vtkErrorMacro("Server error: Could not connect to the client. " 
                      << this->Hostname << " " << this->Port);
        comm->Delete();
        pvApp->Exit();
        this->ReturnValue = 1;
        return;
        }
      if ( start)
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
        sprintf(numbuffer, "%d", this->Port);
        runcommand += numbuffer;
        this->RemoteExecution->SetRemoteHost(this->Hostname);
        if ( this->Username && this->Username[0] )
          {
          this->RemoteExecution->SetSSHUser(this->Username);
          }
        else
          {
          this->RemoteExecution->SetSSHUser(0);
          }
        this->RemoteExecution->RunRemoteCommand(runcommand.c_str());
        start = 0;
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
          if (comm->ConnectTo(this->Hostname, this->Port))
            {
            break;
            }
          }
        if ( cc < max_try )
          {
          break;
          }
        continue;
        }
      if (this->ClientMode)
        {
        this->Script("wm withdraw .");
        vtkPVConnectDialog* dialog = 
          vtkPVConnectDialog::New();
        dialog->SetHostname(this->Hostname);
        dialog->SetSSHUser(this->Username);
        dialog->SetPort(this->Port);
        dialog->SetNumberOfProcesses(this->NumberOfProcesses);
        dialog->SetMultiProcessMode(this->MultiProcessMode);
        dialog->Create(this->GetPVApplication(), 0);
        int res = dialog->Invoke();
        if ( res )
          {
          this->SetHostname(dialog->GetHostName());
          this->SetUsername(dialog->GetSSHUser());
          this->Port = dialog->GetPort();
          this->NumberOfProcesses = dialog->GetNumberOfProcesses();
          this->MultiProcessMode = dialog->GetMultiProcessMode();
          start = 1;
          }
        dialog->Delete();

        if ( !res )
          {
          vtkErrorMacro("Client error: Could not connect to the server.");
          comm->Delete();
          pvApp->Exit();
          this->ReturnValue = 1;
          return;
          }
        }
      }
    this->SocketController = vtkSocketController::New();
    this->SocketController->SetCommunicator(comm);
    comm->Delete();
    }
  else
    {
    this->SocketController = vtkSocketController::New();
    this->SocketController->Initialize();
    vtkSocketCommunicator* comm = vtkSocketCommunicator::New();

    int port= pvApp->GetPort();

    // Establish connection
   if (!comm->WaitForConnection(port))
      {
      vtkErrorMacro("Wait timed out or could not initialize socket.");
      comm->Delete();
      this->ReturnValue = 1;
      return;
      }
    this->SocketController->SetCommunicator(comm);
    comm->Delete();
    comm = NULL;
    }
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
  this->Controller->Finalize();
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
  if ( ! this->ClientMode)
    {
    vtkErrorMacro("Not expecting server to call Exit.");
    return;
    }

  // If we are being called because a connection was not established,
  // we don't need to cleanup the connection.
  if(this->SocketController)
    {
    this->SocketController->TriggerRMI(
      1, vtkMultiProcessController::BREAK_RMI_TAG);
    }
  // Break RMI for MPI controller is in Init method.
}





//----------------------------------------------------------------------------
// I do not think this method is really necessary.
// Eliminate it if possible. !!!!!!!!
int vtkPVClientServerModule::GetPartitionId()
{
  if (this->ClientMode)
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
  if (this->ClientMode)
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
    << this->GetApplicationID() << "GetProcessModule"
    << vtkClientServerStream::End
    << vtkClientServerStream::Invoke
    << vtkClientServerStream::LastResult
    << "GatherInformationInternal" << info->GetClassName() << id
    << vtkClientServerStream::End;
  this->SendStreamToServer();

  // Gather on the client.
  this->GatherInformationInternal(NULL, NULL);
  this->TemporaryInformation = NULL;
}

//----------------------------------------------------------------------------
// This method is broadcast to all processes.
void
vtkPVClientServerModule::GatherInformationInternal(const char* infoClassName,
                                                   vtkObject* object)
{
  vtkClientServerStream css;

  if(this->GetPVApplication()->GetClientMode())
    {
    // Client just receives information from the server.
    int length;
    this->SocketController->Receive(&length, 1, 1, 398798);
    unsigned char* data = new unsigned char[length];
    this->SocketController->Receive(data, length, 1, 398799);
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
  os << indent << "ClientMode: " << this->ClientMode << endl;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::GetDirectoryListing(const char* dir,
                                                 vtkStringList* dirs,
                                                 vtkStringList* files,
                                                 int save)
{
  if(this->ClientMode)
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
vtkKWLoadSaveDialog* vtkPVClientServerModule::NewLoadSaveDialog()
{
  vtkPVServerFileDialog* dialog = vtkPVServerFileDialog::New();
  dialog->SetMasterWindow(this->GetPVApplication()->GetMainWindow());
  return dialog;
}

void vtkPVClientServerModule::ProcessMessage(unsigned char* msg, size_t len)
{
  this->Interpreter->ProcessStream(msg, len);
}

const vtkClientServerStream& vtkPVClientServerModule::GetLastClientResult()
{
  return this->Interpreter->GetLastResult();
}

const vtkClientServerStream& vtkPVClientServerModule::GetLastServerResult()
{
  if(!this->Application)
    {
    vtkErrorMacro("Missing application object.");
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule missing Application object."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }

  if(!this->ClientMode)
    {
    vtkErrorMacro("GetLastServerResult() should not be called on the server.");
    this->LastServerResultStream->Reset();
    *this->LastServerResultStream
      << vtkClientServerStream::Error
      << "vtkPVClientServerModule::GetLastServerResult() should not be called on the server."
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
      << "vtkPVClientServerModule::GetLastServerResult() received no data from the server."
      << vtkClientServerStream::End;
    return *this->LastServerResultStream;
    }
  unsigned char* result = new unsigned char[length];
  this->SocketController->Receive((char*)result, length, 1, VTK_PV_ROOT_RESULT_TAG);
  this->LastServerResultStream->SetData(result, length);
  delete [] result;
  return *this->LastServerResultStream;
}

void vtkPVClientServerModule::SendStreamToClient()
{
  if(!this->ClientMode)
    {
    vtkErrorMacro("Attempt to call SendStreamToClient on server node.");
    return;
    }
  // Just process the stream locally.
  this->Interpreter->ProcessStream(*this->ClientServerStream);
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
// This sends the current stream to the server
void vtkPVClientServerModule::SendStreamToServer()
{
  if(!this->ClientMode)
    {
    vtkErrorMacro("Attempt to call SendStreamToServer on server node.");
    return;
    }
  this->SendStreamToServerInternal();
  this->ClientServerStream->Reset();
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendStreamToServerRoot()
{
  this->SendStreamToServerRootInternal();
  this->ClientServerStream->Reset();
}

void vtkPVClientServerModule::SendStreamToClientAndServer()
{
  if(!this->ClientMode)
    {
    vtkErrorMacro("Attempt to call SendStreamToClientAndServer "
                  "on server node.");
    return;
    }

  // Send to server first, then to client.
  this->SendStreamToServerInternal();
  this->SendStreamToClient();
}

void vtkPVClientServerModule::SendStreamToClientAndServerRoot()
{
  if(!this->ClientMode)
    {
    vtkErrorMacro("Attempt to call SendStreamToClientAndServerRoot "
                  "on server node.");
    return;
    }

  // Send to server root first, then to client.
  this->SendStreamToServerRootInternal();
  this->SendStreamToClient();
}

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
void vtkPVClientServerModule::SendStreamToServerInternal()
{
  const unsigned char* data;
  size_t len;
  this->ClientServerStream->GetData(&data, &len);
  this->SocketController->TriggerRMI(1, (void*)(data), len,
                                     VTK_PV_CLIENTSERVER_RMI_TAG);
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendStreamToServerRootInternal()
{
  const unsigned char* data;
  size_t len;
  this->ClientServerStream->GetData(&data, &len);
  this->SocketController->TriggerRMI(1, (void*)(data), len,
                                     VTK_PV_CLIENTSERVER_ROOT_RMI_TAG);
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::LoadModuleInternal(const char* name)
{
  // Try to load the module on the local process.
  int localResult = this->Interpreter->Load(name);

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
