/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSMMessage.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSession.h"
#include "vtkSMUndoStack.h"

#include <map>
#include <set>

//***************************************************************************
//                        Internal class Definition
//***************************************************************************
class vtkSMStateLocator::vtkInternal
{
public:
  vtkInternal()
    : SessionRegistrationObserverID(0)
    , SessionUnRegistrationObserverID(0)
    , UndoStackRemoveObserverID(0)
    , UndoStackClearObserverID(0)
  {
  }

  void RegisterState(vtkTypeUInt32 globalId, const vtkSMMessage* state)
  {
    this->StateMap[globalId].CopyFrom(*state);
  }

  void UnRegisterState(vtkTypeUInt32 globalId) { this->StateMap.erase(globalId); }

  void UnRegisterAllStates() { this->StateMap.clear(); }

  bool FindState(vtkTypeUInt32 globalId, vtkSMMessage* stateToFill)
  {
    if (this->StateMap.find(globalId) != this->StateMap.end())
    {
      if (stateToFill)
      {
        stateToFill->CopyFrom(this->StateMap[globalId]);
      }
      return true;
    }
    return false;
  }

  void CallBackSession(vtkObject* vtkNotUsed(src), long unsigned int event, void* data)
  {
    vtkTypeUInt32 id = 0;
    if (event != vtkUndoStack::UndoSetClearedEvent)
      switch (event)
      {
        case vtkSMSession::RegisterRemoteObjectEvent:
          // If the object is alive, we should make sure we don't keep it
          // in the list of object to delete after a while...
          id = *reinterpret_cast<vtkTypeUInt32*>(data);
          if (this->TimeToLiveMap.find(id) != this->TimeToLiveMap.end())
          {
            this->TimeToLiveMap.erase(id);
          }
          break;
        case vtkSMSession::UnRegisterRemoteObjectEvent:
          // Init the object for garbage collection
          id = *reinterpret_cast<vtkTypeUInt32*>(data);
          this->TimeToLiveMap[id] = this->UndoStackSize;
          break;
      }
  }

  void CallBackUndoStack(
    vtkObject* vtkNotUsed(src), long unsigned int event, void* vtkNotUsed(data))
  {
    std::map<vtkTypeUInt32, vtkTypeUInt32>::iterator iter;
    iter = this->TimeToLiveMap.begin();
    vtkTypeUInt32 globalId = 0;
    switch (event)
    {
      case vtkUndoStack::UndoSetClearedEvent:
        // Remove all the state that are in the list for deletion
        for (; iter != this->TimeToLiveMap.end(); iter++)
        {
          globalId = iter->first;
          this->StateMap.erase(globalId);
        }
        this->TimeToLiveMap.clear();
        break;
      case vtkUndoStack::UndoSetRemovedEvent:
        // Register for delete all the state that reach the 0 TimeToLive
        std::set<vtkTypeUInt32> itemsToDelete;
        for (; iter != this->TimeToLiveMap.end(); iter++)
        {
          globalId = iter->first;
          vtkTypeUInt32 timeToLive = iter->second - 1;
          this->TimeToLiveMap[globalId] = timeToLive;

          if (timeToLive == 0)
          {
            itemsToDelete.insert(globalId);
          }
        }
        // Clean-up state map and TTL map
        for (std::set<vtkTypeUInt32>::iterator i = itemsToDelete.begin(); i != itemsToDelete.end();
             i++)
        {
          this->TimeToLiveMap.erase(*i);
          this->StateMap.erase(*i);
        }
        break;
    }
  }

  void AttachObserver(vtkSMSession* session)
  {
    if (session)
    {
      this->SessionRegistrationObserverID =
        session->AddObserver(vtkSMSession::RegisterRemoteObjectEvent, this,
          &vtkSMStateLocator::vtkInternal::CallBackSession);
      this->SessionUnRegistrationObserverID =
        session->AddObserver(vtkSMSession::UnRegisterRemoteObjectEvent, this,
          &vtkSMStateLocator::vtkInternal::CallBackSession);
    }
  }

  void DetatchObserver(vtkSMSession* session)
  {
    if (this->SessionRegistrationObserverID && session)
    {
      session->RemoveObserver(this->SessionRegistrationObserverID);
    }
    this->SessionRegistrationObserverID = 0;
    if (this->SessionUnRegistrationObserverID && session)
    {
      session->RemoveObserver(this->SessionUnRegistrationObserverID);
    }
    this->SessionUnRegistrationObserverID = 0;
  }

  void AttachObserver(vtkUndoStack* undostack)
  {
    if (undostack)
    {
      this->UndoStackSize = undostack->GetStackDepth();
      this->UndoStackRemoveObserverID = undostack->AddObserver(vtkUndoStack::UndoSetRemovedEvent,
        this, &vtkSMStateLocator::vtkInternal::CallBackUndoStack);
      this->UndoStackClearObserverID = undostack->AddObserver(vtkUndoStack::UndoSetClearedEvent,
        this, &vtkSMStateLocator::vtkInternal::CallBackUndoStack);
    }
  }

  void DetatchObserver(vtkUndoStack* undostack)
  {
    if (this->UndoStackRemoveObserverID && undostack)
    {
      undostack->RemoveObserver(this->UndoStackRemoveObserverID);
    }
    this->UndoStackRemoveObserverID = 0;
    if (this->UndoStackClearObserverID && undostack)
    {
      undostack->RemoveObserver(this->UndoStackClearObserverID);
    }
    this->UndoStackClearObserverID = 0;
  }

private:
  vtkTypeUInt32 UndoStackSize;
  std::map<vtkTypeUInt32, vtkSMMessage> StateMap;
  std::map<vtkTypeUInt32, vtkTypeUInt32> TimeToLiveMap;
  unsigned int SessionRegistrationObserverID;
  unsigned int SessionUnRegistrationObserverID;
  unsigned int UndoStackRemoveObserverID;
  unsigned int UndoStackClearObserverID;
};
//***************************************************************************
vtkStandardNewMacro(vtkSMStateLocator);
//---------------------------------------------------------------------------
vtkSMStateLocator::vtkSMStateLocator()
{
  this->ParentLocator = nullptr;
  this->Internals = new vtkInternal();
}

//---------------------------------------------------------------------------
vtkSMStateLocator::~vtkSMStateLocator()
{
  this->Internals->DetatchObserver(this->Session);
  this->Internals->DetatchObserver(this->UndoStack);

  this->SetParentLocator(nullptr);
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMStateLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::FindState(
  vtkTypeUInt32 globalID, vtkSMMessage* stateToFill, bool useParent /*=true*/)
{
  if (stateToFill != nullptr)
  {
    stateToFill->Clear();
  }

  if (this->Internals->FindState(globalID, stateToFill))
  {
    return true;
  }
  if (useParent && this->ParentLocator)
  {
    return this->ParentLocator->FindState(globalID, stateToFill);
  }
  return false;
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::RegisterState(const vtkSMMessage* state)
{
  this->Internals->RegisterState(state->global_id(), state);
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::UnRegisterState(vtkTypeUInt32 globalID, bool force)
{
  this->Internals->UnRegisterState(globalID);
  if (force && this->ParentLocator)
  {
    this->ParentLocator->UnRegisterState(globalID, force);
  }
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::UnRegisterAllStates(bool force)
{
  this->Internals->UnRegisterAllStates();
  if (force && this->ParentLocator)
  {
    this->ParentLocator->UnRegisterAllStates(force);
  }
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::IsStateLocal(vtkTypeUInt32 globalID)
{
  return this->Internals->FindState(globalID, nullptr);
}
//---------------------------------------------------------------------------
bool vtkSMStateLocator::IsStateAvailable(vtkTypeUInt32 globalID)
{
  return (this->IsStateLocal(globalID) ||
    (this->ParentLocator && this->ParentLocator->IsStateAvailable(globalID)));
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::RegisterFullState(vtkSMProxy* proxy)
{
  if (!proxy)
  {
    return;
  }

  // Save the current proxy state
  const vtkSMMessage* state = proxy->GetFullState();
  this->Internals->RegisterState(proxy->GetGlobalID(), state);

  // Save sub-proxies
  unsigned int numberOfSubProxy = proxy->GetNumberOfSubProxies();
  for (unsigned int idx = 0; idx < numberOfSubProxy; idx++)
  {
    vtkSMProxy* subproxy = proxy->GetSubProxy(idx);
    this->RegisterFullState(subproxy);
  }

  // Save any proxy that is part of a Proxy/Source Property
  vtkSMPropertyIterator* propIterator = proxy->NewPropertyIterator();
  propIterator->Begin();
  while (!propIterator->IsAtEnd())
  {
    vtkSMProxyProperty* property = vtkSMProxyProperty::SafeDownCast(propIterator->GetProperty());
    if (property)
    {
      for (unsigned int i = 0; i < property->GetNumberOfProxies(); i++)
      {
        this->RegisterFullState(property->GetProxy(i));
      }
    }
    propIterator->Next();
  }
  propIterator->Delete();
}
//---------------------------------------------------------------------------
void vtkSMStateLocator::InitGarbageCollector(vtkSMSession* session, vtkUndoStack* stack)
{
  if (!((session && stack) || (!session && !stack)))
  {
    vtkErrorMacro("Invalid set of parameters");
  }

  this->Internals->DetatchObserver(this->Session);
  this->Internals->DetatchObserver(this->UndoStack);

  this->Session = session;
  this->UndoStack = stack;

  this->Internals->AttachObserver(this->Session);
  this->Internals->AttachObserver(this->UndoStack);
}
