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
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyManager.h"

#include "vtkSMProxyInternals.h"

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtkstd/string>

vtkStandardNewMacro(vtkSMProxy);
vtkCxxRevisionMacro(vtkSMProxy, "1.33.2.1");

vtkCxxSetObjectMacro(vtkSMProxy, XMLElement, vtkPVXMLElement);

//---------------------------------------------------------------------------
// Observer for modified event of the property
class vtkSMProxyObserver : public vtkCommand
{
public:
  static vtkSMProxyObserver *New() 
    { return new vtkSMProxyObserver; }

  virtual void Execute(vtkObject*, unsigned long, void*)
    {
      this->Proxy->SetPropertyModifiedFlag(this->PropertyName, 1);
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
  this->VTKClassName = 0;
  this->XMLGroup = 0;
  this->XMLName = 0;
  this->ObjectsCreated = 0;

  vtkClientServerID nullID = { 0 };
  this->SelfID = nullID;

  this->XMLElement = 0;
  this->DoNotModifyProperty = 0;

  // Assign a unique clientserver id to this object.
  // Note that this ups the reference count to 2.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    vtkErrorMacro("Can not fully initialize without a global "
                  "ProcessModule. This object will not be fully "
                  "functional.");
    return;
    }
  this->SelfID = pm->GetUniqueID();
  vtkClientServerStream initStream;
  initStream << vtkClientServerStream::Assign 
             << this->SelfID << this
             << vtkClientServerStream::End;
  pm->SendStream(vtkProcessModule::CLIENT, initStream);
  // This is done to make the last result message release it's reference 
  // count. Otherwise the object has a reference count of 3.
  if (pm)
    {
    pm->GetInterpreter()->ClearLastResult();
    }
  this->InUpdateVTKObjects = 0;
}

//---------------------------------------------------------------------------
vtkSMProxy::~vtkSMProxy()
{
  if (this->ObjectsCreated)
    {
    this->UnRegisterVTKObjects();
    }
  this->RemoveAllObservers();

  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.begin();
  // To remove cyclic dependancy
  for(; it != this->Internals->Properties.end(); it++)
    {
    it->second.Property.GetPointer()->RemoveAllDependents();
    }
  delete this->Internals;
  this->SetVTKClassName(0);
  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetXMLElement(0);
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
  pm->SendStream(this->Servers, stream);

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
        pm->SendStream(vtkProcessModule::CLIENT, deleteStream);
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
vtkSMProperty* vtkSMProxy::GetProperty(const char* name, int selfOnly)
{
  if (!name)
    {
    return 0;
    }
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
    if (!selfOnly)
      {
      vtkSMProxyInternals::ProxyMap::iterator it2 =
        this->Internals->SubProxies.begin();
      for( ; it2 != this->Internals->SubProxies.end(); it2++)
        {
        vtkSMProperty* prop = it2->second.GetPointer()->GetProperty(name);
        if (prop)
          {
          return prop;
          }
        }
      }
    return 0;
    }
  else
    {
    return it->second.Property.GetPointer();
    }
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
// Push the property to one object on one server.  This is usually
// used to make the property push it's value to an object other than
// the one managed by the proxy. Most of the time, this is the proxy
// itself. This provides a way to make the property call a method
// on the proxy instead of the object managed by the proxy.
void vtkSMProxy::PushProperty(
  const char* name, vtkClientServerID id, vtkTypeUInt32 servers)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
    return;
    }
  if (it->second.ModifiedFlag)
    {
    vtkClientServerStream str;
    it->second.Property.GetPointer()->AppendCommandToStream(this, &str, id);
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream(servers, str);
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
  it->second.ModifiedFlag = flag;

  vtkSMProperty* prop = it->second.Property.GetPointer();
  if (flag && prop->GetImmediateUpdate())
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
    if (prop->GetUpdateSelf())
      {
      this->PushProperty(it->first.c_str(), 
                         this->SelfID, 
                         vtkProcessModule::CLIENT);
      }
    else
      {
      int numObjects = this->Internals->IDs.size();
      
      vtkClientServerStream str;
      
      for (int i=0; i<numObjects; i++)
        {
        prop->AppendCommandToStream(this, &str, this->Internals->IDs[i]);
        }
      
      if (str.GetNumberOfMessages() > 0)
        {
        vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
        pm->SendStream(this->Servers, str);
        }
      }
    it->second.ModifiedFlag = 0;
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateInformation()
{
  this->CreateVTKObjects(1);
  int numObjects = this->Internals->IDs.size();

  if (numObjects > 0)
    {
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
          prop->UpdateInformation(vtkProcessModule::CLIENT, this->SelfID);
          }
        else
          {
          prop->UpdateInformation(this->Servers, this->Internals->IDs[0]);
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
      it2->second.GetPointer()->UpdateInformation();
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMProxy::ArePropertiesModified(int selfOnly /*=0*/)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it  = this->Internals->Properties.begin();
    it != this->Internals->Properties.end();
    ++it)
    {
    if (it->second.ModifiedFlag)
      {
      return 1;
      }
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
  vtkClientServerStream str;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  pm->SendPrepareProgress();

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
  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (prop->IsA("vtkSMInputProperty"))
      {
      if (it->second.ModifiedFlag && !prop->GetImmediateUpdate())
        {
        if (prop->GetUpdateSelf())
          {
          this->PushProperty(it->first.c_str(), 
                             this->SelfID, 
                             vtkProcessModule::CLIENT);
          }
        }
      it->second.ModifiedFlag = 0;
      }
    }

  this->CreateVTKObjects(1);
  int numObjects = this->Internals->IDs.size();

  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (it->second.ModifiedFlag && 
        !prop->GetImmediateUpdate() && 
        !prop->GetInformationOnly())
      {
      if (prop->GetUpdateSelf())
        {
        this->PushProperty(it->first.c_str(), 
                           this->SelfID, 
                           vtkProcessModule::CLIENT);
        }
      else
        {
        for (int i=0; i<numObjects; i++)
          {
          prop->AppendCommandToStream(this, &str, this->Internals->IDs[i]);
          }
        }
      }
    it->second.ModifiedFlag = 0;
    }
  if (str.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, str);
    }
  pm->SendCleanupPendingProgress();

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdateVTKObjects();
    }

  // I am am removing this because it was causing the animation to
  // loose its caches when it Updated the VTK objects.
  // It has to be called separately now.
  //this->MarkConsumersAsModified();

  this->InUpdateVTKObjects = 0;
  // If any properties got modified while pushing them,
  // we need to call UpdateVTKObjects again.
  if (this->ArePropertiesModified())
    {
    this->UpdateVTKObjects();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateSelfAndAllInputs()
{
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  vtkProcessModule::GetProcessModule()->SendPrepareProgress();
  while (!iter->IsAtEnd())
    {
    iter->GetProperty()->UpdateAllInputs();
    iter->Next();
    }
  iter->Delete();
  vtkProcessModule::GetProcessModule()->SendCleanupPendingProgress();

  this->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->ObjectsCreated = 1;

  if (this->VTKClassName)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    for (int i=0; i<numObjects; i++)
      {
      vtkClientServerID objectId = pm->NewStreamObject(this->VTKClassName, stream);

      this->Internals->IDs.push_back(objectId);

      stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
        << "RegisterProgressEvent"
        << objectId << static_cast<int>(objectId.ID)
        << vtkClientServerStream::End;
      }
    if (stream.GetNumberOfMessages() > 0)
      {
      pm->SendStream(this->Servers, stream);
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
    }

  this->Internals->SubProxies[name] = proxy;
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveSubProxy(const char* name)
{
  vtkSMProxyInternals::ProxyMap::iterator it =
    this->Internals->SubProxies.find(name);

  if (it != this->Internals->SubProxies.end())
    {
    this->Internals->SubProxies.erase(it);
    }
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
void vtkSMProxy::MarkConsumersAsModified()
{
  unsigned int numConsumers = this->GetNumberOfConsumers();
  for (unsigned int i=0; i<numConsumers; i++)
    {
    vtkSMProxy* cons = this->GetConsumerProxy(i);
    if (cons)
      {
      cons->MarkConsumersAsModified();
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
    this->DoNotModifyProperty = 1;
    this->AddProperty(name, property);
    if (!property->ReadXMLAttributes(this, propElement))
      {
      vtkErrorMacro("Could not parse property: " << propElement->GetName());
      this->DoNotModifyProperty = 0;
      return 0;
      }
    this->DoNotModifyProperty = 0;
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
            vtkErrorMacro("Failed to create subproxy.");
            return 0;
            }
          this->SetupSharedProperties(subproxy, propElement);
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
      // Read the exceptions.
      vtkstd::set<vtkstd::string> exceptions;
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
        exceptions.insert(exceptions.end(), vtkstd::string(exp_name));
        }
      // Iterate over src_subproxy properties and
      // replace the subproxy properties except those in exceptions.
      vtkSMPropertyIterator* piter = vtkSMPropertyIterator::New();
      piter->SetProxy(src_subproxy);
      for (piter->Begin(); !piter->IsAtEnd(); piter->Next())
        {
        const char* pname = piter->GetKey();
        if (exceptions.find(vtkstd::string(pname)) !=
          exceptions.end())
          {
          continue;
          }
        // Check is subproxy has this property. If not, the property
        // cannot be shared. For a property to be shared, it must be defined 
        // on both proxies.
        if (!subproxy->GetProperty(pname))
          {
          continue;
          }
        subproxy->RemoveProperty(pname);
        subproxy->AddProperty(pname, piter->GetProperty());
        }
      piter->Delete();
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SaveState(const char* name, ostream* file, vtkIndent indent)
{
  *file << indent
        << "<Proxy group=\"" 
        << this->XMLGroup << "\" type=\"" 
        << this->XMLName << "\" id=\""
        << name << "\">" << endl;

  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  while (!iter->IsAtEnd())
    {
    ostrstream propID;
    propID << name << "." << iter->GetKey() << ends;
    iter->GetProperty()->SaveState(propID.str(), file, indent.GetNextIndent());
    delete [] propID.str();
    iter->Next();
    }
  *file << indent << "</Proxy>" << endl;

  iter->Delete();
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator* vtkSMProxy::NewPropertyIterator()
{
  vtkSMPropertyIterator* iter = vtkSMPropertyIterator::New();
  iter->SetProxy(this);
  return iter;
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "VTKClassName: " 
     << (this->VTKClassName ? this->VTKClassName : "(null)")
     << endl;
  os << indent << "XMLName: "
     << (this->XMLName ? this->XMLName : "(null)")
     << endl;
}
