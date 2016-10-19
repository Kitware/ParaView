/*=========================================================================

  Program:   ParaView
  Module:    vtkSIDoubleMapProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIDoubleMapProperty.h"
#include "vtkObjectFactory.h"

#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include <cassert>

vtkStandardNewMacro(vtkSIDoubleMapProperty);
//----------------------------------------------------------------------------
vtkSIDoubleMapProperty::vtkSIDoubleMapProperty()
{
  this->CleanCommand = 0;
}

//----------------------------------------------------------------------------
vtkSIDoubleMapProperty::~vtkSIDoubleMapProperty()
{
  this->SetCleanCommand(0);
}

//----------------------------------------------------------------------------
bool vtkSIDoubleMapProperty::Push(vtkSMMessage* message, int offset)
{
  if (this->InformationOnly || !this->Command)
  {
    return true;
  }

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();

  if (this->CleanCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->CleanCommand
           << vtkClientServerStream::End;
  }

  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  // Save to cache when pulled for collaboration
  this->SaveValueToCache(message, offset);

  const Variant* variant = &prop->value();

  std::vector<double> values(variant->float64_size());
  for (size_t i = 0; i < values.size(); i++)
  {
    values[i] = variant->float64(static_cast<int>(i));
  }

  int size = variant->idtype_size();
  for (int i = 0; i < size; i++)
  {
    vtkIdType id = variant->idtype(i);

    stream << vtkClientServerStream::Invoke << object << this->Command << id
           << vtkClientServerStream::InsertArray(
                &values[i * this->NumberOfComponents], this->NumberOfComponents)
           << vtkClientServerStream::End;
  }

  return this->ProcessMessage(stream);
}

//---------------------------------------------------------------------------
bool vtkSIDoubleMapProperty::ReadXMLAttributes(vtkSIProxy* parent, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(parent, element))
  {
    return false;
  }

  int number_of_components = 0;
  if (element->GetScalarAttribute("number_of_components", &number_of_components))
  {
    this->NumberOfComponents = number_of_components;
  }

  this->SetCleanCommand(element->GetAttribute("clean_command"));

  return true;
}

//----------------------------------------------------------------------------
void vtkSIDoubleMapProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
