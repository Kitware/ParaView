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
#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkDebugLeaks.h"
#include "vtkObjectFactory.h"
#include "vtkProcessModule.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxy);
vtkCxxRevisionMacro(vtkSMProxy, "1.6");

//---------------------------------------------------------------------------
// Internal data structure for storing object IDs, server IDs and
// properties. Each property has associated attributes: 
// * ModifiedFlag : has the property been modified since last update (push)
// * DoUpdate : should the propery be updated (pushed) during UpdateVTKObjects 
// * ObserverTag : the tag returned by AddObserver(). Used to remove the
// observer.
struct vtkSMProxyInternals
{
  struct PropertyInfo
  {
    PropertyInfo() : ModifiedFlag(1), DoUpdate(1), ObserverTag(0) {};
    vtkSmartPointer<vtkSMProperty> Property;
    int ModifiedFlag;
    int DoUpdate;
    unsigned int ObserverTag;
  };
  vtkstd::vector<vtkClientServerID > IDs;
  vtkstd::vector<int> ServerIDs;
  // Note that the name of the property is the map key. That is the
  // only place where name is stored
  typedef vtkstd::map<vtkStdString,  PropertyInfo> PropertyInfoMap;
  PropertyInfoMap Properties;

  typedef vtkstd::map<vtkStdString,  vtkSmartPointer<vtkSMProxy> > ProxyMap;
  ProxyMap SubProxies;
};

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
  // By default, all objects are created on server 1.
  this->Internals->ServerIDs.push_back(1);
  this->VTKClassName = 0;
  this->ObjectsCreated = 0;

  vtkClientServerID nullID = { 0 };
  this->SelfID = nullID;


  // Assign a unique clientserver id to this object.
  // Note that this ups the reference count to 2.
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  if (!cm)
    {
    vtkErrorMacro("Can not fully initialize without a global "
                  "CommunicationModule. This object will not be fully "
                  "functional.");
    return;
    }
  this->SelfID = cm->GetUniqueID();
  vtkClientServerStream initStream;
  initStream << vtkClientServerStream::Assign 
             << this->SelfID << this
             << vtkClientServerStream::End;
  cm->SendStreamToServer(&initStream, 0);
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  // This is done to make the last result message release it's reference 
  // count. Otherwise the object has a reference count of 3.
  if (pm)
    {
    pm->GetInterpreter()->ClearLastResult();
    }

}

//---------------------------------------------------------------------------
vtkSMProxy::~vtkSMProxy()
{
  if (this->ObjectsCreated)
    {
    this->UnRegisterVTKObjects();
    }
  this->RemoveAllObservers();
  delete this->Internals;
  this->SetVTKClassName(0);
}

//---------------------------------------------------------------------------
void vtkSMProxy::UnRegisterVTKObjects()
{
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  if (!cm)
    {
    return;
    }
  vtkClientServerStream stream;

  vtkstd::vector<vtkClientServerID>::iterator it;
  for (it=this->Internals->IDs.begin(); it!=this->Internals->IDs.end(); ++it)
    {
    cm->DeleteStreamObject(*it, stream);
    }
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());

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
        vtkSMCommunicationModule* cm = this->GetCommunicationModule();
        if (cm)
          {
          vtkClientServerID selfid = this->SelfID;
          this->SelfID.ID = 0;
          vtkClientServerStream deleteStream;
          deleteStream << vtkClientServerStream::Delete 
                       << selfid
                       << vtkClientServerStream::End;
          cm->SendStreamToServer(&deleteStream, 0);
          }
        else
          {
          vtkErrorMacro("There is not valid communication module assigned. "
                        "This object can not be cleanly destroyed.");
          }
        }
      }
    }
  this->Superclass::UnRegister(obj);
}


//---------------------------------------------------------------------------
void vtkSMProxy::ClearServerIDsSelf()
{
  this->Internals->ServerIDs.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxy::ClearServerIDs()
{
  this->ClearServerIDsSelf();

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->ClearServerIDsSelf();
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddServerID(int id)
{
  this->AddServerIDToSelf(id);

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->AddServerIDToSelf(id);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddServerIDToSelf(int id)
{
  this->Internals->ServerIDs.push_back(id);
}

//---------------------------------------------------------------------------
int vtkSMProxy::GetNumberOfServerIDs()
{
  return this->Internals->ServerIDs.size();
}

//---------------------------------------------------------------------------
int vtkSMProxy::GetServerID(int i)
{
  return this->Internals->ServerIDs[i];
}

//---------------------------------------------------------------------------
const int* vtkSMProxy::GetServerIDs()
{
  return &this->Internals->ServerIDs[0];
}

//---------------------------------------------------------------------------
int vtkSMProxy::GetNumberOfIDs()
{
  return this->Internals->IDs.size();
}

//---------------------------------------------------------------------------
vtkClientServerID vtkSMProxy::GetID(int idx)
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
vtkSMProperty* vtkSMProxy::GetProperty(const char* name)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
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
  this->AddProperty(name, prop, 1, 1);
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProperty(
  const char* name, vtkSMProperty* prop, int addObserver, int doUpdate)
{
  this->AddProperty(0, name, prop, addObserver, doUpdate);
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddProperty(const char* subProxyName, 
                             const char* name, 
                             vtkSMProperty* prop, 
                             int addObserver, 
                             int doUpdate)
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
    vtkSMProxyInternals::ProxyMap::iterator it2 =
      this->Internals->SubProxies.begin();
    for( ; it2 != this->Internals->SubProxies.end(); it2++)
      {
      vtkSMProperty* oldprop = it2->second.GetPointer()->GetProperty(name);
      if (oldprop)
        {
        it2->second.GetPointer()->AddProperty(name, prop, addObserver, doUpdate);
        return;
        }
      }
    this->AddPropertyToSelf(name, prop, addObserver, doUpdate);
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
    it->second.GetPointer()->AddProperty(name, prop, addObserver, doUpdate);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddPropertyToSelf(
  const char* name, vtkSMProperty* prop, int addObserver, int doUpdate)
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

  if (addObserver)
    {
    vtkSMProxyObserver* obs = vtkSMProxyObserver::New();
    obs->SetProxy(this);
    obs->SetPropertyName(name);
    // We have to store the tag in order to be able to remove
    // the observer later.
    tag = prop->AddObserver(vtkCommand::ModifiedEvent, obs);
    obs->Delete();
    }

  vtkSMProxyInternals::PropertyInfo newEntry;
  newEntry.DoUpdate = doUpdate;
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
  const char* name, vtkClientServerID id, int serverid)
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
    it->second.Property.GetPointer()->AppendCommandToStream(&str, id);
    vtkSMCommunicationModule* cm = this->GetCommunicationModule();
    cm->SendStreamToServer(&str, serverid);
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::SetPropertyModifiedFlag(const char* name, int flag)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
    return;
    }
  it->second.ModifiedFlag = flag;
}

//---------------------------------------------------------------------------
void vtkSMProxy::UpdateVTKObjects()
{
  this->CreateVTKObjects(1);
  int numObjects = this->Internals->IDs.size();

  vtkClientServerStream str;

  // Make each property push their values to each VTK object
  // referred by the proxy. This is done by appending all
  // the command to a streaming and executing that stream
  // at the end.
  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    vtkSMProperty* prop = it->second.Property.GetPointer();
    if (it->second.ModifiedFlag && it->second.DoUpdate)
      {
      for (int i=0; i<numObjects; i++)
        {
        prop->AppendCommandToStream(&str, this->Internals->IDs[i]);
        }
      it->second.ModifiedFlag = 0;
      }
    }
  if (str.GetNumberOfMessages() > 0)
    {
    vtkSMCommunicationModule* cm = this->GetCommunicationModule();
    cm->SendStreamToServers(&str, 
                            this->GetNumberOfServerIDs(), 
                            this->GetServerIDs());
    }

  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    it2->second.GetPointer()->UpdateVTKObjects();
    }
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

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  vtkClientServerStream stream;
  for (int i=0; i<numObjects; i++)
    {
    vtkClientServerID objectId = cm->NewStreamObject(this->VTKClassName, stream);
    
    this->Internals->IDs.push_back(objectId);
    }
  if (stream.GetNumberOfMessages() > 0)
    {
    cm->SendStreamToServers(&stream, 
                            this->GetNumberOfServerIDs(),
                            this->GetServerIDs());
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
void vtkSMProxy::SaveState(const char* name, ofstream* file, vtkIndent indent)
{
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    ostrstream namestr;
    namestr << name << "." << it2->first << ends;
    it2->second.GetPointer()->SaveState(namestr.str(), file, indent);
    delete[] namestr.str();
    }

  *file << indent << this->GetClassName() << " : " << name << " : " << endl;

  vtkSMProxyInternals::PropertyInfoMap::iterator it;
  for (it  = this->Internals->Properties.begin();
       it != this->Internals->Properties.end();
       ++it)
    {
    it->second.Property.GetPointer()->SaveState(
      it->first.c_str(), file, indent.GetNextIndent());
    }
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "VTKClassName: " 
     << (this->VTKClassName ? this->VTKClassName : "(null)")
     << endl;
}
