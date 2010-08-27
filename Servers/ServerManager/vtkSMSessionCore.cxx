/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSessionCore.h"

#include "vtkClientServerID.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerInterpreterInitializer.h"
#include "vtkClientServerStream.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMObject.h"
#include "vtkProcessModule2.h"
#include "vtkSMProxyDefinitionManager.h"

#include "assert.h"

namespace
{
  void RMICallback(void *localArg, void *remoteArg,
    int vtkNotUsed(remoteArgLength), int vtkNotUsed(remoteProcessId))
    {
    vtkSMSessionCore* sessioncore = reinterpret_cast<vtkSMSessionCore*>(localArg);
    unsigned char type = *(reinterpret_cast<unsigned char*>(remoteArg));
    switch (type)
      {
      case vtkSMSessionCore::PUSH_STATE:
      sessioncore->PushStateSatelliteCallback();
      break;
      }
    }
};
//****************************************************************************/
//                        Internal Class
//****************************************************************************/
class vtkSMSessionCore::vtkInternals
{
public:

  //---------------------------------------------------------------------------
  void Delete(vtkTypeUInt32 globalUniqueId)
    {
    PMObjectMapType::iterator iter = this->PMObjectMap.find(globalUniqueId);
    if (iter != this->PMObjectMap.end())
      {
      iter->second.GetPointer()->Finalize();
      this->PMObjectMap.erase(iter);
      }
    }

  //---------------------------------------------------------------------------
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalUniqueId)
    {
    PMObjectMapType::iterator iter = this->PMObjectMap.find(globalUniqueId);
    if (iter != this->PMObjectMap.end())
      {
      return iter->second;
      }

    return NULL; // Did not find it
    }
  //---------------------------------------------------------------------------
  typedef vtkstd::map<vtkTypeUInt32, vtkSmartPointer<vtkPMObject> >
    PMObjectMapType;
  PMObjectMapType PMObjectMap;
};

//****************************************************************************/
vtkStandardNewMacro(vtkSMSessionCore);
//----------------------------------------------------------------------------
vtkSMSessionCore::vtkSMSessionCore()
{
  this->Interpreter =
    vtkClientServerInterpreterInitializer::GetInitializer()->NewInterpreter();
  this->Internals = new vtkInternals();

  this->ProxyDefinitionManager = vtkSMProxyDefinitionManager::New();

  this->ParallelController = vtkMultiProcessController::GetGlobalController();
  if (this->ParallelController &&
    this->ParallelController->GetLocalProcessId() > 0)
    {
    this->ParallelController->AddRMI(&RMICallback, this,
      ROOT_SATELLITE_RMI_TAG);
    }
}

//----------------------------------------------------------------------------
vtkSMSessionCore::~vtkSMSessionCore()
{
  this->Interpreter->Delete();
  if (this->ParallelController &&
    this->ParallelController->GetLocalProcessId() == 0)
    {
    this->ParallelController->TriggerBreakRMIs();
    }
  delete this->Internals;
  this->ProxyDefinitionManager->Delete();
  this->ProxyDefinitionManager = NULL;
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PushStateInternal(vtkSMMessage* message)
{
  // When the control reaches here, we are assured that the PMObject needs be
  // created/exist on the local process.
  vtkPMObject* obj = this->Internals->GetPMObject(message->global_id());
  if (!obj)
    {
    if (!message->HasExtension(DefinitionHeader::server_class))
      {
      vtkErrorMacro("Message missing DefinitionHeader."
        "Aborting for debugging purposes.");
      abort();
      }
    // Create the corresponding PM object.
    vtkClientServerID tempID = this->Interpreter->GetNextAvailableId();
    vtkClientServerStream stream;
    stream << vtkClientServerStream::New
           << message->GetExtension(DefinitionHeader::server_class).c_str()
           << tempID
           << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    obj = vtkPMObject::SafeDownCast(
      this->Interpreter->GetObjectFromID(tempID, /*noerror*/ 0));
    if (obj == NULL)
      {
      vtkErrorMacro("Object must be a vtkPMObject subclass. "
        "Aborting for debugging purposes.");
      abort();
      }
    obj->Initialize(this);
    this->Internals->PMObjectMap[message->global_id()] = obj;

    // release the reference held by the interpreter.
    stream << vtkClientServerStream::Delete << tempID
      << vtkClientServerStream::End;
    this->Interpreter->ProcessStream(stream);
    }

  // Push the message to the PMObject.
  obj->Push(message);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PushState(vtkSMMessage* message)
{
  // This can only be called on the root node.
  assert(this->ParallelController == NULL ||
    this->ParallelController->GetLocalProcessId() == 0);


  if ( (message->location() & vtkProcessModule2::SERVERS) != 0)
    {
    // send message to satellites and then start processing.

    if (this->ParallelController &&
      this->ParallelController->GetNumberOfProcesses() > 1 &&
      this->ParallelController->GetLocalProcessId() == 0)
      {
      // Forward the message to the satellites if the object is expected to exist
      // on the satellites.

      // FIXME: There's one flaw in this logic. If a object is to be created on
      // DATA_SERVER_ROOT, but on all RENDER_SERVER nodes, then in render-server
      // configuration, the message will end up being send to all data-server
      // nodes as well. Although we never do that presently, it's a possibility
      // and we should fix this.
      unsigned char type = PUSH_STATE;
      this->ParallelController->TriggerRMIOnAllChildren(&type, 1,
        ROOT_SATELLITE_RMI_TAG);

      int byte_size = message->ByteSize();
      unsigned char *raw_data = new unsigned char[byte_size + 1];
      message->SerializeToArray(raw_data, byte_size);
      this->ParallelController->Broadcast(&byte_size, 1, 0);
      this->ParallelController->Broadcast(raw_data, byte_size, 0);
      delete [] raw_data;
      }
    }

  // When the control reaches here, we are assured that the PMObject needs be
  // created/exist on the local process.
  this->PushStateInternal(message);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PushStateSatelliteCallback()
{
  cout << "PushStateSatelliteCallback" << endl;
  int byte_size = 0;
  this->ParallelController->Broadcast(&byte_size, 1, 0);

  unsigned char *raw_data = new unsigned char[byte_size + 1];
  this->ParallelController->Broadcast(raw_data, byte_size, 0);

  vtkSMMessage message;
  if (!message.ParseFromArray(raw_data, byte_size))
    {
    vtkErrorMacro("Failed to parse protobuf message.");
    }
  else
    {
    this->PushStateInternal(&message);
    }
  delete [] raw_data;
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::PullState(vtkSMMessage* message)
{
  vtkPMObject* obj;
  if(true &&  // FIXME make sure that the PMObject should be created here
     (obj = this->Internals->GetPMObject(message->global_id())))
    {
    obj->Pull(message);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::Invoke(vtkSMMessage* message)
{
  vtkPMObject* obj;
  if(true &&  // FIXME make sure that the PMObject should be created here
     (obj = this->Internals->GetPMObject(message->global_id())))
    {
    obj->Invoke(message);
    }
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::DeletePMObject(vtkSMMessage* message)
{
  this->Internals->Delete(message->global_id());
}

//----------------------------------------------------------------------------
