/*=========================================================================

  Program:   ParaView
  Module:    vtkSIStringVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSIStringVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"

#include <vector>
#include <string>
#include <assert.h>

class vtkSIStringVectorProperty::vtkVectorOfStrings :
  public std::vector <std::string> {};
class vtkSIStringVectorProperty::vtkVectorOfInts :
  public std::vector<int> {};

vtkStandardNewMacro(vtkSIStringVectorProperty);
//----------------------------------------------------------------------------
vtkSIStringVectorProperty::vtkSIStringVectorProperty()
{
  this->ElementTypes = new vtkVectorOfInts();
}

//----------------------------------------------------------------------------
vtkSIStringVectorProperty::~vtkSIStringVectorProperty()
{
  delete this->ElementTypes;
}

//---------------------------------------------------------------------------
bool vtkSIStringVectorProperty::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property *prop = &message->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  // Save to cache when pulled for collaboration
  this->SaveValueToCache(message, offset);

  const Variant *variant = &prop->value();
  int num_elems = variant->txt_size();
  vtkVectorOfStrings values;
  values.resize(num_elems);
  for (int cc=0; cc < num_elems; cc++)
    {
    values[cc] = variant->txt(cc).c_str();
    }
  return this->Push(values);
}

//---------------------------------------------------------------------------
bool vtkSIStringVectorProperty::Pull(vtkSMMessage* message)
{
  if (!this->InformationOnly)
    {
    return this->Superclass::Pull(message);
    }

  if (!this->GetCommand())
    {
    // I would say that we should return false since an InformationOnly property
    // as no meaning if no command is set, but for some legacy reason we just
    // skip the processing if no command is provided.
    return true;
    }

  // Invoke property's method on the root node of the server
  vtkClientServerStream str;
  str << vtkClientServerStream::Invoke
    << this->GetVTKObject() << this->GetCommand()
    << vtkClientServerStream::End;

  this->ProcessMessage(str);

  // Get the result
  const vtkClientServerStream& res = this->GetLastResult();

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
    {
    return true;
    }

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
    {
    return true;
    }

  // now add the single 'value' to the message.
  ProxyState_Property *prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* var = prop->mutable_value();
  var->set_type(Variant_Type_STRING);

  const char* arg = NULL;
  int retVal = res.GetArgument(0, 0, &arg);
  var->add_txt(arg ? arg : "Invalid result");

  return (retVal != 0);
}

//---------------------------------------------------------------------------
bool vtkSIStringVectorProperty::ReadXMLAttributes(
  vtkSIProxy* proxy, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(proxy, element))
    {
    return false;
    }

  int number_of_elements = 1; // By default there must be at least one element
  int number_of_elements_per_command = 0;
  element->GetScalarAttribute("number_of_elements", &number_of_elements);
  number_of_elements_per_command = number_of_elements;
  if (this->Repeatable)
    {
    number_of_elements_per_command = this->GetNumberOfElementsPerCommand();
    }
  this->ElementTypes->resize(number_of_elements_per_command, STRING);

  element->GetVectorAttribute("element_types",
    number_of_elements_per_command, &(*this->ElementTypes)[0]);
  vtkVectorOfStrings values;
  bool hasDefaultValues = false;
  if (number_of_elements > 0)
    {
    values.resize(number_of_elements);
    const char* tmp = element->GetAttribute("default_values");
    const char* delimiter = element->GetAttribute("default_values_delimiter");
    hasDefaultValues = (tmp != NULL);
    if (tmp && delimiter)
      {
      std::string initVal = tmp;
      std::string delim = delimiter;
      std::string::size_type pos1 = 0;
      std::string::size_type pos2 = 0;
      for (int i=0; i<number_of_elements && pos2 != std::string::npos; i++)
        {
        if (i != 0)
          {
          pos1 += delim.size();
          }
        pos2 = initVal.find(delimiter, pos1);
        std::string value = (pos1 == pos2) ? "" : initVal.substr(pos1, pos2-pos1);
        values[i] = value;
        pos1 = pos2;
        }
      }
    else if(tmp)
      {
      values[0] = tmp;
      }
    }

  // We only push if a default value has been set otherwise we might trigger
  // unwanted behaviour underneath.
  if(hasDefaultValues)
    {
    return this->Push(values);
    }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSIStringVectorProperty::Push(const vtkVectorOfStrings &values)
{
  if (this->InformationOnly || !this->Command)
    {
    return true;
    }

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();
  if (this->CleanCommand)
    {
    stream << vtkClientServerStream::Invoke
      << object << this->CleanCommand
      << vtkClientServerStream::End;
    }

  if (values.size() == 0)
    {
    return this->ProcessMessage(stream);
    }

  if (!this->Repeatable)
    {
    stream << vtkClientServerStream::Invoke << object << this->Command;

    if (this->InitialString)
      {
      stream << InitialString;
      }
      
    vtkVectorOfStrings::const_iterator iter;
    int i=0;
    for (iter = values.begin(); iter != values.end(); ++iter, ++i)
      {
      // Convert to the appropriate type and add to stream
      int type = (i < static_cast<int>(this->ElementTypes->size()))?
        (*this->ElementTypes)[i] : STRING;
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
        << object
        << this->SetNumberCommand
        << numCommands
        << vtkClientServerStream::End;
      }
    for (int i=0; i<numCommands; i++)
      {
      stream << vtkClientServerStream::Invoke << object << this->Command;
      if (this->UseIndex)
        {
        stream << i;
        }
      for (int j=0; j<this->NumberOfElementsPerCommand; j++)
        {
        // Convert to the appropriate type and add to stream
        int type = (j < static_cast<int>(this->ElementTypes->size()))?
          (*this->ElementTypes)[j] : STRING;
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
void vtkSIStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
