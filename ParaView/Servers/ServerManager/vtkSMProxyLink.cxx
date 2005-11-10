/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLink.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyLink.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <vtkstd/list>

vtkStandardNewMacro(vtkSMProxyLink);
vtkCxxRevisionMacro(vtkSMProxyLink, "1.1");

class vtkSMProxyUpdateObserver : public vtkCommand
{
public:

  static vtkSMProxyUpdateObserver* New()
    {
      return new vtkSMProxyUpdateObserver;
    }
  vtkSMProxyUpdateObserver()
    {
      this->Link = 0;
    }
  ~vtkSMProxyUpdateObserver()
    {
      this->Link = 0;
    }
  
  virtual void Execute(vtkObject *caller, unsigned long event, void* pname)
    {
      if (this->Link)
        {
        if (event == vtkCommand::UpdateEvent)
          {
          this->Link->UpdateVTKObjects(caller);
          }
        else if (event == vtkCommand::PropertyModifiedEvent)
          {
          this->Link->UpdateProperties(caller, (const char*)pname);
          }
        }
    }

  vtkSMProxyLink* Link;
};

struct vtkSMProxyLinkInternals
{
  struct LinkedProxy
  {
    LinkedProxy(vtkSMProxy* proxy, int updateDir) : 
      Proxy(proxy), UpdateDirection(updateDir), Observer(0)
      {
      };
    ~LinkedProxy()
      {
        if (this->Observer && this->Proxy.GetPointer())
          {
          this->Proxy.GetPointer()->RemoveObserver(Observer);
          this->Observer = 0;
          }
      }
    vtkSmartPointer<vtkSMProxy> Proxy;
    int UpdateDirection;
    vtkCommand* Observer;
  };

  typedef vtkstd::list<LinkedProxy> LinkedProxiesType;
  LinkedProxiesType LinkedProxies;
};

//---------------------------------------------------------------------------
vtkSMProxyLink::vtkSMProxyLink()
{
  this->Internals = new vtkSMProxyLinkInternals;
  vtkSMProxyUpdateObserver* obs = vtkSMProxyUpdateObserver::New();
  obs->Link = this;
  this->Observer = obs;
}

//---------------------------------------------------------------------------
vtkSMProxyLink::~vtkSMProxyLink()
{
  delete this->Internals;

  this->Observer->Delete();
  this->Observer = 0;
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::AddLinkedProxy(vtkSMProxy* proxy, int updateDir)
{
  int addToList = 1;
  int addObserver = updateDir & INPUT;

  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if (iter->Proxy == proxy)
      {
      if (iter->UpdateDirection != updateDir)
        {
        iter->UpdateDirection = updateDir;
        }
      else
        {
        addObserver = 0;
        }
      addToList = 0;
      }
    }

  if (addToList)
    {
    vtkSMProxyLinkInternals::LinkedProxy link(proxy, updateDir);
    link.Observer = this->Observer;
    this->Internals->LinkedProxies.push_back(
      vtkSMProxyLinkInternals::LinkedProxy(proxy, updateDir));
    }

  if (addObserver)
    {
    proxy->AddObserver(vtkCommand::UpdateEvent, this->Observer);
    proxy->AddObserver(vtkCommand::PropertyModifiedEvent, this->Observer);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::UpdateProperties(vtkObject* caller, const char* pname)
{
  vtkSMProxy* fromProxy = vtkSMProxy::SafeDownCast(caller);
  if (!fromProxy)
    {
    return;
    }
  vtkSMProperty* fromProp = fromProxy->GetProperty(pname);
  if (!fromProp)
    {
    return;
    }

  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      vtkSMProperty* toProp = iter->Proxy->GetProperty(pname);
      if (toProp)
        {
        toProp->Copy(fromProp);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::UpdateVTKObjects(vtkObject* caller)
{
  vtkSMProxyLinkInternals::LinkedProxiesType::iterator iter =
    this->Internals->LinkedProxies.begin();
  for(; iter != this->Internals->LinkedProxies.end(); iter++)
    {
    if ((iter->Proxy.GetPointer() != caller) && 
        (iter->UpdateDirection & OUTPUT))
      {
      iter->Proxy->UpdateVTKObjects();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
