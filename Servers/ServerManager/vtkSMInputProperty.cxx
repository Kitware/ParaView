/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInputProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyIterator.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyIterator.h"
#include "vtkSMProxyLocator.h"

#include <vtkstd/vector>

vtkStandardNewMacro(vtkSMInputProperty);


struct vtkSMInputPropertyInternals
{
  vtkstd::vector<unsigned int> OutputPorts;
  vtkstd::vector<unsigned int> UncheckedOutputPorts;
};

//---------------------------------------------------------------------------
vtkSMInputProperty::vtkSMInputProperty()
{
  this->MultipleInput = 0;
  this->PortIndex = 0;

  this->IPInternals = new vtkSMInputPropertyInternals;
}

//---------------------------------------------------------------------------
vtkSMInputProperty::~vtkSMInputProperty()
{
  delete this->IPInternals;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::WriteTo(vtkSMMessage* message)
{
  ProxyState_Property *prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *var = prop->mutable_value();
  var->set_type(Variant::INPUT);
  for (unsigned int i=0; i<this->GetNumberOfProxies(); i++)
    {
    var->add_proxy_global_id(this->GetProxy(i)->GetGlobalID());
    var->add_port_number(this->GetOutputPortForConnection(i));
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::ReadFrom(vtkSMMessage* message, int message_offset)
{
  (void) message;
  (void) message_offset;
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                          vtkPVXMLElement* element)
{
  this->Superclass::ReadXMLAttributes(parent, element);

  int multiple_input;
  int retVal = element->GetScalarAttribute("multiple_input", &multiple_input);
  if(retVal) 
    { 
    this->SetMultipleInput(multiple_input); 
    this->Repeatable = multiple_input;
    }

  int port_index;
  retVal = element->GetScalarAttribute("port_index", &port_index);
  if(retVal) 
    { 
    this->SetPortIndex(port_index); 
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MultipleInput: " << this->MultipleInput << endl;
  os << indent << "PortIndex: " << this->PortIndex << endl;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetNumberOfProxies(unsigned int num)
{
  if (num != 0)
    {
    this->IPInternals->OutputPorts.resize(num);
    }
  else
    {
    this->IPInternals->OutputPorts.clear();
    }

  this->Superclass::SetNumberOfProxies(num);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetProxies(unsigned int numProxies,
  vtkSMProxy* proxies[], unsigned int outputports[])
{
  this->IPInternals->OutputPorts.clear();
  for (unsigned int cc=0; cc < numProxies; cc++)
    {
    this->IPInternals->OutputPorts.push_back(outputports[cc]);
    }

  this->Superclass::SetProxies(numProxies, proxies);
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::AddInputConnection(vtkSMProxy* proxy, 
                                           unsigned int outputPort,
                                           int modify)
{
  if (this->IPInternals->OutputPorts.size() != 
      this->GetNumberOfProxies())
    {
    this->IPInternals->OutputPorts.resize(
      this->GetNumberOfProxies());
    }
  this->IPInternals->OutputPorts.push_back(outputPort);

  int retval = this->AddProxy(proxy, modify);
  if (retval)
    {
    if (modify)
      {
      this->Modified();
      }
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
int vtkSMInputProperty::SetInputConnection(unsigned int idx, 
                                           vtkSMProxy* proxy, 
                                           unsigned int outputPort)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.resize(idx+1);
    }
  this->IPInternals->OutputPorts[idx] = outputPort;

  int retval = this->SetProxy(idx, proxy);
  if (retval)
    {
    return retval;
    }
  else
    {
    return 0;
    }
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::AddUncheckedInputConnection(vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (this->IPInternals->UncheckedOutputPorts.size() != 
      this->GetNumberOfUncheckedProxies())
    {
    this->IPInternals->UncheckedOutputPorts.resize(
      this->GetNumberOfUncheckedProxies());
    }
  this->IPInternals->UncheckedOutputPorts.push_back(outputPort);

  this->AddUncheckedProxy(proxy);
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::SetUncheckedInputConnection(unsigned int idx, 
                                                    vtkSMProxy* proxy, 
                                                     unsigned int outputPort)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.resize(idx+1);
    }
  this->IPInternals->UncheckedOutputPorts[idx] = outputPort;

  this->SetUncheckedProxy(idx, proxy);
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveProxy(vtkSMProxy* proxy, int modify)
{
  unsigned int idx =
    this->Superclass::RemoveProxy(proxy, modify);
  if (idx < this->IPInternals->OutputPorts.size())
    {
    this->IPInternals->OutputPorts.erase(
      this->IPInternals->OutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::RemoveUncheckedProxy(vtkSMProxy* proxy)
{
  unsigned int idx =
    this->Superclass::RemoveUncheckedProxy(proxy);
  if (idx < this->IPInternals->UncheckedOutputPorts.size())
    {
    this->IPInternals->UncheckedOutputPorts.erase(
      this->IPInternals->UncheckedOutputPorts.begin() + idx);
    }
  return idx;
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllUncheckedProxies()
{
  this->IPInternals->UncheckedOutputPorts.clear();

  this->Superclass::RemoveAllUncheckedProxies();
}

//---------------------------------------------------------------------------
void vtkSMInputProperty::RemoveAllProxies(int modify)
{
  this->IPInternals->OutputPorts.clear();

  this->Superclass::RemoveAllProxies(modify);
  if (modify)
    {
    this->Modified();
    }
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetOutputPortForConnection(unsigned int idx)
{
  if (idx >= this->IPInternals->OutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->OutputPorts[idx];
}

//---------------------------------------------------------------------------
unsigned int vtkSMInputProperty::GetUncheckedOutputPortForConnection(
  unsigned int idx)
{
  if (idx >= this->IPInternals->UncheckedOutputPorts.size())
    {
    return 0;
    }
  return this->IPInternals->UncheckedOutputPorts[idx];
}

