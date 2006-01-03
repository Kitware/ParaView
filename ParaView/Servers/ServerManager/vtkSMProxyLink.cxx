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
vtkCxxRevisionMacro(vtkSMProxyLink, "1.3");

//---------------------------------------------------------------------------
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
}

//---------------------------------------------------------------------------
vtkSMProxyLink::~vtkSMProxyLink()
{
  delete this->Internals;
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
        if (addObserver)
          {
          iter->Observer = this->Observer;
          }
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
    this->Internals->LinkedProxies.push_back(link);
    if (addObserver)
      {
      this->Internals->LinkedProxies.back().Observer = this->Observer;
      }
    }

  if (addObserver)
    {
    this->ObserveProxyUpdates(proxy);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxyLink::UpdateProperties(vtkSMProxy* fromProxy, const char* pname)
{
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
    if ((iter->Proxy.GetPointer() != fromProxy) && 
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
void vtkSMProxyLink::UpdateVTKObjects(vtkSMProxy* caller)
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
