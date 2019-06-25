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
#include "vtkSIVectorPropertyTemplate.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSIProxy.h"
#include "vtkSMMessage.h"

#include <assert.h>
#include <sstream>
#include <vector>

namespace
{
template <typename T, typename ForceIdType>
struct HelperTraits
{
};

template <typename ForceIdType>
struct HelperTraits<int, ForceIdType>
{
  static int size(const Variant& variant) { return variant.integer_size(); }
  static int get(const Variant& variant, int idx) { return variant.integer(idx); }
  static Variant_Type variant_type() { return Variant::INT; }
  static void append(Variant& variant, int value) { variant.add_integer(value); }
};

template <typename ForceIdType>
struct HelperTraits<double, ForceIdType>
{
  static int size(const Variant& variant) { return variant.float64_size(); }
  static double get(const Variant& variant, int idx) { return variant.float64(idx); }
  static Variant_Type variant_type() { return Variant::FLOAT64; }
  static void append(Variant& variant, double value) { variant.add_float64(value); }
};

template <>
struct HelperTraits<vtkIdType, bool>
{
  static int size(const Variant& variant) { return variant.idtype_size(); }
  static vtkIdType get(const Variant& variant, int idx) { return variant.idtype(idx); }
  static Variant_Type variant_type() { return Variant::IDTYPE; }
  static void append(Variant& variant, vtkIdType value) { variant.add_idtype(value); }
};

template <typename T, typename ForceIdType>
std::vector<T> VariantToVector(const Variant& variant)
{
  const int num_elems = HelperTraits<T, ForceIdType>::size(variant);
  std::vector<T> values(num_elems);
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = HelperTraits<T, ForceIdType>::get(variant, cc);
  }
  return values;
}

template <typename T, typename ForceIdType>
void VectorToVariant(const std::vector<T> values, Variant& variant)
{
  variant.set_type(HelperTraits<T, ForceIdType>::variant_type());
  for (const auto& v : values)
  {
    HelperTraits<T, ForceIdType>::append(variant, v);
  }
}

template <typename T, typename ForceIdType>
bool CSStreamToVector(
  const vtkClientServerStream& stream, const int msgIndex, std::vector<T>& values)
{
  const int numArgs = stream.GetNumberOfArguments(msgIndex);
  for (int argIdx = 0; argIdx < numArgs; ++argIdx)
  {
    vtkTypeUInt32 array_length;
    if (!stream.GetArgumentLength(msgIndex, argIdx, &array_length))
    {
      // GetArgumentLength will return failure for non-array types, in which
      // case we read the value directly.
      T val;
      if (stream.GetArgument(msgIndex, argIdx, &val))
      {
        values.push_back(val);
      }
      else
      {
        // type conversion failed -- let's report as error so it's easy to catch
        // such issues.
        return false;
      }
    }
    else if (array_length > 0)
    {
      const auto offset = values.size();
      values.resize(offset + array_length);
      if (!stream.GetArgument(msgIndex, argIdx, &values[offset], array_length))
      {
        // type conversion failed -- let's report as error so it's easy to catch
        // such issues.
        return false;
      }
    }
  }
  return true;
}
}

//----------------------------------------------------------------------------
template <class T, class force_idtype>
vtkSIVectorPropertyTemplate<T, force_idtype>::vtkSIVectorPropertyTemplate()
{
  this->ArgumentIsArray = false;
}

//----------------------------------------------------------------------------
template <class T, class force_idtype>
vtkSIVectorPropertyTemplate<T, force_idtype>::~vtkSIVectorPropertyTemplate()
{
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSIVectorPropertyTemplate<T, force_idtype>::Push(vtkSMMessage* message, int offset)
{
  assert(message->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property* prop = &message->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  // Save to cache when pulled for collaboration
  this->SaveValueToCache(message, offset);

  const Variant* variant = &prop->value();
  std::vector<T> values = VariantToVector<T, force_idtype>(*variant);
  return (values.size() > 0) ? this->Push(&values[0], static_cast<int>(values.size()))
                             : this->Push(static_cast<T*>(NULL), static_cast<int>(values.size()));
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSIVectorPropertyTemplate<T, force_idtype>::Pull(vtkSMMessage* message)
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

  int numArgs = res.GetNumberOfArguments(0);
  if (numArgs < 1)
  {
    return true;
  }

  std::vector<T> values;
  if (!CSStreamToVector<T, force_idtype>(res, 0, values))
  {
    vtkErrorMacro("'" << this->GetXMLName()
                      << "' failed to *pull* values. "
                         "May indicate a type mismatch between values returned by "
                         "the command and the values expected by the property.");
    values.clear();
  }

  // now add 'values' to the message.
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());

  VectorToVariant<T, force_idtype>(values, *prop->mutable_value());
  return true;
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSIVectorPropertyTemplate<T, force_idtype>::Push(T* values, int number_of_elements)
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

  if (this->SetNumberCommand)
  {
    stream << vtkClientServerStream::Invoke << object << this->SetNumberCommand;
    if (this->InitialString)
    {
      stream << this->InitialString;
    }
    stream << number_of_elements / this->NumberOfElementsPerCommand << vtkClientServerStream::End;
  }

  if (!this->Repeatable && number_of_elements > 0)
  {
    stream << vtkClientServerStream::Invoke << object << this->Command;
    if (this->InitialString)
    {
      stream << this->InitialString;
    }
    if (this->ArgumentIsArray)
    {
      stream << vtkClientServerStream::InsertArray(values, number_of_elements);
    }
    else
    {
      for (int i = 0; i < number_of_elements; i++)
      {
        stream << values[i];
      }
    }
    stream << vtkClientServerStream::End;
  }
  else if (this->Repeatable)
  {
    int numCommands = number_of_elements / this->NumberOfElementsPerCommand;
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
      if (this->ArgumentIsArray)
      {
        stream << vtkClientServerStream::InsertArray(
          &(values[i * this->NumberOfElementsPerCommand]), this->NumberOfElementsPerCommand);
      }
      else
      {
        for (int j = 0; j < this->NumberOfElementsPerCommand; j++)
        {
          stream << values[i * this->NumberOfElementsPerCommand + j];
        }
      }
      stream << vtkClientServerStream::End;
    }
  }
  return this->ProcessMessage(stream);
}

//---------------------------------------------------------------------------
template <class T, class force_idtype>
bool vtkSIVectorPropertyTemplate<T, force_idtype>::ReadXMLAttributes(
  vtkSIProxy* proxy, vtkPVXMLElement* element)
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
    std::vector<T> values;
    values.resize(number_of_elements);
    if (element->GetAttribute("default_values") &&
      strcmp("none", element->GetAttribute("default_values")) == 0)
    {
      // initialized to nothing.
      return true;
    }
    else
    {
      int numRead = element->GetVectorAttribute("default_values", number_of_elements, &values[0]);
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
          << this->GetXMLName() << ". This might lead to stability problems");
        return false;
      }
    }
    // don't push the values if the property in "internal".
    return this->GetIsInternal() ? true : this->Push(&values[0], number_of_elements);
  }
  return true;
}

//----------------------------------------------------------------------------
template <class T, class force_idtype>
void vtkSIVectorPropertyTemplate<T, force_idtype>::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
