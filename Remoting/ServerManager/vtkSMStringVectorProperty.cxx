/*=========================================================================

  Program:   ParaView
  Module:    vtkSMStringVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMStringVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMStateLocator.h"
#include "vtkSMVectorPropertyTemplate.h"
#include "vtkStringList.h"

#include <algorithm>
#include <sstream>

vtkStandardNewMacro(vtkSMStringVectorProperty);

class vtkSMStringVectorProperty::vtkInternals : public vtkSMVectorPropertyTemplate<std::string>
{
public:
  std::vector<int> ElementTypes;

  vtkInternals(vtkSMStringVectorProperty* ivp)
    : vtkSMVectorPropertyTemplate<std::string>(ivp)
  {
  }
};

//---------------------------------------------------------------------------
vtkSMStringVectorProperty::vtkSMStringVectorProperty()
{
  this->Internals = new vtkInternals(this);
}

//---------------------------------------------------------------------------
vtkSMStringVectorProperty::~vtkSMStringVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetElementType(unsigned int idx, int type)
{
  unsigned int size = static_cast<unsigned int>(this->Internals->ElementTypes.size());
  if (idx >= size)
  {
    this->Internals->ElementTypes.resize(idx + 1);
  }
  for (unsigned int i = size; i <= idx; i++)
  {
    this->Internals->ElementTypes[i] = STRING;
  }
  this->Internals->ElementTypes[idx] = type;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::GetElementType(unsigned int idx)
{
  if (idx >= this->Internals->ElementTypes.size())
  {
    return STRING;
  }
  return this->Internals->ElementTypes[idx];
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::WriteTo(vtkSMMessage* msg)
{
  ProxyState_Property* prop = msg->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* variant = prop->mutable_value();
  variant->set_type(Variant::STRING);
  for (unsigned int i = 0; i < this->GetNumberOfElements(); i++)
  {
    variant->add_txt(this->GetElement(i));
  }
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::ReadFrom(const vtkSMMessage* msg, int offset, vtkSMProxyLocator*)
{
  assert(msg->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property* prop = &msg->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant* variant = &prop->value();

  int num_elems = variant->txt_size();
  const char** values = new const char*[num_elems + 1];
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = variant->txt(cc).c_str();
  }
  this->SetElements(values, num_elems);
  delete[] values;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->SetNumberOfUncheckedElements(num);
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->SetNumberOfElements(num);
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->GetNumberOfUncheckedElements();
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetNumberOfElements()
{
  return this->Internals->GetNumberOfElements();
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->GetElement(idx).c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::GetElements(vtkStringList* list)
{
  list->RemoveAllItems();

  unsigned int numElems = this->GetNumberOfElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    list->AddString(this->GetElement(cc));
  }
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::GetUncheckedElements(vtkStringList* list)
{
  list->RemoveAllItems();

  unsigned int numElems = this->GetNumberOfUncheckedElements();
  for (unsigned int cc = 0; cc < numElems; cc++)
  {
    list->AddString(this->GetUncheckedElement(cc));
  }
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->GetUncheckedElement(idx).c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetUncheckedElement(unsigned int idx, const char* value)
{
  if (!value)
  {
    value = "";
  }
  this->Internals->SetUncheckedElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElements(vtkStringList* list)
{
  unsigned int count = static_cast<unsigned int>(list->GetLength());
  std::string* values = new std::string[count + 1];
  for (unsigned int cc = 0; cc < count; cc++)
  {
    values[cc] = list->GetString(cc) ? list->GetString(cc) : "";
  }
  int ret_val = this->Internals->SetElements(values, count);
  delete[] values;
  return ret_val;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetUncheckedElements(vtkStringList* list)
{
  unsigned int count = static_cast<unsigned int>(list->GetLength());
  std::string* values = new std::string[count + 1];
  for (unsigned int cc = 0; cc < count; cc++)
  {
    values[cc] = list->GetString(cc) ? list->GetString(cc) : "";
  }
  int ret_val = this->Internals->SetUncheckedElements(values, count);
  delete[] values;
  return ret_val;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElements(const char* values[], unsigned int count)
{
  std::string* std_values = new std::string[count + 1];
  for (unsigned int cc = 0; cc < count; cc++)
  {
    std_values[cc] = values[cc] ? values[cc] : "";
  }
  int ret_val = this->Internals->SetElements(std_values, count);
  delete[] std_values;
  return ret_val;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElements(const std::vector<std::string>& values)
{
  std::vector<std::string> svalues(values.size() + 1);
  std::copy(values.begin(), values.end(), svalues.begin());
  return this->Internals->SetElements(&svalues[0], static_cast<unsigned int>(values.size()));
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetUncheckedElements(const char* values[], unsigned int count)
{
  std::string* std_values = new std::string[count + 1];
  for (unsigned int cc = 0; cc < count; cc++)
  {
    std_values[cc] = values[cc] ? values[cc] : "";
  }
  int ret_val = this->Internals->SetUncheckedElements(std_values, count);
  delete[] std_values;
  return ret_val;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetUncheckedElements(const std::vector<std::string>& values)
{
  std::vector<std::string> svalues(values.size() + 1);
  std::copy(values.begin(), values.end(), svalues.begin());
  return this->Internals->SetUncheckedElements(
    &svalues[0], static_cast<unsigned int>(values.size()));
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElement(unsigned int idx, const char* value)
{
  if (!value)
  {
    value = "";
  }
  return this->Internals->SetElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::AppendElements(const std::vector<std::string>& newvalue)
{
  return this->Internals->AppendElements(
    newvalue.data(), static_cast<unsigned int>(newvalue.size()));
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::AppendUncheckedElements(const std::vector<std::string>& newvalue)
{
  return this->Internals->AppendUncheckedElements(
    newvalue.data(), static_cast<unsigned int>(newvalue.size()));
}

//---------------------------------------------------------------------------
unsigned int vtkSMStringVectorProperty::GetElementIndex(const char* value, int& exists)
{
  unsigned int i;
  for (i = 0; i < this->GetNumberOfElements(); i++)
  {
    if (value && this->Internals->Values[i].c_str() &&
      !strcmp(value, this->Internals->Values[i].c_str()))
    {
      exists = 1;
      return i;
    }
  }
  exists = 0;
  return 0;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::ResetToXMLDefaults()
{
  this->Internals->ResetToXMLDefaults();
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy, vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(proxy, element);
  if (!retVal)
  {
    return retVal;
  }

  int numEls = this->GetNumberOfElements();

  if (this->RepeatCommand)
  {
    numEls = this->GetNumberOfElementsPerCommand();
  }
  int* eTypes = new int[numEls];

  int numElsRead = element->GetVectorAttribute("element_types", numEls, eTypes);
  for (int i = 0; i < numElsRead; i++)
  {
    this->Internals->ElementTypes.push_back(eTypes[i]);
  }
  delete[] eTypes;

  numEls = this->GetNumberOfElements();
  if (numEls > 0)
  {
    const char* tmp = element->GetAttribute("default_values");
    const char* delimiter = element->GetAttribute("default_values_delimiter");
    if (tmp && delimiter)
    {
      std::string initVal = tmp;
      std::string delim = delimiter;
      std::string::size_type pos1 = 0;
      std::string::size_type pos2 = 0;
      for (int i = 0; i < numEls && pos2 != std::string::npos; i++)
      {
        if (i != 0)
        {
          pos1 += delim.size();
        }
        pos2 = initVal.find(delimiter, pos1);
        std::string v = pos1 == pos2 ? "" : initVal.substr(pos1, pos2 - pos1);
        this->Internals->DefaultValues.push_back(v);
        this->Internals->DefaultsValid = true;
        this->SetElement(i, v.c_str());
        pos1 = pos2;
      }
    }
    else if (tmp)
    {
      this->SetElement(0, tmp);
      this->Internals->DefaultValues.push_back(tmp);
      this->Internals->DefaultsValid = true;
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetDefaultValue(int idx)
{
  return this->Internals->GetDefaultValue(idx).c_str();
}

void vtkSMStringVectorProperty::ClearUncheckedElements()
{
  this->Internals->ClearUncheckedElements();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMStringVectorProperty* dsrc = vtkSMStringVectorProperty::SafeDownCast(src);
  if (dsrc)
  {
    this->Internals->Copy(dsrc->Internals);
  }
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Values: ";
  for (unsigned int i = 0; i < this->GetNumberOfElements(); i++)
  {
    os << (this->GetElement(i) ? this->GetElement(i) : "(nil)") << " ";
  }
  os << endl;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  int prevImUpdate = this->ImmediateUpdate;

  // Wait until all values are set before update (if ImmediateUpdate)
  this->ImmediateUpdate = 0;
  int retVal = this->Superclass::LoadState(element, loader);
  if (retVal != 0)
  {
    retVal = this->Internals->LoadStateValues(element) ? 1 : 0;
  }
  this->ImmediateUpdate = prevImUpdate;
  return retVal;
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SaveStateValues(vtkPVXMLElement* propElement)
{
  this->Internals->SaveStateValues(propElement);
}

//---------------------------------------------------------------------------
bool vtkSMStringVectorProperty::IsValueDefault()
{
  return this->Internals->IsValueDefault();
}
