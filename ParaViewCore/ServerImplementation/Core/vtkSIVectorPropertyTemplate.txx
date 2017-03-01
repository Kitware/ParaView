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
// ********* INT *************
vtkMaybeUnused("not used for non-int specializations") std::vector<int>& operator<<(
  std::vector<int>& values, const Variant& variant)
{
  int num_elems = variant.integer_size();
  values.resize(values.size() + num_elems);
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = variant.integer(cc);
  }
  return values;
}

vtkMaybeUnused("not used for non-int specializations") Variant& operator<<(
  Variant& variant, const std::vector<int>& values)
{
  variant.set_type(Variant::INT);
  std::vector<int>::const_iterator iter;
  for (iter = values.begin(); iter != values.end(); ++iter)
  {
    variant.add_integer(*iter);
  }
  return variant;
}

vtkMaybeUnused("not used for non-int specializations") bool operator<<(
  std::vector<int>& values, const vtkClientServerStream& stream)
{
  size_t cur_size = values.size();
  int argType = stream.GetArgumentType(0, 0);

  // If single value, all int types
  if (argType == vtkClientServerStream::int64_value ||
    argType == vtkClientServerStream::uint64_value ||
    argType == vtkClientServerStream::int32_value ||
    argType == vtkClientServerStream::int16_value || argType == vtkClientServerStream::int8_value ||
    argType == vtkClientServerStream::uint32_value ||
    argType == vtkClientServerStream::uint16_value ||
    argType == vtkClientServerStream::uint8_value || argType == vtkClientServerStream::bool_value)
  {
    int ires;
    int retVal = stream.GetArgument(0, 0, &ires);
    if (!retVal)
    {
      return false;
    }
    values.resize(cur_size + 1);
    values[cur_size] = ires;
    return true;
  }
  // if array, only 32 bit ints work
  else if (argType == vtkClientServerStream::int32_array)
  {
    vtkTypeUInt32 length;
    stream.GetArgumentLength(0, 0, &length);
    values.resize(cur_size + length);
    int retVal = stream.GetArgument(0, 0, &values[cur_size], length);
    if (!retVal)
    {
      values.resize(cur_size);
      return false;
    }
    return true;
  }

#define copy_to_vector(type)                                                                       \
  do                                                                                               \
  {                                                                                                \
    vtkTypeUInt32 length;                                                                          \
    stream.GetArgumentLength(0, 0, &length);                                                       \
    std::vector<type> tmp_vec(length);                                                             \
    int retVal = stream.GetArgument(0, 0, &tmp_vec[0], length);                                    \
    if (!retVal)                                                                                   \
    {                                                                                              \
      return false;                                                                                \
    }                                                                                              \
    values.resize(cur_size + length);                                                              \
    std::copy(tmp_vec.begin(), tmp_vec.end(), &values[cur_size]);                                  \
    return true;                                                                                   \
  } while (0)

  // match the type for uint32
  else if (argType == vtkClientServerStream::uint32_array)
  {
    copy_to_vector(vtkTypeUInt32);
  }
  // if 64 bit array, squash into 32 bit
  else if (argType == vtkClientServerStream::int64_array)
  {
    copy_to_vector(vtkTypeInt64);
  }
  else if (argType == vtkClientServerStream::uint64_array)
  {
    copy_to_vector(vtkTypeUInt64);
  }

#undef copy_to_vector

  return false;
}

// ********* DOUBLE *************
vtkMaybeUnused("not used for non-double specializations") std::vector<double>& operator<<(
  std::vector<double>& values, const Variant& variant)
{
  int num_elems = variant.float64_size();
  values.resize(values.size() + num_elems);
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = variant.float64(cc);
  }
  return values;
}

vtkMaybeUnused("not used for non-double specializations") Variant& operator<<(
  Variant& variant, const std::vector<double>& values)
{
  variant.set_type(Variant::FLOAT64);
  std::vector<double>::const_iterator iter;
  for (iter = values.begin(); iter != values.end(); ++iter)
  {
    variant.add_float64(*iter);
  }
  return variant;
}

vtkMaybeUnused("not used for non-double specializations") bool operator<<(
  std::vector<double>& values, const vtkClientServerStream& stream)
{
  size_t cur_size = values.size();
  int argType = stream.GetArgumentType(0, 0);

  // If single value, both float and double works
  if (argType == vtkClientServerStream::float64_value ||
    argType == vtkClientServerStream::float32_value)
  {
    double ires;
    int retVal = stream.GetArgument(0, 0, &ires);
    if (!retVal)
    {
      return false;
    }
    values.resize(cur_size + 1);
    values[cur_size] = ires;
  }
  // If array, handle 32 bit and 64 bit separately
  else if (argType == vtkClientServerStream::float64_array)
  {
    vtkTypeUInt32 length;
    stream.GetArgumentLength(0, 0, &length);
    values.resize(cur_size + length);
    int retVal = stream.GetArgument(0, 0, &values[cur_size], length);
    if (!retVal)
    {
      return false;
    }
  }
  else if (argType == vtkClientServerStream::float32_array)
  {
    vtkTypeUInt32 length;
    stream.GetArgumentLength(0, 0, &length);
    float* fvalues = new float[length + 1];
    int retVal = stream.GetArgument(0, 0, fvalues, length);
    if (!retVal)
    {
      delete[] fvalues;
      return false;
    }

    values.resize(cur_size + length);
    delete[] fvalues;
    std::copy(fvalues, fvalues + length, &values[cur_size]);
  }
  return false;
}

// ********* vtkIdType *************
vtkMaybeUnused("not used for non-vtkIdType specializations") void OperatorIdType(
  std::vector<vtkIdType>& values, const Variant& variant)
{
  int num_elems = variant.idtype_size();
  values.resize(values.size() + num_elems);
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = variant.idtype(cc);
  }
}
vtkMaybeUnused("not used for non-vtkIdType specializations") void OperatorIdType(
  Variant& variant, const std::vector<vtkIdType>& values)
{
  variant.set_type(Variant::IDTYPE);
  std::vector<vtkIdType>::const_iterator iter;
  for (iter = values.begin(); iter != values.end(); ++iter)
  {
    variant.add_idtype(*iter);
  }
}

vtkMaybeUnused("not used for non-vtkIdType specializations") bool OperatorIdType(
  std::vector<vtkIdType>& values, const vtkClientServerStream& stream)
{
  size_t cur_size = values.size();
  int argType = stream.GetArgumentType(0, 0);

  // If single value, all int types
  if (argType == vtkClientServerStream::int32_value ||
    argType == vtkClientServerStream::int64_value)
  {
    vtkIdType ires;
    int retVal = stream.GetArgument(0, 0, &ires);
    if (!retVal)
    {
      return false;
    }
    values.resize(cur_size + 1);
    values[cur_size] = ires;
    return true;
  }
  // if array, only 32 or 64 bit ints work
  else if (argType == vtkClientServerStream::int32_array ||
    argType == vtkClientServerStream::int64_value)
  {
    vtkTypeUInt32 length;
    stream.GetArgumentLength(0, 0, &length);
    values.resize(cur_size + length);
    int retVal = stream.GetArgument(0, 0, &values[cur_size], length);
    if (!retVal)
    {
      values.resize(cur_size);
      return false;
    }
    return true;
  }
  return false;
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
vtkMaybeUnused("not used for non-vtkIdType specializations") std::vector<vtkIdType>& operator<<(
  std::vector<vtkIdType>& values, const Variant& variant)
{
  OperatorIdType(values, variant);
  return values;
}

vtkMaybeUnused("not used for non-vtkIdType specializations") Variant& operator<<(
  Variant& variant, const std::vector<vtkIdType>& values)
{
  OperatorIdType(variant, values);
  return variant;
}

vtkMaybeUnused("not used for non-vtkIdType specializations") bool operator<<(
  std::vector<vtkIdType>& values, const vtkClientServerStream& stream)
{
  return OperatorIdType(values, stream);
}
#endif

// This absurdity is needed for cases where vtkIdType == int.
template <class TARG1, class TARG2, class force_idtype>
void AppendValues(TARG1& arg1, const TARG2& arg2, const force_idtype&)
{
  arg1 << arg2;
}

template <class TARG1, class TARG2>
void AppendValues(TARG1& arg1, const TARG2& arg2, const bool&)
{
  OperatorIdType(arg1, arg2);
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
  std::vector<T> values;

  AppendValues(values, *variant, force_idtype());

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
  AppendValues(values, res, force_idtype());

  // now add 'values' to the message.
  ProxyState_Property* prop = message->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());

  AppendValues(*prop->mutable_value(), values, force_idtype());
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
