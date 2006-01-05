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

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/list>
#include <vtkStdString.h>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMCompoundProxy);
vtkCxxRevisionMacro(vtkSMCompoundProxy, "1.4");

vtkCxxSetObjectMacro(vtkSMCompoundProxy, MainProxy, vtkSMProxy);

struct vtkSMCompoundProxyInternals
{
  vtkstd::set<vtkStdString> ProxyProperties;
};

//----------------------------------------------------------------------------
vtkSMCompoundProxy::vtkSMCompoundProxy()
{
  this->MainProxy = 0;

  this->Internal = new vtkSMCompoundProxyInternals;
}

//----------------------------------------------------------------------------
vtkSMCompoundProxy::~vtkSMCompoundProxy()
{
  if (this->MainProxy)
    {
    this->MainProxy->Delete();
    }
  this->MainProxy = 0;

  delete this->Internal;
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
    this->MainProxy = vtkSMProxy::New();
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
vtkPVXMLElement* vtkSMCompoundProxy::SaveState(vtkPVXMLElement* root)
{
  vtkPVXMLElement* proxyElement = vtkPVXMLElement::New();
  proxyElement->SetName("CompoundProxy");

  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int i=0; i<numProxies; i++)
    {
    vtkPVXMLElement* newElem = this->GetProxy(i)->SaveState(proxyElement);
    newElem->AddAttribute("compound_name", this->GetProxyName(i));
    }

  root->AddNestedElement(proxyElement);
  proxyElement->Delete();
  return proxyElement;
}

//---------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundProxy::SaveDefinition(vtkPVXMLElement* root)
{
  vtkPVXMLElement* defElement = vtkPVXMLElement::New();
  defElement->SetName("CompoundProxyDefinition");

  this->SaveState(defElement);

  this->Internal->ProxyProperties.clear();
  this->TraverseForProperties(defElement->GetNestedElement(0));
  if (root)
    {
    root->AddNestedElement(defElement);
    }

  return defElement;
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




