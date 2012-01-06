/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxyLocator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxyLocator.h"

#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkSMDeserializer.h"
#include "vtkSMSession.h"
#include "vtkSMProxy.h"
#include "vtkCollection.h"

#include <map>

class vtkSMProxyLocator::vtkInternal
{
public:
  typedef std::map<vtkTypeUInt32, vtkSmartPointer<vtkSMProxy> > ProxiesType;
  ProxiesType Proxies;
};

vtkStandardNewMacro(vtkSMProxyLocator);
vtkCxxSetObjectMacro(vtkSMProxyLocator, Deserializer, vtkSMDeserializer);
//----------------------------------------------------------------------------
vtkSMProxyLocator::vtkSMProxyLocator()
{
  this->Internal = new vtkInternal();
  this->Deserializer = 0;
  this->Session = 0;
  this->LocateProxyWithSessionToo = false;
}

//----------------------------------------------------------------------------
vtkSMProxyLocator::~vtkSMProxyLocator()
{
  delete this->Internal;
  this->SetDeserializer(0);
  this->SetSession(0);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyLocator::LocateProxy(vtkTypeUInt32 id)
{
  // Look in the cache first
  vtkInternal::ProxiesType::iterator iter = this->Internal->Proxies.find(id);
  if (iter != this->Internal->Proxies.end())
    {
    return iter->second.GetPointer();
    }

  // Try to lookup in session (if we setup the locator to use the session too)
  vtkSMProxy* proxy;
  if(this->LocateProxyWithSessionToo && this->Session)
    {
    proxy = vtkSMProxy::SafeDownCast(this->Session->GetRemoteObject(id));
    if(proxy)
      {
      this->Internal->Proxies[id] = proxy;
      return proxy;
      }
    }

  // Create a brand new proxy
  proxy = this->NewProxy(id);
  if (proxy)
    {
    this->Internal->Proxies[id].TakeReference(proxy);
    }

  return proxy;
}

//----------------------------------------------------------------------------
void vtkSMProxyLocator::Clear()
{
  this->Internal->Proxies.clear();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMProxyLocator::NewProxy(vtkTypeUInt32 id)
{
  if (this->Deserializer)
    {
    // Ask the deserializer to create a new proxy with the given id. The
    // deserializer will locate the State(XML/Protobuf) for the proxy with
    // that id, and load the state on it and then return this fresh proxy, if possible.
    return this->Deserializer->NewProxy(id, this);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMProxyLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Deserializer: " << this->Deserializer << endl;
}
//----------------------------------------------------------------------------
void vtkSMProxyLocator::GetLocatedProxies(vtkCollection* collectionToFill)
{
  if(!collectionToFill)
    {
    return;
    }

  vtkInternal::ProxiesType::iterator iter = this->Internal->Proxies.begin();
  while(iter != this->Internal->Proxies.end())
    {
    collectionToFill->AddItem(iter->second.GetPointer());
    iter++;
    }
}
//----------------------------------------------------------------------------
vtkSMSession* vtkSMProxyLocator::GetSession()
{
  return this->Session.GetPointer();
}
//----------------------------------------------------------------------------
void vtkSMProxyLocator::SetSession(vtkSMSession* s)
{
  this->Session = s;
  if(this->Deserializer && s)
    {
    this->Deserializer->SetSession(s);
    }
}
