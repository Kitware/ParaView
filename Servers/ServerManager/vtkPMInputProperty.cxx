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
#include "vtkPMInputProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMSourceProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"

#include <assert.h>

vtkStandardNewMacro(vtkPMInputProperty);
//----------------------------------------------------------------------------
vtkPMInputProperty::vtkPMInputProperty()
{
  this->PortIndex = 0;
}

//----------------------------------------------------------------------------
vtkPMInputProperty::~vtkPMInputProperty()
{
}

//----------------------------------------------------------------------------
bool vtkPMInputProperty::ReadXMLAttributes(
  vtkPMProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
    {
    return false;
    }

  int port_index;
  if (element->GetScalarAttribute("port_index", &port_index))
    {
    this->SetPortIndex(port_index);
    }
  return true;
}

//----------------------------------------------------------------------------
bool vtkPMInputProperty::Push(vtkSMMessage* message, int offset)
{
  if(!this->GetCommand())
    {
    // It is fine to have a property without command but then we do nothing
    // here at the server side...
    return true;
    }

  assert(message->ExtensionSize(ProxyState::property) > offset);
  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant *variant = &prop->value();
  assert(variant->proxy_global_id_size() == variant->port_number_size());

  vtkstd::vector<vtkTypeUInt32> proxy_ids;
  vtkstd::vector<int> output_ports;

  proxy_ids.resize(variant->proxy_global_id_size());
  output_ports.resize(proxy_ids.size());
  for (int cc=0; cc < variant->proxy_global_id_size(); cc++)
    {
    proxy_ids[cc] = variant->proxy_global_id(cc);
    output_ports[cc] = variant->port_number(cc);
    }

  vtkClientServerStream stream;
  vtkClientServerID objectId = this->GetVTKObjectID();
  if (this->CleanCommand)
    {
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->CleanCommand
      << vtkClientServerStream::End;
    }

  for (size_t cc=0; cc < proxy_ids.size(); cc++)
    {
    vtkPMSourceProxy* pmproxy = vtkPMSourceProxy::SafeDownCast(
      this->GetPMObject(proxy_ids[cc]));
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->GetCommand();
    if (this->PortIndex > 0)
      {
      stream << this->PortIndex;
      }
    stream
      << (pmproxy? pmproxy->GetOutputPortID(output_ports[cc]) : vtkClientServerID(0))
      << vtkClientServerStream::End;
    }

  if (this->NullOnEmpty && this->CleanCommand == NULL && proxy_ids.size() == 0)
    {
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->GetCommand()
      << vtkClientServerID(0)
      << vtkClientServerStream::End;
    }

  return this->ProcessMessage(stream);
}

//----------------------------------------------------------------------------
void vtkPMInputProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
