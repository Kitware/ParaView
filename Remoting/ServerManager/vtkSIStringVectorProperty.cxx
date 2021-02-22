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
#include "vtkPVFileInformationHelper.h"
#include "vtkPVXMLElement.h"
#include "vtkSISourceProxy.h"
#include "vtkSMMessage.h"

#include <vtksys/SystemTools.hxx>

#include <assert.h>
#include <string>
#include <vector>

class vtkSIStringVectorProperty::vtkVectorOfStrings : public std::vector<std::string>
{
};
class vtkSIStringVectorProperty::vtkVectorOfInts : public std::vector<int>
{
};

vtkStandardNewMacro(vtkSIStringVectorProperty);
//----------------------------------------------------------------------------
vtkSIStringVectorProperty::vtkSIStringVectorProperty()
{
  this->ElementTypes = new vtkVectorOfInts();
  this->NeedReencoding = false;
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

  const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  // Save to cache when pulled for collaboration
  this->SaveValueToCache(message, offset);

  const Variant* variant = &prop->value();
  int num_elems = variant->txt_size();
  vtkVectorOfStrings values;
  values.resize(num_elems);
  for (int cc = 0; cc < num_elems; cc++)
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
  str << vtkClientServerStream::Invoke << this->GetVTKObject() << this->GetCommand()
      << vtkClientServerStream::End;

  this->ProcessMessage(str);

  // Get the result
  const vtkClientServerStream& res = this->GetLastResult();

  int numMsgs = res.GetNumberOfMessages();
  if (numMsgs < 1)
  {
    return true;
  }

  const int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
  {
    return true;
  }

  // now add the single 'value' to the message.
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* var = prop->mutable_value();
  var->set_type(Variant_Type_STRING);

  for (int argIdx = 0; argIdx < numArgs; ++argIdx)
  {
    const char* arg = nullptr;
    int retVal = res.GetArgument(0, argIdx, &arg);
    if (retVal == 0)
    {
      // failed to parse as string.
      return false;
    }

    if (!arg)
    {
      var->add_txt(std::string());
    }
    else
    {
      if (this->NeedReencoding)
      {
        // certain type of string needs to be converted to utf8
        // to be sent back to client
        var->add_txt(vtkPVFileInformationHelper::LocalToUtf8Win32(arg).c_str());
      }
      else
      {
        var->add_txt(arg);
      }
    }
  }
  return true;
}

//---------------------------------------------------------------------------
bool vtkSIStringVectorProperty::ReadXMLAttributes(vtkSIProxy* proxy, vtkPVXMLElement* element)
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

  const std::map<std::string, int> elementTypesStrMap = { { std::string("int"), INT },
    { std::string("double"), DOUBLE }, { std::string("str"), STRING } };

  // This fails if attributes are strings
  // In this case, we treat them by their name
  if (!element->GetVectorAttribute(
        "element_types", number_of_elements_per_command, &(*this->ElementTypes)[0]) &&
    element->GetAttribute("element_types") != nullptr)
  {
    std::string element_types = element->GetAttribute("element_types");
    std::vector<std::string> parts = vtksys::SystemTools::SplitString(element_types, ' ');
    if (parts.size() == this->ElementTypes->size())
    {
      for (std::size_t i = 0; i < parts.size(); ++i)
      {
        auto element_iter = elementTypesStrMap.find(parts[i]);
        if (element_iter == elementTypesStrMap.end())
        {
          vtkGenericWarningMacro("Element type " << parts[i] << " does not exists");
        }
        else
        {
          (*this->ElementTypes)[i] = element_iter->second;
        }
      }
    }
  }
  vtkVectorOfStrings values;
  bool hasDefaultValues = false;
  if (number_of_elements > 0)
  {
    values.resize(number_of_elements);
    const char* tmp = element->GetAttribute("default_values");
    const char* delimiter = element->GetAttribute("default_values_delimiter");
    hasDefaultValues = (tmp != nullptr);
    if (tmp && delimiter)
    {
      std::string initVal = tmp;
      std::string delim = delimiter;
      std::string::size_type pos1 = 0;
      std::string::size_type pos2 = 0;
      for (int i = 0; i < number_of_elements && pos2 != std::string::npos; i++)
      {
        if (i != 0)
        {
          pos1 += delim.size();
        }
        pos2 = initVal.find(delimiter, pos1);
        std::string value = (pos1 == pos2) ? "" : initVal.substr(pos1, pos2 - pos1);
        values[i] = value;
        pos1 = pos2;
      }
    }
    else if (tmp)
    {
      values[0] = tmp;
    }
  }

  // Detect if this property needs utf8 -> Local encoding conversion
  // Only filenames or filepaths need it.
  // It can be a StringVectorProperty called FileName (all writers, see writers.xml)
  // Or FileNameInfo for information property.
  // Or it can be a StringVectorProperty with a fileListDomain.
  const char* name = element->GetAttributeOrEmpty("name");
  if (strcmp(name, "FileName") == 0 || strcmp(name, "FileNameInfo") == 0)
  {
    this->NeedReencoding = true;
  }
  else
  {
    for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); i++)
    {
      vtkPVXMLElement* nested = element->GetNestedElement(i);
      if (strcmp(nested->GetName(), "FileListDomain") == 0)
      {
        this->NeedReencoding = true;
      }
    }
  }

  // We only push if a default value has been set otherwise we might trigger
  // unwanted behaviour underneath.
  if (hasDefaultValues)
  {
    return this->Push(values);
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSIStringVectorProperty::Push(const vtkVectorOfStrings& values)
{
  if (this->InformationOnly || !this->Command)
  {
    return true;
  }

  vtkClientServerStream stream;
  vtkObjectBase* object = this->GetVTKObject();
  if (this->CleanCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->CleanCommand;
    if (this->InitialString)
    {
      stream << this->InitialString;
    }
    stream << vtkClientServerStream::End;
  }

  if (!this->Repeatable)
  {
    stream << vtkClientServerStream::Invoke << object << this->Command;

    if (this->InitialString)
    {
      stream << InitialString;
    }

    vtkVectorOfStrings::const_iterator iter;
    int i = 0;
    for (iter = values.begin(); iter != values.end(); ++iter, ++i)
    {
      // Convert to the appropriate type and add to stream
      int type =
        (i < static_cast<int>(this->ElementTypes->size())) ? (*this->ElementTypes)[i] : STRING;
      switch (type)
      {
        case INT:
          stream << atoi(values[i].c_str());
          break;
        case DOUBLE:
          stream << atof(values[i].c_str());
          break;
        case STRING:
          if (this->NeedReencoding)
          {
            // Some received strings are UTF8 encoded and need conversion to local
            stream << vtkPVFileInformationHelper::Utf8ToLocalWin32(values[i]).c_str();
          }
          else
          {
            stream << values[i].c_str();
          }
          break;
      }
    }
    stream << vtkClientServerStream::End;
  }
  else
  {
    int numCommands = static_cast<int>(values.size()) / this->NumberOfElementsPerCommand;
    if (this->SetNumberCommand)
    {
      stream << vtkClientServerStream::Invoke << object << this->SetNumberCommand;
      if (this->InitialString)
      {
        stream << this->InitialString;
      }
      stream << numCommands << vtkClientServerStream::End;
    }
    for (int i = 0; i < numCommands; i++)
    {
      stream << vtkClientServerStream::Invoke << object << this->Command;
      if (this->InitialString)
      {
        stream << this->InitialString;
      }
      if (this->UseIndex)
      {
        stream << i;
      }
      for (int j = 0; j < this->NumberOfElementsPerCommand; j++)
      {
        // Convert to the appropriate type and add to stream
        int type =
          (j < static_cast<int>(this->ElementTypes->size())) ? (*this->ElementTypes)[j] : STRING;
        switch (type)
        {
          case INT:
            stream << atoi(values[i * this->NumberOfElementsPerCommand + j].c_str());
            break;
          case DOUBLE:
            stream << atof(values[i * this->NumberOfElementsPerCommand + j].c_str());
            break;
          case STRING:
            if (this->NeedReencoding)
            {
              // Some received strings are UTF8 encoded and need conversion to local
              stream << vtkPVFileInformationHelper::Utf8ToLocalWin32(
                          values[i * this->NumberOfElementsPerCommand + j])
                          .c_str();
            }
            else
            {
              stream << values[i * this->NumberOfElementsPerCommand + j].c_str();
            }
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
