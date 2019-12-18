/*=========================================================================

  Program:   ParaView
  Module:    vtkSMLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVSession.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"

#include <list>

//-----------------------------------------------------------------------------
class vtkSMLinkObserver : public vtkCommand
{
public:
  static vtkSMLinkObserver* New() { return new vtkSMLinkObserver; }
  vtkSMLinkObserver()
  {
    this->Link = 0;
    this->InProgress = false;
  }
  ~vtkSMLinkObserver() override { this->Link = 0; }

  void Execute(vtkObject* c, unsigned long event, void* pname) override
  {
    if (this->InProgress)
    {
      return;
    }

    if (this->Link && !this->Link->GetEnabled())
    {
      return;
    }

    this->InProgress = true;
    vtkSMProxy* caller = vtkSMProxy::SafeDownCast(c);
    if (this->Link && caller)
    {
      if (event == vtkCommand::UpdateEvent && this->Link->GetPropagateUpdateVTKObjects())
      {
        this->Link->UpdateVTKObjects(caller);
      }
      else if (event == vtkCommand::PropertyModifiedEvent)
      {
        this->Link->PropertyModified(caller, (const char*)pname);
      }
      else if (event == vtkCommand::UpdatePropertyEvent)
      {
        this->Link->UpdateProperty(caller, reinterpret_cast<char*>(pname));
      }
    }
    this->InProgress = false;
  }

  vtkSMLink* Link;
  bool InProgress;
};

//-----------------------------------------------------------------------------
vtkSMLink::vtkSMLink()
{
  vtkSMLinkObserver* obs = vtkSMLinkObserver::New();
  obs->Link = this;
  this->Observer = obs;
  this->PropagateUpdateVTKObjects = 1;
  this->Enabled = true;

  this->State = new vtkSMMessage();
  this->SetLocation(vtkPVSession::CLIENT);
  this->State->SetExtension(DefinitionHeader::server_class, "vtkSIObject"); // Dummy SIObject
}

//-----------------------------------------------------------------------------
vtkSMLink::~vtkSMLink()
{
  ((vtkSMLinkObserver*)this->Observer)->Link = NULL;
  this->Observer->Delete();
  this->Observer = NULL;
  delete this->State;
  this->State = NULL;
}

//-----------------------------------------------------------------------------
void vtkSMLink::ObserveProxyUpdates(vtkSMProxy* proxy)
{
  proxy->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
  proxy->AddObserver(vtkCommand::UpdateEvent, this->Observer);
  proxy->AddObserver(vtkCommand::UpdatePropertyEvent, this->Observer);
}

//-----------------------------------------------------------------------------
void vtkSMLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "PropagateUpdateVTKObjects: " << this->PropagateUpdateVTKObjects << endl;
}

//-----------------------------------------------------------------------------
void vtkSMLink::PushStateToSession()
{
  if (!this->IsLocalPushOnly() && this->GetSession())
  {
    this->State->SetExtension(DefinitionHeader::client_class, this->GetClassName());
    this->State->SetExtension(LinkState::propagate_update, (this->PropagateUpdateVTKObjects != 0));
    this->State->SetExtension(LinkState::enabled, this->Enabled);
    this->PushState(this->State);
  }
}

//-----------------------------------------------------------------------------
const vtkSMMessage* vtkSMLink::GetFullState()
{
  return this->State;
}

//-----------------------------------------------------------------------------
void vtkSMLink::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(msg, locator);
  this->SetPropagateUpdateVTKObjects(msg->GetExtension(LinkState::propagate_update) ? 1 : 0);
  this->SetEnabled(msg->GetExtension(LinkState::enabled) ? 1 : 0);
}
