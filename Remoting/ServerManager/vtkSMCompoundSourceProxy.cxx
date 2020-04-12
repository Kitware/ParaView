/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCompoundSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMCompoundSourceProxy.h"
#include "vtkSMProxyInternals.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMMessage.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"

#include <vtkSmartPointer.h>

#include <list>
#include <vector>

typedef std::list<vtkSmartPointer<vtkPVXMLElement> > ElementsType;
//*****************************************************************************
class vtkSMCompoundSourceProxy::vtkInternals
{
public:
  struct PortInfo
  {
    std::string ProxyName;
    std::string ExposedName;
    std::string PortName;
    unsigned int PortIndex;
    PortInfo() { this->PortIndex = VTK_UNSIGNED_INT_MAX; }

    bool HasPortIndex() { return this->PortIndex != VTK_UNSIGNED_INT_MAX; }

    bool HasPortName() { return !this->HasPortIndex(); }
  };

  void RegisterExposedPort(const char* proxyName, const char* exposedName, int portIndex)
  {
    PortInfo info;
    info.PortIndex = portIndex;
    info.ProxyName = proxyName;
    info.ExposedName = exposedName;
    this->ExposedPorts.push_back(info);
  }

  void RegisterExposedPort(const char* proxyName, const char* exposedName, const char* portName)
  {
    PortInfo info;
    info.PortName = portName;
    info.ProxyName = proxyName;
    info.ExposedName = exposedName;
    this->ExposedPorts.push_back(info);
  }

  typedef std::vector<PortInfo> VectorOfPortInfo;
  VectorOfPortInfo ExposedPorts;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMCompoundSourceProxy);
//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::vtkSMCompoundSourceProxy()
{
  this->CSInternals = new vtkInternals();
  this->SetSIClassName("vtkSICompoundSourceProxy");
}

//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::~vtkSMCompoundSourceProxy()
{
  delete this->CSInternals;
  this->CSInternals = NULL;
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::CreateOutputPorts()
{
  if (this->Location == 0 || this->OutputPortsCreated)
  {
    return;
  }

  this->OutputPortsCreated = 1;

  this->RemoveAllOutputPorts();

  this->CreateVTKObjects();

  unsigned int index = 0;
  vtkInternals::VectorOfPortInfo::iterator iter;
  iter = this->CSInternals->ExposedPorts.begin();
  while (iter != this->CSInternals->ExposedPorts.end())
  {
    vtkSMProxy* subProxy = this->GetSubProxy(iter->ProxyName.c_str());
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(subProxy);
    if (!source)
    {
      vtkErrorMacro("Failed to locate sub proxy with name " << iter->ProxyName.c_str());
      continue;
    }

    source->CreateOutputPorts();
    vtkSMOutputPort* port = 0;
    vtkSMDocumentation* doc = 0;
    unsigned int port_index = 0;
    if (iter->HasPortIndex())
    {
      port_index = iter->PortIndex;
    }
    else
    {
      port_index = source->GetOutputPortIndex(iter->PortName.c_str());
    }
    port = source->GetOutputPort(port_index);
    doc = source->GetOutputPortDocumentation(port_index);
    if (!port)
    {
      vtkErrorMacro(
        "Failed to locate requested output port of subproxy " << iter->ProxyName.c_str());
      continue;
    }
    port->SetCompoundSourceProxy(this);
    this->SetOutputPort(index, iter->ExposedName.c_str(), port, doc);

    index++;

    // Move forward
    iter++;
  }
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::CreateSelectionProxies()
{
  if (this->DisableSelectionProxies || this->SelectionProxiesCreated)
  {
    return;
  }

  this->SelectionProxiesCreated = true;
  this->RemoveAllExtractSelectionProxies();

  unsigned int numOutputs = this->GetNumberOfOutputPorts();

  for (unsigned int cc = 0; cc < numOutputs; cc++)
  {
    vtkSMOutputPort* port = this->GetOutputPort(cc);
    vtkSMSourceProxy* source = port->SourceProxy.GetPointer();
    if (source && source != this)
    {
      source->CreateSelectionProxies();
      this->SetExtractSelectionProxy(cc, source->GetSelectionOutput(port->GetPortIndex()));
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::UpdateVTKObjects()
{
  if (this->Location == 0)
  {
    return;
  }

  // update subproxies that don't have inputs first.
  // This is required for Readers/Sources that need the ExtractPieces filter to
  // be insered into the pipeline. Before inserting the ExtractPieces filter,
  // the pipeline needs to be updated, which may raise errors if all properties
  // of the source haven't been pushed correctly.
  unsigned int nbSubProxy = this->GetNumberOfSubProxies();
  for (unsigned int i = 0; i < nbSubProxy; i++)
  {
    vtkSMProxy* subProxy = this->GetSubProxy(i);
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(subProxy);
    // If subProxy is a vtkSMCompoundSourceProxy, make sure that UpdateVTKObjects is called.
    vtkSMCompoundSourceProxy* subCompound = vtkSMCompoundSourceProxy::SafeDownCast(subProxy);
    if (!source || subCompound != nullptr || source->GetNumberOfAlgorithmRequiredInputPorts() == 0)
    {
      subProxy->UpdateVTKObjects();
    }
  }

  this->Superclass::UpdateVTKObjects();
}
//----------------------------------------------------------------------------
int vtkSMCompoundSourceProxy::ReadXMLAttributes(
  vtkSMSessionProxyManager* pm, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
  {
    return 0;
  }

  // Initialise the Proxy based on its definition state
  vtkSmartPointer<vtkSMCompoundProxyDefinitionLoader> deserializer;
  deserializer = vtkSmartPointer<vtkSMCompoundProxyDefinitionLoader>::New();
  deserializer->SetRootElement(element);
  deserializer->SetSessionProxyManager(pm);
  vtkSmartPointer<vtkSMProxyLocator> locator;
  locator = vtkSmartPointer<vtkSMProxyLocator>::New();
  locator->SetDeserializer(deserializer.GetPointer());

  // Initialise sub-proxy by registering them as sub-proxy --------------------
  int currentId;
  std::string compoundName;
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Proxy") == 0)
    {
      compoundName = currentElement->GetAttributeOrEmpty("compound_name");
      if (!compoundName.empty())
      {
        if (!currentElement->GetScalarAttribute("id", &currentId))
        {
          continue;
        }
        vtkSMProxy* subProxy = locator->LocateProxy(currentId);
        if (subProxy)
        {
          this->AddSubProxy(compoundName.c_str(), subProxy);
        }
      }
    }
  }

  // Initialise exposed properties --------------------------------------------
  vtkPVXMLElement* exposedProperties = NULL;
  exposedProperties = element->FindNestedElementByName("ExposedProperties");
  numElems = exposedProperties ? exposedProperties->GetNumberOfNestedElements() : 0;
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = exposedProperties->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "Property") == 0)
    {
      const char* name = currentElement->GetAttribute("name");
      const char* proxyName = currentElement->GetAttribute("proxy_name");
      const char* exposedName = currentElement->GetAttribute("exposed_name");
      if (name && proxyName && exposedName)
      {
        this->ExposeSubProxyProperty(proxyName, name, exposedName);
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

  // Initialise exposed output port -------------------------------------------
  int index = 0;
  numElems = element->GetNumberOfNestedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement->GetName() && strcmp(currentElement->GetName(), "OutputPort") == 0)
    {
      const char* exposed_name = currentElement->GetAttribute("name");
      const char* proxy_name = currentElement->GetAttribute("proxy");
      const char* port_name = currentElement->GetAttribute("port_name");
      if (!port_name && !currentElement->GetScalarAttribute("port_index", &index))
      {
        vtkErrorMacro("Missing output port 'index'.");
        return 0;
      }
      if (!exposed_name)
      {
        vtkErrorMacro("Missing output port 'name'.");
        return 0;
      }
      if (!proxy_name)
      {
        vtkErrorMacro("Missing output port 'proxy'.");
        return 0;
      }
      if (port_name)
      {
        this->CSInternals->RegisterExposedPort(proxy_name, exposed_name, port_name);
      }
      else
      {
        this->CSInternals->RegisterExposedPort(proxy_name, exposed_name, index);
      }
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// vtkSMSourceProxy guarantees that the output port size is set up correctly
// after CreateVTKObjects(). Hence, we ensure that it is set up correctly.
void vtkSMCompoundSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated || this->Location == 0)
  {
    return;
  }
  this->Superclass::CreateVTKObjects();

  // --- Specific CompoundSourceProxy behaviour ---
  unsigned int index = 0;
  vtkInternals::VectorOfPortInfo::iterator iter;
  iter = this->CSInternals->ExposedPorts.begin();
  vtkSMProxy* currentProxy;
  while (iter != this->CSInternals->ExposedPorts.end())
  {
    currentProxy = this->GetSubProxy(iter->ProxyName.c_str());
    vtkSMSourceProxy* subProxy = vtkSMSourceProxy::SafeDownCast(currentProxy);

    if (!subProxy)
    {
      vtkErrorMacro("Failed to locate sub proxy with name " << iter->ProxyName.c_str());
      iter++;
      continue;
    }

    if (iter->HasPortIndex() ||
      (subProxy->GetOutputPortIndex(iter->PortName.c_str()) == VTK_UNSIGNED_INT_MAX))
    {
      if (subProxy->GetNumberOfOutputPorts() <= iter->PortIndex)
      {
        vtkErrorMacro(
          "Failed to locate requested output port of subproxy " << iter->ProxyName.c_str());
        iter++;
        continue;
      }
    }

    // We cannot create vtkSMOutputPort proxies right now, since that requires
    // that the input to this filter is setup correctly. Hence we wait for the
    // CreateOutputPorts() call to create the vtkSMOutputPort proxies.
    if (this->GetNumberOfOutputPorts() <= index)
    {
      this->SetOutputPort(index, iter->ExposedName.c_str(), 0, 0);
    }

    // This sets up the dependency chain correctly.
    subProxy->AddConsumer(0, this);
    this->AddProducer(0, subProxy);
    index++;

    // Move forward
    iter++;
  }
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::AddProxy(const char* name, vtkSMProxy* proxy)
{
  // If proxy with the name already exists, this->AddSubProxy raises a warning.
  this->AddSubProxy(name, proxy);
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeProperty(
  const char* proxyName, const char* propertyName, const char* exposedName)
{
  this->ExposeSubProxyProperty(proxyName, propertyName, exposedName);
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeOutputPort(
  const char* proxyName, const char* portName, const char* exposedName)
{
  this->CSInternals->RegisterExposedPort(proxyName, exposedName, portName);
  // We don't access the vtkSMOutputPort from the sub proxy here itself, since
  // the subproxy may not have it ports initialized when this method is called
  // eg. when loading state.
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeOutputPort(
  const char* proxyName, unsigned int portIndex, const char* exposedName)
{
  this->CSInternals->RegisterExposedPort(proxyName, exposedName, portIndex);
  // We don't access the vtkSMOutputPort from the sub proxy here itself, since
  // the subproxy may not have it ports initialized when this method is called
  // eg. when loading state.
}
//----------------------------------------------------------------------------
// Definition is
// * State i.e. exposed property states (execept those refererring to outside
// proxies)
// * Subproxy states.
// * Exposed property information.
// * Exposed output port information.
vtkPVXMLElement* vtkSMCompoundSourceProxy::SaveDefinition(vtkPVXMLElement* root)
{
  vtkPVXMLElement* defElement = this->SaveXMLState(0);
  defElement->SetName("CompoundSourceProxy");
  defElement->RemoveAllNestedElements();

  // * Add subproxy states.
  unsigned int numProxies = this->GetNumberOfSubProxies();
  for (unsigned int cc = 0; cc < numProxies; cc++)
  {
    vtkPVXMLElement* newElem = this->GetSubProxy(cc)->SaveXMLState(defElement);
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
    expElem->AddAttribute("name", iter->second.PropertyName.c_str());
    expElem->AddAttribute("proxy_name", iter->second.SubProxyName.c_str());
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
  vtkInternals::VectorOfPortInfo::iterator iter2 = this->CSInternals->ExposedPorts.begin();
  for (; iter2 != this->CSInternals->ExposedPorts.end(); ++iter2, numExposed++)
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
}

//---------------------------------------------------------------------------
int vtkSMCompoundSourceProxy::ShouldWriteValue(vtkPVXMLElement* valueElem)
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

  unsigned int numProxies = this->GetNumberOfSubProxies();
  for (unsigned int i = 0; i < numProxies; i++)
  {
    vtkSMProxy* proxy = this->GetSubProxy(i);
    if (proxy && strcmp(proxy->GetGlobalIDAsString(), proxyId) == 0)
    {
      return 1;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::StripValues(vtkPVXMLElement* propertyElem)
{
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
void vtkSMCompoundSourceProxy::TraverseForProperties(vtkPVXMLElement* root)
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
