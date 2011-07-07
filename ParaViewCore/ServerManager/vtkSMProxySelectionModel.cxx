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

#include "vtkCollection.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkCollection.h"
#include "vtkSMProxyLocator.h"

#include "vtkSMMessage.h"

#include "vtkPVSession.h"

#include <vtkstd/vector>
#include <vtkstd/set>
#include <vtkNew.h>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMProxySelectionModel);

//-----------------------------------------------------------------------------
class vtkSMProxySelectionModel::vtkInternal
{
public:
  typedef  vtkstd::vector< vtkSmartPointer<vtkSMProxy> >   vtkSMSelection;
  vtkSMSelection  Selection;
  vtkSmartPointer<vtkSMProxy>  Current;

  void ExportSelection(vtkCollection* src, vtkCollection* dst)
    {
    dst->RemoveAllItems();
    src->InitTraversal();
    while (vtkObject* obj = src->GetNextItemAsObject())
      {
      dst->AddItem(obj);
      }
    }
};

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::vtkSMProxySelectionModel()
{
  this->NewlySelected = vtkCollection::New();
  this->NewlyDeselected = vtkCollection::New();
  this->Selection = vtkCollection::New();
  this->Internal = new vtkSMProxySelectionModel::vtkInternal();

  this->State = new vtkSMMessage();
  this->SetLocation(vtkPVSession::CLIENT);
  this->State->SetExtension(DefinitionHeader::server_class, "vtkSIObject"); // Dummy SIObject
}

//-----------------------------------------------------------------------------
vtkSMProxySelectionModel::~vtkSMProxySelectionModel()
{
  this->NewlySelected->Delete();
  this->NewlyDeselected->Delete();
  this->Selection->Delete();
  delete this->Internal;
  delete this->State;
}
//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetCurrentProxy()
{
  return this->Internal->Current;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::SetCurrentProxy(vtkSMProxy*  proxy,  int  command)
{ 
  if (this->Internal->Current != proxy)
    {
    this->Internal->Current = proxy;
    this->Select(proxy, command);
    this->InvokeCurrentChanged(proxy);
    }
}

//-----------------------------------------------------------------------------
bool vtkSMProxySelectionModel::IsSelected(vtkSMProxy*  proxy)
{
  return this->Selection->IsItemPresent(proxy) != 0;
}

//-----------------------------------------------------------------------------
unsigned int vtkSMProxySelectionModel::GetNumberOfSelectedProxies()
{
  return static_cast<unsigned int>(this->Selection->GetNumberOfItems());
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxySelectionModel::GetSelectedProxy(unsigned int idx)
{
  if (idx < this->GetNumberOfSelectedProxies())
    {
    return vtkSMProxy::SafeDownCast(this->Selection->GetItemAsObject(idx));
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkSMProxy* proxy, int command)
{
  vtkCollection* collection = vtkCollection::New();
  if (proxy)
    {
    collection->AddItem(proxy);
    }
  this->Select(collection, command);
  collection->Delete();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::Select(vtkCollection*  proxies, int command)
{ 
  if (command == vtkSMProxySelectionModel::NO_UPDATE)
    {
    return;
    }

  bool changed = false;

  this->NewlyDeselected->RemoveAllItems();
  this->NewlySelected->RemoveAllItems();

  if (command & vtkSMProxySelectionModel::CLEAR)
    {
    this->Internal->ExportSelection(this->Selection, this->NewlyDeselected);
    this->Selection->RemoveAllItems();
    changed = true;
    }

  vtkSMProxy* proxy;
  for (proxies->InitTraversal();
    (proxy = vtkSMProxy::SafeDownCast(proxies->GetNextItemAsObject())) != 0; )
    {
    if ((command & vtkSMProxySelectionModel::SELECT) &&
      !this->Selection->IsItemPresent(proxy))
      {
      this->Selection->AddItem(proxy);
      if (!this->NewlySelected->IsItemPresent(proxy))
        {
        this->NewlySelected->AddItem(proxy);
        changed = true;
        }
      }

    if ((command & vtkSMProxySelectionModel::DESELECT)  &&
      this->Selection->IsItemPresent(proxy))
      {
      this->Selection->RemoveItem(proxy);
      if (!this->NewlyDeselected->IsItemPresent(proxy))
        {
        this->NewlyDeselected->AddItem(proxy);
        changed = true;
        }
      }
    }

  if (changed)
    {
    this->InvokeSelectionChanged(command);
    }

  this->NewlyDeselected->RemoveAllItems();
  this->NewlySelected->RemoveAllItems();

  // Update the local state and push to the session
  this->State->ClearExtension(ProxySelectionModelState::proxy);
  this->Selection->InitTraversal();
  while (vtkSMProxy* obj = vtkSMProxy::SafeDownCast(this->Selection->GetNextItemAsObject()))
    {
    this->State->AddExtension(ProxySelectionModelState::proxy, obj->GetGlobalID());
    }
  this->PushStateToSession();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeCurrentChanged(vtkSMProxy*  proxy)
{
  this->InvokeEvent(vtkCommand::CurrentChangedEvent, proxy);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::InvokeSelectionChanged(int selectionFlag)
{
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, (void*)&selectionFlag);
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PrintSelf(ostream&  os,  vtkIndent  indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Selected Proxies: ";
  this->Selection->InitTraversal();
  while (vtkSMProxy* obj = vtkSMProxy::SafeDownCast(this->Selection->GetNextItemAsObject()))
    {
    os << obj->GetGlobalID() << " ";
    }
  os << endl;
}
//-----------------------------------------------------------------------------
const vtkSMMessage* vtkSMProxySelectionModel::GetFullState()
{
  return this->State;
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::LoadState( const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  if(!this->HasGlobalID())
    {
    this->SetGlobalID(msg->global_id());
    }
  // Compute the diff and send the proper events
  vtkstd::set<vtkTypeUInt32> newProxyInSelection;
  for(int i=0; i < msg->ExtensionSize(ProxySelectionModelState::proxy); i++)
    {
    newProxyInSelection.insert(msg->GetExtension(ProxySelectionModelState::proxy, i));
    }

  // Take care of the deselect first
  vtkNew<vtkCollection> proxyToDeselect;
  this->Selection->InitTraversal();
  while (vtkSMProxy* obj = vtkSMProxy::SafeDownCast(this->Selection->GetNextItemAsObject()))
    {
    if(newProxyInSelection.find(obj->GetGlobalID()) == newProxyInSelection.end())
      {
      proxyToDeselect->AddItem(obj);
      }
    else
      {
      newProxyInSelection.erase(obj->GetGlobalID());
      }
    }

  // Take care of the add-on
  vtkNew<vtkCollection> proxyToSelect;
  for( vtkstd::set<vtkTypeUInt32>::iterator iter = newProxyInSelection.begin();
       iter != newProxyInSelection.end();
       iter++)
    {
    if(vtkSMProxy* proxy = locator->LocateProxy(*iter))
      {
      proxyToSelect->AddItem(proxy);
      }
    }

  // Apply the state
  bool tmp = this->IsLocalPushOnly();
  this->EnableLocalPushOnly();
  if(proxyToDeselect->GetNumberOfItems() > 0)
    {
    this->Select(proxyToDeselect.GetPointer(), DESELECT);
    }
  if(proxyToSelect->GetNumberOfItems() == 1)
    {
    this->Select(proxyToSelect.GetPointer(), SELECT);
    this->SetCurrentProxy(
        vtkSMProxy::SafeDownCast(proxyToSelect->GetItemAsObject(0)), SELECT);
    }
  else if(proxyToSelect->GetNumberOfItems() > 0)
    {
    this->Select(proxyToSelect.GetPointer(), SELECT);
    }
  if(!tmp) this->DisableLocalPushOnly();
}

//-----------------------------------------------------------------------------
void vtkSMProxySelectionModel::PushStateToSession()
{
  if(!this->IsLocalPushOnly() && this->GetSession())
    {
    this->PushState(this->State);
    }
}
