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
void vtkSMSessionCore::PushState(vtkSMMessage* msg)
{
  if (true) // FIXME make sure that the PMObject should be created here
    {
    vtkPMObject* obj = this->Internals->GetPMObject(msg->global_id());
    if (!obj)
      {
      // Create the corresponding PM object.
      vtkClientServerID tempID = this->Interpreter->GetNextAvailableId();
      vtkClientServerStream stream;
      stream << vtkClientServerStream::New
             << msg->GetExtension(DefinitionHeader::server_class).c_str()
             << tempID
             << vtkClientServerStream::End;
      this->Interpreter->ProcessStream(stream);
      obj = vtkPMObject::SafeDownCast(
        this->Interpreter->GetObjectFromID(tempID));
      if (obj == NULL)
        {
        vtkErrorMacro("Object must be a vtkPMObject subclass. "
          "Aborting for debugging purposes.");
        abort();
        }
      obj->Initialize(this);
      this->Internals->PMObjectMap[msg->global_id()] = obj;

      // release the reference held by the interpreter.
      stream << vtkClientServerStream::Delete
             << tempID
             << vtkClientServerStream::End;
      this->Interpreter->ProcessStream(stream);
      }
    // Push default values
    obj->Push(msg);
    }

  if ( (msg->location() & vtkProcessModule2::SERVERS) == 0)
    {
    // the state was pushed only to the CLIENT or ROOT nodes. So we don't
    // forward it to the satellites.
    return;
    }

  if (this->ParallelController &&
    this->ParallelController->GetNumberOfProcesses() > 1 &&
    this->ParallelController->GetLocalProcessId() == 0)
    {
    unsigned char type = PUSH_STATE;
    this->ParallelController->TriggerRMIOnAllChildren(&type, 1,
      ROOT_SATELLITE_RMI_TAG);

    vtkMultiProcessStream stream;
    stream << msg->SerializeAsString();
    this->ParallelController->Broadcast(stream, 0);
    }
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PushStateSatelliteCallback()
{
  vtkMultiProcessStream stream;
  this->ParallelController->Broadcast(stream, 0);

  vtkstd::string string;
  stream >> string;
  Message msg;
  msg.ParseFromString(string);
  cout << ">>> vtkSMSessionCore::PushStateSatelliteCallback" << endl;
  cout << string << endl; // FIXME debug stuff
  msg.PrintDebugString(); // FIXME debug stuff
  cout << "<<< vtkSMSessionCore::PushStateSatelliteCallback" << endl;
  this->PushState(&msg);
}

//----------------------------------------------------------------------------
void vtkSMSessionCore::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::PullState(vtkSMMessage* msg)
{
  vtkPMObject* obj;
  if(true &&  // FIXME make sure that the PMObject should be created here
     (obj = this->Internals->GetPMObject(msg->global_id())))
    {
    obj->Pull(msg);
    }
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::Invoke(vtkSMMessage* msg)
{
  vtkPMObject* obj;
  if(true &&  // FIXME make sure that the PMObject should be created here
     (obj = this->Internals->GetPMObject(msg->global_id())))
    {
    obj->Invoke(msg);
    }
}
//----------------------------------------------------------------------------
void vtkSMSessionCore::DeletePMObject(vtkSMMessage* msg)
{
  this->Internals->Delete(msg->global_id());
}

//----------------------------------------------------------------------------
