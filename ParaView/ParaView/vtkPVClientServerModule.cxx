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

int vtkStringListCommand(ClientData cd, Tcl_Interp *interp,
                         int argc, char *argv[]);

#define VTK_PV_BROADCAST_SCRIPT_RMI_TAG      838422
#define VTK_PV_REMOTE_SCRIPT_RMI_TAG         838427
#define VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG 838428
#define VTK_PV_SATELLITE_SCRIPT              838431

#define VTK_PV_CLIENTSERVER_RMI_TAG          938531

#define VTK_PV_ROOT_SCRIPT_RMI_TAG           838485
#define VTK_PV_ROOT_RESULT_RMI_TAG           838486
#define VTK_PV_ROOT_RESULT_LENGTH_TAG        838487
#define VTK_PV_ROOT_RESULT_TAG               838488

#define VTK_PV_SEND_DATA_OBJECT_TAG          838489
#define VTK_PV_DATA_OBJECT_TAG               923857

//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVRootScript(void *localArg, void *remoteArg, 
                            int vtkNotUsed(remoteArgLength),
                            int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  self->Script((char*)remoteArg);

  // Save the result.
  char* result = self->GetApplication()->GetMainInterp()->result;
  self->SetRootResult(result);
}

//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVRootResult(void *localArg, void* , 
                     int vtkNotUsed(remoteArgLength),
                     int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  const char* result = self->vtkPVProcessModule::GetRootResult();
  int length = static_cast<int>(strlen(result)) + 1;

  self->GetSocketController()->Send(&length, 1, 1, 
                                    VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if (length > 0)
    {
    self->GetSocketController()->Send((char*)(result), length, 1,
                                      VTK_PV_ROOT_RESULT_TAG);  
    }
}



//----------------------------------------------------------------------------
// This RMI is only on MPI controller (procs 1->num-1) of server.
void vtkPVServerSlaveScript(void *localArg, void *remoteArg, 
                            int vtkNotUsed(remoteArgLength),
                            int vtkNotUsed(remoteProcessId))
{
  vtkPVApplication *pvApp = (vtkPVApplication *)(localArg);
  //cerr << " ++++ SlaveScript: " << ((char*)remoteArg) << endl;  
  pvApp->SimpleScript((char*)remoteArg);
}


//----------------------------------------------------------------------------
// RMI only on server 0 socket controller.
void vtkPVBroadcastScript(void *localArg, void *remoteArg, 
                          int vtkNotUsed(remoteArgLength),
                          int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule*)(localArg);
  self->BroadcastScriptRMI((const char*)remoteArg);
}


//----------------------------------------------------------------------------
// RMI only on server 0 socket controller.
// Destination process will be received separately.
// This is used only for debugging.
void vtkPVRelayRemoteScript(void *localArg, void *remoteArg, 
                       int vtkNotUsed(remoteArgLength),
                       int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule*)(localArg);
  self->RelayScriptRMI((const char*)remoteArg);
}

//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVSendPolyData(void* arg, void*, int, int)
{
  vtkPVClientServerModule* self = static_cast<vtkPVClientServerModule*>(arg);
  
  // Get the length of the Tcl object name we are about to receive.
  int length = 0;
  self->GetSocketController()->Receive(&length, 1, 1, VTK_PV_DATA_OBJECT_TAG);
  
  // Allocate space and receive the Tcl object name.
  char* tclName = new char[length+1];
  self->GetSocketController()->Receive(tclName, length, 1, VTK_PV_DATA_OBJECT_TAG);
  tclName[length] = '\0';
  
  // Get the object from the local process.
  vtkPolyData* obj = vtkPolyData::New();
  int retVal = self->vtkPVProcessModule::ReceiveRootPolyData(tclName, obj);
  delete [] tclName;
  
  // Send success/failure flag and the object itself.
  if(retVal)
    {
    int success = 1;
    self->GetSocketController()->Send(&success, 1, 1, VTK_PV_DATA_OBJECT_TAG);
    self->GetSocketController()->Send(obj, 1, VTK_PV_DATA_OBJECT_TAG);
    }
  else
    {
    int failure = 0;
    self->GetSocketController()->Send(&failure, 1, 1, VTK_PV_DATA_OBJECT_TAG);
    }
  obj->Delete();
}


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVClientServerModule);
vtkCxxRevisionMacro(vtkPVClientServerModule, "1.48");

int vtkPVClientServerModuleCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVClientServerModule::vtkPVClientServerModule()
{
  this->ClientServerInterpreter = 0;
  this->ClientServerStream = 0;

  this->Controller = NULL;
  this->SocketController = NULL;
  this->ClientMode = 1;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
  this->RootResult = 0;

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
  this->SetRootResult(0);

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
// Declare the initialization function as external
// this is defined in the PackageInit file
extern void Vtkparaviewcswrapped_Initialize(vtkClientServerInterpreter *arlu);

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

    // send the number of server processes as a handshake.
    this->SocketController->Send(&numProcs, 1, 1, 8843);

    // For root script: Execute script only on process 0 of server.
    this->SocketController->AddRMI(vtkPVRootScript, (void *)(this), 
                                   VTK_PV_ROOT_SCRIPT_RMI_TAG);
    // For root script: Return result back to client.
    this->SocketController->AddRMI(vtkPVRootResult, (void *)(this), 
                                   VTK_PV_ROOT_RESULT_RMI_TAG);
    
    // For ReceiveRootDataObject: Send data object back to the client.
    this->SocketController->AddRMI(vtkPVSendPolyData, this,
                                   VTK_PV_SEND_DATA_OBJECT_TAG);
    
    // Loop listening to the socket for RMI's.
    this->SocketController->AddRMI(vtkPVBroadcastScript, (void *)(this), 
                                   VTK_PV_BROADCAST_SCRIPT_RMI_TAG);
    // Remote script is only really for debugging.
    this->SocketController->AddRMI(vtkPVRelayRemoteScript, (void *)(this), 
                                   VTK_PV_REMOTE_SCRIPT_RMI_TAG);
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
    this->Controller->AddRMI(vtkPVServerSlaveScript, (void *)(pvApp), 
                             VTK_PV_SATELLITE_SCRIPT);
    
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
        runcommand += "${PARAVIEW_EXECUTABLE} --server --port=";
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
// Called only on client
void vtkPVClientServerModule::RootSimpleScript(const char *str)
{
  // Clear any previous result.
  this->SetRootResult(0);
  
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (!str || (strlen(str) < 1))
    {
    return;
    }

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return;
    }

  this->SocketController->TriggerRMI(1, const_cast<char*>(str), 
                                     VTK_PV_ROOT_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
// Called only on client
const char* vtkPVClientServerModule::GetRootResult()
{
  if(!this->Application)
    {
    vtkErrorMacro("Missing application object.");
    return 0;
    }
  
  if(!this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return 0;
    }
  
  if(!this->RootResult)
    {
    int length;    
    this->SocketController->TriggerRMI(1, "", VTK_PV_ROOT_RESULT_RMI_TAG);
    this->SocketController->Receive(&length, 1, 1, VTK_PV_ROOT_RESULT_LENGTH_TAG);
    if(length <= 0)
      {
      return 0;
      }
    this->RootResult = new char[length];
    this->SocketController->Receive(this->RootResult, length, 1, VTK_PV_ROOT_RESULT_TAG);
    }
  return this->RootResult;
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::ServerSimpleScript(const char *str)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (!str || (strlen(str) < 1))
    {
    return;
    }

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return;
    }

  this->SocketController->TriggerRMI(1, const_cast<char*>(str), 
                                     VTK_PV_BROADCAST_SCRIPT_RMI_TAG);
}

//----------------------------------------------------------------------------
// Called only in the client.
void vtkPVClientServerModule::BroadcastSimpleScript(const char *str)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (!str || (strlen(str) < 1))
    {
    return;
    }

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return;
    }

  this->SocketController->TriggerRMI(1, const_cast<char*>(str), 
                                     VTK_PV_BROADCAST_SCRIPT_RMI_TAG);

  // Execute the script locally.  
  // Do reverse order, because 0 will block.
  this->Application->SimpleScript(str);
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::BroadcastScriptRMI(const char *str)
{
  int id, num;
    
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call on the client.");
    return;
    }
  
  // Sanity check.
  id = this->Controller->GetLocalProcessId();
  if (id != 0)
    {
    vtkErrorMacro("Not expecting this call from any other process but 0.");
    return;
    }

  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->TriggerRMI(id, const_cast<char*>(str), 
                                 VTK_PV_SATELLITE_SCRIPT);
    }

  // Execute the script locally.  
  // Do reverse order, because 0 will block.
  this->Application->SimpleScript(str);
  this->SetRootResult(this->Application->GetMainInterp()->result);
}




//----------------------------------------------------------------------------
// Remote scripts are (should be) used only for debugging.
void vtkPVClientServerModule::RemoteSimpleScript(int remoteId, const char *str)
{
  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (!str || (strlen(str) < 1))
    {
    return;
    }
  if (remoteId == 0)
    {
    // Execute the script locally.  
    this->Application->SimpleScript(str);
    return;
    }
  if ( ! this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return;
    }

  this->SocketController->TriggerRMI(1, const_cast<char*>(str), 
                                     VTK_PV_REMOTE_SCRIPT_RMI_TAG);
  // Destination is sent as a separate message.
  this->SocketController->Send(&remoteId, 1, 1, VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG);
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::RelayScriptRMI(const char *str)
{
  int id, num;
  int destination = 0;
    
  this->SocketController->Receive(&destination, 
                                  1, 
                                  1, 
                                  VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG);
  // We count the client as process 0.
  --destination;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return;
    }
  if (this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call on the client.");
    return;
    }
  
  // Sanity check.
  id = this->Controller->GetLocalProcessId();
  if (id != 0)
    {
    vtkErrorMacro("Not expecting this call from any other process but 0.");
    return;
    }

  num = this->Controller->GetNumberOfProcesses();
  if (destination >= num || destination < 0)
    {
    vtkErrorMacro("Bad destination process for remote script.");
    return;
    }

  if (destination == 0)
    {
    // Execute the script locally.  
    this->Application->SimpleScript(str);
    return;
    }

  this->Controller->TriggerRMI(destination, const_cast<char*>(str), 
                               VTK_PV_SATELLITE_SCRIPT);
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
                                                char* objectTclName)
{
  // Just a simple way of passing the information object to the next method.
  this->TemporaryInformation = info;
  // Some objects are not created on the client (data.
  if (!info->GetRootOnly())
    {
    this->ServerScript(
      "[$Application GetProcessModule] GatherInformationInternal %s %s",
      info->GetClassName(), objectTclName);
    }
  else
    {
    this->RootScript(
      "[$Application GetProcessModule] GatherInformationInternal %s %s",
      info->GetClassName(), objectTclName);
    }
  this->GatherInformationInternal(NULL, NULL);
  this->TemporaryInformation = NULL; 
}

//----------------------------------------------------------------------------
// This method is broadcast to all processes.
void vtkPVClientServerModule::GatherInformationInternal(char* infoClassName, 
                                                        vtkObject* object)
{
  int length;
  unsigned char *msg;
  int myId = this->Controller->GetLocalProcessId();

  if (this->GetPVApplication()->GetClientMode())
    { // Client just receives information from the server.
    this->SocketController->Receive(&length, 1, 1, 398798);
    msg = new unsigned char[length];
    this->SocketController->Receive(msg, length, 1, 398799);
    this->TemporaryInformation->CopyFromMessage(msg);
    delete [] msg;
    msg = NULL;
    return;
    }

  if (object == NULL)
    {
    vtkErrorMacro("Deci tcl name must be wrong.");
    return;
    }

  // Create a temporary information object.
  vtkObject* o;
  o = vtkInstantiator::CreateInstance(infoClassName);
  if ( o == NULL)
    {
    vtkErrorMacro("Could not instantiate object " << infoClassName);
    return;
    }
  vtkPVInformation* tmp1;
  tmp1 = vtkPVInformation::SafeDownCast(o);
  o = NULL;

  if (myId != 0 && !tmp1->GetRootOnly())
    {
    tmp1->CopyFromObject(object);
    length = tmp1->GetMessageLength();
    msg = new unsigned char[length];
    tmp1->WriteMessage(msg);
    this->Controller->Send(&length, 1, 0, 498798);
    this->Controller->Send(msg, length, 0, 498799);
    delete [] msg;
    msg = NULL;
    tmp1->Delete();
    tmp1 = NULL;
    return;
    }

  // Node 0.
  tmp1->CopyFromObject(object);
  if (!tmp1->GetRootOnly())
    {
    o = vtkInstantiator::CreateInstance(infoClassName);
    vtkPVInformation* tmp2 = vtkPVInformation::SafeDownCast(o);
    o = NULL;
    
    int numProcs = this->Controller->GetNumberOfProcesses();
    int idx;
    for (idx = 1; idx < numProcs; ++idx)
      {
      this->Controller->Receive(&length, 1, idx, 498798);
      msg = new unsigned char[length];
      this->Controller->Receive(msg, length, idx, 498799);
      tmp2->CopyFromMessage(msg);
      tmp1->AddInformation(tmp2);
      delete [] msg;
      msg = NULL;
      }
    tmp2->Delete();
    tmp2 = NULL;
    }

  // Send final information to client over socket connection.
  length = tmp1->GetMessageLength();
  msg = new unsigned char[length];
  tmp1->WriteMessage(msg);
  this->SocketController->Send(&length, 1, 1, 398798);
  this->SocketController->Send(msg, length, 1, 398799);
  delete [] msg;
  msg = NULL;
  tmp1->Delete();
  tmp1 = NULL;
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
                                                 const char* perm)
{
  if(this->ClientMode)
    {
    this->RootScript(
      "::paraview::vtkPVProcessModule::GetDirectoryListing {%s} {%s}",
      dir, perm);
    char* result = vtkString::Duplicate(this->GetRootResult());
    if(!result || strcmp(result, "<NO_SUCH_DIRECTORY>") == 0)
      {
      dirs->RemoveAllItems();
      files->RemoveAllItems();
      delete [] result;
      return 0;
      }
    vtkTclGetObjectFromPointer(this->Application->GetMainInterp(), dirs,
                               vtkStringListCommand);
    char* dirsTcl = vtkString::Duplicate(
      Tcl_GetStringResult(this->Application->GetMainInterp()));
    vtkTclGetObjectFromPointer(this->Application->GetMainInterp(), files,
                               vtkStringListCommand);
    char* filesTcl = vtkString::Duplicate(
      Tcl_GetStringResult(this->Application->GetMainInterp()));
    this->Application->Script(
      "::paraview::vtkPVProcessModule::ParseDirectoryListing {%s} {%s} {%s}",
      result, dirsTcl, filesTcl
      );
    delete [] dirsTcl;
    delete [] filesTcl;
    delete [] result;
    return 1;
    }
  else
    {
    return this->Superclass::GetDirectoryListing(dir, dirs, files, perm);
    }
}

//----------------------------------------------------------------------------
vtkKWLoadSaveDialog* vtkPVClientServerModule::NewLoadSaveDialog()
{
  vtkPVServerFileDialog* dialog = vtkPVServerFileDialog::New();
  dialog->SetMasterWindow(this->GetPVApplication()->GetMainWindow());
  return dialog;
}

//----------------------------------------------------------------------------
int vtkPVClientServerModule::ReceiveRootPolyData(const char* tclName,
                                                 vtkPolyData* out)
{
  // Make sure we have a named Tcl VTK object.
  if(!tclName || !tclName[0])
    {
    return 0;
    }
  
  // Send the object name length and the name itself.
  int length = static_cast<int>(strlen(tclName));
  this->SocketController->TriggerRMI(1, VTK_PV_SEND_DATA_OBJECT_TAG);
  this->SocketController->Send(&length, 1, 1, VTK_PV_DATA_OBJECT_TAG);
  this->SocketController->Send(const_cast<char*>(tclName), length, 1,
                               VTK_PV_DATA_OBJECT_TAG);
  
  // Receive success/failure flag and the object itself.
  int success;
  this->SocketController->Receive(&success, 1, 1, VTK_PV_DATA_OBJECT_TAG);
  if(!success)
    {
    return 0;
    }
  this->SocketController->Receive(out, 1, VTK_PV_DATA_OBJECT_TAG);
  return 1;
}
