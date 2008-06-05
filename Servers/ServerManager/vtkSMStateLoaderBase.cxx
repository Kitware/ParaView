/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStateLoaderBase.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStateLoaderBase.h"

#include "vtkClientServerInterpreter.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyManager.h"

#include <vtkstd/map>
#include <vtksys/ios/sstream>


//----------------------------------------------------------------------------
class vtkSMStateLoaderBase::vtkInternal
{
public:
  typedef vtkstd::map<int, vtkSmartPointer<vtkSMProxy> >  ProxyMapType;
  ProxyMapType CreatedProxies;
};


vtkCxxRevisionMacro(vtkSMStateLoaderBase, "1.4");
//----------------------------------------------------------------------------
vtkSMStateLoaderBase::vtkSMStateLoaderBase()
{
  this->Internal = new vtkSMStateLoaderBase::vtkInternal();
  this->ReviveProxies = 0;
  this->UseExistingProxies = false;
  this->ConnectionID = 
    vtkProcessModuleConnectionManager::GetNullConnectionID();
}

//----------------------------------------------------------------------------
vtkSMStateLoaderBase::~vtkSMStateLoaderBase()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoaderBase::GetCreatedProxy(int id)
{
  vtkInternal::ProxyMapType::iterator iter =
    this->Internal->CreatedProxies.find(id);
  if (iter != this->Internal->CreatedProxies.end())
    {
    return iter->second;
    }
  return 0;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoaderBase::NewProxy(int id)
{
  vtkSMProxy* proxy = this->GetCreatedProxy(id);
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }

  vtkPVXMLElement* proxyElement = this->LocateProxyElement(id);
  return this->NewProxyFromElement(proxyElement, id);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoaderBase::NewProxyFromElement(vtkPVXMLElement* proxyElement, int id)
{
  // Search for the proxy in the CreatedProxies map.
  vtkSMProxy* proxy = this->GetCreatedProxy(id);
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }

  // If the state loader is set up to treat id as the interpretor id for
  // the proxy, this call will try to find the proxy with the given SelfID.
  proxy = this->GetExistingProxy(id);
  if (proxy)
    {
    proxy->Register(this);
    return proxy;
    }

  if (!proxyElement)
    {
    return 0;
    }

  if (strcmp(proxyElement->GetName(), "Proxy") == 0)
    {
    const char* group = proxyElement->GetAttribute("group");
    const char* type = proxyElement->GetAttribute("type");
    if (!type)
      {
      vtkErrorMacro("Could not create proxy from element, missing 'type'.");
      return 0;
      }

    // if group is 0, implies it is a compound proxy.
    proxy = this->NewProxyInternal(group, type);
    if (!proxy)
      {
      vtkErrorMacro("Could not create a proxy of group: "
                    << (group? group : "(null)")
                    << " type: "
                    << type);
      return 0;
      }
    }

  if (!proxy)
    {
    return 0;
    }

  if (!proxy->GetObjectsCreated())
    {
    proxy->SetConnectionID(this->ConnectionID);
    }

  this->Internal->CreatedProxies[id] = proxy;
  if (!this->LoadProxyState(proxyElement, proxy))
    {
    vtkInternal::ProxyMapType::iterator iter2 =
      this->Internal->CreatedProxies.find(id);
    this->Internal->CreatedProxies.erase(iter2);
    proxy->Delete();
    vtkErrorMacro("Failed to load state.");
    return 0;
    }

  if (this->UseExistingProxies)
    {
    vtkClientServerID csid;
    csid.ID = static_cast<vtkTypeUInt32>(id);
    proxy->SetSelfID(csid);
    }

  vtksys_ios::ostringstream stream;
  stream << "Created New Proxy: " << proxy->GetXMLGroup() << " , " << proxy->GetXMLName();
  vtkProcessModule::DebugLog(stream.str().c_str());
  proxy->UpdateVTKObjects();

  // Give subclasses a chance to process the newly created proxy.
  this->CreatedNewProxy(id, proxy);
  return proxy;
}

//----------------------------------------------------------------------------
int vtkSMStateLoaderBase::LoadProxyState(vtkPVXMLElement* elem, vtkSMProxy* proxy)
{
  return proxy->LoadState(elem, this);
}

//---------------------------------------------------------------------------
void vtkSMStateLoaderBase::ClearCreatedProxies()
{
  this->Internal->CreatedProxies.clear();
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoaderBase::GetExistingProxy(int id)
{
  if (this->UseExistingProxies)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerID csid;
    csid.ID = static_cast<vtkTypeUInt32>(id);
    vtkSMProxy* proxy = vtkSMProxy::SafeDownCast(
      pm->GetInterpreter()->GetObjectFromID(csid, 1));
    return proxy;
    }

  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMStateLoaderBase::NewProxyInternal(const char* xmlgroup, 
  const char* xmlname)
{
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  return pxm->NewProxy(xmlgroup, xmlname);
}

//----------------------------------------------------------------------------
void vtkSMStateLoaderBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ConnectionID: " << this->ConnectionID << endl;
  os << indent << "UseExistingProxies: " << this->UseExistingProxies << endl;
  os << indent << "ReviveProxies: " << this->ReviveProxies << endl;
}


