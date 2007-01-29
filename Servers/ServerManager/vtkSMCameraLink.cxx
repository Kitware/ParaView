/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCameraLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCameraLink.h"

#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkSMStateLoader.h"
#include "vtkSMProperty.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMCameraLink);
vtkCxxRevisionMacro(vtkSMCameraLink, "1.4");

//---------------------------------------------------------------------------
struct vtkSMCameraLinkInternals
{
  static void UpdateViewCallback(vtkObject* caller, unsigned long eid, 
                                 void* clientData, void*)
    {
    if(eid == vtkCommand::EndEvent && clientData && caller)
      {
      vtkSMCameraLink* camLink = reinterpret_cast<vtkSMCameraLink*>(clientData);
      camLink->UpdateViews(vtkSMProxy::SafeDownCast(caller));
      }
    }
  struct LinkedCamera
  {
    LinkedCamera(vtkSMProxy* proxy, vtkSMCameraLink* camLink) : 
      Proxy(proxy)
      {
      this->Observer = vtkSmartPointer<vtkCallbackCommand>::New();
      this->Observer->SetClientData(camLink);
      this->Observer->SetCallback(vtkSMCameraLinkInternals::UpdateViewCallback);
      proxy->AddObserver(vtkCommand::EndEvent, this->Observer);
      };
    ~LinkedCamera()
      {
      this->Proxy->RemoveObserver(this->Observer);
      }
    vtkSmartPointer<vtkSMProxy> Proxy;
    vtkSmartPointer<vtkCallbackCommand> Observer;

    LinkedCamera(const LinkedCamera&);
    LinkedCamera& operator=(const LinkedCamera&);
  };

  typedef vtkstd::list<LinkedCamera*> LinkedProxiesType;
  LinkedProxiesType LinkedProxies;

  bool Updating;

  static const char* LinkedPropertyNames[];

  vtkSMCameraLinkInternals()
    {
    this->Updating = false;
    }
  ~vtkSMCameraLinkInternals()
    {
    LinkedProxiesType::iterator iter;
    for(iter = this->LinkedProxies.begin(); iter != LinkedProxies.end(); ++iter)
      {
      delete *iter;
      }
    }
};

const char* vtkSMCameraLinkInternals::LinkedPropertyNames[] =
{
  /* from */  /* to */
  "CameraPositionInfo", "CameraPosition",
  "CameraFocalPointInfo", "CameraFocalPoint",
  "CameraViewUpInfo", "CameraViewUp",
  0
};

//---------------------------------------------------------------------------
vtkSMCameraLink::vtkSMCameraLink()
{
  this->Internals = new vtkSMCameraLinkInternals;
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
  if(vtkSMRenderModuleProxy::SafeDownCast(proxy))
    {
    Superclass::AddLinkedProxy(proxy, updateDir);
    if(updateDir == vtkSMLink::INPUT)
      {
      this->Internals->LinkedProxies.push_back(
        new vtkSMCameraLinkInternals::LinkedCamera(proxy, this));
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::RemoveLinkedProxy(vtkSMProxy* proxy)
{
  Superclass::RemoveLinkedProxy(proxy);

  vtkSMCameraLinkInternals::LinkedProxiesType::iterator iter;
  for(iter = this->Internals->LinkedProxies.begin();
      iter != this->Internals->LinkedProxies.end();
      ++iter)
    {
    if((*iter)->Proxy == proxy)
      {
      delete *iter;
      this->Internals->LinkedProxies.erase(iter);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::UpdateProperties(vtkSMProxy* vtkNotUsed(fromProxy), 
                                       const char* vtkNotUsed(pname))
{
  return;  // do nothing
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::UpdateVTKObjects(vtkSMProxy* vtkNotUsed(fromProxy))
{
  return;  // do nothing
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::UpdateViews(vtkSMProxy* caller)
{
  if(this->Internals->Updating)
    {
    return;
    }

  this->Internals->Updating = true;

  const char** props = this->Internals->LinkedPropertyNames;

  for(; *props; props+=2)
    {
    vtkSMProperty* fromProp = caller->GetProperty(props[0]);
    
    int numObjects = this->GetNumberOfLinkedProxies();
    for(int i=0; i<numObjects; i++)
      {
      vtkSMProxy* p = this->GetLinkedProxy(i);
      if(p != caller &&
         this->GetLinkedProxyDirection(i) == vtkSMLink::OUTPUT)
        {
        vtkSMProperty* toProp = p->GetProperty(props[1]);
        toProp->Copy(fromProp);
        }
      }
    }

  int numObjects = this->GetNumberOfLinkedProxies();
  for(int i=0; i<numObjects; i++)
    {
    vtkSMProxy* p = this->GetLinkedProxy(i);
    if(this->GetLinkedProxyDirection(i) == vtkSMLink::OUTPUT)
      {
      vtkSMRenderModuleProxy* rmp;
      rmp = vtkSMRenderModuleProxy::SafeDownCast(p);
      if(rmp)
        {
        rmp->StillRender();
        }
      }
    }
  this->Internals->Updating = false;
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::SaveState(const char* linkname, vtkPVXMLElement* parent)
{
  vtkPVXMLElement* root = vtkPVXMLElement::New();
  Superclass::SaveState(linkname, root);
  unsigned int numElems = root->GetNumberOfNestedElements();
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    vtkPVXMLElement* child = root->GetNestedElement(cc);
    child->SetName("CameraLink");
    parent->AddNestedElement(child);
    }
  root->Delete();
}

//---------------------------------------------------------------------------
int vtkSMCameraLink::LoadState(vtkPVXMLElement* linkElement, vtkSMStateLoader* loader)
{
  return this->Superclass::LoadState(linkElement, loader);
}

//---------------------------------------------------------------------------
void vtkSMCameraLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


