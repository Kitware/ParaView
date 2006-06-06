/*=========================================================================

  Program:   ParaView
  Module:    vtkSMProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMProxy.h"

#include "vtkClientServerInterpreter.h"
#include "vtkCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkSMDocumentation.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMStateLoader.h"

#include "vtkSMProxyInternals.h"

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMProxy);
vtkCxxRevisionMacro(vtkSMProxy, "1.72");

vtkCxxSetObjectMacro(vtkSMProxy, XMLElement, vtkPVXMLElement);

//---------------------------------------------------------------------------
// Observer for modified event of the property
class vtkSMProxyObserver : public vtkCommand
{
public:
  static vtkSMProxyObserver *New() 
    { return new vtkSMProxyObserver; }

  virtual void Execute(vtkObject* obj, unsigned long event, void* data)
    {
    if (this->Proxy)
      {
      if (this->PropertyName)
        {
        // This is observing a property.
        this->Proxy->SetPropertyModifiedFlag(this->PropertyName, 1);
        }
      else 
        {
        this->Proxy->ExecuteSubProxyEvent(vtkSMProxy::SafeDownCast(obj), 
          event, data);
        }
      }
    }

  void SetPropertyName(const char* name)
    {
      if ( !this->PropertyName && !name ) { return; } 
      delete [] this->PropertyName;
      if (name) 
        { 
        this->PropertyName = new char[strlen(name)+1]; 
        strcpy(this->PropertyName,name); 
        } 
      else 
        { 
        this->PropertyName = 0; 
        } 
    }

  // Note that Proxy is not reference counted. Since the Proxy has a reference 
  // to the Property and the Property has a reference to the Observer, making
  // Proxy reference counted would cause a loop.
  void SetProxy(vtkSMProxy* proxy)
    {
      this->Proxy = proxy;
    }

protected:
  char* PropertyName;
  vtkSMProxy* Proxy;

  vtkSMProxyObserver() : PropertyName(0) {};
  ~vtkSMProxyObserver() { 
    if (this->PropertyName) 
      {
      delete [] this->PropertyName;
      this->PropertyName = 0;
      }
  };
      
};

//---------------------------------------------------------------------------
vtkSMProxy::vtkSMProxy()
{
  this->Internals = new vtkSMProxyInternals;
  // By default, all objects are created on data server.
  this->Servers = vtkProcessModule::DATA_SERVER;
  this->Name = 0;
  this->VTKClassName = 0;
  this->XMLGroup = 0;
  this->XMLName = 0;
  this->ObjectsCreated = 0;

  vtkClientServerID nullID = { 0 };
  this->SelfID = nullID;
  this->ConnectionID = 
    vtkProcessModuleConnectionManager::GetRootServerConnectionID();

  this->XMLElement = 0;
  this->DoNotUpdateImmediately = 0;
  this->DoNotModifyProperty = 0;
  this->InUpdateVTKObjects = 0;
  this->SelfPropertiesModified = 0;

  this->SubProxyObserver = vtkSMProxyObserver::New();
  this->SubProxyObserver->SetProxy(this);

  this->Documentation = vtkSMDocumentation::New();
}

//---------------------------------------------------------------------------
vtkSMProxy::~vtkSMProxy()
{
  this->SetName(0);
  if (this->ObjectsCreated)
    {
    this->UnRegisterVTKObjects();
    }
  this->RemoveAllObservers();

  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.begin();
  // To remove cyclic dependancy as well as this proxy from
  // the consumer list of all
  for(; it != this->Internals->Properties.end(); it++)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    prop->RemoveAllDependents();
    if (prop->IsA("vtkSMProxyProperty"))
      {
      vtkSMProxyProperty::SafeDownCast(
        prop)->RemoveConsumerFromPreviousProxies(this);
      }
    }
  delete this->Internals;
  this->SetVTKClassName(0);
  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetXMLElement(0);

  if (this->SubProxyObserver)
    {
    this->SubProxyObserver->SetProxy(0);
    this->SubProxyObserver->Delete();
    }
  this->Documentation->Delete();
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetSelfIDAsString()
{
  if (!this->Name)
    {
    this->GetSelfID();
    }

  return this->Name;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetSelfID(vtkClientServerID id)
{
  if (this->SelfID.ID != 0)
    {
    vtkErrorMacro("Cannot change the SelfID after the proxy object"
      " has been assigned an ID.");
    return;
    }
  this->SelfID = id;
  this->RegisterSelfID();
}

//---------------------------------------------------------------------------
void vtkSMProxy::RegisterSelfID()
{
  // Assign a unique clientserver id to this object.
  // Note that this ups the reference count to 2.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("Can not fully initialize without a global "
      "ProcessModule. This object will not be fully "
      "functional.");
    return ;
    }
  
  vtkClientServerStream initStream;
  initStream << vtkClientServerStream::Assign 
    << this->SelfID << this
    << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, initStream);

  // This is done to make the last result message release it's reference 
  // count. Otherwise the object has a reference count of 3.
  pm->GetInterpreter()->ClearLastResult();

  if (!this->Name) 
    {
    ostrstream str;
    str << this->SelfID << ends;
    this->SetName(str.str());
    delete[] str.str();
    }
}

//---------------------------------------------------------------------------
vtkClientServerID vtkSMProxy::GetSelfID()
{
  if (this->SelfID.ID != 0)
    {
    return this->SelfID;
    }
  
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("Can not fully initialize without a global "
      "ProcessModule. This object will not be fully "
      "functional.");
    return this->SelfID;
    }
  this->SelfID = pm->GetUniqueID();
  this->RegisterSelfID();  
  return this->SelfID;
}

//---------------------------------------------------------------------------
void vtkSMProxy::UnRegisterVTKObjects()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    return;
    }
  vtkClientServerStream stream;

  vtkstd::vector<vtkClientServerID>::iterator it;
  for (it=this->Internals->IDs.begin(); it!=this->Internals->IDs.end(); ++it)
    {
    pm->DeleteStreamObject(*it, stream);
    }
  pm->SendStream(this->ConnectionID, this->Servers, stream);

  this->Internals->IDs.clear();

  this->ObjectsCreated = 0;
}

//-----------------------------------------------------------------------------
// UnRegister is overloaded because the object has a reference to itself
// through the clientserver id.
void vtkSMProxy::UnRegister(vtkObjectBase* obj)
{
  if ( this->SelfID.ID != 0 )
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    // If the object is not being deleted by the interpreter and it
    // has a reference count of 2, unassign the clientserver id.
    if ( pm && obj != pm->GetInterpreter() )
      {
      if (this->ReferenceCount == 2)
        {
        vtkClientServerID selfid = this->SelfID;
        this->SelfID.ID = 0;
        vtkClientServerStream deleteStream;
        deleteStream << vtkClientServerStream::Delete 
                     << selfid
                     << vtkClientServerStream::End;
        pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, deleteStream);
        }
      }
    }
  this->Superclass::UnRegister(obj);
}


//---------------------------------------------------------------------------
void vtkSMProxy::SetServers(vtkTypeUInt32 servers)
{
  this->SetServersSelf(servers);

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->SetServersSelf(servers);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetServersSelf(vtkTypeUInt32 servers)
{
  if (this->Servers == servers)
    {
    return;
    }
  this->Servers = servers;
  this->Modified();
}

//---------------------------------------------------------------------------
vtkTypeUInt32 vtkSMProxy::GetServers()
{
  return this->Servers;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetConnectionID(vtkIdType id)
{
  if (this->SelfID.ID)
    {
    // Implies that the proxy performed some processing.
    vtkErrorMacro(
      "Connection ID can be changed immeditely after creating the proxy.");
    return;
    }
  
  this->SetConnectionIDSelf(id);
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->SetConnectionID(id);
    } 
}

//---------------------------------------------------------------------------
vtkIdType vtkSMProxy::GetConnectionID()
{
  return this->ConnectionID;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetConnectionIDSelf(vtkIdType id)
{
  if (this->ConnectionID == id)
    {
    return;
    }
  this->ConnectionID = id;
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfIDs()
{
  return this->Internals->IDs.size();
}

//---------------------------------------------------------------------------
vtkClientServerID vtkSMProxy::GetID(unsigned int idx)
{
  this->CreateVTKObjects(idx+1);

  return this->Internals->IDs[idx];
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetID(unsigned int idx, vtkClientServerID id)
{
  if (idx >= this->Internals->IDs.size())
    {
    this->Internals->IDs.resize(idx+1);
    }
  this->Internals->IDs[idx] = id;
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetPropertyName(vtkSMProperty* prop)
{
  const char* result = 0;
  vtkSMPropertyIterator* piter = this->NewPropertyIterator();
  for(piter->Begin(); !piter->IsAtEnd(); piter->Next())
    {
    if (prop == piter->GetProperty())
      {
      result = piter->GetKey();
      }
    }
  piter->Delete();
  return result;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetProperty(const char* name, int selfOnly)
{
  if (!name)
    {
    return 0;
    }
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it != this->Internals->Properties.end())
    {
    return it->second.Property.GetPointer();
    }
  if (!selfOnly)
    {
    vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter = 
      this->Internals->ExposedProperties.find(name);
    if (eiter == this->Internals->ExposedProperties.end())
      {
      // no such property is being exposed.
      return 0;
      }
    const char* subproxy_name =  eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy * sp = this->GetSubProxy(subproxy_name);
    if (sp)
      {
      return sp->GetProperty(property_name, 0);
      }
    // indicates that the internal dbase for exposed properties is
    // corrupt.. when a subproxy was removed, the exposed properties
    // for that proxy should also have been cleaned up.
    // Flag an error so that it can be debugged.
    vtkWarningMacro("Subproxy required for the exposed property is missing."
      "No subproxy with name : " << subproxy_name);
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveAllObservers()
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (it->second.ObserverTag > 0)
      {
      prop->RemoveObserver(it->second.ObserverTag);
      }
    }
  
  vtkSMProxyInternals::ProxyMap::iterator it2;
  for (it2 = this->Internals->SubProxies.begin();
    it2 != this->Internals->SubProxies.end();
    ++it2)
    {
    it2->second.GetPointer()->RemoveObserver(this->SubProxyObserver);
    }

}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProperty(const char* name, vtkSMProperty* prop)
{
  this->AddProperty(0, name, prop);
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveProperty(const char* name)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->RemoveProperty(name);
    }

  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it != this->Internals->Properties.end())
    {
    this->Internals->Properties.erase(it);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProperty(const char* subProxyName, 
                             const char* name, 
                             vtkSMProperty* prop)
{
  if (!prop)
    {
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Can not add a property without a name.");
    return;
    }

  if (! subProxyName)
    {
    // Check if the property is in a sub-proxy. If so, replace.
    vtkSMProxyInternals::ProxyMap::iterator it2 =
      this->Internals->SubProxies.begin();
    for( ; it2 != this->Internals->SubProxies.end(); it2++)
      {
      vtkSMProperty* oldprop = it2->second.GetPointer()->GetProperty(name);
      if (oldprop)
        {
        it2->second.GetPointer()->AddProperty(name, prop);
        return;
        }
      }
    this->AddPropertyToSelf(name, prop);
    }
  else
    {
    vtkSMProxyInternals::ProxyMap::iterator it =
      this->Internals->SubProxies.find(name);

    if (it == this->Internals->SubProxies.end())
      {
      vtkWarningMacro("Can not find sub-proxy " 
                      << name  
                      << ". Will not add property.");
      return;
      }
    it->second.GetPointer()->AddProperty(name, prop);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddPropertyToSelf(
  const char* name, vtkSMProperty* prop)
{
  if (!prop)
    {
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Can not add a property without a name.");
    return;
    }

  // Check if the property already exists. If it does, we will
  // replace it (and remove the observer from it)
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);

  if (it != this->Internals->Properties.end())
    {
    vtkWarningMacro("Property " << name  << " already exists. Replacing");
    vtkSMProperty* oldProp = it->second.Property.GetPointer();
    if (it->second.ObserverTag > 0)
      {
      oldProp->RemoveObserver(it->second.ObserverTag);
      }
    }

  unsigned int tag=0;

  vtkSMProxyObserver* obs = vtkSMProxyObserver::New();
  obs->SetProxy(this);
  obs->SetPropertyName(name);
  // We have to store the tag in order to be able to remove
  // the observer later.
  tag = prop->AddObserver(vtkCommand::ModifiedEvent, obs);
  obs->Delete();

  vtkSMProxyInternals::PropertyInfo newEntry;
  newEntry.Property = prop;
  newEntry.ObserverTag = tag;
  this->Internals->Properties[name] = newEntry;
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateProperty(const char* name)
{
  // This will ensure that the SelfID is assigned properly.
  this->GetSelfID();

  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
    // Search exposed subproxy properties.
    vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter = 
      this->Internals->ExposedProperties.find(name);
    if (eiter == this->Internals->ExposedProperties.end())
      {
      return;
      }
    const char* subproxy_name =  eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy * sp = this->GetSubProxy(subproxy_name);
    if (sp)
      {
      sp->UpdateProperty(property_name);
      }
    return;
    }
  if (it->second.ModifiedFlag)
    {
    // In case this property is a self property and causes
    // another UpdateVTKObjects(), make sure that it does
    // not cause recursion. If this is not set, UpdateVTKObjects()
    // that is caused by UpdateProperty() can end up calling trying
    // to push the same property.
    it->second.ModifiedFlag = 0;

    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (prop->GetUpdateSelf())
      {
      vtkClientServerStream str;
      prop->AppendCommandToStream(this, &str, this->GetSelfID());
      pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, str);
      this->MarkModified(this);
      }
    else
      {
      vtkClientServerStream str;
      int numObjects = this->Internals->IDs.size();
      for (int i=0; i<numObjects; i++)
        {
        prop->AppendCommandToStream(this, &str, this->Internals->IDs[i]);
        }
      if (str.GetNumberOfMessages() > 0)
        {
        // Make sure that server side objects exist before
        // pushing values to them
        this->CreateVTKObjects(1);

        pm->SendStream(this->ConnectionID, this->Servers, str);
        this->MarkModified(this);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  if (this->DoNotModifyProperty)
    {
    return;
    }

  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
    return;
    }

  this->InvokeEvent(vtkCommand::PropertyModifiedEvent, (void*)name);
  
  vtkSMProperty* prop = it->second.Property.GetPointer();
  if (prop->GetInformationOnly())
    {
    // Information only property is modified...nothing much to do.
    return;
    }

  it->second.ModifiedFlag = flag;

  if (flag && !this->DoNotUpdateImmediately && prop->GetImmediateUpdate())
    {
    // If ImmediateUpdate is set, update the server immediatly.
    // Also set the modified flag to 0.
    //
    // This special condition is necessary because VTK objects cannot
    // be created before the input is set.
    if (!vtkSMInputProperty::SafeDownCast(prop))
      {
      this->CreateVTKObjects(1);
      }
    this->UpdateProperty(it->first.c_str());
    }
  else
    {
    this->SelfPropertiesModified = 1;
    }
}

//-----------------------------------------------------------------------------
void vtkSMProxy::MarkAllPropertiesAsModified()
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it = this->Internals->Properties.begin();
       it != this->Internals->Properties.end(); it++)
    {
    // Not the most efficient way to set the flag, but probably the safest.
    this->SetPropertyModifiedFlag(it->first.c_str(), 1);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformation(vtkSMProperty* prop)
{
  // If property does not belong to this proxy do nothing.
  int found = 0;
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    if (prop == it->second.Property.GetPointer())
      {
      found = 1;
      break;
      }
    }
  if (!found)
    {
    return;
    }

  this->CreateVTKObjects(1);

  if (this->ObjectsCreated)
    {
    if (prop->GetInformationOnly())
      {
      if (prop->GetUpdateSelf())
        {
        prop->UpdateInformation(this->ConnectionID, 
          vtkProcessModule::CLIENT, this->GetSelfID());
        }
      else
        {
        prop->UpdateInformation(this->ConnectionID,
          this->Servers, this->Internals->IDs[0]);
        }
      prop->UpdateDependentDomains();
      }
    this->MarkModified(this);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformation()
{
  this->CreateVTKObjects(1);

  if (!this->ObjectsCreated)
    {
    return;
    }

  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  // Update all properties.
  for (it  = this->Internals->Properties.begin();
    it != this->Internals->Properties.end();
    ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (prop->GetInformationOnly())
      {
      if (prop->GetUpdateSelf())
        {
        prop->UpdateInformation(this->ConnectionID,
          vtkProcessModule::CLIENT, this->GetSelfID());
        }
      else
        {
        prop->UpdateInformation(this->ConnectionID,
          this->Servers, this->Internals->IDs[0]);
        }
      }
    }

  // Make sure all dependent domains are updated. UpdateInformation()
  // might have produced new information that invalidates the domains.
  for (it  = this->Internals->Properties.begin();
    it != this->Internals->Properties.end();
    ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (prop->GetInformationOnly())
      {
      prop->UpdateDependentDomains();
      }
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdatePropertyInformation();
    }
}

//---------------------------------------------------------------------------
int vtkSMProxy::ArePropertiesModified(int selfOnly /*=0*/)
{
  if (this->SelfPropertiesModified)
    {
    return 1;
    }

  if (!selfOnly)
    {
    vtkSMProxyInternals::ProxyMap::iterator it2 =
      this->Internals->SubProxies.begin();
    for( ; it2 != this->Internals->SubProxies.end(); it2++)
      {
      if (it2->second.GetPointer()->ArePropertiesModified())
        {
        return 1;
        }
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateVTKObjects()
{
  if (this->InUpdateVTKObjects)
    {
    return;
    }
  this->InUpdateVTKObjects = 1;

  // This will ensure that the SelfID is assigned properly.
  this->GetSelfID();

  int old_SelfPropertiesModified = this->SelfPropertiesModified;
  
  this->SelfPropertiesModified = 0;

  // Make each property push their values to each VTK object
  // referred by the proxy. This is done by appending all
  // the command to a streaming and executing that stream
  // at the end.
  // The update is done in two passes. First: all input properties
  // are updated, second: all other properties are updated. This
  // is because setting input should create the VTK objects (the
  // number of objects to create is based on the number of inputs
  // in the case of filters)
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  if (old_SelfPropertiesModified)
    {
    for (it  = this->Internals->Properties.begin();
      it != this->Internals->Properties.end();
      ++it)
      {
      vtkSMProperty* prop = it->second.Property.GetPointer();
      if (prop->IsA("vtkSMInputProperty"))
        {
        this->UpdateProperty(it->first.c_str());
        }
      }
    }

  this->CreateVTKObjects(1);
  if (!this->ObjectsCreated)
    {
    this->InUpdateVTKObjects = 0;
    return;
    }

  if (old_SelfPropertiesModified)
    {
    for (it  = this->Internals->Properties.begin();
         it != this->Internals->Properties.end();
         ++it)
      {
      vtkSMProperty* prop = it->second.Property.GetPointer();
      if (!prop->GetInformationOnly())
        {
        this->UpdateProperty(it->first.c_str());
        }
      }
    }

  this->InUpdateVTKObjects = 0;
  
  // If any properties got modified while pushing them,
  // we need to call UpdateVTKObjects again.
  // It is perfectly fine to check the SelfPropertiesModified flag before we 
  // even UpdateVTKObjects on all subproxies, since there is no way that a
  // subproxy can ever set any property on the parent proxy.
  if (this->ArePropertiesModified(1))
    {
    this->UpdateVTKObjects();
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdateVTKObjects();
    }

  this->InvokeEvent(vtkCommand::UpdateEvent, 0);
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateSelfAndAllInputs()
{
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  vtkProcessModule::GetProcessModule()->SendPrepareProgress(
    this->ConnectionID);
  while (!iter->IsAtEnd())
    {
    iter->GetProperty()->UpdateAllInputs();
    iter->Next();
    }
  iter->Delete();
  vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress(
    this->ConnectionID);

  this->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePipelineInformation()
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdatePipelineInformation();
    }

  this->UpdatePropertyInformation();
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->ObjectsCreated = 1;
  this->GetSelfID(); // this will ensure that the SelfID is assigned properly.
  if (this->VTKClassName && this->VTKClassName[0] != '\0')
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    for (int i=0; i<numObjects; i++)
      {
      vtkClientServerID objectId = 
        pm->NewStreamObject(this->VTKClassName, stream);
      
      this->Internals->IDs.push_back(objectId);

      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "RegisterProgressEvent"
        << objectId << static_cast<int>(objectId.ID)
        << vtkClientServerStream::End;
      }
    if (stream.GetNumberOfMessages() > 0)
      {
      pm->SendStream(this->ConnectionID, this->Servers, stream);
      }
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->CreateVTKObjects(numObjects);
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfSubProxies()
{
  return this->Internals->SubProxies.size();
}

//---------------------------------------------------------------------------
const char* vtkSMProxy::GetSubProxyName(unsigned int index)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for(unsigned int idx = 0; 
      it2 != this->Internals->SubProxies.end(); 
      it2++, idx++)
    {
    if (idx == index)
      {
      return it2->first.c_str();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetSubProxy(unsigned int index)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for(unsigned int idx = 0; 
      it2 != this->Internals->SubProxies.end(); 
      it2++, idx++)
    {
    if (idx == index)
      {
      return it2->second.GetPointer();
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetSubProxy(const char* name)
{
  vtkSMProxyInternals::ProxyMap::iterator it =
    this->Internals->SubProxies.find(name);

  if (it == this->Internals->SubProxies.end())
    {
    return 0;
    }

  return it->second.GetPointer();
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddSubProxy(const char* name, vtkSMProxy* proxy)
{
  // Check if the proxy already exists. If it does, we will
  // replace it
  vtkSMProxyInternals::ProxyMap::iterator it =
    this->Internals->SubProxies.find(name);

  if (it != this->Internals->SubProxies.end())
    {
    vtkWarningMacro("Proxy " << name  << " already exists. Replacing");
    // needed to remove any observers.
    this->RemoveSubProxy(name);
    }

  this->Internals->SubProxies[name] = proxy;
  
  proxy->AddObserver(vtkCommand::ModifiedEvent, this->SubProxyObserver);
  proxy->AddObserver(vtkCommand::PropertyModifiedEvent, 
    this->SubProxyObserver);
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveSubProxy(const char* name)
{
  if (!name)
    {
    return;
    }

  vtkSMProxyInternals::ProxyMap::iterator it =
    this->Internals->SubProxies.find(name);

  vtkSmartPointer<vtkSMProxy> subProxy;
  if (it != this->Internals->SubProxies.end())
    {
    subProxy = it->second; // we keep the proxy since we need it to remove links.
    it->second.GetPointer()->RemoveObserver(this->SubProxyObserver);
    // Note, we are assuming here that a proxy cannot be added
    // twice as a subproxy to the same proxy.
    this->Internals->SubProxies.erase(it);
    }
  
  // Now, remove any exposed properties for this subproxy.
  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
    this->Internals->ExposedProperties.begin();
  while ( iter != this->Internals->ExposedProperties.end())
    {
    if (iter->second.SubProxyName == name)
      {
      this->Internals->ExposedProperties.erase(iter);
      // start again.
      iter = this->Internals->ExposedProperties.begin();
      }
    else
      {
      iter++;
      }
    }

  if (subProxy.GetPointer())
    {
    // Now, remove any shared property links for the subproxy.
    vtkSMProxyInternals::SubProxyLinksType::iterator iter2 = 
      this->Internals->SubProxyLinks.begin();
    while (iter2 != this->Internals->SubProxyLinks.end())
      {
      iter2->GetPointer()->RemoveLinkedProxy(subProxy.GetPointer());
      if (iter2->GetPointer()->GetNumberOfLinkedProxies() <= 1)
        {
        // link is useless, remove it.
        this->Internals->SubProxyLinks.erase(iter2);
        iter2 = this->Internals->SubProxyLinks.begin();
        }
      else
        {
        iter2++;
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::ExecuteSubProxyEvent(vtkSMProxy* subproxy, 
  unsigned long event, void* data)
{
  if (subproxy && event == vtkCommand::PropertyModifiedEvent)
    {
    // A Subproxy has been modified.
    const char* name = reinterpret_cast<const char*>(data);
    const char* exposed_name = 0;
    if (name)
      {
      // Check if the property from the subproxy was exposed.
      // If so, we invoke this event with the exposed name.
      
      // First determine the name for this subproxy.
      vtkSMProxyInternals::ProxyMap::iterator proxy_iter =
        this->Internals->SubProxies.begin();
      const char* subproxy_name = 0;
      for (; proxy_iter != this->Internals->SubProxies.end(); ++proxy_iter)
        {
        if (proxy_iter->second.GetPointer() == subproxy)
          {
          subproxy_name = proxy_iter->first.c_str();
          break;
          }
        }
      if (subproxy_name)
        {
        // Now locate the exposed property name.
        vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
          this->Internals->ExposedProperties.begin();
        for (; iter != this->Internals->ExposedProperties.end(); ++iter)
          {
          if (iter->second.SubProxyName == subproxy_name &&
            iter->second.PropertyName == name)
            {
            // This property is indeed exposed. Set the corrrect exposed name.
            exposed_name = iter->first.c_str();
            break;
            }
          }
        }
      }
    // Let the world know that one of the subproxies of this proxy has 
    // been modified. If the subproxy exposed the modified property, we
    // provide the name of the property. Otherwise, 0, indicating
    // some internal property has changed.
    this->InvokeEvent(vtkCommand::PropertyModifiedEvent, (void*)exposed_name);
    }

  // Note we are not throwing vtkCommand::UpdateEvent fired by subproxies.
  // Since doing so would imply that this proxy (as well as all its subproxies)
  // are updated, which is not necessarily true when a subproxy fires
  // an UpdateEvent.
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddConsumer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  int found=0;
  vtkstd::vector<vtkSMProxyInternals::ConsumerInfo>::iterator i = 
    this->Internals->Consumers.begin();
  for(; i != this->Internals->Consumers.end(); i++)
    {
    if ( i->Property == property && i->Proxy == proxy )
      {
      found = 1;
      break;
      }
    }

  if (!found)
    {
    vtkSMProxyInternals::ConsumerInfo info(property, proxy);
    this->Internals->Consumers.push_back(info);
    }
  
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveConsumer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSMProxyInternals::ConsumerInfo>::iterator i = 
    this->Internals->Consumers.begin();
  for(; i != this->Internals->Consumers.end(); i++)
    {
    if ( i->Property == property && i->Proxy == proxy )
      {
      this->Internals->Consumers.erase(i);
      break;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveAllConsumers()
{
  this->Internals->Consumers.erase(this->Internals->Consumers.begin(),
                                   this->Internals->Consumers.end());
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfConsumers()
{
  return this->Internals->Consumers.size();
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetConsumerProxy(unsigned int idx)
{
  return this->Internals->Consumers[idx].Proxy;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetConsumerProperty(unsigned int idx)
{
  return this->Internals->Consumers[idx].Property;
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkModified(vtkSMProxy* modifiedProxy)
{
  /*
   * UpdatePropertyInformation() is now explicitly called in
   * UpdatePipelineInformation(). The calling on UpdatePropertyInformation()
   * was not really buying us much as far as keeping dependent domains updated
   * was concerned, for unless UpdatePipelineInformation was called on the
   * reader/filter, updating infor properties was not going to yeild any
   * changed values. Removing this also allows for linking for info properties
   * and properties using property links.
   * A side effect of this may be that the 3DWidgets information properties wont get
   * updated on setting "action" properties such as PlaceWidget.
  if (this->ObjectsCreated)
    {
    // If not created yet, don't worry syncing the info properties.
    this->UpdatePropertyInformation();
    }
  */
  this->MarkConsumersAsModified(modifiedProxy);
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkConsumersAsModified(vtkSMProxy* modifiedProxy)
{
  unsigned int numConsumers = this->GetNumberOfConsumers();
  for (unsigned int i=0; i<numConsumers; i++)
    {
    vtkSMProxy* cons = this->GetConsumerProxy(i);
    if (cons)
      {
      cons->MarkModified(modifiedProxy);
      }
    }
}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::NewProperty(const char* name)
{
  vtkSMProperty* property = this->GetProperty(name, 1);
  if (property)
    {
    return property;
    }
  vtkPVXMLElement* element = this->XMLElement;
  if (!element)
    {
    return 0;
    }

  vtkPVXMLElement* propElement = 0;
  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "SubProxy") != 0)
      {
      const char* pname = propElement->GetAttribute("name");
      if (pname && strcmp(name, pname) == 0)
        {
        break;
        }
      }
    propElement = 0;
    }
  if (!propElement)
    {
    return 0;
    }
  return this->NewProperty(name, propElement);

}

//----------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::NewProperty(const char* name, 
  vtkPVXMLElement* propElement)
{
  vtkSMProperty* property = this->GetProperty(name, 1);
  if (property)
    {
    return property;
    }
  
  if (!propElement)
    {
    return 0;
    }

  vtkObject* object = 0;
  ostrstream cname;
  cname << "vtkSM" << propElement->GetName() << ends;
  object = vtkInstantiator::CreateInstance(cname.str());
  delete[] cname.str();

  property = vtkSMProperty::SafeDownCast(object);
  if (property)
    {
    int old_val = this->DoNotUpdateImmediately;
    int old_val2 = this->DoNotModifyProperty;
    this->DoNotUpdateImmediately = 1;

    // Internal properties should not be created as modified.
    // Otherwise, properties like ForceUpdate get pushed and
    // cause problems.
    int is_internal;
    if (property->GetIsInternal())
      {
      this->DoNotModifyProperty = 1;
      }
    if (propElement->GetScalarAttribute("is_internal", &is_internal))
      {
      if (is_internal)
        {
        this->DoNotModifyProperty = 1;
        }
      }
    this->AddPropertyToSelf(name, property);
    if (!property->ReadXMLAttributes(this, propElement))
      {
      vtkErrorMacro("Could not parse property: " << propElement->GetName());
      this->DoNotUpdateImmediately = old_val;
      return 0;
      }
    this->DoNotUpdateImmediately = old_val;
    this->DoNotModifyProperty = old_val2;

    // Properties should be created as modified unless they
    // are internal.
//     if (!property->GetIsInternal())
//       {
//       this->Internals->Properties[name].ModifiedFlag = 1;
//       }
    property->Delete();
    }
  else
    {
    vtkErrorMacro("Could not instantiate property: " << propElement->GetName());
    }

  return property;
}

//---------------------------------------------------------------------------
int vtkSMProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  this->SetXMLElement(element);

  const char* className = element->GetAttribute("class");
  if(className)
    {
    this->SetVTKClassName(className);
    }

  const char* xmlname = element->GetAttribute("name");
  if(xmlname)
    {
    this->SetXMLName(xmlname);
    }
  if (!this->CreateProxyHierarchy(pm, element))
    {
    return 0;
    }

  // Locate documentation.
  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); ++cc)
    {
    vtkPVXMLElement* doc_elem = element->GetNestedElement(cc);
    if (strcmp(doc_elem->GetName(), "Documentation") == 0)
      {
      this->Documentation->SetDocumentationElement(doc_elem);
      break;
      }
    }
  this->SetXMLElement(0);
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxy::CreateProxyHierarchy(vtkSMProxyManager* pm,
  vtkPVXMLElement* element)
{
  const char* base_group = element->GetAttribute("base_proxygroup");
  const char* base_name = element->GetAttribute("base_proxyname");
  if (base_group && base_name)
    {
    // Obtain the interface from the base interface.
    vtkPVXMLElement* base_element = pm->GetProxyElement(base_group,
      base_name);
    if (!base_element || !this->CreateProxyHierarchy(pm, base_element))
      {
      vtkErrorMacro("Base interface cannot be found.");
      return 0;
      }
    }
  // Create all sub-proxies and properties
  if (!this->CreateSubProxiesAndProperties(pm, element))
    {
    return 0;
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxy::CreateSubProxiesAndProperties(vtkSMProxyManager* pm, 
  vtkPVXMLElement *element)
{
  if (!element || !pm)
    {
    return 0;
    }
  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "SubProxy")==0)
      {
      vtkPVXMLElement* subElement = propElement->GetNestedElement(0);
      if (subElement)
        {
        const char* name = subElement->GetAttribute("name");
        const char* pname = subElement->GetAttribute("proxyname");
        const char* gname = subElement->GetAttribute("proxygroup");
        if (pname && !gname)
          {
          vtkErrorMacro("proxygroup not specified. Subproxy cannot be created.");
          return 0;
          }
        if (gname && !pname)
          {
          vtkErrorMacro("proxyname not specified. Subproxy cannot be created.");
          return 0;
          }
        if (name)
          {
          vtkSMProxy* subproxy = 0;
          if (pname && gname)
            {
            subproxy = pm->NewProxy(gname, pname);
            }
          else
            {
            subproxy = pm->NewProxy(subElement, 0);
            }
          if (!subproxy)
            {
            vtkErrorMacro("Failed to create subproxy: " 
                          << (pname?pname:"(none"));
            return 0;
            }
          this->SetupSharedProperties(subproxy, propElement);
          this->SetupExposedProperties(name, propElement);
          this->AddSubProxy(name, subproxy);
          subproxy->Delete();
          }
        }
      }
    else
      {
      const char* name = propElement->GetAttribute("name");
      if (name)
        {
        this->NewProperty(name, propElement);
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetupExposedProperties(const char* subproxy_name, 
  vtkPVXMLElement *element)
{
  if (!subproxy_name || !element)
    {
    return;
    }

  for (unsigned int i=0; i < element->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement* exposedElement = element->GetNestedElement(i);
    if (strcmp(exposedElement->GetName(), "ExposedProperties")!=0)
      {
      continue;
      }
     for (unsigned int j=0; 
       j < exposedElement->GetNumberOfNestedElements(); j++)
       {
       vtkPVXMLElement* propertyElement = exposedElement->GetNestedElement(j);
       if (strcmp(propertyElement->GetName(), "Property") != 0)
         {
         vtkErrorMacro("<ExposedProperties> can contain <Property> elements alone.");
         continue;
         }
       const char* name = propertyElement->GetAttribute("name");
       if (!name || !name[0])
         {
         vtkErrorMacro("Attribute name is required!");
         continue;
         }
       const char* exposed_name = propertyElement->GetAttribute("exposed_name");
       if (!exposed_name)
         {
         // use the property name as the exposed name.
         exposed_name = name;
         }
       this->ExposeSubProxyProperty(subproxy_name, name, exposed_name);
       }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetupSharedProperties(vtkSMProxy* subproxy, 
  vtkPVXMLElement *element)
{
  if (!subproxy || !element)
    {
    return;
    }

  for (unsigned int i=0; i < element->GetNumberOfNestedElements(); i++)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "ShareProperties")==0)
      {
      const char* name = propElement->GetAttribute("subproxy");
      if (!name || !name[0])
        {
        continue;
        }
      vtkSMProxy* src_subproxy = this->GetSubProxy(name);
      if (!src_subproxy)
        {
        vtkErrorMacro("Subproxy " << name << " must be defined before "
          "its properties can be shared with another subproxy.");
        continue;
        }
      vtkSMProxyLink* sharingLink = vtkSMProxyLink::New();
      sharingLink->PropagateUpdateVTKObjectsOff();

      // Read the exceptions.
      for (unsigned int j=0; j < propElement->GetNumberOfNestedElements(); j++)
        {
        vtkPVXMLElement* exceptionProp = propElement->GetNestedElement(j);
        if (strcmp(exceptionProp->GetName(), "Exception") != 0)
          {
          continue;
          }
        const char* exp_name = exceptionProp->GetAttribute("name");
        if (!exp_name)
          {
          vtkErrorMacro("Exception tag must have the attribute 'name'.");
          continue;
          }
        sharingLink->AddException(exp_name);
        }
      sharingLink->AddLinkedProxy(src_subproxy, vtkSMLink::INPUT);
      sharingLink->AddLinkedProxy(subproxy, vtkSMLink::OUTPUT);
      this->Internals->SubProxyLinks.push_back(sharingLink);
      sharingLink->Delete();
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMProxy::LoadState(vtkPVXMLElement* proxyElement, 
                          vtkSMStateLoader* loader)
{
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Property") == 0)
      {
      const char* name = currentElement->GetAttribute("name");
      if (!name)
        {
        vtkErrorMacro("Cannon load property without a name.");
        continue;
        }
      vtkSMProperty* property = this->GetProperty(name);
      if (!property)
        {
        vtkErrorMacro("Property " << name << " does not exist.");
        continue;
        }
      if (!property->LoadState(currentElement, loader))
        {
        return 0;
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
  proxyElement->SetName("Proxy");
  proxyElement->AddAttribute("group", this->XMLGroup);
  proxyElement->AddAttribute("type", this->XMLName);
  proxyElement->AddAttribute("id", this->GetSelfIDAsString());

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  while (!iter->IsAtEnd())
    {
    if (!iter->GetProperty()->GetIsInternal())
      {
      ostrstream propID;
      propID << this->GetSelfIDAsString() << "." << iter->GetKey() << ends;
      iter->GetProperty()->SaveState(proxyElement, iter->GetKey(), propID.str());
      delete [] propID.str();
      }
    iter->Next();
    }

  iter->Delete();

  if (root)
    {
    root->AddNestedElement(proxyElement);
    proxyElement->Delete();
    }

  // Now save subproxies.
  this->SaveSubProxyIds(proxyElement);

  return proxyElement;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SaveSubProxyIds(vtkPVXMLElement* root)
{
  vtkSMProxyInternals::ProxyMap::iterator iter =
    this->Internals->SubProxies.begin();
  for (; iter != this->Internals->SubProxies.end(); ++iter)
    {
    vtkPVXMLElement* subproxyElement = vtkPVXMLElement::New();
    subproxyElement->SetName("SubProxy");
    subproxyElement->AddAttribute("name", iter->first.c_str());
    subproxyElement->AddAttribute("id", iter->second.GetPointer()->GetSelfIDAsString());
    iter->second.GetPointer()->SaveSubProxyIds(subproxyElement);
    root->AddNestedElement(subproxyElement);
    subproxyElement->Delete();
    }
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator* vtkSMProxy::NewPropertyIterator()
{
  vtkSMPropertyIterator* iter = vtkSMPropertyIterator::New();
  iter->SetProxy(this);
  return iter;
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src)
{
  this->Copy(src, 0, 
    vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE);
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src, const char* exceptionClass)
{
  this->Copy(src, exceptionClass,
    vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE);
}

//---------------------------------------------------------------------------
void vtkSMProxy::Copy(vtkSMProxy* src, const char* exceptionClass,
  int proxyPropertyCopyFlag)
{
  if (!src)
    {
    return;
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    src->Internals->SubProxies.begin();
  for( ; it2 != src->Internals->SubProxies.end(); it2++)
    {
    vtkSMProxy* sub = this->GetSubProxy(it2->first.c_str());
    if (sub)
      {
      sub->Copy(it2->second, exceptionClass, proxyPropertyCopyFlag); 
      }
    }

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  iter->SetTraverseSubProxies(0);
  for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    const char* key = iter->GetKey();
    vtkSMProperty* dest = iter->GetProperty();
    if (key && dest)
      {
      vtkSMProperty* source = src->GetProperty(key);
      if (source)
        {
        if (!exceptionClass || !dest->IsA(exceptionClass))
          {
          vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(dest);
          if (!pp || proxyPropertyCopyFlag == 
            vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_REFERENCE)
            {
            dest->Copy(source);
            }
          else
            {
            pp->DeepCopy(source, exceptionClass, 
              vtkSMProxy::COPY_PROXY_PROPERTY_VALUES_BY_CLONING);
            }
          }
        }
      }
    }

  iter->Delete();
}

//---------------------------------------------------------------------------
void vtkSMProxy::ExposeSubProxyProperty(const char* subproxy_name, 
  const char* property_name, const char* exposed_name)
{
  if (!subproxy_name || !property_name || !exposed_name)
    {
    vtkErrorMacro("Either subproxy name, property name, or exposed name is NULL.");
    return;
    }

  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
    this->Internals->ExposedProperties.find(exposed_name);
  if (iter != this->Internals->ExposedProperties.end())
    {
    vtkWarningMacro("An exposed property with the name \"" << exposed_name
      << "\" already exists. It will be replaced.");
    }
  
  vtkSMProxyInternals::ExposedPropertyInfo info;
  info.SubProxyName = subproxy_name;
  info.PropertyName = property_name;
  this->Internals->ExposedProperties[exposed_name] = info;
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
 
  os << indent << "Name: " 
     << (this->Name ? this->Name : "(null)")
     << endl;
  os << indent << "VTKClassName: " 
     << (this->VTKClassName ? this->VTKClassName : "(null)")
     << endl;
  os << indent << "XMLName: "
     << (this->XMLName ? this->XMLName : "(null)")
     << endl;
  os << indent << "XMLGroup: " 
    << (this->XMLGroup ? this->XMLGroup : "(null)")
    << endl;
  os << indent << "Documentation: " << this->Documentation << endl;

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();
  if (iter)
    {
    for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      const char* key = iter->GetKey();
      vtkSMProperty* property = iter->GetProperty();
      if (key)
        {
        os << indent << "Property (" << key << "): ";
        if (property)
          {
          os << endl;
          property->PrintSelf(os, indent.GetNextIndent());
          }
        else
          {
          os << "(none)" << endl; 
          }
        }
      }
    iter->Delete();
    }
}
