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
#include "vtkObjectFactory.h"

#include "vtkToolkits.h"
#include "vtkPVConfig.h"
#include "vtkMultiProcessController.h"
#include "vtkDummyController.h"
#include "vtkSocketController.h"
#include "vtkSocketCommunicator.h"
#include "vtkPVApplication.h"
#include "vtkDataSet.h"
#include "vtkPVDataInformation.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkCharArray.h"
#include "vtkLongArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkMapper.h"
#include "vtkString.h"
#ifdef VTK_USE_MPI
#include "vtkMPIController.h"
#include "vtkMPICommunicator.h"
#include "vtkMPIGroup.h"
#endif

#include "vtkPVPart.h"

#define VTK_PV_BROADCAST_SCRIPT_RMI_TAG      838422
#define VTK_PV_REMOTE_SCRIPT_RMI_TAG         838427
#define VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG 838428
#define VTK_PV_SATELLITE_SCRIPT              838431

#define VTK_PV_ROOT_SCRIPT_RMI_TAG           838485
#define VTK_PV_ROOT_RESULT_RMI_TAG           838486
#define VTK_PV_ROOT_RESULT_LENGTH_TAG        838487
#define VTK_PV_ROOT_RESULT_TAG               838488
 


//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVRootScript(void *localArg, void *remoteArg, 
                            int vtkNotUsed(remoteArgLength),
                            int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  self->Script((char*)remoteArg);
}

//----------------------------------------------------------------------------
// This RMI is only on process 0 of server. (socket controller)
void vtkPVRootResult(void *localArg, void* , 
                            int vtkNotUsed(remoteArgLength),
                            int vtkNotUsed(remoteProcessId))
{
  vtkPVClientServerModule *self = (vtkPVClientServerModule *)(localArg);
  char* result = self->GetApplication()->GetMainInterp()->result;
  int length = static_cast<int>(strlen(result)) + 1;

  self->GetSocketController()->Send(&length, 1, 1, VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if (length > 0)
    {
    self->GetSocketController()->Send(result, length, 1, VTK_PV_ROOT_RESULT_TAG);  
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
vtkStandardNewMacro(vtkPVClientServerModule);
vtkCxxRevisionMacro(vtkPVClientServerModule, "1.13");

int vtkPVClientServerModuleCommand(ClientData cd, Tcl_Interp *interp,
                            int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVClientServerModule::vtkPVClientServerModule()
{
  this->Controller = NULL;
  this->SocketController = NULL;
  this->ClientMode = 1;

  this->ArgumentCount = 0;
  this->Arguments = NULL;
  this->ReturnValue = 0;
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
// This method is a bit long, we should probably break it up 
// to simplify it. !!!!!
void vtkPVClientServerModule::Initialize()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int id;
 
#ifdef MPIPROALLOC
  vtkCommunicator::SetUseCopy(1);
#endif

  this->ClientMode = pvApp->GetClientMode();

  if (this->ClientMode)
    {
    this->SocketController = vtkSocketController::New();
    this->SocketController->Initialize();
    vtkSocketCommunicator *comm = vtkSocketCommunicator::New();

    // Get the host name from the command line arguments
    char* hostname = pvApp->GetHostName();
    // Get the port from the command line arguments
    int port = pvApp->GetPort();
    // Establish connection
    if (!comm->ConnectTo(hostname, port))
      {
      vtkErrorMacro("Client error: Could not connect to the server.");
      comm->Delete();
      delete[] hostname;
      this->ReturnValue = 1;
      return;
      }
    this->SocketController->SetCommunicator(comm);
    comm->Delete();
    //this->Controller->CreateOutputWindow();

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
    this->SocketController = vtkSocketController::New();
    this->SocketController->Initialize();
    vtkSocketCommunicator* comm = vtkSocketCommunicator::New();

    int port= pvApp->GetPort();

    // Establish connection
    if (!comm->WaitForConnection(port))
      {
      vtkErrorMacro("Server error: Wait timed out or could not initialize socket.");
      comm->Delete();
      this->ReturnValue = 1;
      return;
      }
    this->SocketController->SetCommunicator(comm);
    comm->Delete();
    comm = NULL;

    // send the number of server processes as a handshake.
    this->SocketController->Send(&numProcs, 1, 1, 8843);

    // For root script: Execute script only on process 0 of server.
    this->SocketController->AddRMI(vtkPVRootScript, (void *)(this), 
                                   VTK_PV_ROOT_SCRIPT_RMI_TAG);
    // For root script: Return result back to client.
    this->SocketController->AddRMI(vtkPVRootResult, (void *)(this), 
                                   VTK_PV_ROOT_RESULT_RMI_TAG);
    
    // Loop listening to the socket for RMI's.
    this->SocketController->AddRMI(vtkPVBroadcastScript, (void *)(this), 
                                   VTK_PV_BROADCAST_SCRIPT_RMI_TAG);
    // Remote script is only really for debugging.
    this->SocketController->AddRMI(vtkPVRelayRemoteScript, (void *)(this), 
                                   VTK_PV_REMOTE_SCRIPT_RMI_TAG);
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

    // Process rmis until the application exits.
    this->Controller->ProcessRMIs();    
    // Now we are exiting.
    }
}





//----------------------------------------------------------------------------
// same as the MPI start.
int vtkPVClientServerModule::Start(int argc, char **argv)
{
  // First we initialize the mpi controller.
  // We are assuming that the client has been started with one process
  // and is linked with MPI.
#ifdef VTK_USE_MPI
  this->Controller = vtkMPIController::New();
  vtkMultiProcessController::SetGlobalController(this->Controller);
  this->Controller->Initialize(&argc, &argv, 1);
  this->ArgumentCount = argc;
  this->Arguments = argv;
  this->Controller->SetSingleMethod(vtkPVClientServerInit, (void *)(this));
  this->Controller->SingleMethodExecute();
  this->Controller->Finalize();
#else
  argc = argc;
  argv = argv;
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

  this->SocketController->TriggerRMI(1, 
                              vtkMultiProcessController::BREAK_RMI_TAG);
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
char* vtkPVClientServerModule::NewRootResult()
{
  int length;
  char *result;

  if (this->Application == NULL)
    {
    vtkErrorMacro("Missing application object.");
    return NULL;
    }

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("NotExpecting this call on the server.");
    return NULL;
    }

  this->SocketController->TriggerRMI(1, "", VTK_PV_ROOT_RESULT_RMI_TAG);
  this->SocketController->Receive(&length, 1, 1, VTK_PV_ROOT_RESULT_LENGTH_TAG);
  if (length <= 0)
    {
    return NULL;
    }
  result = new char[length];
  this->SocketController->Receive(result, length, 1, VTK_PV_ROOT_RESULT_TAG);
  return result;  
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
    
  this->SocketController->Receive(&destination, 1, 1, VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG);
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
// This method is broadcast to all processes.
void vtkPVClientServerModule::GatherDataInformation(vtkDataSet *data)
{
  int length;
  unsigned char *msg;
  int myId = this->Controller->GetLocalProcessId();

  if (data == NULL)
    {
    vtkErrorMacro("Data Tcl name must be wrong.");
    return;
    }

  if (this->ClientMode)
    { // Client just receives information from the server.
    this->SocketController->Receive(&length, 1, 1, 398798);
    msg = new unsigned char[length];
    this->SocketController->Receive(msg, length, 1, 398799);
    this->TemporaryInformation->CopyFromMessage(msg);
    delete [] msg;
    msg = NULL;
    return;
    }

  if (myId != 0)
    {
    vtkPVDataInformation *tmp = vtkPVDataInformation::New();
    tmp->CopyFromData(data);
    msg = tmp->NewMessage(length);
    this->Controller->Send(&length, 1, 0, 398798);
    this->Controller->Send(msg, length, 0, 398799);
    delete [] msg;
    msg = NULL;
    tmp->Delete();
    tmp = NULL;
    return;
    }

  // Node 0.
  int numProcs = this->Controller->GetNumberOfProcesses();
  int idx;
  vtkPVDataInformation *tmp1 = vtkPVDataInformation::New();
  vtkPVDataInformation *tmp2 = vtkPVDataInformation::New();
  tmp1->CopyFromData(data);
  for (idx = 1; idx < numProcs; ++idx)
    {
    this->Controller->Receive(&length, 1, idx, 398798);
    msg = new unsigned char[length];
    this->Controller->Receive(msg, length, idx, 398799);
    tmp2->CopyFromMessage(msg);
    tmp1->AddInformation(tmp2);
    delete [] msg;
    msg = NULL;
    }
  tmp2->Delete();
  tmp2 = NULL;

  // Send final information to client over socket connection.
  msg = tmp1->NewMessage(length);
  this->SocketController->Send(&length, 1, 1, 398798);
  this->SocketController->Send(msg, length, 1, 398799);
  delete [] msg;
  msg = NULL;
  tmp1->Delete();
  tmp1 = NULL;

}    




//----------------------------------------------------------------------------
void vtkPVClientServerModule::InitializePVPartPartition(vtkPVPart *part)
{
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 0",
                        part->GetMapperTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 0",
                        part->GetLODMapperTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 1",
                        part->GetUpdateSuppressorTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 1",
                        part->GetLODUpdateSuppressorTclName());
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::InitializePartition(char *tclName, int updateFlag)
{
  int numPieces;
  int piece;
  if (this->ClientMode)
    {
    piece = 0;
    numPieces = 0;
    }
  else
    {
    numPieces = this->Controller->GetNumberOfProcesses();
    piece = this->Controller->GetLocalProcessId();
    }

  // Need to clean up.  Just Set piece of suppressor?
  if (updateFlag)
    {
    this->Application->Script("%s SetUpdateNumberOfPieces %d", tclName, numPieces);
    this->Application->Script("%s SetUpdatePiece %d", tclName, piece);
    }
  else
    {
    this->Application->Script("%s SetNumberOfPieces %d", tclName, numPieces);
    this->Application->Script("%s SetPiece %d", tclName, piece);
    }
}

//----------------------------------------------------------------------------
void vtkPVClientServerModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Controller: " << this->Controller << endl;;
  os << indent << "SocketController: " << this->SocketController << endl;;
  os << indent << "ClientMode: " << this->ClientMode << endl;
}
