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

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSmartPointer.h"
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMOutputPort.h"
#include "vtkSMProxyInternals.h"
#include "vtkSMProxyLocator.h"

#include <vtkstd/list>
#include <vtkstd/vector>

//----------------------------------------------------------------------------
class vtkSMCompoundSourceProxy::vtkInternal
{
public:
  struct PortInfo
    {
    vtkstd::string ProxyName;
    vtkstd::string ExposedName;
    vtkstd::string PortName;
    unsigned int PortIndex;
    PortInfo()
      {
      this->PortIndex = VTK_UNSIGNED_INT_MAX;
      }
    };

  typedef vtkstd::vector<PortInfo> VectorOfPortInfo;
  VectorOfPortInfo ExposedPorts;
};


vtkStandardNewMacro(vtkSMCompoundSourceProxy);
//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::vtkSMCompoundSourceProxy()
{
  this->CSInternal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::~vtkSMCompoundSourceProxy()
{
  delete this->CSInternal;
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::AddProxy(const char* name, vtkSMProxy* proxy)
{
  if (this->ConnectionID != proxy->GetConnectionID())
    {
    vtkErrorMacro("All proxies in a compound proxy must be on the same connection.");
    return;
    }

  /*
  if (this->Servers != proxy->GetServers())
    {
    vtkErrorMacro("All proxies in a compound proxy must have the same Servers flag.");
    return;
    }
  */

  // If proxy with the name already exists, this->AddSubProxy raises a warning.
  this->AddSubProxy(name, proxy);
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeProperty(const char* proxyName, 
  const char* propertyName, const char* exposedName)
{
  this->ExposeSubProxyProperty(proxyName, propertyName, exposedName);
}


//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeOutputPort(const char* proxyName, 
  const char* portName, const char* exposedName)
{
  vtkInternal::PortInfo info;
  info.ProxyName = proxyName;
  info.ExposedName = exposedName;
  info.PortName = portName;
  this->CSInternal->ExposedPorts.push_back(info);
  // We don't access the vtkSMOutputPort from the sub proxy here itself, since
  // the subproxy may not have it ports initialized when this method is called
  // eg. when loading state.
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::ExposeOutputPort(const char* proxyName, 
  unsigned int portIndex, const char* exposedName)
{
  vtkInternal::PortInfo info;
  info.PortIndex = portIndex;
  info.ProxyName = proxyName;
  info.ExposedName = exposedName;
  this->CSInternal->ExposedPorts.push_back(info);
  // We don't access the vtkSMOutputPort from the sub proxy here itself, since
  // the subproxy may not have it ports initialized when this method is called
  // eg. when loading state.
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::CreateVTKObjects()
{
  if (this->ObjectsCreated)
    {
    return;
    }

  this->Superclass::CreateVTKObjects();
  // vtkSMSourceProxy gurantees that the output port size is set up correctly
  // after CreateVTKObjects(). Hence, we ensure that it is set up correctly.
  // We cannot create vtkSMOutputPort proxies right now, since that requires
  // that the input to this filter is setup correctly. Hence we wait for the 
  // CreateOutputPorts() call to create the vtkSMOutputPort proxies.
  
  unsigned int index=0;
  vtkInternal::VectorOfPortInfo::iterator iter;
  for (iter = this->CSInternal->ExposedPorts.begin(); 
    iter != this->CSInternal->ExposedPorts.end(); ++iter)
    {
    vtkSMSourceProxy* subProxy = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy(iter->ProxyName.c_str()));
    if (!subProxy)
      {
      vtkErrorMacro("Failed to locate sub proxy with name " <<
        iter->ProxyName.c_str());
      continue;
      }

    if (iter->PortIndex != VTK_UNSIGNED_INT_MAX)
      {
      if (subProxy->GetNumberOfOutputPorts() <= iter->PortIndex)
        {
        vtkErrorMacro("Failed to locate requested output port of subproxy "
          << iter->ProxyName.c_str());
        continue;
        }
      }
    else
      {
      if (subProxy->GetOutputPortIndex(iter->PortName.c_str()) == VTK_UNSIGNED_INT_MAX)
        {
        vtkErrorMacro("Failed to locate requested output port of subproxy "
          << iter->ProxyName.c_str());
        continue;
        }
      }
    this->SetOutputPort(index, iter->ExposedName.c_str(), 0, 0);

    // This sets up the dependency chain correctly.
    subProxy->AddConsumer(0, this);
    this->AddProducer(0, subProxy);
    index++;
    }
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::UpdateVTKObjects(vtkClientServerStream& stream)
{
  // update subproxies that don't have inputs first.
  // This is required for Readers/Sources that need the ExtractPieces filter to
  // be insered into the pipeline. Before inserting the ExtractPieces filter,
  // the pipeline needs to be updated, which may raise errors if all properties
  // of the source haven't been pushed correctly.
  vtkSMProxyInternals::ProxyMap::iterator it2 =
    this->Internals->SubProxies.begin();
  for( ; it2 != this->Internals->SubProxies.end(); it2++)
    {
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(
      it2->second.GetPointer());
    // FIXME: for vtkSMCompoundSourceProxy,
    // GetNumberOfAlgorithmRequiredInputPorts always returns 0.
    if (!source || source->GetNumberOfAlgorithmRequiredInputPorts() == 0)
      {
      it2->second.GetPointer()->UpdateVTKObjects(stream);
      }
    }

  this->Superclass::UpdateVTKObjects(stream);
}

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::CreateOutputPorts()
{
  if (this->OutputPortsCreated && this->GetNumberOfOutputPorts())
    {
    return;
    }

  this->OutputPortsCreated=1;

  this->RemoveAllOutputPorts();
  this->CreateVTKObjects();

  unsigned int index=0;
  vtkInternal::VectorOfPortInfo::iterator iter;
  for (iter = this->CSInternal->ExposedPorts.begin(); 
    iter != this->CSInternal->ExposedPorts.end(); ++iter)
    {
    vtkSMSourceProxy* subProxy = vtkSMSourceProxy::SafeDownCast(
      this->GetSubProxy(iter->ProxyName.c_str()));
    if (!subProxy)
      {
      vtkErrorMacro("Failed to locate sub proxy with name " <<
        iter->ProxyName.c_str());
      continue;
      }

    subProxy->CreateOutputPorts();
    vtkSMOutputPort* port = 0;
    vtkSMDocumentation* doc = 0;
    if (iter->PortIndex != VTK_UNSIGNED_INT_MAX)
      {
      port = subProxy->GetOutputPort(iter->PortIndex);
      doc = subProxy->GetOutputPortDocumentation(iter->PortIndex);
      }
    else
      {
      port = subProxy->GetOutputPort(iter->PortName.c_str());
      doc = subProxy->GetOutputPortDocumentation(iter->PortName.c_str());
      }
    if (!port)
      {
      vtkErrorMacro("Failed to locate requested output port of subproxy "
        << iter->ProxyName.c_str());
      continue;
      }
    this->SetOutputPort(index, iter->ExposedName.c_str(), port, doc);
    index++;
    }
}

//----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMCompoundSourceProxy::SaveRevivalState(vtkPVXMLElement* root)
{
  int old_output_ports_created = this->OutputPortsCreated;
  this->OutputPortsCreated=0;
  // Trick superclass into not save the output ports since the output ports
  // really belong to our subproxies and they are the onces who'll revive them.
  vtkPVXMLElement* elem = this->Superclass::SaveRevivalState(root);
  this->OutputPortsCreated = old_output_ports_created;
  return elem;
}

//----------------------------------------------------------------------------
int vtkSMCompoundSourceProxy::LoadRevivalState(vtkPVXMLElement* revivalElem)
{
  if (!this->Superclass::LoadRevivalState(revivalElem))
    {
    return 0;
    }

  this->OutputPortsCreated = 0;
  this->CreateOutputPorts();
  return 1;
}

//----------------------------------------------------------------------------
int vtkSMCompoundSourceProxy::LoadDefinition(vtkPVXMLElement* proxyElement,
  vtkSMProxyLocator* locator)
{
  this->ReadCoreXMLAttributes(proxyElement);

  // * Iterate over all <Proxy /> sub elements and add subproxies.
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
        vtkSMProxy* subProxy = locator->LocateProxy(currentId);
        if (subProxy)
          {
          subProxy->SetConnectionID(this->ConnectionID);
          this->AddProxy(compoundName, subProxy);
          }
        }
      }
    }

  // * Iterate over all <ExposeProperty /> elements and expose those properties.
  // * Iterate over all <OutputPort /> elements and expose the output
  //   ports.
  for (i=0; i<numElems; i++)
    {
    vtkPVXMLElement* currentElement = proxyElement->GetNestedElement(i);
    if (currentElement->GetName() &&
        strcmp(currentElement->GetName(), "ExposedProperties") == 0)
      {
      this->HandleExposedProperties(currentElement);
      }
    if (currentElement->GetName() &&
      strcmp(currentElement->GetName(), "OutputPort") == 0)
      {
      const char* exposed_name = currentElement->GetAttribute("name");
      const char* proxy_name = currentElement->GetAttribute("proxy");
      int index=0;
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
        this->ExposeOutputPort(proxy_name, port_name, exposed_name);
        }
      else
        {
        this->ExposeOutputPort(proxy_name, index, exposed_name);
        }
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::HandleExposedProperties(vtkPVXMLElement* element)
{
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
        this->ExposeProperty(proxyName, name, exposedName);
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


//----------------------------------------------------------------------------
// Definition is 
// * State i.e. exposed property states (execept those refererring to outside
// proxies)
// * Subproxy states.
// * Exposed property information.
// * Exposed output port information.
vtkPVXMLElement* vtkSMCompoundSourceProxy::SaveDefinition(
  vtkPVXMLElement* root)
{
  vtkPVXMLElement* defElement = this->SaveState(0);
  defElement->SetName("CompoundSourceProxy");
  defElement->RemoveAllNestedElements();
  
  // * Add subproxy states.
  unsigned int numProxies = this->GetNumberOfSubProxies();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    vtkPVXMLElement* newElem = 
      this->GetSubProxy(cc)->SaveState(defElement);
    const char* compound_name =this->GetSubProxyName(cc);
    newElem->AddAttribute("compound_name", compound_name);
    }

  // * Clean references to any external proxies.
  this->TraverseForProperties(defElement);
  
  // * Add exposed property information.
  vtkPVXMLElement* exposed = vtkPVXMLElement::New();
  exposed->SetName("ExposedProperties");
  unsigned int numExposed = 0;
  vtkSMProxyInternals* internals = this->Internals;
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
    defElement->AddNestedElement(exposed);
    }
  exposed->Delete();

  // * Add output port information.
  vtkInternal::VectorOfPortInfo::iterator iter2 = 
    this->CSInternal->ExposedPorts.begin();
  for (;iter2 != this->CSInternal->ExposedPorts.end(); ++iter2, numExposed++)
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
  for (unsigned int i=0; i<numProxies; i++)
    {
    vtkSMProxy* proxy = this->GetSubProxy(i);
    if (proxy &&
        strcmp(proxy->GetSelfIDAsString(), proxyId) == 0)
      {
      return 1;
      }
    }
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::StripValues(vtkPVXMLElement* propertyElem)
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
void vtkSMCompoundSourceProxy::TraverseForProperties(vtkPVXMLElement* root)
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

//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}


