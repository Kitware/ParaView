/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCacheBasedProxyLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCacheBasedProxyLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMStateLocator.h"
#include "vtkSMSession.h"
#include "vtkSMProxyManager.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkCollection.h"

#include <vtkstd/map>

class vtkSMCacheBasedProxyLocator::vtkInternal
{
public:
  typedef vtkstd::map<vtkTypeUInt32, vtkSmartPointer<vtkSMProxy> > ProxyMap;
  typedef vtkstd::map<vtkTypeUInt32, vtkSmartPointer<vtkPVXMLElement> > CacheProxyState;

  ProxyMap Proxies;
  CacheProxyState ProxyStates;
};

vtkStandardNewMacro(vtkSMCacheBasedProxyLocator);
vtkCxxSetObjectMacro(vtkSMCacheBasedProxyLocator, StateLocator, vtkSMStateLocator);
//----------------------------------------------------------------------------
vtkSMCacheBasedProxyLocator::vtkSMCacheBasedProxyLocator()
{
  this->Internal = new vtkInternal();
  this->StateLocator = NULL;
}

//----------------------------------------------------------------------------
vtkSMCacheBasedProxyLocator::~vtkSMCacheBasedProxyLocator()
{
  delete this->Internal;
  this->SetStateLocator(NULL);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCacheBasedProxyLocator::LocateProxy(vtkTypeUInt32 id)
{
  if( id == 0 )
    {
    return NULL;
    }

  vtkSMProxy* proxy =
      vtkSMProxy::SafeDownCast(
          this->GetProxyManager()->GetSession()->GetRemoteObject(id));

  // Make sure we have a state locator
  if(!this->StateLocator)
    {
    this->SetStateLocator(this->GetProxyManager()->GetSession()->GetStateLocator());
    }

  if(proxy)
    {
    this->Internal->Proxies[id] = proxy;
    }
  else
    {
    // Need to renew it
    proxy = this->GetProxyManager()->ReNewProxy(id, this->StateLocator);
    this->Internal->Proxies[id].TakeReference(proxy);
    }

  vtkInternal::CacheProxyState::iterator iter = this->Internal->ProxyStates.find(id);
  if (proxy && iter != this->Internal->ProxyStates.end())
    {
    vtkPVXMLElement* root = iter->second;
    proxy->LoadXMLState(root,this);
    proxy->UpdateVTKObjects();
    }
  return proxy;
}

//----------------------------------------------------------------------------
void vtkSMCacheBasedProxyLocator::Clear()
{
  this->Internal->Proxies.clear();
}

//----------------------------------------------------------------------------
void vtkSMCacheBasedProxyLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Deserializer: " << this->Deserializer << endl;
}
//----------------------------------------------------------------------------
void vtkSMCacheBasedProxyLocator::StoreProxyState(vtkSMProxy* proxy)
{
  vtkSmartPointer<vtkPVXMLElement> proxyState;
  proxyState.TakeReference(proxy->SaveXMLState(NULL));
  //proxyState->PrintXML();
  this->Internal->ProxyStates[proxy->GetGlobalID()] = proxyState;
}

//----------------------------------------------------------------------------
void vtkSMCacheBasedProxyLocator::GetLocatedProxies(vtkCollection* collectionToFill)
{
  if(!collectionToFill)
    {
    return;
    }

  vtkInternal::ProxyMap::iterator iter = this->Internal->Proxies.begin();
  for (;iter != this->Internal->Proxies.end();iter++)
    {
    collectionToFill->AddItem(iter->second);
    }
}
