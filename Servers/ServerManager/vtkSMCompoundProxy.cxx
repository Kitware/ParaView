/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompoundProxy.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStateLoader.h"

#include <vtkstd/set>
#include <vtkstd/list>
#include <vtkStdString.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCompoundProxy);
vtkCxxRevisionMacro(vtkSMCompoundProxy, "1.10");

struct vtkSMCompoundProxyInternals
{
  vtkstd::set<vtkStdString> ProxyProperties;
};

//----------------------------------------------------------------------------
class vtkSMCompoundProxyObserver : public vtkCommand
{
public:
  static vtkSMCompoundProxyObserver* New()
    { return new vtkSMCompoundProxyObserver; }

  void SetTarget(vtkSMCompoundProxy* target)
    {
    this->Target = target;
    }
  virtual void Execute(vtkObject*, unsigned long evenid, void* calldata)
    {
    if (this->Target)
      {
      this->Target->InvokeEvent(evenid, calldata);
      }
    }
protected:
  vtkSMCompoundProxyObserver() { this->Target = 0; }
  vtkSMCompoundProxy* Target;
};

//----------------------------------------------------------------------------
vtkSMCompoundProxy::vtkSMCompoundProxy()
{
  this->MainProxy = 0;

  this->Internal = new vtkSMCompoundProxyInternals;
  this->Observer = vtkSMCompoundProxyObserver::New();
  this->Observer->SetTarget(this);
}

//----------------------------------------------------------------------------
vtkSMCompoundProxy::~vtkSMCompoundProxy()
{
  this->SetMainProxy(0);

  this->Observer->SetTarget(NULL);
  this->Observer->Delete();

  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::SetMainProxy(vtkSMProxy* p)
{
  if (this->MainProxy)
    {
    this->MainProxy->RemoveObserver(this->Observer);
    }

  vtkSetObjectBodyMacro(MainProxy, vtkSMProxy, p);
  if (this->MainProxy && !this->MainProxy->ObjectsCreated)
    {
    this->MainProxy->SetConnectionID(this->ConnectionID);
    this->MainProxy->AddObserver(vtkCommand::ModifiedEvent, this->Observer);
    this->MainProxy->AddObserver(vtkCommand::PropertyModifiedEvent, 
      this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::SetConnectionID(vtkIdType id)
{
  if (this->MainProxy)
    {
    this->MainProxy->SetConnectionID(id);
    }
  this->Superclass::SetConnectionID(id);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundProxy::GetProxy(const char* name)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxy(name);
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundProxy::GetProxy(unsigned int index)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxy(index);
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::AddProxy(const char* name, vtkSMProxy* proxy)
{
  if (!this->MainProxy)
    {
    vtkSMProxy* mp = vtkSMProxy::New();
    this->SetMainProxy(mp);
    mp->Delete();
    }
  this->MainProxy->AddSubProxy(name, proxy);
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::RemoveProxy(const char* name)
{
  if (!this->MainProxy)
    {
    return;
    }

  this->MainProxy->RemoveSubProxy(name);
}

//----------------------------------------------------------------------------
const char* vtkSMCompoundProxy::GetProxyName(unsigned int index)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetSubProxyName(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMCompoundProxy::GetNumberOfProxies()
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetNumberOfSubProxies();
}


//---------------------------------------------------------------------------
vtkSMProperty* vtkSMCompoundProxy::GetProperty(const char* name)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetProperty(name);
}

//---------------------------------------------------------------------------
vtkSMProperty* vtkSMCompoundProxy::GetProperty(const char* name, int selfOnly)
{
  if (!this->MainProxy)
    {
    return 0;
    }

  return this->MainProxy->GetProperty(name, selfOnly);
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::UpdateVTKObjects()
{
  if (!this->MainProxy)
    {
    return;
    }
  this->MainProxy->UpdateVTKObjects();
  this->Superclass::UpdateVTKObjects();
}

//---------------------------------------------------------------------------
vtkSMPropertyIterator* vtkSMCompoundProxy::NewPropertyIterator()
{
  if (!this->MainProxy)
    {
    return 0;
    }
  return this->MainProxy->NewPropertyIterator();
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::ExposeProperty(const char* proxyName, 
                                        const char* propertyName,
                                        const char* exposedName)
{
  if (!this->MainProxy)
    {
    return;
    }
  this->MainProxy->ExposeSubProxyProperty(proxyName, 
                                          propertyName,
                                          exposedName);
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::ExposeExternalProperties()
{
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numProxies; i++)
    {
    vtkSMProxy* subProxy = this->GetProxy(i);
    vtkSMPropertyIterator* iter = subProxy->NewPropertyIterator();
    for(iter->Begin(); !iter->IsAtEnd(); iter->Next())
      {
      vtkSMProxyProperty* proxyProp = vtkSMProxyProperty::SafeDownCast(
        iter->GetProperty());
      if (proxyProp)
        {
        int expose = 1;
        unsigned int numProxiesInProp = proxyProp->GetNumberOfProxies();
        if (numProxiesInProp < 1)
          {
          expose = 0;
          }
        for (unsigned int j=0; j<numProxiesInProp; j++)
          {
          vtkSMProxy* aProxy = proxyProp->GetProxy(j);

          for (unsigned int k=0; k<numProxies; k++)
            {
            if (aProxy == this->GetProxy(k))
              {
              expose = 0;
              break;
              }
            }
          if (!expose)
            {
            break;
            }
          }
        if (expose)
          {
          this->MainProxy->ExposeSubProxyProperty(this->GetProxyName(i),
                                                  iter->GetKey(),
                                                  iter->GetKey());
          }
        }
      }
    iter->Delete();
    }
}

//---------------------------------------------------------------------------
int vtkSMCompoundProxy::ShouldWriteValue(vtkPVXMLElement* valueElem)
{
  if (strcmp(valueElem->GetName(), "Proxy") != 0)
    {
    return 1;
    }

  const char* proxyId = valueElem->GetAttribute("value");
  if (!proxyId)
    {
    return 1;
    }

  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numProxies; i++)
    {
    vtkSMProxy* proxy = this->GetProxy(i);
    if (proxy &&
        strcmp(proxy->GetSelfIDAsString(), proxyId) == 0)
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::StripValues(vtkPVXMLElement* propertyElem)
{
  typedef vtkstd::list<vtkSmartPointer<vtkPVXMLElement> > ElementsType;
  ElementsType elements;

  // Find all elements we want to keep
  unsigned int numElements = propertyElem->GetNumberOfNestedElements();
  for(unsigned int i=0; i<numElements; i++)
    {
    vtkPVXMLElement* nested = propertyElem->GetNestedElement(i);
    if (this->ShouldWriteValue(nested))
      {
      elements.push_back(nested);
      }
    }

  // Delete all
  propertyElem->RemoveAllNestedElements();

  // Add the one we want to keep
  ElementsType::iterator iter = elements.begin();
  for (; iter != elements.end(); iter++)
    {
    propertyElem->AddNestedElement(iter->GetPointer());
    }
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::TraverseForProperties(vtkPVXMLElement* root)
{
  unsigned int numProxies = root->GetNumberOfNestedElements();
  for(unsigned int i=0; i<numProxies; i++)
    {
    vtkPVXMLElement* proxyElem = root->GetNestedElement(i);
    if (strcmp(proxyElem->GetName(), "Proxy") == 0)
      {
      unsigned int numProperties = proxyElem->GetNumberOfNestedElements();
      for(unsigned int j=0; j<numProperties; j++)
        {
        vtkPVXMLElement* propertyElem = proxyElem->GetNestedElement(j);
        if (strcmp(propertyElem->GetName(), "Property") == 0)
          {
          this->StripValues(propertyElem);
          }
        }
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMCompoundProxy::HandleExposedProperties(vtkPVXMLElement* element)
{
  if (!this->MainProxy)
    {
    return;
    }

  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Property") == 0)
      {
      const char* name = currentElement->GetAttribute("name");
      const char* proxyName = currentElement->GetAttribute("proxy_name");
      const char* exposedName = currentElement->GetAttribute("exposed_name");
      if (name && proxyName && exposedName)
        {
        this->MainProxy->ExposeSubProxyProperty(proxyName, name, exposedName);
        }
      else if (!name)
        {
        vtkErrorMacro("Required attribute name could not be found.");
        }
      else if (!proxyName)
        {
        vtkErrorMacro("Required attribute proxy_name could not be found.");
        }
      else if (!exposedName)
        {
        vtkErrorMacro("Required attribute exposed_name could not be found.");
        }
      }
    }
}

//---------------------------------------------------------------------------
int vtkSMCompoundProxy::LoadState(vtkPVXMLElement* proxyElement, 
                                  vtkSMStateLoader* loader)
{
  unsigned int i;
  unsigned int numElems = proxyElement->GetNumberOfNestedElements();
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "Proxy") == 0)
      {
      const char* compoundName = currentElement->GetAttribute(
        "compound_name");
      if (compoundName && compoundName[0] != '\0')
        {
        int currentId;
        if (!currentElement->GetScalarAttribute("id", &currentId))
          {
          continue;
          }
        vtkSMProxy* subProxy = loader->NewProxyFromElement(
          currentElement, currentId);
        if (subProxy)
          {
          this->AddProxy(compoundName, subProxy);
          subProxy->Delete();
          }
        }
      }
    }


  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "ExposedProperties") == 0)
      {
      this->HandleExposedProperties(currentElement);
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundProxy::SaveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* proxyElement = this->Superclass::SaveState(root);
  proxyElement->SetName("CompoundProxy");

  vtkPVXMLElement* exposed = vtkPVXMLElement::New();
  exposed->SetName("ExposedProperties");
  unsigned int numExposed = 0;
  vtkSMProxyInternals* internals = this->MainProxy->Internals;
  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter =
    internals->ExposedProperties.begin();
  for(; iter != internals->ExposedProperties.end(); iter++)
    {
    numExposed++;
    vtkPVXMLElement* expElem = vtkPVXMLElement::New();
    expElem->SetName("Property");
    expElem->AddAttribute("name", iter->second.PropertyName);
    expElem->AddAttribute("proxy_name", iter->second.SubProxyName);
    expElem->AddAttribute("exposed_name", iter->first.c_str());
    exposed->AddNestedElement(expElem);
    expElem->Delete();
    }
  if (numExposed > 0)
    {
    proxyElement->AddNestedElement(exposed);
    }
  exposed->Delete();

  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numProxies; i++)
    {
    vtkPVXMLElement* newElem = this->GetProxy(i)->SaveState(proxyElement);
    newElem->AddAttribute("compound_name", this->GetProxyName(i));
    }

  return proxyElement;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundProxy::SaveDefinition(vtkPVXMLElement* root)
{
  vtkPVXMLElement* defElement = this->SaveState(0);

  this->Internal->ProxyProperties.clear();
  this->TraverseForProperties(defElement);
  if (root)
    {
    root->AddNestedElement(defElement);
    defElement->Delete();
    }

  return defElement;
}

//----------------------------------------------------------------------------
vtkSMSourceProxy* vtkSMCompoundProxy::GetUnconsumedProxy()
{
  for (unsigned int i = this->GetNumberOfProxies(); i > 0; i--)
    {
    vtkSMSourceProxy* src = vtkSMSourceProxy::SafeDownCast(this->GetProxy(i-1));
    if (src)
      {
      return src;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
void vtkSMCompoundProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MainProxy: " << this->MainProxy;
  if (this->MainProxy)
    {
    os << ": ";
    this->MainProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << endl;
    }
}




