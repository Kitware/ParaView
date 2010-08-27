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
#include "vtkPMIntVectorProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkClientServerStream.h"

#include <assert.h>


vtkStandardNewMacro(vtkPMIntVectorProperty);
//----------------------------------------------------------------------------
vtkPMIntVectorProperty::vtkPMIntVectorProperty()
{
  this->ArgumentIsArray = false;
}

//----------------------------------------------------------------------------
vtkPMIntVectorProperty::~vtkPMIntVectorProperty()
{
}

//---------------------------------------------------------------------------
bool vtkPMIntVectorProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant *variant = &prop->value(0); // Only one type
  int num_elems = variant->integer_size();
  vtkstd::vector<int> values;
  values.resize(num_elems + 1);
  for (int cc=0; cc < num_elems; cc++)
    {
    values[cc] = variant->integer(cc);
    }

  return this->Push(&values[0], num_elems);
}

//---------------------------------------------------------------------------
bool vtkPMIntVectorProperty::Pull(vtkSMMessage*)
{
  return true;
}

//---------------------------------------------------------------------------
bool vtkPMIntVectorProperty::Push(int* values, int number_of_elements)
{
  if (this->InformationOnly || !this->Command)
    {
    return true;
    }

  vtkClientServerStream stream;
  vtkClientServerID objectId = this->GetVTKObjectID();

  if (this->CleanCommand)
    {
    stream << vtkClientServerStream::Invoke
      << objectId << this->CleanCommand
      << vtkClientServerStream::End;
    }

  if (this->SetNumberCommand)
    {
    stream << vtkClientServerStream::Invoke
         << objectId
         << this->SetNumberCommand
         << number_of_elements / this->NumberOfElementsPerCommand
         << vtkClientServerStream::End;
    }

  if (!this->Repeatable)
    {
    stream << vtkClientServerStream::Invoke << objectId << this->Command;
    if (this->ArgumentIsArray)
      {
      stream << vtkClientServerStream::InsertArray(values, number_of_elements);
      }
    else
      {
      for (int i=0; i<number_of_elements; i++)
        {
        stream << values[i];
        }
      }
    stream << vtkClientServerStream::End;
    }
  else
    {
    int numCommands = number_of_elements / this->NumberOfElementsPerCommand;
    for(int i=0; i<numCommands; i++)
      {
      stream << vtkClientServerStream::Invoke << objectId << this->Command;
      if (this->UseIndex)
        {
        stream << i;
        }
      if (this->ArgumentIsArray)
        {
        stream << vtkClientServerStream::InsertArray(
          &(values[i*this->NumberOfElementsPerCommand]),
          this->NumberOfElementsPerCommand);
        }
      else
        {
        for (int j=0; j<this->NumberOfElementsPerCommand; j++)
          {
          stream << values[i*this->NumberOfElementsPerCommand+j];
          }
        }
      stream << vtkClientServerStream::End;
      }
    }
  return this->ProcessMessage(stream);
}

//---------------------------------------------------------------------------
bool vtkPMIntVectorProperty::ReadXMLAttributes(
  vtkPMProxy* proxy, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxy, element))
    {
    return false;
    }

  int number_of_elements = 0;
  element->GetScalarAttribute("number_of_elements", &number_of_elements);

  int arg_is_array;
  if (element->GetScalarAttribute("argument_is_array", &arg_is_array))
    {
    this->ArgumentIsArray = (arg_is_array != 0);
    }

  if (number_of_elements > 0)
    {
    vtkstd::vector<int> values;
    values.resize(number_of_elements);
    if (element->GetAttribute("default_values") &&
      strcmp("none", element->GetAttribute("default_values")) == 0 )
      {
      // initialized to nothing.
      }
    else
      {
      int numRead = element->GetVectorAttribute("default_values",
        number_of_elements, &values[0]);
      if (numRead > 0)
        {
        if (numRead != number_of_elements)
          {
          vtkErrorMacro("The number of default values does not match the "
            "number of elements. Initialization failed.");
          return false;
          }
        }
      else
        {
        vtkErrorMacro("No default value is specified for property: "
          << this->GetXMLName()
          << ". This might lead to stability problems");
        return false;
        }
      }
    return this->Push(&values[0], number_of_elements);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkPMIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
