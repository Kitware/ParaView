/*=========================================================================

  Program:   ParaView
  Module:    vtkCompositeMultiProcessController.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeMultiProcessController.h"

#include "vtkCommand.h"
#include "vtkMultiProcessController.h"
#include "vtkSocketCommunicator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkPVSessionServer.h"

#include <vtkstd/string>
#include <vtksys/ios/sstream>
#include <vtksys/RegularExpression.hxx>

#include <assert.h>
#include <vtkstd/vector>
#include <vtkstd/map>

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkCompositeMultiProcessController::vtkCompositeInternals
{
public:
struct RMICallbackInfo
{
  RMICallbackInfo(unsigned long observerTagID, vtkRMIFunctionType function, void* arg, int tag)
    {
    this->RMIObserverID = observerTagID;
    this->Function = function;
    this->Arg = arg;
    this->Tag = tag;
    }

  vtkRMIFunctionType Function;
  void* Arg;
  int Tag;
  unsigned long RMIObserverID;
};

struct Controller
{
  Controller(vtkMultiProcessController* controller)
    {
    this->MultiProcessController = controller;
    this->ActivateObserverId = 0;
    }

  void AddRMICallbacks(vtkstd::vector<RMICallbackInfo>& callbacks)
    {
    vtkstd::vector<RMICallbackInfo>::iterator iter = callbacks.begin();
    while(iter != callbacks.end())
      {
      this->AddRMICallback(iter->RMIObserverID, iter->Function, iter->Arg , iter->Tag);
      iter++;
      }
    }

  void DetachCallBacks(vtkstd::vector<RMICallbackInfo>& callbacks)
    {
    // Remove activate observer
    this->MultiProcessController->RemoveObserver(this->ActivateObserverId);
    this->ActivateObserverId = 0;

    // Remove all registered RMICallbacks
    vtkstd::vector<RMICallbackInfo>::iterator iter = callbacks.begin();
    while(iter != callbacks.end())
      {
      unsigned long rmiObserverID = iter->RMIObserverID;
      vtkstd::vector<unsigned long>::iterator observerIdIter =
          this->RMICallbackIdMapping[rmiObserverID].begin();
      while(observerIdIter != this->RMICallbackIdMapping[rmiObserverID].end())
        {
        this->RemoveRMICallback(*observerIdIter);
        observerIdIter++;
        }
      iter++;
      }
    }

  void AddRMICallback(unsigned long observerTagId, vtkRMIFunctionType function, void *arg, int tag)
    {
    this->RMICallbackIdMapping[observerTagId].push_back(
        this->MultiProcessController->AddRMICallback(function, arg, tag));
    }

  bool RemoveRMICallback(unsigned long observerTagId)
    {
    int size = this->RMICallbackIdMapping[observerTagId].size();
    bool result = false;
    for(int i=0;i<size;i++)
      {
      result = this->MultiProcessController->RemoveRMICallback(
          this->RMICallbackIdMapping[observerTagId][i]) || result;

      }
    return result;
    }

  unsigned long ActivateObserverId;
  vtkSmartPointer<vtkMultiProcessController> MultiProcessController;
  vtkstd::map<unsigned long, vtkstd::vector<unsigned long> > RMICallbackIdMapping;
};

public:
  vtkCompositeInternals(vtkCompositeMultiProcessController* owner)
    {
    this->RMICallbackIdCounter = 1;
    this->Owner = owner;
    this->NeedToInitializeControllers = false;
    }
  //-----------------------------------------------------------------
  void RegisterController(vtkMultiProcessController* ctrl)
    {
    assert(ctrl->IsA("vtkSocketController"));
    if(this->NeedToInitializeControllers)
      {
      // CAUTION: This initialization is only correct for vtkSocketController
      ctrl->Initialize(0,0);
      }
    this->Controllers.push_back(Controller(ctrl));
    this->ActiveController = &this->Controllers.back();
    this->ActiveController->ActivateObserverId = ctrl->AddObserver(
        vtkCommand::StartEvent, this, &vtkCompositeInternals::ActivateController);

    // Attach RMICallbacks
    this->ActiveController->AddRMICallbacks(this->RMICallbacks);

    this->UpdateActiveCommunicator();
    }
  //-----------------------------------------------------------------
  void UnRegisterController(vtkMultiProcessController* ctrl)
    {
    vtkstd::vector<Controller>::iterator iter, iterToDel;
    bool found = false;
    for(iter = this->Controllers.begin(); iter != this->Controllers.end(); iter++)
      {
      if(iter->MultiProcessController.GetPointer() == ctrl)
        {
        if(this->GetActiveController() == ctrl)
          {
          this->ActiveController = NULL;
          this->UpdateActiveCommunicator();
          }
        iterToDel = iter;
        found = true;
        break;
        }
      }
    if(found)
      {
      iterToDel->DetachCallBacks(this->RMICallbacks);
      this->Controllers.erase(iterToDel);
      }
    }
  //-----------------------------------------------------------------
  Controller* FindController(vtkMultiProcessController* ctrl)
    {
    vtkstd::vector<Controller>::iterator iter = this->Controllers.begin();
    while(iter != this->Controllers.end())
      {
      if(iter->MultiProcessController.GetPointer() == ctrl)
        {
        return &(*iter);
        }
      iter++;
      }
    return NULL;
    }
  //-----------------------------------------------------------------
  vtkMultiProcessController* GetActiveController()
    {
    if(this->ActiveController)
      {
      return this->ActiveController->MultiProcessController;
      }
    return NULL;
    }
  //-----------------------------------------------------------------
  void ActivateController(vtkObject* src, unsigned long event, void* data)
    {
    if(this->GetActiveController() != src)
      {
      this->ActiveController =
          this->FindController(vtkMultiProcessController::SafeDownCast(src));
      this->UpdateActiveCommunicator();
      }
    }
  //-----------------------------------------------------------------
  void InitializeControllers()
    {
    this->NeedToInitializeControllers = true;
    vtkstd::vector<Controller>::iterator iter = this->Controllers.begin();
    while(iter != this->Controllers.end())
      {
      // CAUTION: This initialization only correct for vtkSocketController
      iter->MultiProcessController->Initialize(0,0);
      iter++;
      }
    }
  //-----------------------------------------------------------------
  unsigned long AddRMICallback(vtkRMIFunctionType function, void* arg, int tag)
    {
    // Save call back for new vtkMultiProcessController
    this->RMICallbackIdCounter++;
    this->RMICallbacks.push_back(RMICallbackInfo(this->RMICallbackIdCounter, function, arg, tag));

    // Register it to the previously registered controllers
    vtkstd::vector<Controller>::iterator iter = this->Controllers.begin();
    while(iter != this->Controllers.end())
      {
      iter->AddRMICallback(this->RMICallbackIdCounter, function, arg, tag);
      iter++;
      }
    return this->RMICallbackIdCounter;
    }
  //-----------------------------------------------------------------
  void RemoveAllRMICallbacks(int tag)
    {
    // Clear registered RMICallbacks
    vtkstd::vector<int> callbackToRemove;
    vtkstd::vector<RMICallbackInfo> callbackToKeep;
    vtkstd::vector<RMICallbackInfo>::iterator iter = this->RMICallbacks.begin();
    while(iter != this->RMICallbacks.end())
      {
      if(tag == iter->Tag)
        {
        callbackToRemove.push_back(tag);
        }
      else
        {
        callbackToKeep.push_back(*iter);
        }
      iter++;
      }
    this->RMICallbacks = callbackToKeep;

    // Remove RMICallbacks on controllers
    vtkstd::vector<Controller>::iterator ctrlIter = this->Controllers.begin();
    vtkstd::vector<int>::iterator tagIter;
    while(ctrlIter != this->Controllers.end())
      {
      tagIter = callbackToRemove.begin();
      while(tagIter != callbackToRemove.end())
        {
        ctrlIter->MultiProcessController->RemoveAllRMICallbacks(*tagIter);
        tagIter++;
        }
      ctrlIter++;
      }
    }

  //-----------------------------------------------------------------
  bool RemoveRMICallback(unsigned long observerTagId)
    {
    // Remove RMICallback on controllers
    bool result = false;
    vtkstd::vector<Controller>::iterator ctrlIter = this->Controllers.begin();
    while(ctrlIter != this->Controllers.end())
      {
      result = ctrlIter->RemoveRMICallback(observerTagId)
               || result;
      ctrlIter++;
      }
    return result;
    }

  //-----------------------------------------------------------------
  int GetNumberOfRegisteredControllers()
    {
    return this->Controllers.size();
    }
  //-----------------------------------------------------------------
  vtkCommunicator* GetActiveCommunicator()
    {
    if(this->GetActiveController())
      {
      return this->GetActiveController()->GetCommunicator();
      }
    return NULL;
    }
  //-----------------------------------------------------------------
  void UpdateActiveCommunicator()
    {
    this->Owner->Communicator = this->GetActiveCommunicator();
    this->Owner->RMICommunicator = this->GetActiveCommunicator();
    }
  //-----------------------------------------------------------------
  void CleanNonConnectedControllers()
    {
    vtkstd::vector<vtkMultiProcessController*> controllersToDelete;
    vtkstd::vector<Controller>::iterator iter = this->Controllers.begin();
    while(iter != this->Controllers.end())
      {
      vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
          iter->MultiProcessController->GetCommunicator());
      if(!comm->GetIsConnected())
        {
        controllersToDelete.push_back(iter->MultiProcessController.GetPointer());
        }
      iter++;
      }

    // Clean up the invalid controllers
    vtkstd::vector<vtkMultiProcessController*>::iterator iter2 =
        controllersToDelete.begin();
    while(iter2 != controllersToDelete.end())
      {
      this->UnRegisterController(*iter2);
      iter2++;
      }
    }
  //-----------------------------------------------------------------
  void TriggerRMI2NonActives(int remoteProcessId, void* data, int argLength,
                             int tag)
    {
    vtkstd::vector<vtkMultiProcessController*> controllersToNotify;
    vtkstd::vector<Controller>::iterator iter = this->Controllers.begin();
    while(iter != this->Controllers.end())
      {
      if( iter->MultiProcessController.GetPointer() !=
          this->ActiveController->MultiProcessController.GetPointer() )
        {
        vtkSocketCommunicator* comm = vtkSocketCommunicator::SafeDownCast(
            iter->MultiProcessController->GetCommunicator());
        if(comm->GetIsConnected())
          {
          controllersToNotify.push_back(iter->MultiProcessController.GetPointer());
          }
        }
      iter++;
      }

    // Do the notification now...
    vtkstd::vector<vtkMultiProcessController*>::iterator iter2 =
        controllersToNotify.begin();
    while(iter2 != controllersToNotify.end())
      {
      vtkMultiProcessController* ctrl = (*iter2);
      //cout << "Notify: " << ctrl->GetCommunicator() << endl;
      ctrl->TriggerRMI(remoteProcessId, data, argLength, tag);
      iter2++;
      }
    }
  //-----------------------------------------------------------------

private:
  Controller* ActiveController;
  vtkWeakPointer<vtkCompositeMultiProcessController> Owner;
  vtkstd::vector<RMICallbackInfo> RMICallbacks;
  vtkstd::vector<Controller> Controllers;
  bool NeedToInitializeControllers;
  unsigned long RMICallbackIdCounter;
};
//****************************************************************************/
vtkStandardNewMacro(vtkCompositeMultiProcessController);
//----------------------------------------------------------------------------
vtkCompositeMultiProcessController::vtkCompositeMultiProcessController()
{
  this->Internal = new vtkCompositeInternals(this);
}

//----------------------------------------------------------------------------
vtkCompositeMultiProcessController::~vtkCompositeMultiProcessController()
{
  delete this->Internal; this->Internal = NULL;
}

//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::Initialize()
{
  this->Internal->InitializeControllers();
}

//----------------------------------------------------------------------------
//                            Composite API
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::RegisterController(vtkMultiProcessController* controller)
{
  assert(controller->IsA("vtkSocketController"));
  this->Internal->RegisterController(controller);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::UnRegisterController(vtkMultiProcessController* controller)
{
  assert(controller->IsA("vtkSocketController"));
  this->Internal->UnRegisterController(controller);
}
//----------------------------------------------------------------------------
//                       RMI Callback management API
//----------------------------------------------------------------------------
unsigned long vtkCompositeMultiProcessController::AddRMICallback(
    vtkRMIFunctionType func, void* localArg, int tag)
{
  return this->Internal->AddRMICallback(func, localArg, tag);
}

//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::RemoveAllRMICallbacks(int tag)
{
  this->Internal->RemoveAllRMICallbacks(tag);
}
//----------------------------------------------------------------------------
bool vtkCompositeMultiProcessController::RemoveRMICallback(unsigned long observerTagId)
{
  return this->Internal->RemoveRMICallback(observerTagId);
}

//----------------------------------------------------------------------------
int vtkCompositeMultiProcessController::UnRegisterActiveController()
{
  this->UnRegisterController(this->Internal->GetActiveController());
  return this->Internal->GetNumberOfRegisteredControllers();
}
//----------------------------------------------------------------------------
vtkCommunicator* vtkCompositeMultiProcessController::GetCommunicator()
{
  return this->Internal->GetActiveCommunicator();
}
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::TriggerRMI2NonActives(int remote,
                                                               void* data,
                                                               int length,
                                                               int tag)
{
  this->Internal->CleanNonConnectedControllers();
  this->Internal->TriggerRMI2NonActives(
      1, data, length, vtkPVSessionServer::SERVER_NOTIFICATION_MESSAGE_RMI);
}
