/*=========================================================================

  Program:   ParaView
  Module:    vtkClientConnection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkClientConnection.h"

#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkPVConfig.h" // for PARAVIEW_VERSION_MAJOR etc.
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkSocketCommunicator.h"
#include "vtkSocketController.h"

#include <vtkstd/new>

//-----------------------------------------------------------------------------
// RMI Callbacks.

// Called when requesting the last result.
void vtkClientConnectionLastResultRMI(void* localArg, void* , int, int)
{
  vtkClientConnection* self = (vtkClientConnection*)localArg;
  self->SendLastResult();
}

//-----------------------------------------------------------------------------
// Called when requesting to process the stream on Server (root and satellites).
void vtkClientConnectionRMI(void *localArg, void *remoteArg,
  int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  try
    {
    vtkClientServerStream stream;
    stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);

    vtkClientConnection* self = (vtkClientConnection*)localArg;
    self->Activate();

    // Tell process module to send it to SelfConnection.
    vtkProcessModule::GetProcessModule()->SendStream(
      vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER, stream);

    self->Deactivate();
    }
  catch (vtkstd::bad_alloc)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_BAD_ALLOC);
    }
  catch (...)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_UNKNOWN);
    }
}

//-----------------------------------------------------------------------------
// Called when requesting to process the stream on Root Node only.
void vtkClientConnectionRootRMI(void *localArg, void *remoteArg,
  int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  try
    {
    vtkClientServerStream stream;
    stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);

    vtkClientConnection* self = (vtkClientConnection*)localArg;
    self->Activate();

    // Tell process module to send it to SelfConnection Root.
    vtkProcessModule::GetProcessModule()->SendStream(
      vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);

    self->Deactivate();
    }
  catch (vtkstd::bad_alloc)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_BAD_ALLOC);
    }
  catch (...)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_UNKNOWN);
    }
}

//-----------------------------------------------------------------------------
// Called on client is requesting Information from this server.
void vtkClientConnectionGatherInformationRMI(void *localArg, 
  void *remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  try
    {
    vtkClientServerStream stream;
    stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);

    vtkClientConnection* self = (vtkClientConnection*)localArg;
    self->SendInformation(stream);
    }
  catch (vtkstd::bad_alloc)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_BAD_ALLOC);
    }
  catch (...)
    {
    vtkProcessModule::GetProcessModule()->ExceptionEvent(
      vtkProcessModule::EXCEPTION_UNKNOWN);
    }
}

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkClientConnection);
vtkCxxRevisionMacro(vtkClientConnection, "1.6");
//-----------------------------------------------------------------------------
vtkClientConnection::vtkClientConnection()
{
}

//-----------------------------------------------------------------------------
vtkClientConnection::~vtkClientConnection()
{
}

//-----------------------------------------------------------------------------
int vtkClientConnection::Initialize(int vtkNotUsed(argc), char** vtkNotUsed(argv))
{
  // Ensure that we are indeed the root node.
  if (vtkMultiProcessController::GetGlobalController()->
    GetLocalProcessId() != 0)
    {
    vtkErrorMacro("vtkClientConnection can only be initialized on the Root node.");
    return 1;
    }

  if (!this->AuthenticateWithClient())
    {
    vtkErrorMacro("Failed to authenticate with client.");
    return 1;
    }
  
  this->SetupRMIs();
  
  return 0;
}

//-----------------------------------------------------------------------------
void vtkClientConnection::Finalize()
{
  this->GetSocketController()->CloseConnection();
  this->Superclass::Finalize();
}

//-----------------------------------------------------------------------------
int vtkClientConnection::AuthenticateWithClient()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVOptions* options = pm->GetOptions();
  vtkMultiProcessController* globalController = 
    vtkMultiProcessController::GetGlobalController();

  
  // Check Connection ID.
  int connectID = 0;
  // Receive the connect id from client
  this->Controller->Receive(&connectID, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
 
  int match = (connectID == options->GetConnectID());
  
  // Tell the client the result of id check
  this->Controller->Send(&match, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  if (!match)
    {
    // The ids don't match. Flag an error. This connection is doomed.
    // The ProcessModule will simply drop it.
    vtkErrorMacro("Connection ID mismatch: " << connectID << " != " 
      << options->GetConnectID() );
    return 0;
    }

  // Check Version.
  // Receive the client version
  int majorVersion =0, minorVersion =0, patchVersion =0;
  this->Controller->Receive(&majorVersion, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  this->Controller->Receive(&minorVersion, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  this->Controller->Receive(&patchVersion, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);

  match = ( (majorVersion == PARAVIEW_VERSION_MAJOR) ||
    (minorVersion == PARAVIEW_VERSION_MINOR) );
  // Tell the client the result of version check
  this->Controller->Send(&match, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG); 
  
  if (!match)
    {
    vtkErrorMacro("Client-Server Version mismatch. "
      << "Connection will be aborted.");
    return 0;
    }

  int numProcs = globalController->GetNumberOfProcesses();
  
  // send the number of server processes as a handshake. 
  this->Controller->Send(&numProcs, 1, 1, 
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);

  // Now get the vtkClientServerID to assign to this connection.
  // The client tells the ID to assign.
  int id = 0;
  this->Controller->Receive(&id, 1, 1,
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);
  if (id == 0)
    {
    vtkErrorMacro("Failed to get correct ID to assign to this connection.");
    }
  else
    {
    this->SelfID.ID = static_cast<vtkTypeUInt32>(id);
    // We will assign the SelfID to this connection on the local interpreter.

    // Now, on satellites, we want this ID to be assigned to a null object,
    // since, this connection has no representation on the satellites.
    // So we first assign the ID as null object on every node and then 
    // reassign the id to this connection on the root node.
    
    vtkClientServerStream stream;
    stream << vtkClientServerStream::Assign
      << this->SelfID << 0 //NULL.
      << vtkClientServerStream::End;

    pm->SendStream(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER, stream, 1);

    // Reassign the ID on the root.
    stream << vtkClientServerStream::Delete
      << this->SelfID << vtkClientServerStream::End;
    stream << vtkClientServerStream::Assign
      << this->SelfID << this
      << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);
    }

  // Let the client know the ID we assigned.
  this->Controller->Send(&id, 1, 1,
    vtkRemoteConnection::CLIENT_SERVER_COMMUNICATION_TAG);

  return 1; //SUCCESS.
}

//-----------------------------------------------------------------------------
void vtkClientConnection::SetupRMIs()
{
  // We have succesfully authenticated with the client. The connection
  // is deemed valid. Now set up RMIs so that we can communicate.

  this->Controller->AddRMI(vtkClientConnectionLastResultRMI,
    (void *)(this),
    vtkRemoteConnection::CLIENT_SERVER_LAST_RESULT_TAG);
  
  // for SendMessages
  this->Controller->AddRMI(vtkClientConnectionRMI, 
    (void *)(this),
    vtkRemoteConnection::CLIENT_SERVER_RMI_TAG);
  
  this->Controller->AddRMI(vtkClientConnectionRootRMI, 
    (void *)(this),
    vtkRemoteConnection::CLIENT_SERVER_ROOT_RMI_TAG);

  this->Controller->AddRMI(vtkClientConnectionGatherInformationRMI,
    (void*)(this),
    vtkRemoteConnection::CLIENT_SERVER_GATHER_INFORMATION_RMI_TAG);
  
  this->Controller->CreateOutputWindow();
  
  vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
    this->Controller->GetCommunicator());
  if (comm)
    {
    comm->SetReportErrors(0);
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::SendLastResult()
{
  const unsigned char* data;
  size_t length = 0;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerInterpreter* interpreter = pm->GetInterpreter();
  interpreter->GetLastResult().GetData(&data, &length);

  int len = static_cast<int>(length);
  
  this->GetSocketController()->Send(&len, 1, 1,
    vtkRemoteConnection::ROOT_RESULT_LENGTH_TAG);
  if(length > 0)
    {
    this->GetSocketController()->Send((char*)(data), length, 1,
      vtkRemoteConnection::ROOT_RESULT_TAG);
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::SendInformation(vtkClientServerStream& stream)
{
  // First, gather local information.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  
  const char* infoClassName;
  vtkClientServerID id;
  stream.GetArgument(0, 0, &infoClassName);
  stream.GetArgument(0, 1, &id);
  
  vtkObject* o = vtkInstantiator::CreateInstance(infoClassName);
  vtkPVInformation* info = vtkPVInformation::SafeDownCast(o);
  if (info)
    {
    pm->GatherInformation(vtkProcessModuleConnectionManager::GetSelfConnectionID(), 
      vtkProcessModule::DATA_SERVER, info, id);
    
    vtkClientServerStream css;
    info->CopyToStream(&css);
    size_t length;
    const unsigned char* data;
    css.GetData(&data, &length);
    int len = length;
    this->GetSocketController()->Send(&len, 1, 1,
      vtkRemoteConnection::ROOT_INFORMATION_LENGTH_TAG);
    this->GetSocketController()->Send(const_cast<unsigned char*>(data),
      length, 1, vtkRemoteConnection::ROOT_INFORMATION_TAG);
    }
  else
    {
    vtkErrorMacro("Could not create information object.");
    // let client know.
    int len = 0; 
    this->GetSocketController()->Send(&len, 1, 1,
      vtkRemoteConnection::ROOT_INFORMATION_LENGTH_TAG);
    }
  
  if (o) 
    { 
    o->Delete(); 
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


