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
#include "vtkCollection.h"
#include "vtkInstantiator.h"
#include "vtkMemberFunctionCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkProcessModule.h"
#include "vtkPVInformation.h"
#include "vtkPVOptions.h"
#include "vtkSmartPointer.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyDefinitionManager.h"
#include "vtkSMRemoteObject.h"
#include "vtkSMSessionCoreInterpreterHelper.h"
#include "vtkSMSession.h"

#include "assert.h"
#include <fstream>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

#define LOG(x)\
  if (this->LogStream)\
    {\
    (*this->LogStream) << "" x << endl;\
    }

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

    case vtkSMSessionCore::GATHER_INFORMATION:
      sessioncore->GatherInformationStatelliteCallback();
      break;

    case vtkSMSessionCore::EXECUTE_STREAM:
      sessioncore->ExecuteStreamSatelliteCallback();
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
  vtkInternals()
    {
    }
  ~vtkInternals()
    {
    // Remove PMObject left in the right order...
    PMObjectMapType::iterator iter;
    int nbFound, nbDelete;
    nbFound = nbDelete = 1;
    while(nbFound > 0)
      {
      nbDelete = nbFound = 0;
      for(iter = this->PMObjectMap.begin();iter != this->PMObjectMap.end(); iter++)
        {
        vtkPMObject* obj = iter->second;
        if(obj)
          {
          nbFound++;
          if(obj->GetReferenceCount() == 1)
            {
            nbDelete++;
            obj->Delete();
            }
          }
        }
      }
    }
  //---------------------------------------------------------------------------
  void Delete(vtkTypeUInt32 globalUniqueId)
    {
    // Remove PM
    PMObjectMapType::iterator iter = this->PMObjectMap.find(globalUniqueId);
    if (iter != this->PMObjectMap.end())
      {
      if(iter->second)
        {
        iter->second->Delete();
        }
      }
    }
  //---------------------------------------------------------------------------
  void DeleteRemoteObject(vtkTypeUInt32 globalUniqueId)
    {
    // Remove Remote
    RemoteObjectMapType::iterator iter2 = this->RemoteObjectMap.find(globalUniqueId);
    if (iter2 != this->RemoteObjectMap.end())
      {
      this->RemoteObjectMap.erase(iter2);
      }
    }

  //---------------------------------------------------------------------------
  vtkPMObject* GetPMObject(vtkTypeUInt32 globalUniqueId)
    {
    PMObjectMapType::iterator iter = this->PMObjectMap.find(globalUniqueId);
    if (iter != this->PMObjectMap.end())
      {
      return iter->second.GetPointer();
      }

    return NULL; // Did not find it
    }
  //---------------------------------------------------------------------------
  vtkSMRemoteObject* GetRemoteObject(vtkTypeUInt32 globalUniqueId)
    {
    RemoteObjectMapType::iterator iter = this->RemoteObjectMap.find(globalUniqueId);
    if (iter != this->RemoteObjectMap.end())
      {
      return iter->second.GetPointer();
      }

    return NULL; // Did not find it
    }

  //---------------------------------------------------------------------------
  void GetAllRemoteObjects(vtkCollection* collection)
    {
    RemoteObjectMapType::iterator iter = this->RemoteObjectMap.begin();
    while(iter != this->RemoteObjectMap.end())
      {
      if(iter->second)
        {
        collection->AddItem(iter->second);
        }
      iter++;
      }
    }
  //---------------------------------------------------------------------------
  void PrintRemoteMap()
    {
    RemoteObjectMapType::iterator iter = this->RemoteObjectMap.begin();
    while(iter != this->RemoteObjectMap.end())
      {
      cout << "RemoteObject map - Id: " << iter->first << endl; //" - RefCount: "
           //<< iter->second->GetReferenceCount() << endl;
      iter++;
      }
    }
  //---------------------------------------------------------------------------
  typedef vtkstd::map<vtkTypeUInt32, vtkWeakPointer<vtkPMObject> >
    PMObjectMapType;
  typedef vtkstd::map<vtkTypeUInt32, vtkWeakPointer<vtkSMRemoteObject> >
    RemoteObjectMapType;
  PMObjectMapType PMObjectMap;
  RemoteObjectMapType RemoteObjectMap;
};

//****************************************************************************/
vtkStandardNewMacro(vtkSMSessionCore);
//----------------------------------------------------------------------------
vtkSMSessionCore::vtkSMSessionCore()
{
  this->Interpreter =
    vtkClientServerInterpreterInitializer::GetInitializer()->NewInterpreter();

  vtkSMSessionCoreInterpreterHelper* helper =
    vtkSMSessionCoreInterpreterHelper::New();
  helper->SetCore(this);

  vtkClientServerStream stream;
  stream << vtkClientServerStream::Assign
         << vtkClientServerID(1)
         << helper
         << vtkClientServerStream::End;
  this->Interpreter->ProcessStream(stream);
  helper->Delete();

  vtkMemberFunctionCommand<vtkSMSessionCore>* observer =
      vtkMemberFunctionCommand<vtkSMSessionCore>::New();
  observer->SetCallback(*this, &vtkSMSessionCore::OnInterpreterError);
  this->Interpreter->AddObserver( vtkCommand::UserEvent, observer );
  observer->Delete();

  this->Internals = new vtkInternals();

  this->ProxyDefinitionManager = vtkSMProxyDefinitionManager::New();

  this->ParallelController = vtkMultiProcessController::GetGlobalController();
  if (this->ParallelController &&
    this->ParallelController->GetLocalProcessId() > 0)
    {
    this->ParallelController->AddRMI(&RMICallback, this,
      ROOT_SATELLITE_RMI_TAG);
    }

  this->LogStream = NULL;
  // Initialize logging, if enabled.
  if(vtkProcessModule::GetProcessModule())
    {
    vtkPVOptions* options = vtkProcessModule::GetProcessModule()->GetOptions();
    if (options->GetLogFileName())
      {
      vtksys_ios::ostringstream filename;
      filename  << options->GetLogFileName();
      if (this->ParallelController->GetNumberOfProcesses() > 1)
        {
        filename << this->ParallelController->GetLocalProcessId();
        }
      this->LogStream = new ofstream(filename.str().c_str());
      LOG("Log for " << options->GetArgv0() << " ("
          << this->ParallelController->GetLocalProcessId() << ")");
      }
    }
}

//----------------------------------------------------------------------------
vtkSMSessionCore::~vtkSMSessionCore()
{
  LOG("Closing session");
  this->Interpreter->Delete();
  this->Interpreter = 0;
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
void vtkSMSessionCore::OnInterpreterError( vtkObject*, unsigned long,
                                           void* calldata)
{
  if (!vtkProcessModule::GetProcessModule()->GetReportInterpreterErrors())
    {
    return;
    }
  const char* errorMessage;
  vtkClientServerInterpreterErrorCallbackInfo* info
    = static_cast<vtkClientServerInterpreterErrorCallbackInfo*>(calldata);
  const vtkClientServerStream& last = this->Interpreter->GetLastResult();
  if(last.GetNumberOfMessages() > 0 &&
    (last.GetCommand(0) == vtkClientServerStream::Error) &&
    last.GetArgument(0, 0, &errorMessage) && this->Interpreter->GetGlobalWarningDisplay())
    {
    vtksys_ios::ostringstream error;
    error << "\nwhile processing\n";
    info->css->PrintMessage(error, info->message);
    error << ends;
    vtkErrorMacro(<< errorMessage << error.str().c_str());
    vtkErrorMacro("Aborting execution for debugging purposes.");
    abort();
    }
}

//----------------------------------------------------------------------------
int vtkSMSessionCore::GetNumberOfProcesses()
{
  return this->ParallelController->GetNumberOfProcesses();
}

//----------------------------------------------------------------------------
vtkPMObject* vtkSMSessionCore::GetPMObject(vtkTypeUInt32 globalid)
{
  return this->Internals->GetPMObject(globalid);
}
//----------------------------------------------------------------------------
vtkSMRemoteObject* vtkSMSessionCore::GetRemoteObject(vtkTypeUInt32 globalid)
{
  return this->Internals->GetRemoteObject(globalid);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PushStateInternal(vtkSMMessage* message)
{
  LOG(
    << "----------------------------------------------------------------\n"
    << "Push State ( " << message->ByteSize() << " bytes )\n"
    << "----------------------------------------------------------------\n"
    << message->DebugString().c_str());

  vtkTypeUInt32 globalId = message->global_id();

  // FIXME handle this part as well for collaboration
  // 10 are the reserved area for non proxy object that must be shared
  if(globalId < 10)
    {
    // Special ParaView specific main components
    switch(globalId)
      {
      case 1: // ProxyManager
        // TODO - FIXME: Forward registration proxy to other clients...
        break;
      }
    return;
    }
  // FIXME ----------------------------------------------------------

  // When the control reaches here, we are assured that the PMObject needs be
  // created/exist on the local process.
  vtkPMObject* obj = this->Internals->GetPMObject(globalId);
  if (!obj)
    {
    if (!message->HasExtension(DefinitionHeader::server_class))
      {
      vtkErrorMacro("Message missing DefinitionHeader."
        "Aborting for debugging purposes.");
      abort();
      }
    // Create the corresponding PM object.
    vtkstd::string classname = message->GetExtension(DefinitionHeader::server_class);
    vtkObject* object;
    object = vtkInstantiator::CreateInstance(classname.c_str());
    if (!object)
      {
      vtkErrorMacro("Failed to instantiate " << classname.c_str());
      abort();
      }
    obj = vtkPMObject::SafeDownCast(object);
    if (obj == NULL)
      {
      vtkErrorMacro("Object must be a vtkPMObject subclass. "
        "Aborting for debugging purposes.");
      abort();
      }
    obj->SetGlobalID(globalId);
    obj->Initialize(this);
    this->Internals->PMObjectMap[globalId] = obj; // WeakPointer map

    LOG (
      << "----------------------------------------------------------------\n"
      << "New " << globalId << " : " << obj->GetClassName() <<"\n");
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

  if ( (message->location() & vtkProcessModule::SERVERS) != 0)
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
  LOG(
    << "----------------------------------------------------------------\n"
    << "Pull State ( " << message->ByteSize() << " bytes )\n"
    << "----------------------------------------------------------------\n"
    << message->DebugString().c_str());

  vtkPMObject* obj;

  if(message->global_id() < 10)
    {
    // Special ParaView specific main components
    switch(message->global_id())
      {
      case 1: // ProxyManager
        this->GetProxyDefinitionManager()->GetXMLDefinitionState(message);
        break;
      }
    }
  else if(true &&  // FIXME make sure that the PMObject should be created here
          (obj = this->Internals->GetPMObject(message->global_id())))
    {
    // Generic PMObject
    obj->Pull(message);
    }

  LOG(
    << "----------------------------------------------------------------\n"
    << "Pull State Reply ( " << message->ByteSize() << " bytes )\n"
    << "----------------------------------------------------------------\n"
    << message->DebugString().c_str());
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::ExecuteStream(
  vtkTypeUInt32 location, const vtkClientServerStream& stream,
  bool ignore_errors/*=false*/)
{
  if (stream.GetNumberOfMessages() == 0)
    {
    return;
    }

  // This can only be called on the root node.
  assert(this->ParallelController == NULL ||
    this->ParallelController->GetLocalProcessId() == 0);

  size_t byte_size;
  const unsigned char *raw_data;
  stream.GetData(&raw_data, &byte_size);

  if ( (location & vtkProcessModule::SERVERS) != 0)
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
      unsigned char type = EXECUTE_STREAM;
      this->ParallelController->TriggerRMIOnAllChildren(&type, 1,
        ROOT_SATELLITE_RMI_TAG);
      int size[2];
      size[0] = static_cast<int>(byte_size);
      size[1] = (ignore_errors? 1 : 0);
      this->ParallelController->Broadcast(size, 2, 0);
      this->ParallelController->Broadcast(
        const_cast<unsigned char*>(raw_data), size[0], 0);
      }
    }

  this->ExecuteStreamInternal(raw_data, byte_size, ignore_errors);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::ExecuteStreamSatelliteCallback()
{
  int byte_size[2] = {0, 0};
  this->ParallelController->Broadcast(byte_size, 2, 0);
  unsigned char *raw_data = new unsigned char[byte_size[0] + 1];
  this->ParallelController->Broadcast(raw_data, byte_size[0], 0);
  this->ExecuteStreamInternal(raw_data, byte_size[0], byte_size[1] != 0);
  delete [] raw_data;
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::ExecuteStreamInternal(
  const unsigned char* raw_message, size_t size, bool ignore_errors)
{
  LOG ("ExecuteStream\n");
  // FIXME_COLLABORATION : need to log the contents of the stream.

  this->Interpreter->ClearLastResult();

  int temp = this->Interpreter->GetGlobalWarningDisplay();
  this->Interpreter->SetGlobalWarningDisplay(ignore_errors? 0 : 1);
  this->Interpreter->ProcessStream(raw_message, size);
  this->Interpreter->SetGlobalWarningDisplay(temp);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::DeletePMObject(vtkSMMessage* message)
{
  LOG(
    << "----------------------------------------------------------------\n"
    << "Delete ( " << message->ByteSize() << " bytes )\n"
    << "----------------------------------------------------------------\n"
    << message->DebugString().c_str());
  this->Internals->Delete(message->global_id());
}

//----------------------------------------------------------------------------
bool vtkSMSessionCore::GatherInformationInternal(
  vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  if (globalid == 0)
    {
    information->CopyFromObject(NULL);
    return true;
    }

  // default is to gather information from VTKObject, if FromPMObject is true,
  // then gather from PMObject.
  vtkPMObject* pmobject = this->GetPMObject(globalid);
  if (!pmobject)
    {
    vtkErrorMacro("No object with global-id: " << globalid);
    return false;
    }

  vtkPMProxy* pmproxy = vtkPMProxy::SafeDownCast(pmobject);
  if (pmproxy /*&& !information->GetUsePMObject()*/)
    {
    vtkObject* object = vtkObject::SafeDownCast(pmproxy->GetVTKObject());
    information->CopyFromObject(object);
    }
  else
    {
    // gather information from PMObject itself.
    information->CopyFromObject(pmobject);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMSessionCore::GatherInformation(vtkTypeUInt32 location,
  vtkPVInformation* information, vtkTypeUInt32 globalid)
{
  // This can only be called on the root node.
  assert(this->ParallelController == NULL ||
    this->ParallelController->GetLocalProcessId() == 0);

  if (!this->GatherInformationInternal(information, globalid))
    {
    return false;
    }

  if (information->GetRootOnly() || (location & vtkProcessModule::SERVERS) == 0)
    {
    return true;
    }

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
    unsigned char type = GATHER_INFORMATION;
    this->ParallelController->TriggerRMIOnAllChildren(&type, 1,
      ROOT_SATELLITE_RMI_TAG);

    vtkMultiProcessStream stream;
    stream << information->GetClassName() << globalid;

    // serialize information parameters so all processes have the same ivars.
    information->CopyParametersToStream(stream);

    this->ParallelController->Broadcast(stream, 0);
    }

  return this->CollectInformation(information);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::GatherInformationStatelliteCallback()
{
  vtkMultiProcessStream stream;
  this->ParallelController->Broadcast(stream, 0);

  vtkstd::string classname;
  vtkTypeUInt32 globalid;
  stream >> classname >> globalid;

  vtkSmartPointer<vtkObject> o;
  o.TakeReference(vtkInstantiator::CreateInstance(classname.c_str()));
  vtkPVInformation* info = vtkPVInformation::SafeDownCast(o);
  if (info)
    {
    info->CopyParametersFromStream(stream);
    this->GatherInformationInternal(info, globalid);
    this->CollectInformation(info);
    }
  else
    {
    vtkErrorMacro("Could not gather information on Satellite.");
    // let the parent know, otherwise root will hang.
    this->CollectInformation(NULL);
    }
}

//----------------------------------------------------------------------------
bool vtkSMSessionCore::CollectInformation(vtkPVInformation* info)
{
  vtkMultiProcessController* controller = this->ParallelController;
  int myid = controller->GetLocalProcessId();
  int numProcs = controller->GetNumberOfProcesses();

  int children[2] = {2*myid + 1, 2*myid + 2};
  int parent = myid > 0? (myid-1)/2 : -1;

  // General rule is: receive from children and send to parent
  for (int childno=0; childno < 2; childno++)
    {
    int childid = children[childno];
    if (childid >= numProcs)
      {
      // Skip nonexistent children.
      continue;
      }

    int length;
    controller->Receive(&length, 1, childid, ROOT_SATELLITE_INFO_TAG);
    if (length <= 0)
      {
      vtkErrorMacro(
        "Failed to Gather Information from satellite no: " << childid);
      continue;
      }

    unsigned char* data = new unsigned char[length];
    controller->Receive(data, length, childid, ROOT_SATELLITE_INFO_TAG);
    vtkClientServerStream stream;
    stream.SetData(data, length);
    vtkPVInformation* tempInfo = info->NewInstance();
    tempInfo->CopyFromStream(&stream);
    info->AddInformation(tempInfo);
    tempInfo->Delete();
    delete [] data;
    }

  // Now send to parent, if parent is indeed valid.
  if (parent >= 0)
    {
    if (info)
      {
      vtkClientServerStream css;
      info->CopyToStream(&css);
      size_t length;
      const unsigned char* data;
      css.GetData(&data, &length);
      int len = static_cast<int>(length);
      controller->Send(&len, 1, parent, ROOT_SATELLITE_INFO_TAG);
      controller->Send(const_cast<unsigned char*>(data),
        length, parent, ROOT_SATELLITE_INFO_TAG);
      }
    else
      {
      int len = 0;
      controller->Send(&len, 1, parent, ROOT_SATELLITE_INFO_TAG);
      }
    }
  return true;
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::RegisterRemoteObject(vtkSMRemoteObject* obj)
{
  assert (obj != NULL);
  this->Internals->RemoteObjectMap[obj->GetGlobalID()] = obj;
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::UnRegisterRemoteObject(vtkSMRemoteObject* obj)
{
  assert (obj != NULL);

  this->Internals->DeleteRemoteObject(obj->GetGlobalID());
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::GetAllRemoteObjects(vtkCollection* collection)
{
  this->Internals->GetAllRemoteObjects(collection);
}

//----------------------------------------------------------------------------
const vtkClientServerStream& vtkSMSessionCore::GetLastResult()
{
  return this->Interpreter->GetLastResult();
}
