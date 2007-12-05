/*=========================================================================

  Program:   ParaView
  Module:    vtkSMSelectionLinkProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMSelectionLinkProxy.h"

#include "vtkClientServerID.h"
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSMClientDeliveryStrategyProxy.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSelectionHelper.h"

class vtkSMSelectionLinkProxyObserver : public vtkCommand
{
public:
  static vtkSMSelectionLinkProxyObserver* New()
  { return new vtkSMSelectionLinkProxyObserver; }
  
  void SetTarget(vtkSMSelectionLinkProxy* t)
  { this->Target = t; }
  
  virtual void Execute(vtkObject*, unsigned long int e, void*)
  {
    if (e == vtkCommand::SelectionChangedEvent)
      {
      this->Target->ClientSelectionChanged();
      }
    else if (e == vtkCommand::StartEvent)
      {
      this->Target->ClientRequestData();
      }
  }
  
protected:
  vtkSMSelectionLinkProxyObserver() : Target(0) { }
  ~vtkSMSelectionLinkProxyObserver() { }
  
  vtkSMSelectionLinkProxy* Target;
};

vtkStandardNewMacro(vtkSMSelectionLinkProxy);
vtkCxxRevisionMacro(vtkSMSelectionLinkProxy, "1.3");
//---------------------------------------------------------------------------
vtkSMSelectionLinkProxy::vtkSMSelectionLinkProxy()
{
  this->ClientObserver = vtkSMSelectionLinkProxyObserver::New();
  this->ClientObserver->SetTarget(this);
  this->MostRecentSelectionOnClient = false;
  this->SettingClientSelection = false;
  this->SetServers(vtkProcessModule::CLIENT | vtkProcessModule::DATA_SERVER);
}

//---------------------------------------------------------------------------
vtkSMSelectionLinkProxy::~vtkSMSelectionLinkProxy()
{
  this->ClientObserver->Delete();
}

//---------------------------------------------------------------------------
void vtkSMSelectionLinkProxy::SetSelection(vtkSMProxy* selectionProxy)
{
  // Set the server-side selection
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke;
  stream << this->GetID() << "SetSelection" << selectionProxy->GetID();
  stream << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, 
                 this->Servers & selectionProxy->GetServers(), 
                 stream);
  
  // Mark the server-side selection link source as modified
  this->MarkModified(this);
  
  if (pm->IsRemote(this->ConnectionID))
    {
    // Mark the client-side selection link as modified
    vtkSelectionLink* link = vtkSelectionLink::SafeDownCast(
      pm->GetObjectFromID(this->GetID()));
    link->Modified();
    }

  this->MostRecentSelectionOnClient = false;
}

vtkSelectionLink* vtkSMSelectionLinkProxy::GetSelectionLink()
{
  return vtkSelectionLink::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetObjectFromID(this->GetID()));
}

//---------------------------------------------------------------------------
void vtkSMSelectionLinkProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkSelectionLink* link = vtkSelectionLink::SafeDownCast(
    pm->GetObjectFromID(this->GetID()));
  link->AddObserver(vtkCommand::SelectionChangedEvent, this->ClientObserver);
  link->AddObserver(vtkCommand::StartEvent, this->ClientObserver);
}

//---------------------------------------------------------------------------
// Called when the client's vtkSelectionLink selection changes.
void vtkSMSelectionLinkProxy::ClientSelectionChanged()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm->IsRemote(this->ConnectionID))
    {
    this->MarkModified(this);
    return;
    }
  if (this->SettingClientSelection)
    {
    return;
    }
  
  // Immediately move the selection to the data servers.
  vtkSelectionLink* link = vtkSelectionLink::SafeDownCast(
    pm->GetObjectFromID(this->GetID()));
  vtkSelection* selection = link->GetSelection();
  vtkClientServerStream stream;
  vtkSMProxy* selectionProxy = this->GetProxyManager()->NewProxy(
    "selection_helpers", "Selection");
  selectionProxy->SetServers(vtkProcessModule::DATA_SERVER);
  selectionProxy->UpdateVTKObjects();
  vtkSMSelectionHelper::SendSelection(selection, selectionProxy);
  this->SetSelection(selectionProxy);
  selectionProxy->Delete();
  
  this->MostRecentSelectionOnClient = true;
}

//---------------------------------------------------------------------------
// Called when the client's vtkSelectionLink is executing.
void vtkSMSelectionLinkProxy::ClientRequestData()
{
  // If we are setting the client selection or the most recent selection 
  // is already on the client, do nothing.
  if (this->SettingClientSelection || this->MostRecentSelectionOnClient)
    {
    return;
    }
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm->IsRemote(this->ConnectionID))
    {
    return;
    }
  
  // The most recent selection is on the server, so move it to the client.
  this->SettingClientSelection = true;
  vtkSMClientDeliveryStrategyProxy* strategy = vtkSMClientDeliveryStrategyProxy::SafeDownCast(
    this->GetProxyManager()->NewProxy("strategies", "ClientDeliveryStrategy"));
  strategy->AddInput(this, 0);
  strategy->SetPostGatherHelper("vtkAppendSelection");
  strategy->Update();

  vtkAlgorithm* alg = vtkAlgorithm::SafeDownCast(
    pm->GetObjectFromID(strategy->GetOutput()->GetID()));
  vtkSelection* selection = vtkSelection::SafeDownCast(
    alg->GetOutputDataObject(0));

  vtkSelectionLink* link = vtkSelectionLink::SafeDownCast(
    pm->GetObjectFromID(this->GetID()));
  link->SetSelection(selection);
  strategy->Delete();
  this->SettingClientSelection = false;
  
  this->MostRecentSelectionOnClient = true;
}

//-----------------------------------------------------------------------------
void vtkSMSelectionLinkProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MostRecentSelectionOnClient: " 
     << (this->MostRecentSelectionOnClient ? "yes" : "no") << endl;
  os << indent << "SettingClientSelection: " 
     << (this->SettingClientSelection ? "yes" : "no") << endl;
}
