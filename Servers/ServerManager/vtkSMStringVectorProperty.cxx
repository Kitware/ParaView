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
#include "vtkStringList.h"
#include "vtkSMVectorPropertyTemplate.h"
#include "vtkStdString.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkSMStringVectorProperty);

class vtkSMStringVectorProperty::vtkInternals :
  public vtkSMVectorPropertyTemplate<vtkStdString>
{
public:
  vtkstd::vector<int> ElementTypes;

  vtkInternals(vtkSMStringVectorProperty* ivp):
    vtkSMVectorPropertyTemplate<vtkStdString>(ivp)
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
  unsigned int size = this->Internals->ElementTypes.size();
  if (idx >= size)
    {
    this->Internals->ElementTypes.resize(idx+1);
    }
  for (unsigned int i=size; i<=idx; i++)
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
  ProxyState_Property *prop = msg->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *var = 0;
  for(unsigned int i=0;i<this->GetNumberOfElements();i++)
    {
    // One element by variant
    var = prop->add_value();
    switch (this->GetElementType(i))
      {
      case INT:
        var->set_type(Variant::INT);
        var->add_integer(atoi(this->GetElement(i)));
        break;
      case DOUBLE:
        var->set_type(Variant::FLOAT64);
        var->add_float64(atof(this->GetElement(i)));
        break;
      case STRING:
        var->set_type(Variant::STRING);
        var->add_txt(this->GetElement(i));
        break;
      }
    }
}
//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::ReadFrom(vtkSMMessage* msg, int message_offset)
{
  //cout << ">>>>>>>>>>>>" << endl;
  //msg->PrintDebugString();
  //cout << "<<<<<<<<<<<<" << endl;

  bool found = false;
  for(int i=0;i<msg->ExtensionSize(ProxyState::property);++i)
    {
    const ProxyState_Property *prop = &msg->GetExtension(ProxyState::property, i);
    if(strcmp(prop->name().c_str(), this->GetXMLName()) == 0)
      {
      // Several variant but one element by variant
      this->SetNumberOfElements(prop->value_size());

      // Loop over variants
      for(int i=0;i<prop->value_size();i++)
        {
        const Variant *value = &prop->value(i);
        vtksys_ios::ostringstream oss;
        switch(value->type())
          {
          case Variant::INT:
            oss << value->integer(0); // One element by variant
            this->SetElement(i, oss.str().c_str());
            break;
          case Variant::FLOAT64:
            oss << value->float64(0); // One element by variant
            this->SetElement(i, oss.str().c_str());
            break;
          case Variant::STRING:
            this->SetNumberOfElements(value->txt_size());
            this->SetElement(i, value->txt(0).c_str()); // One element by variant
            break;
          default:
            break;
          }
        }

      // Found the property, so exit the loop
      found = true;
      break;
      }
    }
  if(!found)
    {
    cout << "Not found " << this->GetXMLName() << endl;
    // FIXME do nothing or throw exception ==================================================================================
    }
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
  for (unsigned int cc=0; cc < numElems; cc++)
    {
    list->AddString(this->GetElement(cc));
    }
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->GetUncheckedElement(idx).c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::SetUncheckedElement(
  unsigned int idx, const char* value)
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
  vtkStdString* values = new vtkStdString[count+1];
  for (unsigned int cc=0; cc < count; cc++)
    {
    values[cc] = list->GetString(cc)? list->GetString(cc): "";
    }
  int ret_val = this->Internals->SetElements(values, count);
  delete[] values;
  return ret_val;
}

//---------------------------------------------------------------------------
int vtkSMStringVectorProperty::SetElements(unsigned int count, const char* values[])
{
  vtkStdString* std_values = new vtkStdString[count+1];
  for (unsigned int cc=0; cc < count; cc++)
    {
    std_values[cc] = values[cc]? values[cc] : "";
    }
  int ret_val = this->Internals->SetElements(std_values, count);
  delete[] std_values;
  return ret_val;
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
unsigned int vtkSMStringVectorProperty::GetElementIndex(
  const char *value, int& exists)
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
int vtkSMStringVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy,
                                                 vtkPVXMLElement* element)
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
  for (int i=0; i<numElsRead; i++)
    {
    this->Internals->ElementTypes.push_back(eTypes[i]);
    }
  delete[] eTypes;

  numEls = this->GetNumberOfElements();
  if (numEls > 0)
    {
    const char* tmp = element->GetAttribute("default_values");
    const char* delimiter = element->GetAttribute("default_values_delimiter");
    if(tmp && delimiter)
      {
      vtkStdString initVal = tmp;
      vtkStdString delim = delimiter;
      vtkStdString::size_type pos1 = 0;
      vtkStdString::size_type pos2 = 0;
      for(int i=0; i<numEls && pos2 != vtkStdString::npos; i++)
        {
        if(i != 0)
          {
          pos1 += delim.size();
          }
        pos2 = initVal.find(delimiter, pos1);
        vtkStdString v = pos1 == pos2 ? "" : initVal.substr(pos1, pos2-pos1);
        this->Internals->DefaultValues.push_back(v);
        this->SetElement(i, v.c_str());
        pos1 = pos2;
        }
      }
    else if(tmp)
      {
      this->SetElement(0, tmp);
      this->Internals->DefaultValues.push_back(tmp);
      }
    }
  return 1;
}

//---------------------------------------------------------------------------
const char* vtkSMStringVectorProperty::GetDefaultValue(int idx)
{
  return this->Internals->GetDefaultValue(idx).c_str();
}

//---------------------------------------------------------------------------
void vtkSMStringVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMStringVectorProperty* dsrc = vtkSMStringVectorProperty::SafeDownCast(
    src);
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
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << (this->GetElement(i)?this->GetElement(i):"(nil)") << " ";
    }
  os << endl;
}
