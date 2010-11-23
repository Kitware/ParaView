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
#include "vtkPMStringVectorProperty.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkClientServerStream.h"

#include <assert.h>


vtkStandardNewMacro(vtkPMStringVectorProperty);
//----------------------------------------------------------------------------
vtkPMStringVectorProperty::vtkPMStringVectorProperty()
{
}

//----------------------------------------------------------------------------
vtkPMStringVectorProperty::~vtkPMStringVectorProperty()
{
}

//---------------------------------------------------------------------------
bool vtkPMStringVectorProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant *variant = &prop->value();
  int num_elems = variant->txt_size();
  vtkstd::vector<vtkstd::string> values;
  values.resize(num_elems);
  for (int cc=0; cc < num_elems; cc++)
    {
    values[cc] = variant->txt(cc).c_str();
    }
  return this->Push(values);
}

//---------------------------------------------------------------------------
bool vtkPMStringVectorProperty::Pull(vtkSMMessage*)
{
  return true;
}

//---------------------------------------------------------------------------
bool vtkPMStringVectorProperty::ReadXMLAttributes(
  vtkPMProxy* proxy, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxy, element))
    {
    return false;
    }

  int number_of_elements = 0;
  int number_of_elements_per_command = 0;
  element->GetScalarAttribute("number_of_elements", &number_of_elements);
  number_of_elements_per_command = number_of_elements;
  if (this->Repeatable)
    {
    number_of_elements_per_command = this->GetNumberOfElementsPerCommand();
    }
  this->ElementTypes.resize(number_of_elements_per_command, STRING);

  element->GetVectorAttribute("element_types",
    number_of_elements_per_command, &this->ElementTypes[0]);
  vtkstd::vector<vtkstd::string> values;
  if (number_of_elements > 0)
    {
    values.resize(number_of_elements);
    const char* tmp = element->GetAttribute("default_values");
    const char* delimiter = element->GetAttribute("default_values_delimiter");
    if (tmp && delimiter)
      {
      vtkstd::string initVal = tmp;
      vtkstd::string delim = delimiter;
      vtkstd::string::size_type pos1 = 0;
      vtkstd::string::size_type pos2 = 0;
      for (int i=0; i<number_of_elements && pos2 != vtkstd::string::npos; i++)
        {
        if (i != 0)
          {
          pos1 += delim.size();
          }
        pos2 = initVal.find(delimiter, pos1);
        vtkstd::string value = (pos1 == pos2) ? "" : initVal.substr(pos1, pos2-pos1);
        values[i] = value;
        pos1 = pos2;
        }
      }
    else if(tmp)
      {
      values[0] = tmp;
      }
    }

  return this->Push(values);
}

//----------------------------------------------------------------------------
bool vtkPMStringVectorProperty::Push(const vtkstd::vector<vtkstd::string> &values)
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

  if (values.size() == 0)
    {
    return this->ProcessMessage(stream);
    }

  if (!this->Repeatable)
    {
    stream << vtkClientServerStream::Invoke << objectId << this->Command;
    vtkstd::vector<vtkstd::string>::const_iterator iter;
    int i=0;
    for (iter = values.begin(); iter != values.end(); ++iter, ++i)
      {
      // Convert to the appropriate type and add to stream
      int type = (i < static_cast<int>(this->ElementTypes.size()))?
        this->ElementTypes[i] : STRING;
      switch (type)
        {
      case INT:
        stream << atoi(values[i].c_str());
        break;
      case DOUBLE:
        stream << atof(values[i].c_str());
        break;
      case STRING:
        stream << values[i].c_str();
        break;
        }
      }
    stream << vtkClientServerStream::End;
    }
  else
    {
    int numCommands = values.size() / this->NumberOfElementsPerCommand;
    if (this->SetNumberCommand)
      {
      stream << vtkClientServerStream::Invoke
        << objectId
        << this->SetNumberCommand
        << numCommands
        << vtkClientServerStream::End;
      }
    for (int i=0; i<numCommands; i++)
      {
      stream << vtkClientServerStream::Invoke << objectId << this->Command;
      if (this->UseIndex)
        {
        stream << i;
        }
      for (int j=0; j<this->NumberOfElementsPerCommand; j++)
        {
        // Convert to the appropriate type and add to stream
        int type = (j < static_cast<int>(this->ElementTypes.size()))?
          this->ElementTypes[j] : STRING;
        switch (type)
          {
        case INT:
          stream << atoi(values[i*this->NumberOfElementsPerCommand+j].c_str());
          break;
        case DOUBLE:
          stream << atof(values[i*this->NumberOfElementsPerCommand+j].c_str());
          break;
        case STRING:
          stream << values[i*this->NumberOfElementsPerCommand+j].c_str();
          break;
          }
        }
      stream << vtkClientServerStream::End;
      }
    }

  return this->ProcessMessage(stream);
}

//----------------------------------------------------------------------------
void vtkPMStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
