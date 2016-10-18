/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompoundSourceProxyDefinitionBuilder.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSmartPointer.h"

#include <list>
#include <map>
#include <string>

class vtkSMCompoundSourceProxyDefinitionBuilder::vtkInternals
{
public:
  typedef std::map<std::string, vtkSmartPointer<vtkSMProxy> > MapOfProxies;
  MapOfProxies Proxies;

  struct PropertyInfo
  {
    std::string ProxyName;
    std::string PropertyName;
  };
  typedef std::map<std::string, PropertyInfo> MapOfProperties;
  MapOfProperties Properties;

  struct PortInfo
  {
    std::string ProxyName;
    std::string PortName;
    unsigned int PortIndex;
    PortInfo() { this->PortIndex = VTK_UNSIGNED_INT_MAX; }
  };
  typedef std::map<std::string, PortInfo> MapOfPorts;
  MapOfPorts Ports;
};

vtkStandardNewMacro(vtkSMCompoundSourceProxyDefinitionBuilder);
//----------------------------------------------------------------------------
vtkSMCompoundSourceProxyDefinitionBuilder::vtkSMCompoundSourceProxyDefinitionBuilder()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSMCompoundSourceProxyDefinitionBuilder::~vtkSMCompoundSourceProxyDefinitionBuilder()
{
  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::Reset()
{
  delete this->Internals;
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::AddProxy(const char* name, vtkSMProxy* proxy)
{
  if (this->Internals->Proxies.find(name) != this->Internals->Proxies.end())
  {
    vtkErrorMacro("Proxy already exists: " << name);
    return;
  }

  this->Internals->Proxies[name] = proxy;
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::ExposeProperty(
  const char* proxyName, const char* propertyName, const char* exposedName)
{
  if (this->Internals->Properties.find(exposedName) != this->Internals->Properties.end())
  {
    vtkErrorMacro("Property already exists: " << exposedName);
    return;
  }
  vtkInternals::PropertyInfo info;
  info.ProxyName = proxyName;
  info.PropertyName = propertyName;
  this->Internals->Properties[exposedName] = info;
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::ExposeOutputPort(
  const char* proxyName, const char* portName, const char* exposedName)
{
  if (this->Internals->Ports.find(exposedName) != this->Internals->Ports.end())
  {
    vtkErrorMacro("Port already exists: " << exposedName);
    return;
  }

  vtkInternals::PortInfo info;
  info.ProxyName = proxyName;
  info.PortName = portName;
  this->Internals->Ports[exposedName] = info;
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::ExposeOutputPort(
  const char* proxyName, unsigned int portIndex, const char* exposedName)
{
  if (this->Internals->Ports.find(exposedName) != this->Internals->Ports.end())
  {
    vtkErrorMacro("Port already exists: " << exposedName);
    return;
  }

  vtkInternals::PortInfo info;
  info.ProxyName = proxyName;
  info.PortIndex = portIndex;
  this->Internals->Ports[exposedName] = info;
}

//----------------------------------------------------------------------------
unsigned int vtkSMCompoundSourceProxyDefinitionBuilder::GetNumberOfProxies()
{
  return static_cast<unsigned int>(this->Internals->Proxies.size());
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundSourceProxyDefinitionBuilder::GetProxy(unsigned int index)
{
  vtkInternals::MapOfProxies::iterator iter = this->Internals->Proxies.begin();
  for (unsigned int cc = 0; iter != this->Internals->Proxies.end(); ++iter, ++cc)
  {
    if (cc == index)
    {
      return iter->second;
    }
  }
  return NULL;
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMCompoundSourceProxyDefinitionBuilder::GetProxy(const char* name)
{
  vtkInternals::MapOfProxies::iterator iter = this->Internals->Proxies.find(name);
  if (iter != this->Internals->Proxies.end())
  {
    return iter->second;
  }

  return NULL;
}

//----------------------------------------------------------------------------
const char* vtkSMCompoundSourceProxyDefinitionBuilder::GetProxyName(unsigned int index)
{
  vtkInternals::MapOfProxies::iterator iter = this->Internals->Proxies.begin();
  for (unsigned int cc = 0; iter != this->Internals->Proxies.end(); ++iter, ++cc)
  {
    if (cc == index)
    {
      return iter->first.c_str();
    }
  }

  return NULL;
}

//----------------------------------------------------------------------------
// Definition is
// * State i.e. exposed property states (execept those refererring to outside
// proxies)
// * Subproxy states.
// * Exposed property information.
// * Exposed output port information.
vtkPVXMLElement* vtkSMCompoundSourceProxyDefinitionBuilder::SaveDefinition(vtkPVXMLElement* root)
{
  (void)root;
  return NULL;
#ifdef FIXME
  vtkPVXMLElement* defElement = this->SaveState(0);
  defElement->SetName("CompoundSourceProxy");
  defElement->RemoveAllNestedElements();

  // * Add subproxy states.
  unsigned int numProxies = this->GetNumberOfSubProxies();
  vtkInternals::MapOfProxies::iterator iter;
  for (iter = this->Internals->Proxies.begin(); iter != this->Internals->Proxies.end(); ++iter)
  {
    vtkPVXMLElement* newElem = iter->second.GetPointer()->SaveState(defElement);
    const char* compound_name = this->GetSubProxyName(cc);
    newElem->AddAttribute("compound_name", compound_name);
  }

  // * Clean references to any external proxies.
  this->TraverseForProperties(defElement);

  // * Add exposed property information.
  vtkPVXMLElement* exposed = vtkPVXMLElement::New();
  exposed->SetName("ExposedProperties");
  unsigned int numExposed = 0;
  vtkSMProxyInternals* internals = this->Internals;
  vtkSMProxyInternals::ExposedPropertyInfoMap::iterator iter = internals->ExposedProperties.begin();
  for (; iter != internals->ExposedProperties.end(); iter++)
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
    defElement->AddNestedElement(exposed);
  }
  exposed->Delete();

  // * Add output port information.
  vtkInternal::VectorOfPortInfo::iterator iter2 = this->CSInternal->ExposedPorts.begin();
  for (; iter2 != this->CSInternal->ExposedPorts.end(); ++iter2, numExposed++)
  {
    vtkPVXMLElement* expElem = vtkPVXMLElement::New();
    expElem->SetName("OutputPort");
    expElem->AddAttribute("name", iter2->ExposedName.c_str());
    expElem->AddAttribute("proxy", iter2->ProxyName.c_str());
    if (iter2->PortIndex != VTK_UNSIGNED_INT_MAX)
    {
      expElem->AddAttribute("port_index", iter2->PortIndex);
    }
    else
    {
      expElem->AddAttribute("port_name", iter2->PortName.c_str());
    }
    defElement->AddNestedElement(expElem);
    expElem->Delete();
  }

  if (root)
  {
    root->AddNestedElement(defElement);
    defElement->Delete();
  }

  return defElement;
#endif
}

//---------------------------------------------------------------------------
int vtkSMCompoundSourceProxyDefinitionBuilder::ShouldWriteValue(vtkPVXMLElement* valueElem)
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

#ifdef FIXME
  unsigned int numProxies = this->GetNumberOfProxies();
  for (unsigned int i = 0; i < numProxies; i++)
  {
    vtkSMProxy* proxy = this->GetProxy(i);
    if (proxy && strcmp(proxy->GetSelfIDAsString(), proxyId) == 0)
    {
      return 1;
    }
  }
#endif
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::StripValues(vtkPVXMLElement* propertyElem)
{
  typedef std::list<vtkSmartPointer<vtkPVXMLElement> > ElementsType;
  ElementsType elements;

  // Find all elements we want to keep
  unsigned int numElements = propertyElem->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElements; i++)
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
void vtkSMCompoundSourceProxyDefinitionBuilder::TraverseForProperties(vtkPVXMLElement* root)
{
  unsigned int numProxies = root->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numProxies; i++)
  {
    vtkPVXMLElement* proxyElem = root->GetNestedElement(i);
    if (strcmp(proxyElem->GetName(), "Proxy") == 0)
    {
      unsigned int numProperties = proxyElem->GetNumberOfNestedElements();
      for (unsigned int j = 0; j < numProperties; j++)
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

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxyDefinitionBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
