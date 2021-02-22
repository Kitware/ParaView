/*=========================================================================

  Program:   ParaView
  Module:    vtkSMPropertyHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
* Copyright (c) 2007, Sandia Corporation
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the Sandia Corporation nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Sandia Corporation ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Sandia Corporation BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "vtkSMPropertyHelper.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMEnumerationDomain.h"
#include "vtkSMIdTypeVectorProperty.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringList.h"

#include <sstream>

#include <algorithm>
#include <cassert>
#include <cstdlib>

#define vtkSMPropertyHelperWarningMacro(blah)                                                      \
  if (this->Quiet == false)                                                                        \
  {                                                                                                \
    vtkGenericWarningMacro(blah);                                                                  \
  }

//----------------------------------------------------------------------------
template <typename T>
inline T vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  (void)index;

  return T();
}

//----------------------------------------------------------------------------
template <>
inline int vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  switch (this->Type)
  {
    case INT:
      return this->UseUnchecked ? this->IntVectorProperty->GetUncheckedElement(index)
                                : this->IntVectorProperty->GetElement(index);
    case DOUBLE:
      return static_cast<int>(this->UseUnchecked
          ? this->DoubleVectorProperty->GetUncheckedElement(index)
          : this->DoubleVectorProperty->GetElement(index));
    case IDTYPE:
      return this->UseUnchecked ? this->IdTypeVectorProperty->GetUncheckedElement(index)
                                : this->IdTypeVectorProperty->GetElement(index);
    case STRING:
      return this->UseUnchecked ? std::atoi(this->StringVectorProperty->GetUncheckedElement(index))
                                : std::atoi(this->StringVectorProperty->GetElement(index));

    default:
      return 0;
  }
}

//----------------------------------------------------------------------------
template <>
inline double vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  switch (this->Type)
  {
    case INT:
      return this->UseUnchecked ? this->IntVectorProperty->GetUncheckedElement(index)
                                : this->IntVectorProperty->GetElement(index);
    case DOUBLE:
      return this->UseUnchecked ? this->DoubleVectorProperty->GetUncheckedElement(index)
                                : this->DoubleVectorProperty->GetElement(index);
    case IDTYPE:
      return this->UseUnchecked ? this->IdTypeVectorProperty->GetUncheckedElement(index)
                                : this->IdTypeVectorProperty->GetElement(index);

    case STRING:
      return this->UseUnchecked ? std::atof(this->StringVectorProperty->GetUncheckedElement(index))
                                : std::atof(this->StringVectorProperty->GetElement(index));

    default:
      return 0;
  }
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
//----------------------------------------------------------------------------
template <>
inline vtkIdType vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  switch (this->Type)
  {
    case INT:
      return this->UseUnchecked ? this->IntVectorProperty->GetUncheckedElement(index)
                                : this->IntVectorProperty->GetElement(index);
    case IDTYPE:
      return this->UseUnchecked ? this->IdTypeVectorProperty->GetUncheckedElement(index)
                                : this->IdTypeVectorProperty->GetElement(index);
    default:
      return 0;
  }
}
#endif

//----------------------------------------------------------------------------
template <>
inline const char* vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  if (this->Type == STRING)
  {
    return this->UseUnchecked ? this->StringVectorProperty->GetUncheckedElement(index)
                              : this->StringVectorProperty->GetElement(index);
  }
  else if (this->Type == INT)
  {
    // enumeration domain
    auto domain = this->Property->FindDomain<vtkSMEnumerationDomain>();
    if (domain != nullptr)
    {
      const char* entry = domain->GetEntryTextForValue(
        (this->UseUnchecked ? this->IntVectorProperty->GetUncheckedElement(index)
                            : this->IntVectorProperty->GetElement(index)));
      if (entry)
      {
        return entry;
      }
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
template <>
inline vtkSMProxy* vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  switch (this->Type)
  {
    case PROXY:
    case INPUT:
      return this->UseUnchecked ? this->ProxyProperty->GetUncheckedProxy(index)
                                : this->ProxyProperty->GetProxy(index);
    default:
      return nullptr;
  }
}

//----------------------------------------------------------------------------
template <>
inline vtkVariant vtkSMPropertyHelper::GetProperty(unsigned int index) const
{
  switch (this->Type)
  {
    case INT:
      return this->UseUnchecked ? this->IntVectorProperty->GetUncheckedElement(index)
                                : this->IntVectorProperty->GetElement(index);
    case DOUBLE:
      return this->UseUnchecked ? this->DoubleVectorProperty->GetUncheckedElement(index)
                                : this->DoubleVectorProperty->GetElement(index);
    case IDTYPE:
      return this->UseUnchecked ? this->IdTypeVectorProperty->GetUncheckedElement(index)
                                : this->IdTypeVectorProperty->GetElement(index);
    case STRING:
      return this->UseUnchecked ? this->StringVectorProperty->GetUncheckedElement(index)
                                : this->StringVectorProperty->GetElement(index);
    case PROXY:
    case INPUT:
      return this->UseUnchecked ? this->ProxyProperty->GetUncheckedProxy(index)
                                : this->ProxyProperty->GetProxy(index);
    default:
      return vtkVariant();
  }
}

//----------------------------------------------------------------------------
template <typename T>
inline std::vector<T> vtkSMPropertyHelper::GetPropertyArray() const
{
  std::vector<T> array;

  for (unsigned int i = 0; i < this->GetNumberOfElements(); i++)
  {
    array.push_back(this->GetProperty<T>(i));
  }

  return array;
}

//----------------------------------------------------------------------------
template <typename T>
unsigned int vtkSMPropertyHelper::GetPropertyArray(T* values, unsigned int count) const
{
  count = std::min(count, this->GetNumberOfElements());

  for (unsigned int i = 0; i < count; i++)
  {
    values[i] = this->GetProperty<T>(i);
  }

  return count;
}

//----------------------------------------------------------------------------
template <typename T>
inline void vtkSMPropertyHelper::SetProperty(unsigned int index, T value)
{
  (void)index;
  (void)value;
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetProperty(unsigned int index, int value)
{
  switch (this->Type)
  {
    case INT:
      if (this->UseUnchecked)
      {
        this->IntVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->IntVectorProperty->SetElement(index, value);
      }
      break;
    case DOUBLE:
      if (this->UseUnchecked)
      {
        this->DoubleVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->DoubleVectorProperty->SetElement(index, value);
      }
      break;
    case IDTYPE:
      if (this->UseUnchecked)
      {
        this->IdTypeVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->IdTypeVectorProperty->SetElement(index, value);
      }
      break;
    case STRING:
    {
      std::ostringstream str;
      str << value;
      if (this->UseUnchecked)
      {
        this->StringVectorProperty->SetUncheckedElement(index, str.str().c_str());
      }
      else
      {
        this->StringVectorProperty->SetElement(index, str.str().c_str());
      }
    }
    break;
    default:
      break;
  }
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetProperty(unsigned int index, double value)
{
  switch (this->Type)
  {
    case INT:
      if (this->UseUnchecked)
      {
        this->IntVectorProperty->SetUncheckedElement(index, static_cast<int>(value));
      }
      else
      {
        this->IntVectorProperty->SetElement(index, static_cast<int>(value));
      }
      break;
    case DOUBLE:
      if (this->UseUnchecked)
      {
        this->DoubleVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->DoubleVectorProperty->SetElement(index, value);
      }
      break;
    default:
      break;
  }
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetProperty(unsigned int index, vtkIdType value)
{
  switch (this->Type)
  {
    case INT:
      if (this->UseUnchecked)
      {
        this->IntVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->IntVectorProperty->SetElement(index, value);
      }
      break;
    case IDTYPE:
      if (this->UseUnchecked)
      {
        this->IdTypeVectorProperty->SetUncheckedElement(index, value);
      }
      else
      {
        this->IdTypeVectorProperty->SetElement(index, value);
      }
      break;
    default:
      break;
  }
}
#endif

template <>
inline void vtkSMPropertyHelper::SetProperty(unsigned int index, const char* value)
{
  if (this->Type == STRING)
  {
    if (this->UseUnchecked)
    {
      this->StringVectorProperty->SetUncheckedElement(index, value);
    }
    else
    {
      this->StringVectorProperty->SetElement(index, value);
    }
  }
  else if (this->Type == INT)
  {
    // enumeration domain
    auto domain = this->Property->FindDomain<vtkSMEnumerationDomain>();
    if (domain != nullptr && domain->HasEntryText(value))
    {
      int valid; // We already know that the entry exist...
      if (this->UseUnchecked)
      {
        this->IntVectorProperty->SetUncheckedElement(index, domain->GetEntryValue(value, valid));
      }
      else
      {
        this->IntVectorProperty->SetElement(index, domain->GetEntryValue(value, valid));
      }
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
template <typename T>
inline void vtkSMPropertyHelper::SetPropertyArray(const T* values, unsigned int count)
{
  (void)values;
  (void)count;
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetPropertyArray(const int* values, unsigned int count)
{
  if (this->Type == INT)
  {
    if (this->UseUnchecked)
    {
      this->IntVectorProperty->SetUncheckedElements(values, count);
    }
    else
    {
      this->IntVectorProperty->SetElements(values, count);
    }
  }
  else if (this->Type == IDTYPE)
  {
    vtkIdType* temp = new vtkIdType[count + 1];
    for (unsigned int cc = 0; cc < count; cc++)
    {
      temp[cc] = static_cast<vtkIdType>(values[cc]);
    }
    this->SetPropertyArrayIdType(temp, count);
    delete[] temp;
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetPropertyArray(const double* values, unsigned int count)
{
  if (this->Type == DOUBLE)
  {
    if (this->UseUnchecked)
    {
      this->DoubleVectorProperty->SetUncheckedElements(values, count);
    }
    else
    {
      this->DoubleVectorProperty->SetElements(values, count);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
inline void vtkSMPropertyHelper::SetPropertyArrayIdType(const vtkIdType* values, unsigned int count)
{
  if (this->Type == IDTYPE)
  {
    if (this->UseUnchecked)
    {
      this->IdTypeVectorProperty->SetUncheckedElements(values, count);
    }
    else
    {
      this->IdTypeVectorProperty->SetElements(values, count);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::SetPropertyArray(const vtkIdType* values, unsigned int count)
{
  this->SetPropertyArrayIdType(values, count);
}
#endif

//----------------------------------------------------------------------------
template <typename T>
inline void vtkSMPropertyHelper::AppendPropertyArray(const T* values, unsigned int count)
{
  (void)values;
  (void)count;
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::AppendPropertyArray(const int* values, unsigned int count)
{
  if (this->Type == INT)
  {
    if (this->UseUnchecked)
    {
      this->IntVectorProperty->AppendUncheckedElements(values, count);
    }
    else
    {
      this->IntVectorProperty->AppendElements(values, count);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
template <>
inline void vtkSMPropertyHelper::AppendPropertyArray(const double* values, unsigned int count)
{
  if (this->Type == DOUBLE)
  {
    if (this->UseUnchecked)
    {
      this->DoubleVectorProperty->AppendUncheckedElements(values, count);
    }
    else
    {
      this->DoubleVectorProperty->AppendElements(values, count);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
template <>
inline void vtkSMPropertyHelper::AppendPropertyArray(const vtkIdType* values, unsigned int count)
{
  if (this->Type == IDTYPE)
  {
    if (this->UseUnchecked)
    {
      this->IdTypeVectorProperty->AppendUncheckedElements(values, count);
    }
    else
    {
      this->IdTypeVectorProperty->AppendElements(values, count);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}
#endif

//----------------------------------------------------------------------------
vtkSMPropertyHelper::vtkSMPropertyHelper(vtkSMProxy* proxy, const char* pname, bool quiet)
{
  this->Proxy = proxy;
  this->Quiet = quiet;

  // If pname is nullptr, on some platforms the warning macro can segfault
  assert(pname);

  vtkSMProperty* property = proxy->GetProperty(pname);

  if (!property)
  {
    vtkSMPropertyHelperWarningMacro("Failed to locate property: " << (pname ? pname : "(null)"));
  }

  this->Initialize(property);
}

//----------------------------------------------------------------------------
vtkSMPropertyHelper::vtkSMPropertyHelper(vtkSMProperty* property, bool quiet)
{
  this->Proxy = nullptr;
  this->Quiet = quiet;

  this->Initialize(property);
}

//----------------------------------------------------------------------------
vtkSMPropertyHelper::~vtkSMPropertyHelper() = default;

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Initialize(vtkSMProperty* property)
{
  this->Property = property;
  this->Type = vtkSMPropertyHelper::NONE;
  this->UseUnchecked = false;

  if (property != nullptr)
  {
    if (property->IsA("vtkSMIntVectorProperty"))
    {
      this->Type = vtkSMPropertyHelper::INT;
    }
    else if (property->IsA("vtkSMDoubleVectorProperty"))
    {
      this->Type = vtkSMPropertyHelper::DOUBLE;
    }
    else if (property->IsA("vtkSMIdTypeVectorProperty"))
    {
      this->Type = vtkSMPropertyHelper::IDTYPE;
    }
    else if (property->IsA("vtkSMStringVectorProperty"))
    {
      this->Type = vtkSMPropertyHelper::STRING;
    }
    else if (property->IsA("vtkSMInputProperty"))
    {
      this->Type = vtkSMPropertyHelper::INPUT;
    }
    else if (property->IsA("vtkSMProxyProperty"))
    {
      this->Type = vtkSMPropertyHelper::PROXY;
    }
    else
    {
      vtkSMPropertyHelperWarningMacro("Unhandled property type : " << property->GetClassName());
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::UpdateValueFromServer()
{
  if (this->Proxy)
  {
    this->Proxy->UpdatePropertyInformation(this->Property);
  }
  else
  {
    vtkGenericWarningMacro("No proxy set.");
  }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetNumberOfElements(unsigned int elems)
{
  switch (this->Type)
  {
    case INT:
    case DOUBLE:
    case IDTYPE:
    case STRING:
      if (this->UseUnchecked)
      {
        this->VectorProperty->SetNumberOfUncheckedElements(elems);
      }
      else
      {
        this->VectorProperty->SetNumberOfElements(elems);
      }
      break;
    case PROXY:
    case INPUT:
      if (this->UseUnchecked)
      {
        this->ProxyProperty->SetNumberOfUncheckedProxies(elems);
      }
      else
      {
        this->ProxyProperty->SetNumberOfProxies(elems);
      }
      break;
    default:
      vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::GetNumberOfElements() const
{
  switch (this->Type)
  {
    case INT:
    case DOUBLE:
    case IDTYPE:
    case STRING:
      if (this->UseUnchecked)
      {
        return this->VectorProperty->GetNumberOfUncheckedElements();
      }
      return this->VectorProperty->GetNumberOfElements();

    case PROXY:
    case INPUT:
      if (this->UseUnchecked)
      {
        return this->ProxyProperty->GetNumberOfUncheckedProxies();
      }
      return this->ProxyProperty->GetNumberOfProxies();
    default:
      vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
      return 0;
  }
}

//----------------------------------------------------------------------------
vtkVariant vtkSMPropertyHelper::GetAsVariant(unsigned int index) const
{
  return GetProperty<vtkVariant>(index);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, int value)
{
  this->SetProperty<int>(index, value);
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetAsInt(unsigned int index /*=0*/) const
{
  return this->GetProperty<int>(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(int* values, unsigned int count /*=1*/) const
{
  return this->GetPropertyArray<int>(values, count);
}

//----------------------------------------------------------------------------
std::vector<int> vtkSMPropertyHelper::GetIntArray() const
{
  return this->GetPropertyArray<int>();
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const int* values, unsigned int count)
{
  this->SetPropertyArray<int>(values, count);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Append(const int* values, unsigned int count)
{
  this->AppendPropertyArray(values, count);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, double value)
{
  this->SetProperty<double>(index, value);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Append(const double* values, unsigned int count)
{
  this->AppendPropertyArray(values, count);
}

//----------------------------------------------------------------------------
double vtkSMPropertyHelper::GetAsDouble(unsigned int index /*=0*/) const
{
  return this->GetProperty<double>(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(double* values, unsigned int count /*=1*/) const
{
  return this->GetPropertyArray<double>(values, count);
}

//----------------------------------------------------------------------------
std::vector<double> vtkSMPropertyHelper::GetDoubleArray() const
{
  return this->GetPropertyArray<double>();
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const double* values, unsigned int count)
{
  this->SetPropertyArray<double>(values, count);
}

#if VTK_SIZEOF_ID_TYPE != VTK_SIZEOF_INT
//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, vtkIdType value)
{
  this->SetProperty<vtkIdType>(index, value);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(const vtkIdType* values, unsigned int count)
{
  this->SetPropertyArray<vtkIdType>(values, count);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Append(const vtkIdType* values, unsigned int count)
{
  this->AppendPropertyArray(values, count);
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::Get(vtkIdType* values, unsigned int count /*=1*/) const
{
  return this->GetPropertyArray<vtkIdType>(values, count);
}
#endif

//----------------------------------------------------------------------------
vtkIdType vtkSMPropertyHelper::GetAsIdType(unsigned int index /*=0*/) const
{
  return this->GetProperty<vtkIdType>(index);
}

//----------------------------------------------------------------------------
std::vector<vtkIdType> vtkSMPropertyHelper::GetIdTypeArray() const
{
  return this->GetPropertyArray<vtkIdType>();
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, const char* value)
{
  this->SetProperty<const char*>(index, value);
}

//----------------------------------------------------------------------------
const char* vtkSMPropertyHelper::GetAsString(unsigned int index /*=0*/) const
{
  return this->GetProperty<const char*>(index);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(unsigned int index, vtkSMProxy* value, unsigned int outputport /*=0*/)
{
  if (this->Type == PROXY)
  {
    if (this->UseUnchecked)
    {
      this->ProxyProperty->SetUncheckedProxy(index, value);
    }
    else
    {
      this->ProxyProperty->SetProxy(index, value);
    }
  }
  else if (this->Type == INPUT)
  {
    if (this->UseUnchecked)
    {
      this->InputProperty->SetUncheckedInputConnection(index, value, outputport);
    }
    else
    {
      this->InputProperty->SetInputConnection(index, value, outputport);
    }
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Set(
  vtkSMProxy** value, unsigned int count, unsigned int* outputports /*=nullptr*/)
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return;
  }

  if (this->Type == PROXY)
  {
    this->ProxyProperty->SetProxies(count, value);
  }
  else if (this->Type == INPUT)
  {
    this->InputProperty->SetProxies(count, value, outputports);
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Add(vtkSMProxy* value, unsigned int outputport /*=0*/)
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return;
  }
  if (this->Type == PROXY)
  {
    this->ProxyProperty->AddProxy(value);
  }
  else if (this->Type == INPUT)
  {
    this->InputProperty->AddInputConnection(value, outputport);
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::Remove(vtkSMProxy* value)
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return;
  }
  if (this->Type == PROXY || this->Type == INPUT)
  {
    this->ProxyProperty->RemoveProxy(value);
  }
  else
  {
    vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkSMPropertyHelper::GetAsProxy(unsigned int index /*=0*/) const
{
  return this->GetProperty<vtkSMProxy*>(index);
}

//----------------------------------------------------------------------------
unsigned int vtkSMPropertyHelper::GetOutputPort(unsigned int index /*=0*/) const
{
  if (this->Type == INPUT)
  {
    if (this->UseUnchecked)
    {
      return this->InputProperty->GetUncheckedOutputPortForConnection(index);
    }
    else
    {
      return this->InputProperty->GetOutputPortForConnection(index);
    }
  }

  vtkSMPropertyHelperWarningMacro("Call not supported for the current property type.");
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, int value)
{
  std::ostringstream str;
  str << value;
  this->SetStatus(key, str.str().c_str());
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetStatus(const char* key, int default_value /*=0*/) const
{
  std::ostringstream str;
  str << default_value;
  const char* value = vtkSMPropertyHelper::GetStatus(key, str.str().c_str());
  return std::atoi(value);
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, double* values, int num_values)
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return;
  }

  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);
  if (svp->GetNumberOfElementsPerCommand() != num_values + 1)
  {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
    return;
  }

  if (!svp->GetRepeatCommand())
  {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
  }

  vtkStringList* list = vtkStringList::New();
  svp->GetElements(list);

  bool append = true;
  for (unsigned int cc = 0; (cc + num_values + 1) <= svp->GetNumberOfElements();
       cc += (num_values + 1))
  {
    if (strcmp(svp->GetElement(cc), key) == 0)
    {
      for (int kk = 0; kk < num_values; kk++)
      {
        std::ostringstream str;
        str << values[kk];
        list->SetString(cc + kk + 1, str.str().c_str());
      }
      append = false;
    }
  }

  if (append)
  {
    list->AddString(key);
    for (int kk = 0; kk < num_values; kk++)
    {
      std::ostringstream str;
      str << values[kk];
      list->AddString(str.str().c_str());
    }
  }
  svp->SetElements(list);
  list->Delete();
}

//----------------------------------------------------------------------------
bool vtkSMPropertyHelper::GetStatus(const char* key, double* values, int num_values) const
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return false;
  }

  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return false;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);

  while (svp)
  {
    if (svp->GetNumberOfElementsPerCommand() != num_values + 1)
    {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
      return false;
    }

    if (!svp->GetRepeatCommand())
    {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return false;
    }

    for (unsigned int cc = 0; (cc + num_values + 1) <= svp->GetNumberOfElements();
         cc += (num_values + 1))
    {
      if (strcmp(svp->GetElement(cc), key) == 0)
      {
        for (int kk = 0; kk < num_values; kk++)
        {
          values[kk] = atof(svp->GetElement(cc + kk + 1));
        }
        return true;
      }
    }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0
      ? vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty())
      : nullptr;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const int key, int* values, int num_values)
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return;
  }

  if (this->Type != vtkSMPropertyHelper::INT)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMIntVectorProperty.");
    return;
  }

  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(this->Property);
  if (svp->GetNumberOfElementsPerCommand() != num_values + 1)
  {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
    return;
  }

  if (!svp->GetRepeatCommand())
  {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
  }

  bool append = true;
  for (unsigned int cc = 0; (cc + num_values + 1) <= svp->GetNumberOfElements();
       cc += (num_values + 1))
  {
    if (svp->GetElement(cc) == key)
    {
      for (int kk = 0; kk < num_values; kk++)
      {
        svp->SetElement(cc + kk + 1, values[kk]);
      }
      append = false;
    }
  }

  if (append)
  {
    std::vector<int> list(svp->GetElements(), svp->GetElements() + svp->GetNumberOfElements());
    list.push_back(key);
    for (int kk = 0; kk < num_values; kk++)
    {
      list.push_back(values[kk]);
    }
    svp->SetElements(&list[0]);
  }
}

//----------------------------------------------------------------------------
bool vtkSMPropertyHelper::GetStatus(const int key, int* values, int num_values) const
{
  if (this->UseUnchecked)
  {
    // FIXME
    vtkSMPropertyHelperWarningMacro("Call not supported for unchecked values");
    return false;
  }

  if (this->Type != vtkSMPropertyHelper::INT)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMIntVectorProperty.");
    return false;
  }

  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(this->Property);

  while (svp)
  {
    if (svp->GetNumberOfElementsPerCommand() != num_values + 1)
    {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != " << num_values + 1);
      return false;
    }

    if (!svp->GetRepeatCommand())
    {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return false;
    }

    for (unsigned int cc = 0; (cc + num_values + 1) <= svp->GetNumberOfElements();
         cc += (num_values + 1))
    {
      if (svp->GetElement(cc) == key)
      {
        for (int kk = 0; kk < num_values; kk++)
        {
          values[kk] = svp->GetElement(cc + kk + 1);
        }
        return true;
      }
    }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0
      ? vtkSMIntVectorProperty::SafeDownCast(svp->GetInformationProperty())
      : nullptr;
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const char* key, const char* value)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);
  if (svp->GetNumberOfElementsPerCommand() != 2)
  {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
    return;
  }

  if (!svp->GetRepeatCommand())
  {
    vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
    return;
  }

  if (this->UseUnchecked)
  {
    for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfUncheckedElements(); cc += 2)
    {
      if (strcmp(svp->GetUncheckedElement(cc), key) == 0)
      {
        svp->SetUncheckedElement(cc + 1, value);
        return;
      }
    }
  }
  else
  {
    for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfElements(); cc += 2)
    {
      if (strcmp(svp->GetElement(cc), key) == 0)
      {
        svp->SetElement(cc + 1, value);
        return;
      }
    }
  }

  vtkStringList* list = vtkStringList::New();
  if (this->UseUnchecked)
  {
    svp->GetUncheckedElements(list);
  }
  else
  {
    svp->GetElements(list);
  }
  list->AddString(key);
  list->AddString(value);
  if (this->UseUnchecked)
  {
    svp->SetUncheckedElements(list);
  }
  else
  {
    svp->SetElements(list);
  }
  list->Delete();
}

//----------------------------------------------------------------------------
const char* vtkSMPropertyHelper::GetStatus(const char* key, const char* default_value) const
{
  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMStringVectorProperty.");
    return default_value;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);
  while (svp)
  {
    if (svp->GetNumberOfElementsPerCommand() != 2)
    {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
      return default_value;
    }

    if (!svp->GetRepeatCommand())
    {
      vtkSMPropertyHelperWarningMacro("Property is non-repeatable.");
      return default_value;
    }

    if (this->UseUnchecked)
    {
      for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfUncheckedElements(); cc += 2)
      {
        if (strcmp(svp->GetUncheckedElement(cc), key) == 0)
        {
          return svp->GetUncheckedElement(cc + 1);
        }
      }
    }
    else
    {
      for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfElements(); cc += 2)
      {
        if (strcmp(svp->GetElement(cc), key) == 0)
        {
          return svp->GetElement(cc + 1);
        }
      }
    }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0
      ? vtkSMStringVectorProperty::SafeDownCast(svp->GetInformationProperty())
      : nullptr;
  }

  return default_value;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetStatus(const int key, const int value)
{
  if (this->Type != vtkSMPropertyHelper::INT)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMIntVectorProperty.");
    return;
  }

  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(this->Property);
  if (svp->GetNumberOfElementsPerCommand() != 2)
  {
    vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
    return;
  }

  if (this->UseUnchecked)
  {
    for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfUncheckedElements(); cc += 2)
    {
      if (svp->GetUncheckedElement(cc) == key)
      {
        svp->SetUncheckedElement(cc + 1, value);
        return;
      }
    }
  }
  else
  {
    for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfElements(); cc += 2)
    {
      if (svp->GetElement(cc) == key)
      {
        svp->SetElement(cc + 1, value);
        return;
      }
    }
  }

  std::vector<int> list;
  if (this->UseUnchecked)
  {
    list.assign(
      svp->GetUnCheckedElements(), svp->GetUnCheckedElements() + svp->GetNumberOfElements());
  }
  else
  {
    list.assign(svp->GetElements(), svp->GetElements() + svp->GetNumberOfElements());
  }
  list.push_back(key);
  list.push_back(value);

  if (this->UseUnchecked)
  {
    svp->SetUncheckedElements(&list[0]);
  }
  else
  {
    svp->SetElements(&list[0]);
  }
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetStatus(const int key, const int default_value) const
{
  if (this->Type != vtkSMPropertyHelper::INT)
  {
    vtkSMPropertyHelperWarningMacro("Status properties can only be vtkSMIntVectorProperty.");
    return default_value;
  }

  vtkSMIntVectorProperty* svp = vtkSMIntVectorProperty::SafeDownCast(this->Property);
  while (svp)
  {
    if (svp->GetNumberOfElementsPerCommand() != 2)
    {
      vtkSMPropertyHelperWarningMacro("NumberOfElementsPerCommand != 2");
      return default_value;
    }

    if (this->UseUnchecked)
    {
      for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfUncheckedElements(); cc += 2)
      {
        if (svp->GetUncheckedElement(cc) == key)
        {
          return svp->GetUncheckedElement(cc + 1);
        }
      }
    }
    else
    {
      for (unsigned int cc = 0; (cc + 1) < svp->GetNumberOfElements(); cc += 2)
      {
        if (svp->GetElement(cc) == key)
        {
          return svp->GetElement(cc + 1);
        }
      }
    }

    // Now check if the information_property has the value.
    svp = svp->GetInformationOnly() == 0
      ? vtkSMIntVectorProperty::SafeDownCast(svp->GetInformationProperty())
      : nullptr;
  }

  return default_value;
}

//----------------------------------------------------------------------------
void vtkSMPropertyHelper::SetInputArrayToProcess(int fieldAssociation, const char* arrayName)
{
  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro(
      "Property for 'InputArrayToProcess' can only be vtkSMStringVectorProperty.");
    return;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);

  if (svp->GetNumberOfElements() != 2 && svp->GetNumberOfElements() != 5)
  {
    vtkSMPropertyHelperWarningMacro("We only support 2 or 5 element properties.");
    return;
  }

  std::ostringstream str;
  str << fieldAssociation;

  vtkNew<vtkStringList> vals;
  svp->GetElements(vals.GetPointer());
  if (svp->GetNumberOfElements() == 2)
  {
    vals->SetString(0, str.str().c_str());
    vals->SetString(1, (arrayName ? arrayName : ""));
  }
  else
  {
    vals->SetString(3, str.str().c_str());
    vals->SetString(4, (arrayName ? arrayName : ""));
  }

  if (this->GetUseUnchecked())
  {
    svp->SetUncheckedElements(vals.GetPointer());
  }
  else
  {
    svp->SetElements(vals.GetPointer());
  }
}

//----------------------------------------------------------------------------
int vtkSMPropertyHelper::GetInputArrayAssociation() const
{
  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro(
      "Property for 'InputArrayToProcess' can only be vtkSMStringVectorProperty.");
    return -1;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);

  if (this->GetUseUnchecked())
  {
    if (svp->GetNumberOfUncheckedElements() != 2 && svp->GetNumberOfUncheckedElements() != 5)
    {
      vtkSMPropertyHelperWarningMacro("We only support 2 or 5 element properties.");
      return -1;
    }

    return svp->GetNumberOfUncheckedElements() == 2 ? std::atoi(svp->GetUncheckedElement(0))
                                                    : std::atoi(svp->GetUncheckedElement(3));
  }
  else
  {
    if (svp->GetNumberOfElements() != 2 && svp->GetNumberOfElements() != 5)
    {
      vtkSMPropertyHelperWarningMacro("We only support 2 or 5 element properties.");
      return -1;
    }

    return svp->GetNumberOfElements() == 2 ? std::atoi(svp->GetElement(0))
                                           : std::atoi(svp->GetElement(3));
  }
}

//----------------------------------------------------------------------------
const char* vtkSMPropertyHelper::GetInputArrayNameToProcess() const
{
  if (this->Type != vtkSMPropertyHelper::STRING)
  {
    vtkSMPropertyHelperWarningMacro(
      "Property for 'InputArrayToProcess' can only be vtkSMStringVectorProperty.");
    return nullptr;
  }

  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(this->Property);
  if (this->GetUseUnchecked())
  {
    if (svp->GetNumberOfUncheckedElements() != 2 && svp->GetNumberOfUncheckedElements() != 5)
    {
      vtkSMPropertyHelperWarningMacro("We only support 2 or 5 element properties.");
      return nullptr;
    }

    return svp->GetNumberOfUncheckedElements() == 2 ? svp->GetUncheckedElement(1)
                                                    : svp->GetUncheckedElement(4);
  }
  else
  {
    if (svp->GetNumberOfElements() != 2 && svp->GetNumberOfElements() != 5)
    {
      vtkSMPropertyHelperWarningMacro("We only support 2 or 5 element properties.");
      return nullptr;
    }

    return svp->GetNumberOfElements() == 2 ? svp->GetElement(1) : svp->GetElement(4);
  }
}

//----------------------------------------------------------------------------
template <typename T>
bool vtkSMPropertyHelper::CopyInternal(const vtkSMPropertyHelper& source)
{
  std::vector<T> values = source.GetPropertyArray<T>();
  this->SetPropertyArray<T>(
    values.size() > 0 ? &values[0] : nullptr, static_cast<unsigned int>(values.size()));
  return true;
}

//----------------------------------------------------------------------------
bool vtkSMPropertyHelper::Copy(const vtkSMPropertyHelper& source)
{
  if (this->Type != source.Type)
  {
    vtkSMPropertyHelperWarningMacro("Property types much match for 'Copy' to work.");
    return false;
  }
  switch (this->Type)
  {
    case INT:
      return this->CopyInternal<int>(source);
    case DOUBLE:
      return this->CopyInternal<double>(source);
    case IDTYPE:
      return this->CopyInternal<vtkIdType>(source);
    default:
      vtkSMPropertyHelperWarningMacro(
        "Copy currently only supported for int/double/idtype properties.");
      return false;
  }
}

vtkSMPropertyHelper& vtkSMPropertyHelper::Modified()
{
  this->Property->Modified();
  return *this;
}
