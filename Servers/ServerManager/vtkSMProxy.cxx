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

#include "vtkClientServerStream.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkSMCommunicationModule.h"
#include "vtkSMProperty.h"
#include "vtkSmartPointer.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include "vtkStdString.h"

vtkStandardNewMacro(vtkSMProxy);
vtkCxxRevisionMacro(vtkSMProxy, "1.2");

struct vtkSMProxyInternals
{
  struct PropertyInfo
  {
    PropertyInfo() : ModifiedFlag(1), DoUpdate(1) {};
    vtkSmartPointer<vtkSMProperty> Property;
    int ModifiedFlag;
    int DoUpdate;
  };
  vtkstd::vector<vtkClientServerID > IDs;
  vtkstd::vector<int> ServerIDs;
  typedef vtkstd::map<vtkStdString,  PropertyInfo> PropertyInfoMap;
  PropertyInfoMap Properties;
};

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
  this->Internals->ServerIDs.push_back(1);
  this->VTKClassName = 0;
  this->ObjectsCreated = 0;
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
void vtkSMProxy::ClearServerIDs()
{
  this->Internals->ServerIDs.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxy::AddServerID(int id)
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
int* vtkSMProxy::GetServerIDs()
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
vtkSMProperty* vtkSMProxy::GetProperty(const char* name)
{
  vtkSMProxyInternals::PropertyInfoMap::iterator it =
    this->Internals->Properties.find(name);
  if (it == this->Internals->Properties.end())
    {
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
    prop->RemoveObservers(vtkCommand::ModifiedEvent);
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
  if (!prop)
    {
    return;
    }
  if (!name)
    {
    vtkErrorMacro("Can not add a property without a name.");
    return;
    }
  if (this->Internals->Properties.find(name) != 
      this->Internals->Properties.end())
    {
    vtkWarningMacro("Property " << name  << " already exists. Replacing");
    // TODO Have to remove old observer here.
    }

  if (addObserver)
    {
    vtkSMProxyObserver* obs = vtkSMProxyObserver::New();
    obs->SetProxy(this);
    obs->SetPropertyName(name);
    prop->AddObserver(vtkCommand::ModifiedEvent, obs);
    obs->Delete();
    }

  vtkSMProxyInternals::PropertyInfo newEntry;
  newEntry.DoUpdate = doUpdate;
  newEntry.Property = prop;
  this->Internals->Properties[name] = newEntry;
}

//---------------------------------------------------------------------------
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
    str.Print(cout);
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
  // TODO should this be here?
  this->CreateVTKObjects(1);
  int numObjects = this->Internals->IDs.size();

  vtkClientServerStream str;

  // Make each property push their values to each VTK object
  // referred by the proxy
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
    str.Print(cout);
    }

}

//---------------------------------------------------------------------------
void vtkSMProxy::AddVTKObject(vtkClientServerID id)
{
  this->Internals->IDs.push_back(id);

  vtkClientServerStream str;
  vtkClientServerID nullID = { 0 };
  str << vtkClientServerStream::Invoke 
      << id << "Register" 
      << nullID 
      << vtkClientServerStream::End;

  // TODO: This should be generalized. AddVTKObject() should take another
  // argument that tells where the command stream should go.
  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  cm->SendStreamToServers(&str, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  str.Print(cout);
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
  // TODO: This should be generalized. This class should have an
  // ivar describing on which "cluster" the objects should be created.
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  stream.Print(cout);

  this->Internals->IDs.clear();
}

//---------------------------------------------------------------------------
void vtkSMProxy::CreateVTKObjects(int numObjects)
{
  if (this->ObjectsCreated)
    {
    return;
    }
  this->ObjectsCreated = 1;
  if (!this->VTKClassName)
    {
    vtkErrorMacro("VTKClassName is not defined. Can not create objects.");
    return;
    }

  vtkSMCommunicationModule* cm = this->GetCommunicationModule();
  vtkClientServerStream stream;
  for (int i=0; i<numObjects; i++)
    {
    vtkClientServerID objectId = cm->NewStreamObject(this->VTKClassName, stream);
    
    this->Internals->IDs.push_back(objectId);
    }
  // TODO: This should be generalized. This class should have an
  // ivar describing on which "cluster" the objects should be created.
  cm->SendStreamToServers(&stream, 
                          this->GetNumberOfServerIDs(),
                          this->GetServerIDs());
  stream.Print(cout);
}

//---------------------------------------------------------------------------
void vtkSMProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
