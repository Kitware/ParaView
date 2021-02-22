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
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSocketCommunicator.h"
#include "vtkWeakPointer.h"

#include <assert.h>
#include <map>
#include <vector>

#define GENERATE_DEBUG_LOG 0

#if GENERATE_DEBUG_LOG
#include "vtksys/FStream.hxx"
#endif

//****************************************************************************/
//                    Internal Classes and typedefs
//****************************************************************************/
class vtkCompositeMultiProcessController::vtkCompositeInternals
{
public:
#if GENERATE_DEBUG_LOG
  vtksys::ofstream LogFile;
#endif
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
    Controller(int id, vtkMultiProcessController* controller)
    {
      this->Id = id;
      this->MultiProcessController = controller;
      this->ActivateObserverId = 0;
      this->IsMaster = false;
    }

    void AddRMICallbacks(std::vector<RMICallbackInfo>& callbacks)
    {
      std::vector<RMICallbackInfo>::iterator iter = callbacks.begin();
      while (iter != callbacks.end())
      {
        this->AddRMICallback(iter->RMIObserverID, iter->Function, iter->Arg, iter->Tag);
        iter++;
      }
    }

    void DetachCallBacks(std::vector<RMICallbackInfo>& callbacks)
    {
      // Remove activate observer
      this->MultiProcessController->RemoveObserver(this->ActivateObserverId);
      this->ActivateObserverId = 0;

      // Remove all registered RMICallbacks
      std::vector<RMICallbackInfo>::iterator iter = callbacks.begin();
      while (iter != callbacks.end())
      {
        unsigned long rmiObserverID = iter->RMIObserverID;
        std::vector<unsigned long>::iterator observerIdIter =
          this->RMICallbackIdMapping[rmiObserverID].begin();
        while (observerIdIter != this->RMICallbackIdMapping[rmiObserverID].end())
        {
          this->RemoveRMICallback(*observerIdIter);
          observerIdIter++;
        }
        iter++;
      }
    }

    void AddRMICallback(
      unsigned long observerTagId, vtkRMIFunctionType function, void* arg, int tag)
    {
      this->RMICallbackIdMapping[observerTagId].push_back(
        this->MultiProcessController->AddRMICallback(function, arg, tag));
    }

    bool RemoveRMICallback(unsigned long observerTagId)
    {
      int size = static_cast<int>(this->RMICallbackIdMapping[observerTagId].size());
      bool result = false;
      for (int i = 0; i < size; i++)
      {
        result = this->MultiProcessController->RemoveRMICallback(
                   this->RMICallbackIdMapping[observerTagId][i]) ||
          result;
      }
      return result;
    }

    unsigned long ActivateObserverId;
    int Id;
    bool IsMaster;
    vtkSmartPointer<vtkMultiProcessController> MultiProcessController;
    std::map<unsigned long, std::vector<unsigned long> > RMICallbackIdMapping;
  };

public:
  vtkCompositeInternals(vtkCompositeMultiProcessController* owner)
  {
    this->ControllerID = 1;
    this->RMICallbackIdCounter = 1;
    this->Owner = owner;
    this->NeedToInitializeControllers = false;
  }
  //-----------------------------------------------------------------
  void RegisterController(vtkMultiProcessController* ctrl)
  {
    assert(ctrl->IsA("vtkSocketController"));
    if (this->NeedToInitializeControllers)
    {
      // CAUTION: This initialization is only correct for vtkSocketController
      ctrl->Initialize(nullptr, nullptr);
    }
    this->Controllers.push_back(Controller(this->ControllerID++, ctrl));
    this->ActiveController = &this->Controllers.back();
#if GENERATE_DEBUG_LOG
    this->LogFile << endl << "---- Making Active: " << ctrl << endl;
#endif

    this->ActiveController->ActivateObserverId =
      ctrl->AddObserver(vtkCommand::StartEvent, this, &vtkCompositeInternals::ActivateController);

    // Attach RMICallbacks
    this->ActiveController->AddRMICallbacks(this->RMICallbacks);

    this->UpdateActiveCommunicator();

    this->Owner->InvokeEvent(CompositeMultiProcessControllerChanged);
  }
  //-----------------------------------------------------------------
  void UnRegisterController(vtkMultiProcessController* ctrl)
  {
    std::vector<Controller>::iterator iter, iterToDel;
    bool found = false;
    for (iter = this->Controllers.begin(); iter != this->Controllers.end(); iter++)
    {
      if (iter->MultiProcessController.GetPointer() == ctrl)
      {
        if (this->GetActiveController() == ctrl)
        {
          this->ActiveController = nullptr;
          this->UpdateActiveCommunicator();
        }
        iterToDel = iter;
        found = true;
        break;
      }
    }
    if (found)
    {
      iterToDel->DetachCallBacks(this->RMICallbacks);
      this->Controllers.erase(iterToDel);
    }
  }
  //-----------------------------------------------------------------
  Controller* FindController(vtkMultiProcessController* ctrl)
  {
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      if (iter->MultiProcessController.GetPointer() == ctrl)
      {
        return &(*iter);
      }
      iter++;
    }
    return nullptr;
  }
  //-----------------------------------------------------------------
  vtkMultiProcessController* GetActiveController()
  {
    if (this->ActiveController)
    {
      return this->ActiveController->MultiProcessController;
    }
    return nullptr;
  }
  //-----------------------------------------------------------------
  int GetActiveControllerID()
  {
    if (this->ActiveController)
    {
      return this->ActiveController->Id;
    }
    return 0;
  }

  //-----------------------------------------------------------------
  void ActivateController(vtkObject* src, unsigned long vtkNotUsed(event), void* vtkNotUsed(data))
  {
    if (this->GetActiveController() != src)
    {
#if GENERATE_DEBUG_LOG
      this->LogFile << endl << "---- Making Active: " << src << endl;
#endif
      this->ActiveController = this->FindController(vtkMultiProcessController::SafeDownCast(src));
      this->UpdateActiveCommunicator();
    }
  }
  //-----------------------------------------------------------------
  void InitializeControllers()
  {
    this->NeedToInitializeControllers = true;
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      // CAUTION: This initialization only correct for vtkSocketController
      iter->MultiProcessController->Initialize(nullptr, nullptr);
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
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
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
    std::vector<int> callbackToRemove;
    std::vector<RMICallbackInfo> callbackToKeep;
    std::vector<RMICallbackInfo>::iterator iter = this->RMICallbacks.begin();
    while (iter != this->RMICallbacks.end())
    {
      if (tag == iter->Tag)
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
    std::vector<Controller>::iterator ctrlIter = this->Controllers.begin();
    std::vector<int>::iterator tagIter;
    while (ctrlIter != this->Controllers.end())
    {
      tagIter = callbackToRemove.begin();
      while (tagIter != callbackToRemove.end())
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
    std::vector<Controller>::iterator ctrlIter = this->Controllers.begin();
    while (ctrlIter != this->Controllers.end())
    {
      result = ctrlIter->RemoveRMICallback(observerTagId) || result;
      ctrlIter++;
    }
    return result;
  }

  //-----------------------------------------------------------------
  int GetNumberOfRegisteredControllers() { return static_cast<int>(this->Controllers.size()); }
  //-----------------------------------------------------------------
  vtkCommunicator* GetActiveCommunicator()
  {
    if (this->GetActiveController())
    {
      return this->GetActiveController()->GetCommunicator();
    }
    return nullptr;
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
    std::vector<vtkMultiProcessController*> controllersToDelete;
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      vtkSocketCommunicator* comm =
        vtkSocketCommunicator::SafeDownCast(iter->MultiProcessController->GetCommunicator());
      if (!comm->GetIsConnected())
      {
        controllersToDelete.push_back(iter->MultiProcessController.GetPointer());
      }
      iter++;
    }

    // Clean up the invalid controllers
    std::vector<vtkMultiProcessController*>::iterator iter2 = controllersToDelete.begin();
    while (iter2 != controllersToDelete.end())
    {
      this->UnRegisterController(*iter2);
      iter2++;
    }

    if (controllersToDelete.size() > 0)
    {
      this->Owner->InvokeEvent(CompositeMultiProcessControllerChanged);
    }
  }
  //-----------------------------------------------------------------
  void TriggerRMI2All(
    int remoteProcessId, void* data, int argLength, int tag, bool sendToActiveController)
  {
    std::vector<vtkMultiProcessController*> controllersToNotify;
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      if (sendToActiveController ||
        iter->MultiProcessController.GetPointer() !=
          this->ActiveController->MultiProcessController.GetPointer())
      {
        vtkSocketCommunicator* comm =
          vtkSocketCommunicator::SafeDownCast(iter->MultiProcessController->GetCommunicator());
        if (comm->GetIsConnected())
        {
          controllersToNotify.push_back(iter->MultiProcessController.GetPointer());
        }
      }
      iter++;
    }

    // Do the notification now...
    std::vector<vtkMultiProcessController*>::iterator iter2 = controllersToNotify.begin();
    while (iter2 != controllersToNotify.end())
    {
      vtkMultiProcessController* ctrl = (*iter2);
      // cout << "Notify: " << ctrl->GetCommunicator() << endl;
      ctrl->TriggerRMI(remoteProcessId, data, argLength, tag);
      iter2++;
    }
  }
  //-----------------------------------------------------------------
  int GetNumberOfControllers() { return static_cast<int>(this->Controllers.size()); }
  //-----------------------------------------------------------------
  int GetControllerId(int idx) { return this->Controllers.at(idx).Id; }
  //-----------------------------------------------------------------
  vtkMultiProcessController* GetController(int idx)
  {
    return this->Controllers.at(idx).MultiProcessController;
  }

  //-----------------------------------------------------------------
  void SetMasterController(int controllerId)
  {
    bool found = false;
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      iter->IsMaster = (iter->Id == controllerId);
      found = found || iter->IsMaster;
      iter++;
    }

    // If not found try to elect a new master
    if (!found)
    {
      int newMaster = this->ElectNewMaster();
      if (newMaster != -1)
      {
        this->SetMasterController(newMaster);
      }
    }

    if (found)
    {
      this->Owner->InvokeEvent(CompositeMultiProcessControllerChanged);
    }
  }
  //-----------------------------------------------------------------
  int GetMasterController()
  {
    std::vector<Controller>::iterator iter = this->Controllers.begin();
    while (iter != this->Controllers.end())
    {
      if (iter->IsMaster)
      {
        return iter->Id;
      }
      iter++;
    }
    int electedMaster = this->ElectNewMaster();
    if (electedMaster != -1)
    {
      this->SetMasterController(electedMaster);
    }
    return electedMaster;
  }
  //-----------------------------------------------------------------
  int ElectNewMaster()
  {
    if (this->ActiveController)
    {
      return this->ActiveController->Id;
    }
    else if (this->Controllers.begin() != this->Controllers.end())
    {
      return this->Controllers.begin()->Id;
    }
    else
    {
      return -1;
    }
  }

private:
  int ControllerID;
  Controller* ActiveController;
  vtkWeakPointer<vtkCompositeMultiProcessController> Owner;
  std::vector<RMICallbackInfo> RMICallbacks;
  std::vector<Controller> Controllers;
  bool NeedToInitializeControllers;
  unsigned long RMICallbackIdCounter;
};
//****************************************************************************/
vtkStandardNewMacro(vtkCompositeMultiProcessController);
//----------------------------------------------------------------------------
vtkCompositeMultiProcessController::vtkCompositeMultiProcessController()
{
  this->Internal = new vtkCompositeInternals(this);
#if GENERATE_DEBUG_LOG
  this->Internal->LogFile.open("/tmp/server.log");
#endif
}

//----------------------------------------------------------------------------
vtkCompositeMultiProcessController::~vtkCompositeMultiProcessController()
{
  delete this->Internal;
  this->Internal = nullptr;
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
#if GENERATE_DEBUG_LOG
  vtkSocketCommunicator::SafeDownCast(controller->GetCommunicator())
    ->SetLogStream(&this->Internal->LogFile);
#endif
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
  this->InvokeEvent(CompositeMultiProcessControllerChanged);
  return this->Internal->GetNumberOfRegisteredControllers();
}

vtkMultiProcessController* vtkCompositeMultiProcessController::GetActiveController()
{
  return this->Internal->GetActiveController();
}

//----------------------------------------------------------------------------
vtkCommunicator* vtkCompositeMultiProcessController::GetCommunicator()
{
  return this->Internal->GetActiveCommunicator();
}
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::TriggerRMI2All(
  int vtkNotUsed(remote), void* data, int length, int tag, bool sendToActiveToo)
{
  this->Internal->CleanNonConnectedControllers();
  this->Internal->TriggerRMI2All(1, data, length, tag, sendToActiveToo);
}
//----------------------------------------------------------------------------
int vtkCompositeMultiProcessController::GetActiveControllerID()
{
  return this->Internal->GetActiveControllerID();
}
//----------------------------------------------------------------------------
int vtkCompositeMultiProcessController::GetNumberOfControllers()
{
  return this->Internal->GetNumberOfControllers();
}
//----------------------------------------------------------------------------
int vtkCompositeMultiProcessController::GetControllerId(int idx)
{
  return this->Internal->GetControllerId(idx);
}
//----------------------------------------------------------------------------
vtkMultiProcessController* vtkCompositeMultiProcessController::GetController(int idx)
{
  return this->Internal->GetController(idx);
}
//----------------------------------------------------------------------------
void vtkCompositeMultiProcessController::SetMasterController(int id)
{
  this->Internal->SetMasterController(id);
}
//----------------------------------------------------------------------------
int vtkCompositeMultiProcessController::GetMasterController()
{
  return this->Internal->GetMasterController();
}
