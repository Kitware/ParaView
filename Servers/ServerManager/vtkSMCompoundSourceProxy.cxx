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
#include "vtkSMCompoundProxyDefinitionLoader.h"
#include "vtkSMMessage.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"

#include <vtkSmartPointer.h>

//*****************************************************************************
class vtkSMCompoundSourceProxy::vtkInternals
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

    bool HasPortIndex()
      {
      return this->PortIndex != VTK_UNSIGNED_INT_MAX;
      }

    bool HasPortName()
      {
      return !this->HasPortIndex();
      }
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

  typedef vtkstd::vector<PortInfo> VectorOfPortInfo;
  VectorOfPortInfo ExposedPorts;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSMCompoundSourceProxy);
//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::vtkSMCompoundSourceProxy()
{
  this->Internals = new vtkInternals();
  this->SetKernelClassName("vtkPMCompoundSourceProxy");
}

//----------------------------------------------------------------------------
vtkSMCompoundSourceProxy::~vtkSMCompoundSourceProxy()
{
  delete this->Internals;
  this->Internals = NULL;
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::CreateOutputPorts()
{
  if (this->Location == 0 ||
      (this->OutputPortsCreated && this->GetNumberOfOutputPorts() > 0))
    {
    return;
    }
  this->OutputPortsCreated = 1;

  this->RemoveAllOutputPorts();
  this->CreateVTKObjects();

  unsigned int index = 0;
  vtkInternals::VectorOfPortInfo::iterator iter;
  iter = this->Internals->ExposedPorts.begin();
  while ( iter != this->Internals->ExposedPorts.end() )
    {
    vtkSMProxy* subProxy = this->GetSubProxy(iter->ProxyName.c_str());
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(subProxy);
    if (!source)
      {
      vtkErrorMacro("Failed to locate sub proxy with name "
                    << iter->ProxyName.c_str());
      continue;
      }

    source->CreateOutputPorts();
    vtkSMOutputPort* port = 0;
    vtkSMDocumentation* doc = 0;
    if (iter->HasPortIndex())
      {
      port = source->GetOutputPort(iter->PortIndex);
      doc = source->GetOutputPortDocumentation(iter->PortIndex);
      }
    else
      {
      port = source->GetOutputPort(iter->PortName.c_str());
      doc = source->GetOutputPortDocumentation(iter->PortName.c_str());
      }
    if (!port)
      {
      vtkErrorMacro( "Failed to locate requested output port of subproxy "
                     << iter->ProxyName.c_str());
      continue;
      }
    this->SetOutputPort(index, iter->ExposedName.c_str(), port, doc);
    index++;

    // Move forward
    iter++;
    }
}
//----------------------------------------------------------------------------
void vtkSMCompoundSourceProxy::UpdateVTKObjects()
{
  if(this->Location == 0)
    {
    return;
    }

  // update subproxies that don't have inputs first.
  // This is required for Readers/Sources that need the ExtractPieces filter to
  // be insered into the pipeline. Before inserting the ExtractPieces filter,
  // the pipeline needs to be updated, which may raise errors if all properties
  // of the source haven't been pushed correctly.
  unsigned int nbSubProxy = this->GetNumberOfSubProxies();
  for( unsigned int i = 0; i < nbSubProxy; i++)
    {
    vtkSMProxy* subProxy = this->GetSubProxy(i);
    vtkSMSourceProxy* source = vtkSMSourceProxy::SafeDownCast(subProxy);
    // FIXME: for vtkSMCompoundSourceProxy,
    // GetNumberOfAlgorithmRequiredInputPorts always returns 0.
    if (!source || source->GetNumberOfAlgorithmRequiredInputPorts() == 0)
      {
      subProxy->UpdateVTKObjects();
      }
    }

  this->Superclass::UpdateVTKObjects();
}
//----------------------------------------------------------------------------
int vtkSMCompoundSourceProxy::ReadXMLAttributes( vtkSMProxyManager* pm,
                                                 vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(pm, element))
    {
    return 0;
    }

  // Initialise the Proxy based on its definition state
  vtkSmartPointer<vtkSMCompoundProxyDefinitionLoader> deserializer;
  deserializer = vtkSmartPointer<vtkSMCompoundProxyDefinitionLoader>::New();
  deserializer->SetRootElement(element);
  deserializer->SetSession(pm->GetSession());
  vtkSmartPointer<vtkSMProxyLocator> locator;
  locator = vtkSmartPointer<vtkSMProxyLocator>::New();
  locator->SetDeserializer(deserializer.GetPointer());

  // Initialise sub-proxy by registering them as sub-proxy --------------------
  int currentId;
  const char* compoundName;
  unsigned int numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i < numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if ( currentElement->GetName() &&
         strcmp(currentElement->GetName(), "Proxy") == 0)
      {
      compoundName = currentElement->GetAttribute("compound_name");
      if (compoundName && compoundName[0] != '\0')
        {
        if (!currentElement->GetScalarAttribute("id", &currentId))
          {
          continue;
          }
        vtkSMProxy* subProxy = locator->LocateProxy(currentId);
        if (subProxy)
          {
          this->AddSubProxy(compoundName, subProxy);
          }
        }
      }
    }

  // Initialise exposed properties --------------------------------------------
  vtkPVXMLElement* exposedProperties = NULL;
  exposedProperties = element->FindNestedElementByName("ExposedProperties");
  numElems =
    exposedProperties? exposedProperties->GetNumberOfNestedElements() : 0;
  for (unsigned int i=0; i < numElems; i++)
    {
    vtkPVXMLElement* currentElement = exposedProperties->GetNestedElement(i);
    if ( currentElement->GetName() &&
         strcmp(currentElement->GetName(), "Property") == 0)
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
  int index=0;
  numElems = element->GetNumberOfNestedElements();
  for (unsigned int i=0; i < numElems; i++)
    {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if ( currentElement->GetName() &&
         strcmp(currentElement->GetName(), "OutputPort") == 0)
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
        this->Internals->RegisterExposedPort(proxy_name, exposed_name, port_name);
        }
      else
        {
        this->Internals->RegisterExposedPort(proxy_name, exposed_name, index);
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
  iter = this->Internals->ExposedPorts.begin();
  vtkSMProxy* currentProxy;
  while( iter != this->Internals->ExposedPorts.end() )
    {
    currentProxy = this->GetSubProxy(iter->ProxyName.c_str());
    vtkSMSourceProxy* subProxy = vtkSMSourceProxy::SafeDownCast(currentProxy);

    if (!subProxy)
      {
      vtkErrorMacro("Failed to locate sub proxy with name "
                    << iter->ProxyName.c_str());
      iter++;
      continue;
      }

    if ( iter->HasPortIndex() ||
         ( subProxy->GetOutputPortIndex(iter->PortName.c_str())
           == VTK_UNSIGNED_INT_MAX ) )
      {
      if (subProxy->GetNumberOfOutputPorts() <= iter->PortIndex)
        {
        vtkErrorMacro("Failed to locate requested output port of subproxy "
                      << iter->ProxyName.c_str());
        iter++;
        continue;
        }
      }

    // We cannot create vtkSMOutputPort proxies right now, since that requires
    // that the input to this filter is setup correctly. Hence we wait for the
    // CreateOutputPorts() call to create the vtkSMOutputPort proxies.
    if(this->GetNumberOfOutputPorts() <= index)
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
