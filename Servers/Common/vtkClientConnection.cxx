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
#include "vtkUndoSet.h"
#include "vtkUndoStack.h"

#include <vtkstd/new>
#include <vtkstd/string>

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
void vtkClientConnectionRMI(void *vtkNotUsed(localArg), void *remoteArg,
  int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  try
    {
    vtkClientServerStream stream;
    stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);

    // Tell process module to send it to SelfConnection.
    vtkProcessModule::GetProcessModule()->SendStream(
      vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER, stream);

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
void vtkClientConnectionRootRMI(void *vtkNotUsed(localArg), void *remoteArg,
  int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  try
    {
    vtkClientServerStream stream;
    stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);

    // Tell process module to send it to SelfConnection Root.
    vtkProcessModule::GetProcessModule()->SendStream(
      vtkProcessModuleConnectionManager::GetSelfConnectionID(),
      vtkProcessModule::DATA_SERVER_ROOT, stream);
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
  vtkClientServerStream stream;
  stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);
  vtkClientConnection* self = (vtkClientConnection*)localArg;
  try
    {
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
// Called when the client wants to push undo set.
void vtkClientConnectionPushUndoXML(void* localArg,
  void* remoteArg, int remoteArgLength, int vtkNotUsed(remoteProcessId))
{
  vtkClientServerStream stream;
  stream.SetData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);
  const char* data;
  const char* label;
  stream.GetArgument(0, 0, &label);
  stream.GetArgument(0, 1, &data);
  vtkClientConnection* self = (vtkClientConnection*)localArg;
  self->PushUndoXMLRMI(label, data);
}

//-----------------------------------------------------------------------------
void vtkClientConnectionUndo(void* localArg, void* , int , int )
{
  vtkClientConnection* self = (vtkClientConnection*)localArg;
  self->UndoRMI();
}

//-----------------------------------------------------------------------------
void vtkClientConnectionRedo(void* localArg, void* , int , int )
{
  vtkClientConnection* self = (vtkClientConnection*)localArg;
  self->RedoRMI();
}

//-----------------------------------------------------------------------------
class vtkClientConnectionUndoSet : public vtkUndoSet
{
public:
  static vtkClientConnectionUndoSet* New();
  vtkTypeMacro(vtkClientConnectionUndoSet, vtkUndoSet);
 
  virtual int Undo() 
    {
    if (!this->Connection)
      {
      return 0;
      }
    // Send the undo XML to the client.
    // We may want to let all clients know that someone is undoing. This will unsure
    // that while the undo is taking place, no one changes the server state.
    this->Connection->SendUndoXML(this->XMLData.c_str());
    return 1;
    }
  
  virtual int Redo() 
    {
    if (!this->Connection)
      {
      return 0;
      }
    // Send the undo XML to the client.
    // We may want to let all clients know that someone is redoing. This will unsure
    // that while the redo is taking place, no one changes the server state.
    this->Connection->SendRedoXML(this->XMLData.c_str());
    return 1;
    }

  void SetXMLData(const char* data)
    {
    this->XMLData = vtkstd::string(data);
    }
  const char* GetXMLData(int &length)
    {
    length = static_cast<int>(this->XMLData.length());
    return this->XMLData.c_str();
    }
  void SetConnection(vtkClientConnection* con)
    {
    this->Connection = con;
    }
protected:
  vtkClientConnectionUndoSet() 
    {
    this->Connection = 0;
    };
  ~vtkClientConnectionUndoSet(){ };

  vtkstd::string XMLData;
  vtkClientConnection* Connection;
private:
  vtkClientConnectionUndoSet(const vtkClientConnectionUndoSet&);
  void operator=(const vtkClientConnectionUndoSet&);
};

vtkStandardNewMacro(vtkClientConnectionUndoSet);
//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkClientConnection);
//-----------------------------------------------------------------------------
vtkClientConnection::vtkClientConnection()
{
  this->UndoRedoStack = vtkUndoStack::New();
}

//-----------------------------------------------------------------------------
vtkClientConnection::~vtkClientConnection()
{
  this->UndoRedoStack->Delete();
}

//-----------------------------------------------------------------------------
int vtkClientConnection::Initialize(int argc, char** argv, int *partitionId)
{
  this->Superclass::Initialize(argc, argv, partitionId);

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

  this->Controller->AddRMI(vtkClientConnectionPushUndoXML,
    (void*)(this),
    vtkRemoteConnection::CLIENT_SERVER_PUSH_UNDO_XML_TAG);
 
  this->Controller->AddRMI(vtkClientConnectionUndo,
    (void*)this, vtkRemoteConnection::UNDO_XML_TAG);

  this->Controller->AddRMI(vtkClientConnectionRedo,
    (void*)this, vtkRemoteConnection::REDO_XML_TAG);
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
    int len = static_cast<int>(length);
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
void vtkClientConnection::PushUndoXMLRMI(const char* label, const char* data)
{
  vtkClientConnectionUndoSet* elem = vtkClientConnectionUndoSet::New();
  elem->SetXMLData(data);
  elem->SetConnection(this);
  this->UndoRedoStack->Push(label, elem);
  elem->Delete();
}

//-----------------------------------------------------------------------------
void vtkClientConnection::UndoRMI()
{
  if (this->UndoRedoStack->CanUndo())
    {
    this->UndoRedoStack->Undo();
    }
  else
    {
    vtkErrorMacro("Nothing to undo.");
    this->SendUndoXML(""); // essential the send to the client an empty string 
      // otherwise the client keeps on waiting.
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::RedoRMI()
{
  if (this->UndoRedoStack->CanRedo())
    {
    this->UndoRedoStack->Redo();
    }
  else
    {
    vtkErrorMacro("Nothing to redo.");
    this->SendRedoXML(""); // essential the send to the client an empty string 
      // otherwise the client keeps on waiting.
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::SendUndoXML(const char* xml)
{
  vtkSocketController* controller = this->GetSocketController();
  int len = static_cast<int>(strlen(xml));
  controller->Send(&len, 1, 1, vtkRemoteConnection::UNDO_XML_TAG);
  if (len > 0)
    {
    controller->Send(const_cast<char*>(xml), 
      len, 1, vtkRemoteConnection::UNDO_XML_TAG);
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::SendRedoXML(const char* xml)
{
  vtkSocketController* controller = this->GetSocketController();
  int len = static_cast<int>(strlen(xml));
  controller->Send(&len, 1, 1, vtkRemoteConnection::REDO_XML_TAG);
  if (len > 0)
    {
    controller->Send(const_cast<char*>(xml), 
      len, 1, vtkRemoteConnection::REDO_XML_TAG);
    }
}

//-----------------------------------------------------------------------------
void vtkClientConnection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


