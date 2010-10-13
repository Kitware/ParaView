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
#include "vtkGarbageCollector.h"
#include "vtkInstantiator.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModuleConnectionManager.h"
#include "vtkProcessModule.h"
#include "vtkPVOptions.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDocumentation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

#include "vtkSMProxyInternals.h"

#include <vtkstd/algorithm>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMProxy);

vtkCxxSetObjectMacro(vtkSMProxy, XMLElement, vtkPVXMLElement);
vtkCxxSetObjectMacro(vtkSMProxy, Hints, vtkPVXMLElement);
vtkCxxSetObjectMacro(vtkSMProxy, Deprecated, vtkPVXMLElement);

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
  this->XMLLabel = 0;
  this->ObjectsCreated = 0;

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
  this->InMarkModified = 0;

  this->NeedsUpdate = true;
  
  this->Hints = 0;
  this->Deprecated = 0;
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
    prop->SetParent(0);
    }
  delete this->Internals;
  this->SetVTKClassName(0);
  this->SetXMLGroup(0);
  this->SetXMLName(0);
  this->SetXMLLabel(0);
  this->SetXMLElement(0);

  if (this->SubProxyObserver)
    {
    this->SubProxyObserver->SetProxy(0);
    this->SubProxyObserver->Delete();
    }
  this->Documentation->Delete();
  this->SetHints(0);
  this->SetDeprecated(0);
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
  pm->ReserveID(this->SelfID);

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
    vtksys_ios::ostringstream str;
    str << this->SelfID << ends;
    this->SetName(str.str().c_str());
    }
}

//---------------------------------------------------------------------------
vtkClientServerID vtkSMProxy::GetSelfID()
{
  if (this->SelfID.ID == 0)
    {
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
    }
  
  return this->GetSelfIDInternal();
}

//---------------------------------------------------------------------------
void vtkSMProxy::UnRegisterVTKObjects()
{
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  if (!pm)
    {
    return;
    }

  if (!this->VTKObjectID.IsNull())
    {
    vtkClientServerStream stream;
    pm->DeleteStreamObject(this->VTKObjectID, stream);
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }

  this->ObjectsCreated = 0;
}

//-----------------------------------------------------------------------------
void vtkSMProxy::Register(vtkObjectBase* obj)
{
//  this->RegisterInternal(obj, 1);
  this->Superclass::Register(obj);
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
//this->UnRegisterInternal(obj, 1);
 this->Superclass::UnRegister(obj);
}


//---------------------------------------------------------------------------
void vtkSMProxy::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);

  /*
  // Report references held by this object that may be in a loop.
  // This includes the consumers.
  vtkstd::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = 
    this->Internals->Consumers.begin();
  for(; i != this->Internals->Consumers.end(); i++)
    {
    if (i->Proxy)
      {
      vtkGarbageCollectorReport(collector, i->Proxy, "Consumer");
      }
    }
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for(; it2 != this->Internals->SubProxies.end(); it2++)
    {
    vtkGarbageCollectorReport(collector, it2->second, "SubProxy");
    }
    */
}


//---------------------------------------------------------------------------
void vtkSMProxy::SetServers(vtkTypeUInt32 servers)
{
  this->SetServersSelf(servers);

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->SetServers(servers);
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
  if (this->ConnectionID == id)
    {
    return;
    }
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
vtkClientServerID vtkSMProxy::GetID()
{
  this->CreateVTKObjects();
  return this->VTKObjectID;
}

//---------------------------------------------------------------------------
vtkObjectBase* vtkSMProxy::GetClientSideObject()
{
  return vtkProcessModule::GetProcessModule()->GetObjectFromID(
    this->GetID(), true); // the second argument means that no error
                          // will be printed if the ID is invalid.
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
      break;
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
  if (!name)
    {
    return;
    }

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
    it->second.Property->SetParent(0);
    this->Internals->Properties.erase(it);
    }

  vtkstd::vector<vtkStdString>::iterator iter =
    vtkstd::find(this->Internals->PropertyNamesInOrder.begin(),
                 this->Internals->PropertyNamesInOrder.end(),
                 name);
  if(iter != this->Internals->PropertyNamesInOrder.end())
    {
    this->Internals->PropertyNamesInOrder.erase(iter);
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
    oldProp->SetParent(0);
    }

  unsigned int tag=0;

  vtkSMProxyObserver* obs = vtkSMProxyObserver::New();
  obs->SetProxy(this);
  obs->SetPropertyName(name);
  // We have to store the tag in order to be able to remove
  // the observer later.
  tag = prop->AddObserver(vtkCommand::ModifiedEvent, obs);
  obs->Delete();

  prop->SetParent(this);

  vtkSMProxyInternals::PropertyInfo newEntry;
  newEntry.Property = prop;
  newEntry.ObserverTag = tag;
  this->Internals->Properties[name] = newEntry;

  // Add the property name to the vector of property names.
  // This vector keeps track of the order in which properties
  // were added.
  this->Internals->PropertyNamesInOrder.push_back(name);
}

//---------------------------------------------------------------------------
bool vtkSMProxy::UpdatePropertyInternal(const char* name, bool force,
  vtkClientServerStream& stream)
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
      return false;
      }
    const char* subproxy_name =  eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy * sp = this->GetSubProxy(subproxy_name);
    if (sp && sp->UpdatePropertyInternal(property_name, force, stream))
      {
      this->MarkModified(this);
      return true;
      }
    return false;
    }

  if (!it->second.ModifiedFlag && !force)
    {
    return false;
    }

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
    if (str.GetNumberOfMessages() > 0)
      {
      pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, str);

      // Fire event to let everyone know that a property has been updated.
      this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)name);
      if (!this->InUpdateVTKObjects) // if updating multiple properties
        {                            // MarkModified() is called only once.
        this->MarkModified(this);
        }
      return true;
      }
    }
  else
    {
    if (this->VTKObjectID.IsNull())
      {
      // Make sure that server side objects exist before
      // pushing values to them
      this->CreateVTKObjects();
      }
    if (!this->VTKObjectID.IsNull())
      {
      int old_count = stream.GetNumberOfMessages();
      prop->AppendCommandToStream(this, &stream, this->VTKObjectID);
      if (stream.GetNumberOfMessages() > old_count)
        {
        //pm->SendStream(this->ConnectionID, this->Servers, str);

        // Fire event to let everyone know that a property has been updated.
        this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)name);
        if (!this->InUpdateVTKObjects) // if updating multiple properties
          {                            // MarkModified() is called only once.
          this->MarkModified(this);
          }
        return true;  
        }
      }
    }

  return false;
}

//---------------------------------------------------------------------------
bool vtkSMProxy::UpdateProperty(const char* name, int force)
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
      return false;
      }
    const char* subproxy_name =  eiter->second.SubProxyName.c_str();
    const char* property_name = eiter->second.PropertyName.c_str();
    vtkSMProxy * sp = this->GetSubProxy(subproxy_name);
    if (sp && sp->UpdateProperty(property_name, force))
      {
      this->MarkModified(this);
      return true;
      }
    return false;
    }
    
  if (it->second.ModifiedFlag || force)
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
      if (str.GetNumberOfMessages() > 0)
        {
        pm->SendStream(this->ConnectionID, vtkProcessModule::CLIENT, str);
  
        // Fire event to let everyone know that a property has been updated.
        this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)name);
        if (!this->InUpdateVTKObjects) // if updating multiple properties
          {                            // MarkModified() is called only once.
          this->MarkModified(this);
          }
        return true;
        }
      }
    else
      {
      if (this->VTKObjectID.IsNull())
        {
        // Make sure that server side objects exist before
        // pushing values to them
        this->CreateVTKObjects();
        }
      if (!this->VTKObjectID.IsNull())
        {
        vtkClientServerStream str;
        prop->AppendCommandToStream(this, &str, this->VTKObjectID);
        if (str.GetNumberOfMessages() > 0)
          {
          pm->SendStream(this->ConnectionID, this->Servers, str);

          // Fire event to let everyone know that a property has been updated.
          this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)name);
          if (!this->InUpdateVTKObjects) // if updating multiple properties
            {                            // MarkModified() is called only once.
            this->MarkModified(this);
            }
          return true;  
          }
        }
      }
    }
    
    return false;
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
      this->CreateVTKObjects();
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
    // Check if the property is an exposed property
    const char *exposed_name = this->GetPropertyName(prop);
    if (exposed_name)
      {
      vtkSMProxyInternals::ExposedPropertyInfoMap::iterator eiter = 
      this->Internals->ExposedProperties.find(exposed_name);
      if (eiter != this->Internals->ExposedProperties.end())
        {
        const char* subproxy_name =  eiter->second.SubProxyName.c_str();
        const char* property_name = eiter->second.PropertyName.c_str();
        vtkSMProxy * sp = this->GetSubProxy(subproxy_name);
        if (sp)
          {
          sp->UpdatePropertyInformation(sp->GetProperty(property_name, 0));
          }
        }
      }

    return;
    }

  this->CreateVTKObjects();
  this->UpdatePropertyInformationInternal(prop);
  prop->UpdateDependentDomains();
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdatePropertyInformation()
{
  this->CreateVTKObjects();

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
    this->UpdatePropertyInformationInternal(prop);
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
void vtkSMProxy::UpdatePropertyInformationInternal(vtkSMProperty* prop)
{
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
          this->Servers, this->VTKObjectID);
        }
      }
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
  vtkClientServerStream stream;
  this->UpdateVTKObjects(stream);
  if (stream.GetNumberOfMessages() > 0)
    {
    //cout << "Message Count: " << stream.GetNumberOfMessages() << endl;
    vtkProcessModule::GetProcessModule()->SendStream(
      this->ConnectionID,
      this->Servers,
      stream);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateVTKObjects(vtkClientServerStream& buffer)
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
      if (prop->IsA("vtkSMProxyProperty"))
        {
        if (vtkSMProxyManager::GetProxyManager()->GetUpdateInputProxies())
          {
          // If proxy manager says that input proxies must be updated
          // before updating ourselves, we must update all
          // input proxies. Generally speaking, we never want to internally
          // call UpdateVTKObjects on input proxy. However, while loading
          // state or creating a compound proxy, we must update the
          // proxies in order, otherwise the connections may not
          // be set up correctly resulting in errors. Hence we have
          // this mechanism that uses the "UpdateInputProxies"
          // flag on the proxy manager.
          vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
            it->second.Property);
          if (pp)
            {
            for (unsigned int cc=0; cc < pp->GetNumberOfProxies(); ++cc)
              {
              vtkSMProxy* inputProxy= pp->GetProxy(cc);
              if (inputProxy)
                {
                inputProxy->UpdateVTKObjects();
                }
              }
            }
          }
        }
      if (prop->IsA("vtkSMInputProperty"))
        {
        this->UpdateProperty(it->first.c_str());
        }
      }
    }

  this->CreateVTKObjects();
  if (!this->ObjectsCreated)
    {
    this->InUpdateVTKObjects = 0;
    return;
    }

  bool modified = false;
  if (old_SelfPropertiesModified)
    {
    for (it  = this->Internals->Properties.begin();
         it != this->Internals->Properties.end();
         ++it)
      {
      vtkSMProperty* prop = it->second.Property.GetPointer();
      if (!prop->GetInformationOnly())
        {
        this->UpdatePropertyInternal(it->first.c_str(), false, buffer);
        }
      }
    modified = true;
    }

  this->InUpdateVTKObjects = 0;
  
  // If any properties got modified while pushing them,
  // we need to call UpdateVTKObjects again.
  // It is perfectly fine to check the SelfPropertiesModified flag before we 
  // even UpdateVTKObjects on all subproxies, since there is no way that a
  // subproxy can ever set any property on the parent proxy.
  if (this->ArePropertiesModified(1))
    {
    this->UpdateVTKObjects(buffer);
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    modified = modified || it2->second.GetPointer()->ArePropertiesModified();
    if (it2->second.GetPointer()->Servers == this->Servers)
      {
      it2->second.GetPointer()->UpdateVTKObjects(buffer);
      }
    else
      {
      it2->second.GetPointer()->UpdateVTKObjects();
      }
    }

  if (modified)
    {
    this->MarkModified(this);
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
void vtkSMProxy::InitializeWithID(vtkClientServerID id)
{
  if (this->ObjectsCreated || id.IsNull())
    {
    return;
    }
  this->ObjectsCreated = 1;
  this->GetSelfID(); // this will ensure that the SelfID is assigned properly.
  this->VTKObjectID = id;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  stream << vtkClientServerStream::Invoke 
         << pm->GetProcessModuleID()
         << "RegisterProgressEvent" 
         << this->VTKObjectID
         << static_cast<int>(this->VTKObjectID.ID)
         << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->CreateVTKObjects();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::InitializeAndCopyFromProxy(vtkSMProxy* fromP)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  fromP->CreateVTKObjects();
  this->SetConnectionID(fromP->GetConnectionID());
  this->SetServers(fromP->GetServers());

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID newid = pm->GetUniqueID();
  stream  << vtkClientServerStream::Assign
          << newid
          << fromP->GetID()
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  this->InitializeWithID(newid);
}


//---------------------------------------------------------------------------
void vtkSMProxy::InitializeAndCopyFromID(vtkClientServerID id)
{
  if (this->ObjectsCreated)
    {
    return;
    }

  vtkClientServerStream stream;
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerID newid = pm->GetUniqueID();
  stream  << vtkClientServerStream::Assign
          << newid
          << id
          << vtkClientServerStream::End;
  pm->SendStream(this->ConnectionID, this->Servers, stream);
  this->InitializeWithID(newid);
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->ObjectsCreated = 1;
  this->GetSelfID(); // this will ensure that the SelfID is assigned properly.
  this->WarnIfDeprecated();

  if (this->VTKClassName && this->VTKClassName[0] != '\0')
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    vtkClientServerStream stream;
    this->VTKObjectID =
      pm->NewStreamObject(this->VTKClassName, stream);
    stream << vtkClientServerStream::Invoke 
           << pm->GetProcessModuleID()
           << "RegisterProgressEvent" 
           << this->VTKObjectID
           << static_cast<int>(this->VTKObjectID.ID)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->CreateVTKObjects();
    }
}

//---------------------------------------------------------------------------
bool vtkSMProxy::WarnIfDeprecated()
{
  if (this->Deprecated)
    {
    vtkWarningMacro("Proxy (" << this->XMLGroup << ", " << this->XMLName 
      << ")  has been deprecated in ParaView " <<
      this->Deprecated->GetAttribute("deprecated_in") << 
      " and will be removed by ParaView " <<
      this->Deprecated->GetAttribute("to_remove_in") << ". " <<
      (this->Deprecated->GetCharacterData()? 
       this->Deprecated->GetCharacterData() : ""));
    return true;
    }
  return false;
}

//---------------------------------------------------------------------------
void vtkSMProxy::ReviveVTKObjects()
{
  if (this->ObjectsCreated)
    {
    vtkErrorMacro(
      "Cannot revive VTK objects, they have already been created.");
    return;
    }

  this->ObjectsCreated = 1;
  this->GetSelfID();

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();

  vtkPVOptions* options = pm->GetOptions();
  if (options->GetServerMode() || options->GetRenderServerMode())
    {
    // This server manager is running on the server. 
    // Reviving such VTK objects means creating the objects only
    // when the Servers == CLIENT only.
    if (this->Servers != vtkProcessModule::CLIENT &&
        !this->VTKObjectID.IsNull())
      {
      // We still need to update the PM to make sure that the ID doesn't get
      // accidently reused.
      pm->ReserveID(this->VTKObjectID);
      return;
      }
    }

  if (this->VTKClassName && this->VTKClassName[0] != '\0')
    {
    if (this->VTKObjectID.IsNull())
      {
      vtkErrorMacro("No ID set to revive.");
      return;
      }
    vtkClientServerStream stream;
    pm->NewStreamObject(this->VTKClassName, stream, this->VTKObjectID);

    stream << vtkClientServerStream::Invoke << pm->GetProcessModuleID()
           << "RegisterProgressEvent"
           << this->VTKObjectID 
           << static_cast<int>(this->VTKObjectID.ID)
           << vtkClientServerStream::End;
    pm->SendStream(this->ConnectionID, this->Servers, stream);
    }

  // * No need to iterate over subproxies, since a call to LoadRevivalState() on
  // * subproxies will call ReviveVTKObjects() on the subproxies as well. (Look at
  // * implementation of LoadRevivalState().
  //vtkSMProxyInternals::ProxyMap::iterator it2 =
  //  this->Internals->SubProxies.begin();
  //for( ; it2 != this->Internals->SubProxies.end(); it2++)
  //  {
  //  it2->second.GetPointer()->ReviveVTKObjects();
  //  }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfSubProxies()
{
  return static_cast<unsigned int>(this->Internals->SubProxies.size());
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
const char* vtkSMProxy::GetSubProxyName(vtkSMProxy* proxy)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for(;it2 != this->Internals->SubProxies.end(); it2++)
    {
    if (it2->second.GetPointer() == proxy)
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
void vtkSMProxy::AddSubProxy(const char* name, vtkSMProxy* proxy, 
  int override)
{
  // Check if the proxy already exists. If it does, we will
  // replace it
  vtkSMProxyInternals::ProxyMap::iterator it =
    this->Internals->SubProxies.find(name);

  if (it != this->Internals->SubProxies.end())
    {
    if (!override)
      {
      vtkWarningMacro("Proxy " << name  << " already exists. Replacing");
      }
    // needed to remove any observers.
    this->RemoveSubProxy(name);
    }

  this->Internals->SubProxies[name] = proxy;
  
  proxy->AddObserver(vtkCommand::PropertyModifiedEvent, 
    this->SubProxyObserver);
  proxy->AddObserver(vtkCommand::UpdatePropertyEvent, this->SubProxyObserver);
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
  if (subproxy && 
    (event == vtkCommand::PropertyModifiedEvent ||
     event == vtkCommand::UpdatePropertyEvent))
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

    if (event == vtkCommand::PropertyModifiedEvent)
      {
      // Let the world know that one of the subproxies of this proxy has 
      // been modified. If the subproxy exposed the modified property, we
      // provide the name of the property. Otherwise, 0, indicating
      // some internal property has changed.
      this->InvokeEvent(vtkCommand::PropertyModifiedEvent, (void*)exposed_name);
      }
    else if (exposed_name && event == vtkCommand::UpdatePropertyEvent)
      {
      // UpdatePropertyEvent is fired only for exposed properties.
      this->InvokeEvent(vtkCommand::UpdatePropertyEvent, (void*)exposed_name);
      this->MarkModified(this);
      }
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
  vtkstd::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = 
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
    vtkSMProxyInternals::ConnectionInfo info(property, proxy);
    this->Internals->Consumers.push_back(info);
    }
  
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveConsumer(vtkSMProperty* property, vtkSMProxy*)
{
  vtkstd::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = 
    this->Internals->Consumers.begin();
  for(; i != this->Internals->Consumers.end(); i++)
    {
    if ( i->Property == property )
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
  return static_cast<unsigned int>(this->Internals->Consumers.size());
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

//---------------------------------------------------------------------------
void vtkSMProxy::AddProducer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  int found=0;
  vtkstd::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = 
    this->Internals->Producers.begin();
  for(; i != this->Internals->Producers.end(); i++)
    {
    if ( i->Property == property && i->Proxy == proxy )
      {
      found = 1;
      break;
      }
    }

  if (!found)
    {
    vtkSMProxyInternals::ConnectionInfo info(property, proxy);
    this->Internals->Producers.push_back(info);
    }
  
}

//---------------------------------------------------------------------------
void vtkSMProxy::RemoveProducer(vtkSMProperty* property, vtkSMProxy* proxy)
{
  vtkstd::vector<vtkSMProxyInternals::ConnectionInfo>::iterator i = 
    this->Internals->Producers.begin();
  for(; i != this->Internals->Producers.end(); i++)
    {
    if ( i->Property == property && i->Proxy == proxy )
      {
      this->Internals->Producers.erase(i);
      break;
      }
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMProxy::GetNumberOfProducers()
{
  return static_cast<unsigned int>(this->Internals->Producers.size());
}

//---------------------------------------------------------------------------
vtkSMProxy* vtkSMProxy::GetProducerProxy(unsigned int idx)
{
  return this->Internals->Producers[idx].Proxy;
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMProxy::GetProducerProperty(unsigned int idx)
{
  return this->Internals->Producers[idx].Property;
}

//----------------------------------------------------------------------------
void vtkSMProxy::PostUpdateData()
{
  unsigned int numProducers = this->GetNumberOfProducers();
  for (unsigned int i=0; i<numProducers; i++)
    {
    if (this->GetProducerProxy(i)->NeedsUpdate)
      {
      this->GetProducerProxy(i)->PostUpdateData();
      }
    }
  if (this->NeedsUpdate)
    {
    this->InvokeEvent(vtkCommand::UpdateDataEvent, 0);
    this->NeedsUpdate = false;
    }
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
  if (!this->InMarkModified)
    {
    this->InMarkModified = 1;
    this->InvokeEvent(vtkCommand::ModifiedEvent, (void*)modifiedProxy);
    this->MarkDirty(modifiedProxy);
    this->InMarkModified = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkDirty(vtkSMProxy* modifiedProxy)
{
  if (this->NeedsUpdate)
    {
    return;
    }
    
  this->MarkConsumersAsDirty(modifiedProxy);
  this->NeedsUpdate = true;
}

//----------------------------------------------------------------------------
void vtkSMProxy::MarkConsumersAsDirty(vtkSMProxy* modifiedProxy)
{
  unsigned int numConsumers = this->GetNumberOfConsumers();
  for (unsigned int i=0; i<numConsumers; i++)
    {
    vtkSMProxy* cons = this->GetConsumerProxy(i);
    if (cons)
      {
      cons->MarkDirty(modifiedProxy);
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
  vtksys_ios::ostringstream cname;
  cname << "vtkSM" << propElement->GetName() << ends;
  object = vtkInstantiator::CreateInstance(cname.str().c_str());

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
void vtkSMProxy::ReadCoreXMLAttributes(vtkPVXMLElement* element)
{
  const char* className = element->GetAttribute("class");
  if(className)
    {
    this->SetVTKClassName(className);
    }

  const char* xmlname = element->GetAttribute("name");
  if(xmlname)
    {
    this->SetXMLName(xmlname);
    this->SetXMLLabel(xmlname);
    }

  const char* xmllabel = element->GetAttribute("label");
  if (xmllabel)
    {
    this->SetXMLLabel(xmllabel);
    }

  const char* processes = element->GetAttribute("processes");
  if (processes)
    {
    vtkTypeUInt32 uiprocesses = 0;
    vtkstd::string strprocesses = processes;
    if (strprocesses.find("client") != vtkstd::string::npos)
      {
      uiprocesses |= vtkProcessModule::CLIENT;
      }
    if (strprocesses.find("renderserver") != vtkstd::string::npos)
      {
      uiprocesses |= vtkProcessModule::RENDER_SERVER;
      }
    if (strprocesses.find("dataserver") != vtkstd::string::npos)
      {
      uiprocesses |= vtkProcessModule::DATA_SERVER;
      }
    this->SetServersSelf(uiprocesses);
    }

  // Locate documentation.
  for (unsigned int cc=0; cc < element->GetNumberOfNestedElements(); ++cc)
    {
    vtkPVXMLElement* subElem = element->GetNestedElement(cc);
    if (strcmp(subElem->GetName(), "Documentation") == 0)
      {
      this->Documentation->SetDocumentationElement(subElem);
      }
    else if (strcmp(subElem->GetName(), "Hints") == 0)
      {
      this->SetHints(subElem);
      }
    else if (strcmp(subElem->GetName(), "Deprecated") == 0)
      {
      this->SetDeprecated(subElem);
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMProxy::ReadXMLAttributes(
  vtkSMProxyManager* pm, vtkPVXMLElement* element)
{
  this->SetXMLElement(element);

  // Read the common attributes.
  this->ReadCoreXMLAttributes(element);

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
        int override = 0;
        if (!subElement->GetScalarAttribute("override", &override))
          {
          override = 0;
          }
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
            subproxy = pm->NewProxy(subElement, 0, 0);
            }
          if (!subproxy)
            {
            vtkErrorMacro("Failed to create subproxy: " 
                          << (pname?pname:"(none"));
            return 0;
            }
          this->AddSubProxy(name, subproxy, override);
          this->SetupSharedProperties(subproxy, propElement);
          this->SetupExposedProperties(name, propElement);
          subproxy->Delete();
          }
        }
      }
    else
      {
      const char* name = propElement->GetAttribute("name");
      vtkstd::string tagName = propElement->GetName();
      if (name && tagName.find("Property") == (tagName.size()-8))
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
       int override = 0;
       if (!propertyElement->GetScalarAttribute("override", &override))
         {
         override = 0;
         }

       if (propertyElement->GetAttribute("default_values"))
         {
         vtkSMProxy* subproxy = this->GetSubProxy(subproxy_name);
         vtkSMProperty* prop = subproxy->GetProperty(name);
         if (!prop)
           {
           vtkWarningMacro("Failed to locate property '" << name
             << "' on subproxy '" << subproxy_name << "'");
           return;
           }
         if (!prop->ReadXMLAttributes(subproxy, propertyElement))
           {
           return;
           }
         }
       this->ExposeSubProxyProperty(subproxy_name, name, exposed_name, override);
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
int vtkSMProxy::LoadState(
  vtkPVXMLElement* proxyElement, vtkSMProxyLocator* locator)
{
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  //int servers =0;
  //if (proxyElement->GetScalarAttribute("servers", &servers))
  //  {
  //  this->SetServersSelf(servers);
  //  }

  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    const char* name =  currentElement->GetName();
    if (!name)
      {
      continue;
      }
    if (strcmp(name, "Property") == 0)
      {
      const char* prop_name = currentElement->GetAttribute("name");
      if (!prop_name)
        {
        vtkErrorMacro("Cannot load property without a name.");
        continue;
        }
      vtkSMProperty* property = this->GetProperty(prop_name);
      if (!property)
        {
        vtkDebugMacro("Property " << prop_name<< " does not exist.");
        continue;
        }
      if (property->GetInformationOnly())
        {
        // don't load state for information only property.
        continue;
        }
      if (!property->LoadState(currentElement, locator))
        {
        return 0;
        }
      }
    else if (strcmp(name, "SubProxy") == 0)
      {
      this->LoadSubProxyState(currentElement, locator);
      }
    else if (strcmp(name, "RevivalState") == 0 &&
      locator && locator->GetReviveProxies())
      {
      if (!this->LoadRevivalState(currentElement))
        {
        return 0;
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxy::LoadRevivalState(vtkPVXMLElement* revivalElem)
{
  if (this->ObjectsCreated)
    {
    vtkErrorMacro("Cannot revive a proxy when the VTK objects for the proxy "
      "have already been created.");
    return 0;
    }

  vtkClientServerID selfid;
  int temp = 0;

  if (revivalElem->GetScalarAttribute("servers", &temp))
    {
    this->SetServersSelf(static_cast<vtkTypeUInt32>(temp));
    }
  else
    {
    vtkErrorMacro("Missing attribute 'servers'.");
    return 0;
    }

  if (revivalElem->GetScalarAttribute("id", &temp))
    {
    selfid.ID = static_cast<vtkTypeUInt32>(temp);
    }
  if (!selfid.ID)
    {
    vtkErrorMacro("Invalid self ID or attribute 'id' missing.");
    return 0;
    }
  this->SetSelfID(selfid);
  for (unsigned int cc=0; cc < revivalElem->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* currentElement = revivalElem->GetNestedElement(cc);
    const char* name = currentElement->GetName();
    if (name && strncmp(name, "VTKObjectID", strlen("VTKObjectID")) == 0)
      {
      vtkClientServerID id;
      int int_id;
      if (currentElement->GetScalarAttribute("id", &int_id) && int_id)
        {
        this->VTKObjectID.ID = int_id;
        }
      else
        {
        // Some proxies may not have any vtk object they represent (such as
        // all the animation proxies). It's not an error.
        // vtkErrorMacro("Element with id attribute not found.");
        }
      }
    else if (name && strcmp(name, "SubProxy") == 0)
      {
      vtkSMProxy* subProxy = 
        this->GetSubProxy(currentElement->GetAttribute("name"));
      if (!subProxy)
        {
        vtkErrorMacro("Failed to load subproxy with name: " 
                      << currentElement->GetAttribute("name") 
                      << ". Cannot revive the subproxy.");
        return 0;
        }
      if (!subProxy->LoadRevivalState(currentElement->GetNestedElement(0)))
        {
        return 0;
        }
      }
    }
  // This will create the client side objects needed.
  this->ReviveVTKObjects();
  return 1;
}

//---------------------------------------------------------------------------
int vtkSMProxy::RevertState(
  vtkPVXMLElement* vtkNotUsed(element),
  vtkSMProxyLocator* vtkNotUsed(locator))
{
  //vtkWarningMacro("RevertState not supported by this proxy");
  return 0;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveState(vtkPVXMLElement* root)
{
  vtkSMPropertyIterator *iter=this->NewPropertyIterator();
  vtkPVXMLElement *proxyXml=this->SaveState(root,iter,1);
  iter->Delete();
  return proxyXml;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveState(
    vtkPVXMLElement* root,
    vtkSMPropertyIterator *iter,
    int saveSubProxies)
{
  vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
  proxyElement->SetName("Proxy");
  proxyElement->AddAttribute("group", this->XMLGroup);
  proxyElement->AddAttribute("type", this->XMLName);
  proxyElement->AddAttribute("id", this->GetSelfIDAsString());
  proxyElement->AddAttribute("servers", 
    static_cast<unsigned int>(this->Servers));

  for (iter->Begin(); !iter->IsAtEnd(); iter->Next())
    {
    if (!iter->GetProperty())
      {
      vtkWarningMacro("Missing property with name: " << iter->GetKey()
        << " on " << this->GetXMLName());
      continue;
      }
    if (!iter->GetProperty()->GetIsInternal())
      {
      vtksys_ios::ostringstream propID;
      propID << this->GetSelfIDAsString() << "." << iter->GetKey() << ends;
      iter->GetProperty()->SaveState(proxyElement, iter->GetKey(),
        propID.str().c_str());
      }
    }

  if (root)
    {
    root->AddNestedElement(proxyElement);
    proxyElement->Delete();
    }

  if (saveSubProxies)
    {
    this->SaveSubProxyState(proxyElement);
    }

  return proxyElement;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMProxy::SaveRevivalState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
  proxyElement->SetName("RevivalState");
  proxyElement->AddAttribute("id", this->GetSelfIDAsString());
  proxyElement->AddAttribute("servers", 
    static_cast<unsigned int>(this->Servers));
  root->AddNestedElement(proxyElement);
  proxyElement->Delete();

  // Save VTKObject ID as well.
  vtkPVXMLElement* idRoot = vtkPVXMLElement::New();
  idRoot->SetName("VTKObjectID");
  idRoot->AddAttribute("id", 
                     static_cast<unsigned int>(this->VTKObjectID.ID));
  proxyElement->AddNestedElement(idRoot);
  idRoot->Delete();

  vtkSMProxyInternals::ProxyMap::iterator iter =
    this->Internals->SubProxies.begin();
  for (; iter != this->Internals->SubProxies.end(); ++iter)
    {
    vtkPVXMLElement* subproxyElement = vtkPVXMLElement::New();
    subproxyElement->SetName("SubProxy");
    subproxyElement->AddAttribute("name", iter->first.c_str());
    iter->second.GetPointer()->SaveRevivalState(subproxyElement);
    proxyElement->AddNestedElement(subproxyElement);
    subproxyElement->Delete();
    }
  return proxyElement;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SaveSubProxyState(vtkPVXMLElement* root)
{
  vtkSMProxyInternals::ProxyMap::iterator iter =
    this->Internals->SubProxies.begin();
  for (; iter != this->Internals->SubProxies.end(); ++iter)
    {
    vtkPVXMLElement* subproxyElement = vtkPVXMLElement::New();
    subproxyElement->SetName("SubProxy");
    subproxyElement->AddAttribute("name", iter->first.c_str());
    subproxyElement->AddAttribute("servers", 
      static_cast<unsigned int>(iter->second->GetServers()));
    iter->second.GetPointer()->SaveSubProxyState(subproxyElement);
    root->AddNestedElement(subproxyElement);
    subproxyElement->Delete();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::LoadSubProxyState(vtkPVXMLElement* subproxyElement, 
  vtkSMProxyLocator* locator)
{
  (void) subproxyElement;
  (void) locator;
  /* Restoring of proxy location from state is no longer needed.
  const char* name = subproxyElement->GetAttribute("name");
  if (name)
    {
    int servers=0;
    vtkSMProxy* proxy = this->GetSubProxy(name);
    if (proxy && subproxyElement->GetScalarAttribute("servers", &servers))
      {
      proxy->SetServersSelf(servers);
      for (unsigned int cc=0; cc < subproxyElement->GetNumberOfNestedElements();
        cc++)
        {
        vtkPVXMLElement* nested = subproxyElement->GetNestedElement(cc);
        if (nested->GetName() && strcmp(nested->GetName(), "SubProxy")==0)
          {
          proxy->LoadSubProxyState(nested, locator);
          }
        }
      }
    }
    */
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
  const char* property_name, const char* exposed_name,
  int override)
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
    if (!override)
      {
      vtkWarningMacro("An exposed property with the name \"" << exposed_name
        << "\" already exists. It will be replaced.");
      }
    }

  vtkSMProxyInternals::ExposedPropertyInfo info;
  info.SubProxyName = subproxy_name;
  info.PropertyName = property_name;
  this->Internals->ExposedProperties[exposed_name] = info;

  // Add the exposed property name to the vector of property names.
  // This vector keeps track of the order in which properties
  // were added.
  this->Internals->PropertyNamesInOrder.push_back(exposed_name);
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
  os << indent << "XMLLabel: " 
    << (this->XMLLabel? this->XMLLabel : "(null)")
    << endl;
  os << indent << "Documentation: " << this->Documentation << endl;
  os << indent << "ObjectsCreated: " << this->ObjectsCreated << endl;
  os << indent << "Hints: " ;
  if (this->Hints)
    {
    this->Hints->PrintSelf(os, indent);
    }
  else
    {
    os << "(null)" << endl;
    }

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
