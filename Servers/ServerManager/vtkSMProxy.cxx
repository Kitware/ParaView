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

vtkStandardNewMacro(vtkSMProxy);
vtkCxxRevisionMacro(vtkSMProxy, "1.18");

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
  pm->SendStream(vtkProcessModule::CLIENT, initStream, 0);
  // This is done to make the last result message release it's reference 
  // count. Otherwise the object has a reference count of 3.
  if (pm)
    {
    pm->GetInterpreter()->ClearLastResult();
    }

  this->XMLElement = 0;
  this->DoNotModifyProperty = 0;
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
  pm->SendStream(this->Servers, stream, 0);

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
        pm->SendStream(vtkProcessModule::CLIENT, deleteStream, 0);
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
    pm->SendStream(servers, str, 0);
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
        pm->SendStream(this->Servers, str, 0);
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
    for (it  = this->Internals->Properties.begin();
         it != this->Internals->Properties.end();
         ++it)
      {
      vtkSMProperty* prop = it->second.Property.GetPointer();
      if (prop->GetInformationOnly())
        {
        prop->UpdateInformation(this->Internals->IDs[0]);
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateVTKObjects()
{
  vtkClientServerStream str;

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
        it->second.ModifiedFlag = 0;
        }
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
      it->second.ModifiedFlag = 0;
      }
    }
  if (str.GetNumberOfMessages() > 0)
    {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    pm->SendStream(this->Servers, str, 0);
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdateVTKObjects();
    }

  this->MarkConsumersAsModified();
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateSelfAndAllInputs()
{
  vtkSMPropertyIterator* iter = this->NewPropertyIterator();

  while (!iter->IsAtEnd())
    {
    iter->GetProperty()->UpdateAllInputs();
    iter->Next();
    }
  iter->Delete();

  this->UpdateVTKObjects();
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  if (!this->VTKClassName)
    {
    vtkErrorMacro("VTKClassName is not defined. Can not create objects.");
    return;
    }
  this->ObjectsCreated = 1;

  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkClientServerStream stream;
  for (int i=0; i<numObjects; i++)
    {
    vtkClientServerID objectId = pm->NewStreamObject(this->VTKClassName, stream);
    
    this->Internals->IDs.push_back(objectId);
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    pm->SendStream(this->Servers, stream, 0);
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

  // Create all sub-proxies and properties
  for(unsigned int i=0; i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* propElement = element->GetNestedElement(i);
    if (strcmp(propElement->GetName(), "SubProxy")==0)
      {
      vtkPVXMLElement* subElement = propElement->GetNestedElement(0);
      if (subElement)
        {
        const char* name = subElement->GetAttribute("name");
        if (name)
          {
          vtkSMProxy* subproxy = pm->NewProxy(subElement, 0);
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
        this->NewProperty(name);
        }
      }
    }

  this->SetXMLElement(0);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMProxy::SaveState(const char* name, ofstream* file, vtkIndent indent)
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
}
