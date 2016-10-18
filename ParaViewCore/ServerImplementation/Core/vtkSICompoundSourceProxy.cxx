/*=========================================================================

  Program:   ParaView
  Module:    vtkSICompoundSourceProxy.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSICompoundSourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <sstream>
#include <vector>

#include <assert.h>

//*****************************************************************************
class vtkSICompoundSourceProxy::vtkInternals
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
    this->NeedOutputPortCreation = true;
  }

  void RegisterExposedPort(const char* proxyName, const char* exposedName, const char* portName)
  {
    PortInfo info;
    info.PortName = portName;
    info.ProxyName = proxyName;
    info.ExposedName = exposedName;
    this->ExposedPorts.push_back(info);
    this->NeedOutputPortCreation = true;
  }

  int GetNumberOfOutputPorts() { return static_cast<int>(this->ExposedPorts.size()); }

  typedef std::vector<PortInfo> VectorOfPortInfo;
  VectorOfPortInfo ExposedPorts;
  std::vector<vtkSmartPointer<vtkAlgorithmOutput> > OutputPorts;
  bool NeedOutputPortCreation;
};
//*****************************************************************************
vtkStandardNewMacro(vtkSICompoundSourceProxy);
//----------------------------------------------------------------------------
vtkSICompoundSourceProxy::vtkSICompoundSourceProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkSICompoundSourceProxy::~vtkSICompoundSourceProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkAlgorithmOutput* vtkSICompoundSourceProxy::GetOutputPort(int port)
{
  if (this->Internals->NeedOutputPortCreation)
  {
    this->CreateOutputPorts();
  }

  if (static_cast<int>(this->Internals->OutputPorts.size()) > port)
  {
    return this->Internals->OutputPorts[port];
  }

  return NULL;
}

//----------------------------------------------------------------------------
bool vtkSICompoundSourceProxy::CreateOutputPorts()
{
  if (this->Internals->NeedOutputPortCreation)
  {
    int ports = this->Internals->GetNumberOfOutputPorts();
    this->Internals->OutputPorts.resize(ports);

    for (int cc = 0; cc < ports; cc++)
    {
      vtkSISourceProxy* subProxy = vtkSISourceProxy::SafeDownCast(
        this->GetSubSIProxy(this->Internals->ExposedPorts[cc].ProxyName.c_str()));
      if (!subProxy)
      {
        vtkErrorMacro(
          "Failed to locate subproxy: " << this->Internals->ExposedPorts[cc].ProxyName.c_str());
        return false;
      }

      this->Internals->OutputPorts[cc] =
        subProxy->GetOutputPort(this->Internals->ExposedPorts[cc].PortIndex);
    }
    this->Internals->NeedOutputPortCreation = false;
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSICompoundSourceProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(element))
  {
    return false;
  }

  // Initialise exposed output port -------------------------------------------
  int index = 0;
  unsigned int numElems = element->GetNumberOfNestedElements();
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
        this->Internals->RegisterExposedPort(proxy_name, exposed_name, port_name);
      }
      else
      {
        this->Internals->RegisterExposedPort(proxy_name, exposed_name, index);
      }
    }
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkSICompoundSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
