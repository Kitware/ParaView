// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMCameraLink.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSMMessage.h"
#include "vtkSMProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSmartPointer.h"

#include <list>

vtkStandardNewMacro(vtkSMCameraLink);

//---------------------------------------------------------------------------
class vtkSMCameraLink::vtkInternals
{
public:
  static void UpdateViewCallback(
    vtkObject* caller, unsigned long eid, void* clientData, void* callData)
  {
    vtkSMCameraLink* camLink = reinterpret_cast<vtkSMCameraLink*>(clientData);
    if (!camLink || !camLink->GetEnabled())
    {
      return;
    }

    if (eid == vtkCommand::EndEvent && clientData && caller && callData)
    {
      int* interactive = reinterpret_cast<int*>(callData);
      camLink->UpdateViews(vtkSMProxy::SafeDownCast(caller), (*interactive == 1));
    }
    else if (eid == vtkCommand::ResetCameraEvent && clientData && caller)
    {
      camLink->ResetCamera(caller);
    }
  }

  struct LinkedCamera
  {
    LinkedCamera(vtkSMProxy* proxy, vtkSMCameraLink* camLink)
      : Proxy(proxy)
    {
      this->Observer = vtkSmartPointer<vtkCallbackCommand>::New();
      this->Observer->SetClientData(camLink);
      this->Observer->SetCallback(vtkInternals::UpdateViewCallback);
      proxy->AddObserver(vtkCommand::EndEvent, this->Observer);

      vtkSMRenderViewProxy* rmp = vtkSMRenderViewProxy::SafeDownCast(proxy);
      if (rmp)
      {
        rmp->AddObserver(vtkCommand::ResetCameraEvent, this->Observer);
      }
    };
    ~LinkedCamera()
    {
      this->Proxy->RemoveObserver(this->Observer);
      vtkSMRenderViewProxy* rmp = vtkSMRenderViewProxy::SafeDownCast(this->Proxy);
      if (rmp)
      {
        rmp->RemoveObserver(this->Observer);
      }
    }
    vtkSmartPointer<vtkSMProxy> Proxy;
    vtkSmartPointer<vtkCallbackCommand> Observer;

    LinkedCamera(const LinkedCamera&);
    LinkedCamera& operator=(const LinkedCamera&);
  };

  typedef std::list<LinkedCamera*> LinkedProxiesType;
  LinkedProxiesType LinkedProxies;

  bool Updating;

  vtkInternals() { this->Updating = false; }
  ~vtkInternals()
  {
    LinkedProxiesType::iterator iter;
    for (iter = this->LinkedProxies.begin(); iter != LinkedProxies.end(); ++iter)
    {
      delete *iter;
    }
  }
};

std::set<std::pair<std::string, std::string>> vtkSMCameraLink::CameraProperties()
{
  return { /* from */ /* to */
    { "CameraPositionInfo", "CameraPosition" }, { "CameraViewAngleInfo", "CameraViewAngle" },
    { "CameraFocalPointInfo", "CameraFocalPoint" }, { "CameraViewUpInfo", "CameraViewUp" },
    { "CenterOfRotation", "CenterOfRotation" },
    { "CameraParallelScaleInfo", "CameraParallelScale" }, { "RotationFactor", "RotationFactor" },
    { "CameraParallelProjection", "CameraParallelProjection" },
    { "CameraFocalDiskInfo", "CameraFocalDisk" },
    { "CameraFocalDistanceInfo", "CameraFocalDistance" },
    { "InteractionMode", "InteractionMode" } };
}

//---------------------------------------------------------------------------
vtkSMCameraLink::vtkSMCameraLink()
{
  this->Internals = new vtkInternals;
  this->SynchronizeInteractiveRenders = 1;
}

//---------------------------------------------------------------------------
vtkSMCameraLink::~vtkSMCameraLink()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::AddLinkedProxy(vtkSMProxy* proxy, int updateDir)
{
  // must be render module to link cameras
  if (vtkSMRenderViewProxy::SafeDownCast(proxy))
  {
    this->Superclass::AddLinkedProxy(proxy, updateDir);
    if (updateDir == vtkSMLink::INPUT)
    {
      proxy->CreateVTKObjects();
      // ensure that the proxy is created.
      // This is necessary since when loading state the proxy may not yet be
      // created, however we want to observer events on the
      // interactor.
      this->Internals->LinkedProxies.push_back(new vtkInternals::LinkedCamera(proxy, this));
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::RemoveLinkedProxy(vtkSMProxy* proxy)
{
  this->Superclass::RemoveLinkedProxy(proxy);

  vtkInternals::LinkedProxiesType::iterator iter;
  for (iter = this->Internals->LinkedProxies.begin(); iter != this->Internals->LinkedProxies.end();
       ++iter)
  {
    if ((*iter)->Proxy == proxy)
    {
      delete *iter;
      this->Internals->LinkedProxies.erase(iter);
      break;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::PropertyModified(vtkSMProxy* fromProxy, const char* pname)
{
  if (pname && strcmp(pname, "CenterOfRotation") == 0)
  {
    // We are linking center of rotations for linked views as well.
    // Center of rotation is not changed during interaction, hence
    // we listen to center of rotation changed events explicitly.
    this->UpdateViews(fromProxy, false);
  }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::UpdateVTKObjects(vtkSMProxy* vtkNotUsed(fromProxy))
{
  // do nothing
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::CopyProperties(vtkSMProxy* caller)
{
  for (const auto& propPair : vtkSMCameraLink::CameraProperties())
  {
    vtkSMProperty* fromProp = caller->GetProperty(propPair.first.c_str());

    int numObjects = this->GetNumberOfLinkedObjects();
    for (int i = 0; i < numObjects; i++)
    {
      vtkSMProxy* p = this->GetLinkedProxy(i);
      if (p != caller && this->GetLinkedObjectDirection(i) == vtkSMLink::OUTPUT)
      {
        vtkSMProperty* toProp = p->GetProperty(propPair.second.c_str());
        toProp->Copy(fromProp);
        p->UpdateProperty(propPair.second.c_str());
      }
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::UpdateViews(vtkSMProxy* caller, bool interactive)
{
  if (this->Internals->Updating)
  {
    return;
  }

  this->Internals->Updating = true;
  this->CopyProperties(caller);

  int numObjects = this->GetNumberOfLinkedObjects();
  for (int i = 0; i < numObjects; i++)
  {
    vtkSMProxy* p = this->GetLinkedProxy(i);
    if (this->GetLinkedObjectDirection(i) == vtkSMLink::OUTPUT && p != caller)
    {
      vtkSMRenderViewProxy* rmp;
      rmp = vtkSMRenderViewProxy::SafeDownCast(p);
      if (rmp)
      {
        if (interactive)
        {
          if (this->SynchronizeInteractiveRenders)
          {
            rmp->InteractiveRender();
          }
        }
        else
        {
          rmp->StillRender();
        }
      }
    }
  }
  this->Internals->Updating = false;
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::ResetCamera(vtkObject* caller)
{
  if (this->Internals->Updating)
  {
    return;
  }

  this->Internals->Updating = true;
  this->CopyProperties(vtkSMProxy::SafeDownCast(caller));
  this->Internals->Updating = false;
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SynchronizeInteractiveRenders: " << this->SynchronizeInteractiveRenders << endl;
}

//-----------------------------------------------------------------------------
void vtkSMCameraLink::LoadState(const vtkSMMessage* msg, vtkSMProxyLocator* locator)
{
  this->Superclass::LoadState(msg, locator);
  this->SetSynchronizeInteractiveRenders(
    msg->GetExtension(LinkState::sync_interactive_renders) ? 1 : 0);
}

//-----------------------------------------------------------------------------
void vtkSMCameraLink::UpdateState()
{
  this->Superclass::UpdateState();
  this->State->SetExtension(
    LinkState::sync_interactive_renders, !!this->GetSynchronizeInteractiveRenders());
}
