/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMIntVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMMessage.h"
#include "vtkSMVectorPropertyTemplate.h"
#include "vtkSMStateLocator.h"

vtkStandardNewMacro(vtkSMIntVectorProperty);

class vtkSMIntVectorProperty::vtkInternals :
  public vtkSMVectorPropertyTemplate<int>
{
public:
  vtkInternals(vtkSMIntVectorProperty* ivp):
    vtkSMVectorPropertyTemplate<int>(ivp)
  {
  }
};

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::vtkSMIntVectorProperty()
{
  this->Internals = new vtkInternals(this);
  this->ArgumentIsArray = 0;
}

//---------------------------------------------------------------------------
vtkSMIntVectorProperty::~vtkSMIntVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::WriteTo(vtkSMMessage* msg)
{
  ProxyState_Property *prop = msg->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant *variant = prop->mutable_value();
  variant->set_type(Variant::INT);

  std::vector<int>::iterator iter;
  for (iter = this->Internals->Values.begin(); iter!=
    this->Internals->Values.end(); ++iter)
    {
    variant->add_integer(*iter);
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::ReadFrom(const vtkSMMessage* msg, int offset,
                                      vtkSMProxyLocator*)
{
  //cout << ">>>>>>>>>>>>" << endl;
  //msg->PrintDebugString();
  //cout << "<<<<<<<<<<<<" << endl;

  assert(msg->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property *prop = &msg->GetExtension(ProxyState::property,
    offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant *variant = &prop->value();
  int num_elems = variant->integer_size();
  int *values = new int[num_elems+1];
  for (int cc=0; cc < num_elems; cc++)
    {
    values[cc] = variant->integer(cc);
    }
  this->SetElements(values, num_elems);
  delete[] values;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->SetNumberOfUncheckedElements(num);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->SetNumberOfElements(num);
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->GetNumberOfUncheckedElements();
}

//---------------------------------------------------------------------------
unsigned int vtkSMIntVectorProperty::GetNumberOfElements()
{
  return this->Internals->GetNumberOfElements();
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->GetElement(idx);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetDefaultValue(int idx)
{
  return this->Internals->GetDefaultValue(idx);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::ClearUncheckedElements()
{
  this->Internals->ClearUncheckedElements();
}

//---------------------------------------------------------------------------
int *vtkSMIntVectorProperty::GetElements()
{
  return this->Internals->GetElements();
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->GetUncheckedElement(idx);
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SetUncheckedElement(unsigned int idx, int value)
{
  this->Internals->SetUncheckedElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElement(unsigned int idx, int value)
{
  return this->Internals->SetElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements1(int value0)
{
  return this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements2(int value0, int value1)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  return (retVal1 && retVal2);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements3(int value0, 
                                          int value1, 
                                          int value2)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  return (retVal1 && retVal2 && retVal3);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements(const int* values)
{
  return this->Internals->SetElements(values);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElements(const int* values, unsigned int numElems)
{
  return this->Internals->SetElements(values, numElems);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetUncheckedElements(const int* values)
{
  return this->Internals->SetUncheckedElements(values);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetUncheckedElements(const int* values, unsigned int numValues)
{
  return this->Internals->SetUncheckedElements(values, numValues);
}

//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::ReadXMLAttributes(vtkSMProxy* parent,
                                              vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(parent, element);
  if (!retVal)
    {
    return retVal;
    }

  int arg_is_array;
  retVal = element->GetScalarAttribute("argument_is_array", &arg_is_array);
  if(retVal) 
    { 
    this->SetArgumentIsArray(arg_is_array); 
    }

  int numElems = this->GetNumberOfElements();
  if (numElems > 0)
    {
    if (element->GetAttribute("default_values") &&
        strcmp("none", element->GetAttribute("default_values")) == 0 )
      {
      this->Internals->Initialized = false;
      }
    else
      {
      int* initVal = new int[numElems];
      int numRead = element->GetVectorAttribute("default_values",
                                                numElems,
                                                initVal);
      
      if (numRead > 0)
        {
        if (numRead != numElems)
          {
          vtkErrorMacro("The number of default values does not match the "
                        "number of elements. Initialization failed.");
          delete[] initVal;
          return 0;
          }
        this->SetElements(initVal);
        this->Internals->UpdateDefaultValues();
        }
      else if (!this->Internals->Initialized)
        {
        vtkErrorMacro("No default value is specified for property: "
                      << this->GetXMLName()
                      << ". This might lead to stability problems");
        }
      delete[] initVal;
      }
    }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMIntVectorProperty* dsrc = vtkSMIntVectorProperty::SafeDownCast(
    src);
  if (dsrc)
    {
    this->Internals->Copy(dsrc->Internals);
    }
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::ResetToDefaultInternal()
{
  this->Internals->ResetToDefaultInternal();
}

//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
  os << indent << "Values: ";
  for (unsigned int i=0; i<this->GetNumberOfElements(); i++)
    {
    os << this->GetElement(i) << " ";
    }
  os << endl;
}
//---------------------------------------------------------------------------
void vtkSMIntVectorProperty::SaveStateValues(vtkPVXMLElement* propElement)
{
  this->Internals->SaveStateValues(propElement);
}
//---------------------------------------------------------------------------
int vtkSMIntVectorProperty::SetElementAsString(int idx, const char* value)
{
  return this->Internals->SetElementAsString(idx, value);
}
