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

#include "vtkPVData.h"

#define VTK_PV_BROADCAST_SCRIPT_RMI_TAG      838422
#define VTK_PV_REMOTE_SCRIPT_RMI_TAG         838427
#define VTK_PV_REMOTE_SCRIPT_DESTINATION_TAG 838428
#define VTK_PV_SATELLITE_SCRIPT              838431
 

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
vtkCxxRevisionMacro(vtkPVClientServerModule, "1.5");

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
void vtkPVClientServerInit(vtkMultiProcessController *controller, void *arg )
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
    int numServerProcs;
    this->SocketController->Receive(&numServerProcs, 1, 1, 8843);
    this->NumberOfServerProcesses = numServerProcs;
   
    // Start the application (UI). 
    // For SGI pipe option.
    pvApp->SetNumberOfPipes(numServerProcs);
    
#ifdef PV_HAVE_TRAPS_FOR_SIGNALS
    pvApp->SetupTrapsForSignals(myId);   
#endif // PV_HAVE_TRAPS_FOR_SIGNALS

    pvApp->Script("wm withdraw .");
    pvApp->Start(this->ArgumentCount,this->Arguments);

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
#else
  this->Controller = vtkDummyController::New();
  // This would be simpler if vtkDummyController::SingleMethodExecute
  // did its job correctly.
  vtkMultiProcessController::SetGlobalController(this->Controller);
  vtkPVClientServerInit(this->Controller, (void*)this);
  return this->ReturnValue;
#endif

  vtkMultiProcessController::SetGlobalController(this->Controller);

  this->Controller->Initialize(&argc, &argv, 1);

  this->ArgumentCount = argc;
  this->Arguments = argv;

  this->Controller->SetSingleMethod(vtkPVClientServerInit, (void *)(this));
  this->Controller->SingleMethodExecute();
  this->Controller->Finalize();

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
  int destination;
    
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
void vtkPVClientServerModule::GetPVDataBounds(vtkPVData *pvd, float bounds[6])
{
  if ( ! this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call an the server.");
    return;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendDataBounds %s", 
                        pvd->GetVTKDataTclName());

  this->SocketController->Receive(bounds, 6, 1, 1967);
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendDataBounds(vtkDataSet *data)
{
  float bounds[6];
  
  if (this->ClientMode)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() > 0)
    {
    data->GetBounds(bounds);
    this->Controller->Send(bounds, 6, 0, 1967);
    return;
    }

  // Only process 0 of the server gets to this point.
  int num, id;
  float tmp[6];

  data->GetBounds(bounds);
  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(tmp, 6, id, 1967);
    if (tmp[0] < bounds[0])
      {
      bounds[0] = tmp[0];
      }
    if (tmp[1] > bounds[1])
      {
      bounds[1] = tmp[1];
      }
    if (tmp[2] < bounds[2])
      {
      bounds[2] = tmp[2];
      }
    if (tmp[3] > bounds[3])
      {
      bounds[3] = tmp[3];
      }
    if (tmp[4] < bounds[4])
      {
      bounds[4] = tmp[4];
      }
    if (tmp[5] > bounds[5])
      {
      bounds[5] = tmp[5];
      }
    }
  
  // Now send the results back to the client.
  this->SocketController->Send(bounds, 6, 1, 1967);
}




//----------------------------------------------------------------------------
int vtkPVClientServerModule::GetPVDataNumberOfCells(vtkPVData *pvd)
{
  float numCells;

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call an the server.");
    return 0;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendDataNumberOfCells %s", 
                        pvd->GetVTKDataTclName());

  this->SocketController->Receive(&numCells, 1, 1, 1968);
  return numCells;
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendDataNumberOfCells(vtkDataSet *data)
{
  float numCells;
  
  if (this->ClientMode)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() > 0)
    {
    numCells = data->GetNumberOfCells();
    this->Controller->Send(&numCells, 1, 0, 1968);
    return;
    }

  // Only process 0 of the server gets to this point.
  int num, id;
  float tmp;

  numCells = data->GetNumberOfCells();
  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(&tmp, 1, id, 1968);
    numCells += tmp;
    }
  
  // Now send the results back to the client.
  this->SocketController->Send(&numCells, 1, 1, 1968);
}



//----------------------------------------------------------------------------
int vtkPVClientServerModule::GetPVDataNumberOfPoints(vtkPVData *pvd)
{
  int numPoints;

  if ( ! this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call an the server.");
    return 0;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendDataNumberOfPoints %s", 
                        pvd->GetVTKDataTclName());

  this->SocketController->Receive(&numPoints, 1, 1, 1969);
  return numPoints;
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendDataNumberOfPoints(vtkDataSet *data)
{
  int numPoints;
  
  if (this->ClientMode)
    {
    return;
    }

  if (this->Controller->GetLocalProcessId() > 0)
    {
    numPoints = data->GetNumberOfPoints();
    this->Controller->Send(&numPoints, 1, 0, 1969);
    return;
    }

  // Only process 0 of the server gets to this point.
  int num, id;
  float tmp;

  numPoints = data->GetNumberOfPoints();
  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(&tmp, 1, id, 1969);
    numPoints += tmp;
    }
  
  // Now send the results back to the client.
  this->SocketController->Send(&numPoints, 1, 1, 1969);
}





//----------------------------------------------------------------------------
void vtkPVClientServerModule::GetPVDataArrayComponentRange(vtkPVData *pvd, int pointDataFlag,
                            const char *arrayName, int component, float *range)
{
  if ( ! this->ClientMode)
    {
    vtkErrorMacro("Not expecting this call an the server.");
    return;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendDataArrayRange %s %d {%s} %d",
                         pvd->GetVTKDataTclName(),
                         pointDataFlag, arrayName, component);

  this->SocketController->Receive(range, 2, 1, 1970);
}
//----------------------------------------------------------------------------
void vtkPVClientServerModule::SendDataArrayRange(vtkDataSet *data, 
                                                 int pointDataFlag, 
                                                 char *arrayName,
                                                 int component)
{
  float range[2];
  vtkDataArray *array;
  
  if (this->ClientMode)
    {
    return;
    }

  range[0] = VTK_LARGE_FLOAT;
  range[1] = -VTK_LARGE_FLOAT;

  if (pointDataFlag)
    {
    array = data->GetPointData()->GetArray(arrayName);
    }
  else
    {
    array = data->GetCellData()->GetArray(arrayName);
    }
  if (array)
    {
    array->GetRange(range, component);  
    }

  if (this->Controller->GetLocalProcessId() > 0)
    {
    this->Controller->Send(range, 2, 0, 1970);
    return;
    }

  // Only process 0 of the server gets to this point.
  int num, id;
  float tmp[2];

  num = this->Controller->GetNumberOfProcesses();
  for (id = 1; id < num; ++id)
    {
    this->Controller->Receive(tmp, 2, id, 1970);
    if (tmp[0] < range[0])
      {
      range[0] = tmp[0];
      }
    if (tmp[1] > range[1])
      {
      range[1] = tmp[1];
      }
    }
  
  // Now send the results back to the client.
  this->SocketController->Send(range, 2, 1, 1970);
}







//----------------------------------------------------------------------------
// Ask for one process to return a listing of arrays.
// This method is designed to use BroadcastScript 
// instead of RemoteScript.
void vtkPVClientServerModule::CompleteArrays(vtkMapper *mapper, char *mapperTclName)
{
  int j;
  int nonEmptyFlag = 0;
  int activeAttributes[5];

  if (mapper->GetInput() == NULL || this->Controller == NULL)
    {
    return;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendCompleteArrays %s", mapperTclName);

  int num = 0;
  vtkDataArray *array = 0;
  char *name;
  int nameLength = 0;
  int type = 0;
  int numComps = 0;
      
  // First Point data.
  this->SocketController->Receive(&num, 1, 1, 987244);
  for (j = 0; j < num; ++j)
    {
    this->SocketController->Receive(&type, 1, 1, 987245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->SocketController->Receive(&numComps, 1, 1, 987246);
    array->SetNumberOfComponents(numComps);
    this->SocketController->Receive(&nameLength, 1, 1, 987247);
    name = new char[nameLength];
    this->SocketController->Receive(name, nameLength, 1, 987248);
    array->SetName(name);
    delete [] name;
    mapper->GetInput()->GetPointData()->AddArray(array);
    array->Delete();
    } // end of loop over point arrays.
  // Which scalars, ... are active?
  this->SocketController->Receive(activeAttributes, 5, 1, 987258);
  mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
  mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
  mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
  mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
  mapper->GetInput()->GetPointData()->SetActiveAttribute(activeAttributes[4],4);

  // Next Cell data.
  this->SocketController->Receive(&num, 1, 1, 987244);
  for (j = 0; j < num; ++j)
    {
    this->SocketController->Receive(&type, 1, 1, 987245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->SocketController->Receive(&numComps, 1, 1, 987246);
    array->SetNumberOfComponents(numComps);
    this->SocketController->Receive(&nameLength, 1, 1, 987247);
    name = new char[nameLength];
    this->SocketController->Receive(name, nameLength, 1, 987248);
    array->SetName(name);
    delete [] name;
    mapper->GetInput()->GetCellData()->AddArray(array);
    array->Delete();
    } // end of loop over cell arrays.
  // Which scalars, ... are active?
  this->SocketController->Receive(activeAttributes, 5, 1, 987258);
  mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
  mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
  mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
  mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
  mapper->GetInput()->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
}
//----------------------------------------------------------------------------
// This method is broadcast, so is called on every process (even the client).
void vtkPVClientServerModule::SendCompleteArrays(vtkMapper *mapper)
{
  if (this->ClientMode)
    {
    return;
    }

  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  vtkDataSet *data = mapper->GetInput();
  int nonEmptyFlag;
  int requestFlag;
  int i;

  // Info for sends.
  int num;
  int type;
  int numComps;
  int nameLength;
  const char *name = NULL;
  vtkDataArray *array;
  int activeAttributes[5];


  // ---- Are we a candidate to send.
  if (data->GetNumberOfPoints() == 0 &&
      data->GetNumberOfCells() == 0)
    {
    nonEmptyFlag = 0;
    }
  else
    {
    nonEmptyFlag = 1;
    }
  // Special case: All procs are empty.
  // The last proc should send anyway.
  if (myId == numProcs-1)
    {
    nonEmptyFlag = 1;
    }

  // ---- Does the client want us to send.
  // Process 0 is always requested.
  requestFlag = 1;
  if (myId > 0)
    {
    this->Controller->Receive(&requestFlag, 1, 0, 778432);
    // If this process is requested to send info,
    // respond whether we have info to send..
    if (requestFlag)
      {
      this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
      }
    else
      { // Our information is not needed. Our job is finished.
      return;
      }
    if ( ! nonEmptyFlag)
      { // We do not have anything to send. Our job is finished.
      return;
      }
    }

  // ---- Send info.  Convert arrays to info and send.
  if (requestFlag && nonEmptyFlag)
    { // We are the process choosen to send.
    vtkMultiProcessController *controller;
    int toId;
    
    if (myId == 0)
      { // Root node has to dismiss all other processes.
      for (i = 1; i < numProcs; ++i)
        {
        requestFlag = 0;
        this->Controller->Send(&requestFlag, 1, i, 778432);
        }
      // Root has to send through the socket controller.
      controller = this->SocketController;
      toId = 1;
      }
    else
      { // Other processes have to relay through Root (process 0).
      controller = this->Controller;
      toId = 0;
      }

    // Send all the array info.
    // First point data.
    num = data->GetPointData()->GetNumberOfArrays();
    controller->Send(&num, 1, toId, 987244);
    for (i = 0; i < num; ++i)
      {
      array = data->GetPointData()->GetArray(i);
      type = array->GetDataType();

      controller->Send(&type, 1, toId, 987245);
      numComps = array->GetNumberOfComponents();

      controller->Send(&numComps, 1, toId, 987246);
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      nameLength = vtkString::Length(name)+1;
      controller->Send(&nameLength, 1, toId, 987247);
      // I am pretty sure that Send does not modify the string.
      controller->Send(const_cast<char*>(name), nameLength, toId, 987248);
      }
    data->GetPointData()->GetAttributeIndices(activeAttributes);
    controller->Send(activeAttributes, 5, toId, 987258);
    // Next cell data.
    num = data->GetCellData()->GetNumberOfArrays();
    controller->Send(&num, 1, toId, 987244);
    for (i = 0; i < num; ++i)
      {
      array = mapper->GetInput()->GetCellData()->GetArray(i);
      type = array->GetDataType();
      controller->Send(&type, 1, toId, 987245);
      numComps = array->GetNumberOfComponents();
      controller->Send(&numComps, 1, toId, 987246);
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      nameLength = vtkString::Length(name+1);
      controller->Send(&nameLength, 1, toId, 987247);
      controller->Send(const_cast<char*>(name), nameLength, toId, 987248);
      }
    data->GetCellData()->GetAttributeIndices(activeAttributes);
    controller->Send(activeAttributes, 5, toId, 987258);
    
    return;
    } // End sending array info.

  // ---- Relay information
  // Only process 0 (that has no data) should make it to this point.
  // Tell the satellites (one bby one) we need their information.
  requestFlag = 1;
  for (i = 1; i < numProcs; ++i)
    {
    this->Controller->Send(&requestFlag, 1, i, 778432);
    if (requestFlag)
      {
      this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
      if (nonEmptyFlag)
        {
        // We have picked this process to send information.
        // Do not request processes from any other process.
        requestFlag = 0;

        // Relay.
        // First point data.
        this->Controller->Receive(&num, 1, i, 987244);
        this->SocketController->Send(&num, 1, 1, 987244);
        for (i = 0; i < num; ++i)
          {
          this->Controller->Receive(&type, 1, i, 987245);
          this->SocketController->Send(&type, 1, 1, 987245);
          this->Controller->Receive(&numComps, 1, i, 987246);
          this->SocketController->Send(&numComps, 1, 1, 987246);
          this->Controller->Receive(&nameLength, 1, i, 987247);
          this->SocketController->Send(&nameLength, 1, 1, 987247);
          char *buffer = new char[nameLength+1];
          this->Controller->Receive(const_cast<char*>(buffer), nameLength, i, 987248);
          this->SocketController->Send(const_cast<char*>(buffer), nameLength, 1, 987248);
          delete [] buffer;
          }
        this->Controller->Receive(activeAttributes, 5, i, 987258);
        this->SocketController->Send(activeAttributes, 5, 1, 987258);
        // Next cell data.
        this->Controller->Receive(&num, 1, i, 987244);
        this->SocketController->Send(&num, 1, 1, 987244);
        for (i = 0; i < num; ++i)
          {
          this->Controller->Receive(&type, 1, i, 987245);
          this->SocketController->Send(&type, 1, 1, 987245);
          this->Controller->Receive(&numComps, 1, i, 987246);
          this->SocketController->Send(&numComps, 1, 1, 987246);
          this->Controller->Receive(&nameLength, 1, i, 987247);
          this->SocketController->Send(&nameLength, 1, 1, 987247);
          this->Controller->Receive(const_cast<char*>(name), nameLength, i, 987248);
          this->SocketController->Send(const_cast<char*>(name), nameLength, 1, 987248);
          }
        this->Controller->Receive(activeAttributes, 5, i, 987258);
        this->SocketController->Send(activeAttributes, 5, 1, 987258);
        } // end choosen process (for sending)
      } // if requested
    } // i numProcs loop
}

//----------------------------------------------------------------------------
// Ask for one process to return a listing of arrays.
// This method is designed to use BroadcastScript 
// instead of RemoteScript.
void vtkPVClientServerModule::CompleteArrays(vtkDataSet *data, char *dataTclName)
{
  int j;
  int activeAttributes[5];

  if (data == NULL || this->Controller == NULL)
    {
    return;
    }

  this->BroadcastScript("[$Application GetProcessModule] SendCompleteArrays %s", dataTclName);

  int num = 0;
  vtkDataArray *array = 0;
  char *name;
  int nameLength = 0;
  int type = 0;
  int numComps = 0;
      
  // First Point data.
  this->SocketController->Receive(&num, 1, 1, 987244);
  for (j = 0; j < num; ++j)
    {
    this->SocketController->Receive(&type, 1, 1, 987245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->SocketController->Receive(&numComps, 1, 1, 987246);
    array->SetNumberOfComponents(numComps);
    this->SocketController->Receive(&nameLength, 1, 1, 987247);
    name = new char[nameLength];
    this->SocketController->Receive(name, nameLength, 1, 987248);
    array->SetName(name);
    delete [] name;
    data->GetPointData()->AddArray(array);
    array->Delete();
    } // end of loop over point arrays.
  // Which scalars, ... are active?
  this->SocketController->Receive(activeAttributes, 5, 1, 987258);
  data->GetPointData()->SetActiveAttribute(activeAttributes[0],0);
  data->GetPointData()->SetActiveAttribute(activeAttributes[1],1);
  data->GetPointData()->SetActiveAttribute(activeAttributes[2],2);
  data->GetPointData()->SetActiveAttribute(activeAttributes[3],3);
  data->GetPointData()->SetActiveAttribute(activeAttributes[4],4);

  // Next Cell data.
  this->SocketController->Receive(&num, 1, 1, 987244);
  for (j = 0; j < num; ++j)
    {
    this->SocketController->Receive(&type, 1, 1, 987245);
    switch (type)
      {
      case VTK_INT:
        array = vtkIntArray::New();
        break;
      case VTK_FLOAT:
        array = vtkFloatArray::New();
        break;
      case VTK_DOUBLE:
        array = vtkDoubleArray::New();
        break;
      case VTK_CHAR:
        array = vtkCharArray::New();
        break;
      case VTK_LONG:
        array = vtkLongArray::New();
        break;
      case VTK_SHORT:
        array = vtkShortArray::New();
        break;
      case VTK_UNSIGNED_CHAR:
        array = vtkUnsignedCharArray::New();
        break;
      case VTK_UNSIGNED_INT:
        array = vtkUnsignedIntArray::New();
        break;
      case VTK_UNSIGNED_LONG:
        array = vtkUnsignedLongArray::New();
        break;
      case VTK_UNSIGNED_SHORT:
        array = vtkUnsignedShortArray::New();
        break;
      }
    this->SocketController->Receive(&numComps, 1, 1, 987246);
    array->SetNumberOfComponents(numComps);
    this->SocketController->Receive(&nameLength, 1, 1, 987247);
    name = new char[nameLength];
    this->SocketController->Receive(name, nameLength, 1, 987248);
    array->SetName(name);
    delete [] name;
    data->GetCellData()->AddArray(array);
    array->Delete();
    } // end of loop over cell arrays.
  // Which scalars, ... are active?
  this->SocketController->Receive(activeAttributes, 5, 1, 987258);
  data->GetCellData()->SetActiveAttribute(activeAttributes[0],0);
  data->GetCellData()->SetActiveAttribute(activeAttributes[1],1);
  data->GetCellData()->SetActiveAttribute(activeAttributes[2],2);
  data->GetCellData()->SetActiveAttribute(activeAttributes[3],3);
  data->GetCellData()->SetActiveAttribute(activeAttributes[4],4);
}
//----------------------------------------------------------------------------
// This method is broadcast, so is called on every process (even the client).
void vtkPVClientServerModule::SendCompleteArrays(vtkDataSet *data)
{
  if (this->ClientMode)
    {
    return;
    }

  int myId = this->Controller->GetLocalProcessId();
  int numProcs = this->Controller->GetNumberOfProcesses();
  int nonEmptyFlag;
  int requestFlag;
  int i;

  // Info for sends.
  int num;
  int type;
  int numComps;
  int nameLength;
  const char *name = NULL;
  vtkDataArray *array;
  int activeAttributes[5];


  // ---- Are we a candidate to send.
  if (data->GetNumberOfPoints() == 0 &&
      data->GetNumberOfCells() == 0)
    {
    nonEmptyFlag = 0;
    }
  else
    {
    nonEmptyFlag = 1;
    }
  // Special case: All procs are empty.
  // The last proc should send anyway.
  if (myId == numProcs-1)
    {
    nonEmptyFlag = 1;
    }

  // ---- Does the client want us to send.
  // Process 0 is always requested.
  requestFlag = 1;
  if (myId > 0)
    {
    this->Controller->Receive(&requestFlag, 1, 0, 778432);
    // If this process is requested to send info,
    // respond whether we have info to send..
    if (requestFlag)
      {
      this->Controller->Send(&nonEmptyFlag, 1, 0, 987243);
      }
    else
      { // Our information is not needed. Our job is finished.
      return;
      }
    if ( ! nonEmptyFlag)
      { // We do not have anything to send. Our job is finished.
      return;
      }
    }

  // ---- Send info.  Convert arrays to info and send.
  if (requestFlag && nonEmptyFlag)
    { // We are the process choosen to send.
    vtkMultiProcessController *controller;
    int toId;
    
    if (myId == 0)
      { // Root node has to dismiss all other processes.
      for (i = 1; i < numProcs; ++i)
        {
        requestFlag = 0;
        this->Controller->Send(&requestFlag, 1, i, 778432);
        }
      // Root has to send through the socket controller.
      controller = this->SocketController;
      toId = 1;
      }
    else
      { // Other processes have to relay through Root (process 0).
      controller = this->Controller;
      toId = 0;
      }

    // Send all the array info.
    // First point data.
    num = data->GetPointData()->GetNumberOfArrays();
    controller->Send(&num, 1, toId, 987244);
    for (i = 0; i < num; ++i)
      {
      array = data->GetPointData()->GetArray(i);
      type = array->GetDataType();

      controller->Send(&type, 1, toId, 987245);
      numComps = array->GetNumberOfComponents();

      controller->Send(&numComps, 1, toId, 987246);
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      nameLength = vtkString::Length(name)+1;
      controller->Send(&nameLength, 1, toId, 987247);
      // I am pretty sure that Send does not modify the string.
      controller->Send(const_cast<char*>(name), nameLength, toId, 987248);
      }
    data->GetPointData()->GetAttributeIndices(activeAttributes);
    controller->Send(activeAttributes, 5, toId, 987258);
    // Next cell data.
    num = data->GetCellData()->GetNumberOfArrays();
    controller->Send(&num, 1, toId, 987244);
    for (i = 0; i < num; ++i)
      {
      array = data->GetCellData()->GetArray(i);
      type = array->GetDataType();
      controller->Send(&type, 1, toId, 987245);
      numComps = array->GetNumberOfComponents();
      controller->Send(&numComps, 1, toId, 987246);
      name = array->GetName();
      if (name == NULL)
        {
        name = "";
        }
      nameLength = vtkString::Length(name+1);
      controller->Send(&nameLength, 1, toId, 987247);
      controller->Send(const_cast<char*>(name), nameLength, toId, 987248);
      }
    data->GetCellData()->GetAttributeIndices(activeAttributes);
    controller->Send(activeAttributes, 5, toId, 987258);
    
    return;
    } // End sending array info.

  // ---- Relay information
  // Only process 0 (that has no data) should make it to this point.
  // Tell the satellites (one bby one) we need their information.
  requestFlag = 1;
  for (i = 1; i < numProcs; ++i)
    {
    this->Controller->Send(&requestFlag, 1, i, 778432);
    if (requestFlag)
      {
      this->Controller->Receive(&nonEmptyFlag, 1, i, 987243);
      if (nonEmptyFlag)
        {
        // We have picked this process to send information.
        // Do not request processes from any other process.
        requestFlag = 0;

        // Relay.
        // First point data.
        this->Controller->Receive(&num, 1, i, 987244);
        this->SocketController->Send(&num, 1, 1, 987244);
        for (i = 0; i < num; ++i)
          {
          this->Controller->Receive(&type, 1, i, 987245);
          this->SocketController->Send(&type, 1, 1, 987245);
          this->Controller->Receive(&numComps, 1, i, 987246);
          this->SocketController->Send(&numComps, 1, 1, 987246);
          this->Controller->Receive(&nameLength, 1, i, 987247);
          this->SocketController->Send(&nameLength, 1, 1, 987247);
          char *buffer = new char[nameLength+1];
          this->Controller->Receive(const_cast<char*>(buffer), nameLength, i, 987248);
          this->SocketController->Send(const_cast<char*>(buffer), nameLength, 1, 987248);
          delete [] buffer;
          }
        this->Controller->Receive(activeAttributes, 5, i, 987258);
        this->SocketController->Send(activeAttributes, 5, 1, 987258);
        // Next cell data.
        this->Controller->Receive(&num, 1, i, 987244);
        this->SocketController->Send(&num, 1, 1, 987244);
        for (i = 0; i < num; ++i)
          {
          this->Controller->Receive(&type, 1, i, 987245);
          this->SocketController->Send(&type, 1, 1, 987245);
          this->Controller->Receive(&numComps, 1, i, 987246);
          this->SocketController->Send(&numComps, 1, 1, 987246);
          this->Controller->Receive(&nameLength, 1, i, 987247);
          this->SocketController->Send(&nameLength, 1, 1, 987247);
          this->Controller->Receive(const_cast<char*>(name), nameLength, i, 987248);
          this->SocketController->Send(const_cast<char*>(name), nameLength, 1, 987248);
          }
        this->Controller->Receive(activeAttributes, 5, i, 987258);
        this->SocketController->Send(activeAttributes, 5, 1, 987258);
        } // end choosen process (for sending)
      } // if requested
    } // i numProcs loop
}


//----------------------------------------------------------------------------
void vtkPVClientServerModule::InitializePVDataPartition(vtkPVData *pvd)
{
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 0",
                        pvd->GetMapperTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 0",
                        pvd->GetLODMapperTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 1",
                        pvd->GetUpdateSuppressorTclName());
  this->BroadcastScript("[$Application GetProcessModule] InitializePartition %s 1",
                        pvd->GetLODUpdateSuppressorTclName());
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
