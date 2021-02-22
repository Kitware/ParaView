/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxySelectionModel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxySelectionModel.h"

#include "vtkBoundingBox.h"
#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSession.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMSession.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMTrace.h"
#include "vtkSmartPointer.h"

#include <vtkNew.h>

#include <algorithm>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMProxySelectionModel);

//-----------------------------------------------------------------------------
class vtkSMProxySelectionModel::vtkInternal
{
public:
  unsigned int CollaborationManagerObserverID;
  vtkSMProxySelectionModel* Owner;
  bool FollowinMaster;
  bool Initialized;
  bool DisableSessionStatePush;
  std::map<int, vtkSMMessage> ClientsCachedState;

  vtkInternal(vtkSMProxySelectionModel* owner)
  {
    this->DisableSessionStatePush = false;
    this->Owner = owner;
    this->CollaborationManagerObserverID = 0;
    this->FollowinMaster = true;
    this->Initialized = false;
  }

  ~vtkInternal()
  {
    if (this->Owner->Session && this->CollaborationManagerObserverID)
    {
      this->Owner->Session->GetCollaborationManager()->RemoveObserver(
        this->CollaborationManagerObserverID);
    }
    this->CollaborationManagerObserverID = 0;
  }

  void MasterChangedCallBack(
    vtkObject* vtkNotUsed(src), unsigned long vtkNotUsed(event), void* vtkNotUsed(data))
  {
    if (this->FollowinMaster && this->GetMasterId() != -1 &&
      this->ClientsCachedState.find(this->GetMasterId()) != this->ClientsCachedState.end())
    {
      this->Owner->LoadState(
        &this->ClientsCachedState[this->GetMasterId()], this->Owner->Session->GetProxyLocator());
      this->Owner->PushStateToSession();
    }
  }

  void ExportSelection(vtkCollection* src, vtkCollection* dst)
  {
    dst->RemoveAllItems();
    src->InitTraversal();
    while (vtkObject* obj = src->GetNextItemAsObject())
    {
      dst->AddItem(obj);
    }
  }

  int GetMasterId()
  {
    if (!this->Owner->Session || !this->Owner->Session->GetCollaborationManager())
    {
      return -1;
    }
    return this->Owner->Session->GetCollaborationManager()->GetMasterId();
  }
};

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::vtkSMProxySelectionModel()
{
  this->Internal = new vtkSMProxySelectionModel::vtkInternal(this);

  this->State = new vtkSMMessage();
  this->SetLocation(vtkPVSession::CLIENT);
  this->State->SetExtension(DefinitionHeader::server_class, "vtkSIObject"); // Dummy SIObject
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::~vtkSMProxySelectionModel()
{
  delete this->Internal;
  delete this->State;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetCurrentProxy()
{
  return this->Current;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetCurrentProxy(vtkSMProxy* proxy, int command)
{
  if (this->Current != proxy)
  {
    SM_SCOPED_TRACE(SetCurrentProxy)
      .arg("selmodel", this)
      .arg("proxy", proxy)
      .arg("command", command);
    this->Internal->DisableSessionStatePush = true;
    this->Current = proxy;
    this->Select(proxy, command);
    this->Internal->DisableSessionStatePush = false;
    this->InvokeCurrentChanged(proxy);
  }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsSelected(vtkSMProxy* proxy)
{
  return std::find(this->Selection.begin(), this->Selection.end(), proxy) != this->Selection.end();
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxySelectionModel::GetNumberOfSelectedProxies()
{
  return static_cast<unsigned int>(this->Selection.size());
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetSelectedProxy(unsigned int idx)
{
  if (idx < this->GetNumberOfSelectedProxies())
  {
    SelectionType::iterator iter = this->Selection.begin();
    for (unsigned int cc = 0; cc < idx; ++cc, ++iter)
    {
    }
    return vtkSMProxy::SafeDownCast(iter->GetPointer());
  }

  return nullptr;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkSMProxy* proxy, int command)
{
  SelectionType selection;
  if (proxy)
  {
    selection.push_back(proxy);
  }
  this->Select(selection, command);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(
  const vtkSMProxySelectionModel::SelectionType& proxies, int command)
{
  if (command == vtkSMProxySelectionModel::NO_UPDATE)
  {
    return;
  }

  SelectionType new_selection;

  if (command & vtkSMProxySelectionModel::CLEAR)
  {
    // everything from old-selection needs to be removed.
  }
  else
  {
    // start with existing selection.
    new_selection = this->Selection;
  }

  for (SelectionType::const_iterator iter = proxies.begin(); iter != proxies.end(); ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    if (proxy && (command & vtkSMProxySelectionModel::SELECT) != 0)
    {
      if (std::find(new_selection.begin(), new_selection.end(), proxy) == new_selection.end())
      {
        new_selection.push_back(proxy);
      }
    }
    if (proxy && (command & vtkSMProxySelectionModel::DESELECT) != 0)
    {
      new_selection.remove(proxy);
    }
  }

  bool changed = (this->Selection != new_selection);
  if (changed)
  {
    this->Selection = new_selection;
    this->InvokeSelectionChanged();
  }
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeCurrentChanged(vtkSMProxy* proxy)
{
  this->InvokeEvent(vtkCommand::CurrentChangedEvent, proxy);
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeSelectionChanged()
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent);
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::GetSelectionDataBounds(double bounds[6])
{
  vtkBoundingBox bbox;
  for (SelectionType::iterator iter = this->Selection.begin(); iter != this->Selection.end();
       ++iter)
  {
    vtkSMProxy* proxy = iter->GetPointer();
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
    vtkSMOutputPort* opPort = vtkSMOutputPort::SafeDownCast(proxy);
    if (source)
    {
      for (unsigned int kk = 0; kk < source->GetNumberOfOutputPorts(); kk++)
      {
        bbox.AddBounds(source->GetDataInformation(kk)->GetBounds());
      }
    }
    else if (opPort)
    {
      bbox.AddBounds(opPort->GetDataInformation()->GetBounds());
    }
  }
  if (bbox.IsValid())
  {
    bbox.GetBounds(bounds);
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << "Current Proxy: " << (this->Current ? this->Current->GetGlobalIDAsString() : "NULL")
     << endl;
  os << indent << "Selected Proxies: ";
  for (SelectionType::iterator iter = this->Selection.begin(); iter != this->Selection.end();
       iter++)
  {
    os << iter->GetPointer()->GetGlobalIDAsString() << " ";
  }
  os << endl;
}

//-----------------------------------------------------------------------------
const vtkSMMessage* vtkSMProxySelectionModel::GetFullState()
{
  return this->State;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  // Store in cache the state
  this->Internal->ClientsCachedState[msg->client_id()] = *msg;

  if (!this->HasGlobalID())
  {
    this->SetGlobalID(msg->global_id());
  }

  // Make sure we are loading the master state and we want to follow it.
  // Otherwise don't try to load that state
  // If we did not get initialized yet, we don't filter
  if (this->Internal->GetMasterId() != -1 &&
    !(!this->Internal->Initialized ||
        (this->IsFollowingMaster() &&
          this->Internal->GetMasterId() == static_cast<int>(msg->client_id()))))
  {
    return;
  }

  // Has we are going to load a state, we can consider to be initialized
  this->Internal->Initialized = true;

  // Load current proxy
  vtkSMProxy* currentProxy = nullptr;
  if (msg->GetExtension(ProxySelectionModelState::current_proxy) != 0)
  {
    vtkTypeUInt32 currentProxyId = msg->GetExtension(ProxySelectionModelState::current_proxy);
    currentProxy = locator->LocateProxy(currentProxyId);
    if (currentProxy)
    {
      if (msg->GetExtension(ProxySelectionModelState::current_port) != -1)
      {
        // We have to select an output port
        vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(currentProxy);
        assert("Try to select an output port of a non source proxy" && source);

        currentProxy =
          source->GetOutputPort(msg->GetExtension(ProxySelectionModelState::current_port));
      }
    }
    else
    {
      // Switch to warning as the selected proxy could be simply removed by
      // master when we finally decided to select it.
      vtkWarningMacro(
        "Did not find the CURRENT proxy for selection Model with ID: " << currentProxyId);
    }
  }

  SelectionType new_selection;

  // Load the proxy in the state
  for (int i = 0; i < msg->ExtensionSize(ProxySelectionModelState::proxy); i++)
  {
    vtkSMProxy* proxy = locator->LocateProxy(msg->GetExtension(ProxySelectionModelState::proxy, i));
    if (proxy)
    {
      if (msg->GetExtension(ProxySelectionModelState::port, i) != -1)
      {
        // We have to select an output port
        vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(proxy);
        assert("Try to select an output port of a non source proxy" && source);

        proxy = source->GetOutputPort(msg->GetExtension(ProxySelectionModelState::port, i));
      }

      // Just add the proxy in the set
      if (std::find(new_selection.begin(), new_selection.end(), proxy) == new_selection.end())
      {
        new_selection.push_back(proxy);
      }
    }
  }

  // Apply the state
  bool tmp = this->IsLocalPushOnly();
  this->EnableLocalPushOnly();
  this->Select(new_selection, CLEAR_AND_SELECT);
  this->SetCurrentProxy(currentProxy, new_selection.size() == 0 ? SELECT : NO_UPDATE);
  if (!tmp)
    this->DisableLocalPushOnly();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PushStateToSession()
{
  if (this->Internal->DisableSessionStatePush)
  {
    return;
  }

  // Update the local state and push to the session
  this->State->ClearExtension(ProxySelectionModelState::current_proxy);
  this->State->ClearExtension(ProxySelectionModelState::current_port);
  this->State->ClearExtension(ProxySelectionModelState::proxy);
  this->State->ClearExtension(ProxySelectionModelState::port);

  // - Selection
  for (SelectionType::iterator iter = this->Selection.begin(); iter != this->Selection.end();
       ++iter)
  {
    vtkSMProxy* obj = iter->GetPointer();
    if (vtkSMOutputPort* port = vtkSMOutputPort::SafeDownCast(obj))
    {
      this->State->AddExtension(
        ProxySelectionModelState::proxy, port->GetSourceProxy()->GetGlobalID());
      this->State->AddExtension(ProxySelectionModelState::port, port->GetPortIndex());
    }
    else
    {
      this->State->AddExtension(ProxySelectionModelState::proxy, obj->GetGlobalID());
      this->State->AddExtension(ProxySelectionModelState::port, -1); // Not an outputport
    }
  }
  // - Current
  if (this->Current)
  {
    if (vtkSMOutputPort* port = vtkSMOutputPort::SafeDownCast(this->Current.GetPointer()))
    {
      this->State->SetExtension(
        ProxySelectionModelState::current_proxy, port->GetSourceProxy()->GetGlobalID());
      this->State->SetExtension(ProxySelectionModelState::current_port, port->GetPortIndex());
    }
    else
    {
      this->State->SetExtension(
        ProxySelectionModelState::current_proxy, this->Current->GetGlobalID());
      this->State->SetExtension(ProxySelectionModelState::current_port, -1); // Not an outputport
    }
  }

  // If we push our state we are correctly initialized
  this->Internal->Initialized = true;

  if (!this->IsLocalPushOnly() && this->GetSession())
  {
    this->PushState(this->State);
  }
}
//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetSession(vtkSMSession* session)
{
  // Unregister observer if meaningful
  if (this->Session && this->Internal->CollaborationManagerObserverID)
  {
    this->Session->GetCollaborationManager()->RemoveObserver(
      this->Internal->CollaborationManagerObserverID);
    this->Internal->CollaborationManagerObserverID = 0;
  }

  this->Superclass::SetSession(session);

  if (this->Session && this->Session->GetCollaborationManager())
  {
    this->Internal->CollaborationManagerObserverID =
      this->Session->GetCollaborationManager()->AddObserver(
        vtkSMCollaborationManager::UpdateMasterUser, this->Internal,
        &vtkSMProxySelectionModel::vtkInternal::MasterChangedCallBack);
  }
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetFollowingMaster(bool following)
{
  this->Internal->FollowinMaster = following;
  if (following)
  {
    this->Internal->MasterChangedCallBack(nullptr, 0, nullptr);
  }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsFollowingMaster()
{
  return this->Internal->FollowinMaster;
}
