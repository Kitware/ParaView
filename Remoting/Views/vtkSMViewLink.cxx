// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMViewLink.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMCameraLink.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMViewProxy.h"

vtkStandardNewMacro(vtkSMViewLink);

//---------------------------------------------------------------------------
vtkSMViewLink::vtkSMViewLink()
{
  this->AddException("Representations");
  this->AddException("ViewSize");
}

//---------------------------------------------------------------------------
vtkSMViewLink::~vtkSMViewLink()
{
  for (auto& obs : this->RenderObservers)
  {
    obs.first->RemoveObserver(obs.second);
  }
}

//---------------------------------------------------------------------------
void vtkSMViewLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSMViewLink::EnableCameraLink(bool enable)
{
  if (enable)
  {
    for (auto propPair : vtkSMCameraLink::CameraProperties())
    {
      this->RemoveException(propPair.first.c_str());
      this->RemoveException(propPair.second.c_str());
    }
    this->UpdateViewsOnEndEvent = true;
  }
  else
  {
    // do not link cameras: this is the purpose of the vtkSMCameraLink
    for (auto propPair : vtkSMCameraLink::CameraProperties())
    {
      this->AddException(propPair.first.c_str());
      this->AddException(propPair.second.c_str());
    }
    this->UpdateViewsOnEndEvent = false;
  }
}

//---------------------------------------------------------------------------
void vtkSMViewLink::UpdateViews(vtkSMProxy* caller)
{
  if (this->Updating)
  {
    return;
  }

  this->Updating = true;

  int numObjects = this->GetNumberOfLinkedObjects();
  for (int i = 0; i < numObjects; i++)
  {
    vtkSMProxy* proxy = this->GetLinkedProxy(i);
    // if here from a callback, do not render the source view again.
    if (this->GetLinkedObjectDirection(i) == vtkSMLink::OUTPUT && proxy != caller)
    {
      vtkSMViewProxy* viewProxy = vtkSMViewProxy::SafeDownCast(proxy);
      if (viewProxy)
      {
        viewProxy->StillRender();
      }
    }
  }
  this->Updating = false;
}

//---------------------------------------------------------------------------
void vtkSMViewLink::UpdateViewCallback(
  vtkObject* caller, unsigned long eid, void* clientData, void*)
{
  vtkSMLink* link = reinterpret_cast<vtkSMLink*>(clientData);
  if (!link || !link->GetEnabled())
  {
    return;
  }

  vtkSMViewLink* viewLink = vtkSMViewLink::SafeDownCast(link);
  if (!viewLink)
  {
    return;
  }

  if (viewLink->UpdateViewsOnEndEvent && eid == vtkCommand::EndEvent && clientData && caller)
  {
    viewLink->UpdateViews(vtkSMProxy::SafeDownCast(caller));
  }
}

//---------------------------------------------------------------------------
void vtkSMViewLink::AddLinkedProxy(vtkSMProxy* proxy, int updateDir)
{
  vtkSMViewProxy* viewProxy = vtkSMViewProxy::SafeDownCast(proxy);
  if (!viewProxy)
  {
    vtkErrorMacro("Proxy should be a view.");
    return;
  }

  this->Superclass::AddLinkedProxy(proxy, updateDir);

  if (updateDir == vtkSMLink::INPUT)
  {
    vtkNew<vtkCallbackCommand> observer;
    observer->SetClientData(this);
    observer->SetCallback(vtkSMViewLink::UpdateViewCallback);
    proxy->AddObserver(vtkCommand::EndEvent, observer);
    this->RenderObservers[proxy] = observer;
  }
}

//---------------------------------------------------------------------------
void vtkSMViewLink::RemoveLinkedProxy(vtkSMProxy* proxy)
{
  this->Superclass::RemoveLinkedProxy(proxy);
  this->RenderObservers.erase(proxy);
}

//---------------------------------------------------------------------------
void vtkSMViewLink::UpdateVTKObjects(vtkSMProxy* proxy)
{
  this->Superclass::UpdateVTKObjects(proxy);
  this->UpdateViews(nullptr);
}
