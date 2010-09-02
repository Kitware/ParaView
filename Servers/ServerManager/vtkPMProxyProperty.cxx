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
#include "vtkPMProxyProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPMProxy.h"
#include "vtkPVXMLElement.h"

#include <assert.h>

vtkStandardNewMacro(vtkPMProxyProperty);
//----------------------------------------------------------------------------
vtkPMProxyProperty::vtkPMProxyProperty()
{
  this->CleanCommand = 0;
  this->RemoveCommand = 0;
  this->NullOnEmpty = false;
}

//----------------------------------------------------------------------------
vtkPMProxyProperty::~vtkPMProxyProperty()
{
  this->SetCleanCommand(0);
  this->SetRemoveCommand(0);
}

//----------------------------------------------------------------------------
bool vtkPMProxyProperty::ReadXMLAttributes(
  vtkPMProxy* proxyhelper, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxyhelper, element))
    {
    return false;
    }

  const char* clean_command = element->GetAttribute("clean_command");
  this->SetCleanCommand(clean_command);

  const char* remove_command = element->GetAttribute("remove_command");
  this->SetRemoveCommand(remove_command);

  int null_on_empty;
  if (element->GetScalarAttribute("null_on_empty", &null_on_empty))
    {
    this->SetNullOnEmpty(null_on_empty);
    }

  if (this->InformationOnly)
    {
    vtkErrorMacro("InformationOnly proxy properties are not supported!");
    return false;
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkPMProxyProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);
  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant *variant = &prop->value();
  vtkstd::vector<vtkTypeUInt32> proxy_ids;
  proxy_ids.resize(variant->proxy_global_id_size());
  for (int cc=0; cc < variant->proxy_global_id_size(); cc++)
    {
    proxy_ids[cc] = variant->proxy_global_id(cc);
    }

  if (this->RemoveCommand)
    {
    // in all honesty, do we really need this anymore? It isn't a huge
    // performance bottleneck if all inputs to a filter are re-set every time
    // one changes.
    // FIXME handle remove command.
    abort();
    return false;
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
    vtkPMProxy* pmproxy = vtkPMProxy::SafeDownCast(
      this->GetPMObject(proxy_ids[cc]));
    stream << vtkClientServerStream::Invoke
      << objectId
      << this->GetCommand()
      << (pmproxy? pmproxy->GetVTKObjectID() : vtkClientServerID(0))
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
bool vtkPMProxyProperty::Pull(vtkSMMessage*)
{
  // since proxy-properties cannot be InformationOnly, return false, so that the
  // Proxy can simply return the cached property value, if any.
  return false;
}

//----------------------------------------------------------------------------
void vtkPMProxyProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
