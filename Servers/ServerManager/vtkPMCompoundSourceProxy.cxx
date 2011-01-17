/*=========================================================================

  Program:   ParaView
  Module:    vtkPMCompoundSourceProxy

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPMCompoundSourceProxy.h"

#include "vtkAlgorithm.h"
#include "vtkClientServerInterpreter.h"
#include "vtkClientServerStream.h"
#include "vtkInformation.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtentTranslator.h"
#include "vtkPVXMLElement.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtkstd/vector>
#include <vtksys/ios/sstream>

#include <assert.h>

//*****************************************************************************
class vtkPMCompoundSourceProxy::vtkInternals
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

  int GetNumberOfOutputPorts()
    {
    return this->ExposedPorts.size();
    }

  typedef vtkstd::vector<PortInfo> VectorOfPortInfo;
  VectorOfPortInfo ExposedPorts;
  vtkstd::vector<vtkClientServerID> OutputPortIDs;
  bool NeedOutputPortCreation;
};
//*****************************************************************************
vtkStandardNewMacro(vtkPMCompoundSourceProxy);
//----------------------------------------------------------------------------
vtkPMCompoundSourceProxy::vtkPMCompoundSourceProxy()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkPMCompoundSourceProxy::~vtkPMCompoundSourceProxy()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPMCompoundSourceProxy::GetOutputPortID(int port)
{
  if(this->Internals->NeedOutputPortCreation)
    {
    this->CreateOutputPorts();
    }

  if (static_cast<int>(this->Internals->OutputPortIDs.size()) > port)
    {
    return this->Internals->OutputPortIDs[port];
    }

  return vtkClientServerID();
}

//----------------------------------------------------------------------------
bool vtkPMCompoundSourceProxy::CreateOutputPorts()
{
  if(this->Internals->NeedOutputPortCreation)
    {
    int ports = this->Internals->GetNumberOfOutputPorts();
    this->Internals->OutputPortIDs.resize(ports, vtkClientServerID(0));

    vtkClientServerStream stream;
    for (int cc=0; cc < ports; cc++)
      {
      vtkClientServerID portID = this->Interpreter->GetNextAvailableId();
      this->Internals->OutputPortIDs[cc] = portID;

      vtkPMProxy* algo =
          this->GetSubProxyHelper(
              this->Internals->ExposedPorts[cc].ProxyName.c_str());

      stream << vtkClientServerStream::Invoke
             << algo->GetVTKObject()
             << "GetOutputPort"
             << this->Internals->ExposedPorts[cc].PortIndex
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Assign << portID
             << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    this->Interpreter->ProcessStream(stream);
    this->Internals->NeedOutputPortCreation = false;
    }

  return true;
}


//----------------------------------------------------------------------------
bool vtkPMCompoundSourceProxy::ReadXMLAttributes(vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(element))
    {
    return false;
    }

  // Initialise exposed output port -------------------------------------------
  int index=0;
  unsigned int numElems = element->GetNumberOfNestedElements();
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
  return true;
}

//----------------------------------------------------------------------------
void vtkPMCompoundSourceProxy::UpdateInformation()
{
  for (unsigned int cc=0; cc < this->GetNumberOfSubProxyHelpers(); cc++)
    {
    vtkPMSourceProxy* subproxy = vtkPMSourceProxy::SafeDownCast(
      this->GetSubProxyHelper(cc));
    if (subproxy)
      {
      subproxy->UpdateInformation();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPMCompoundSourceProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
