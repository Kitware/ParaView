/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleVectorProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDoubleVectorProperty.h"

#include "vtkClientServerStream.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkProcessModule.h"
#include "vtkSMMessage.h"
#include "vtkSMStateLocator.h"
#include "vtkSMVectorPropertyTemplate.h"

vtkStandardNewMacro(vtkSMDoubleVectorProperty);

class vtkSMDoubleVectorProperty::vtkInternals : public vtkSMVectorPropertyTemplate<double>
{
public:
  vtkInternals(vtkSMDoubleVectorProperty* ivp)
    : vtkSMVectorPropertyTemplate<double>(ivp)
  {
  }
};

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::vtkSMDoubleVectorProperty()
{
  this->Internals = new vtkInternals(this);
  this->ArgumentIsArray = 0;
}

//---------------------------------------------------------------------------
vtkSMDoubleVectorProperty::~vtkSMDoubleVectorProperty()
{
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::WriteTo(vtkSMMessage* msg)
{
  ProxyState_Property* prop = msg->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* variant = prop->mutable_value();
  variant->set_type(Variant::FLOAT64);
  std::vector<double>::iterator iter;
  for (iter = this->Internals->Values.begin(); iter != this->Internals->Values.end(); ++iter)
  {
    variant->add_float64(*iter);
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::ReadFrom(const vtkSMMessage* msg, int offset, vtkSMProxyLocator*)
{
  assert(msg->ExtensionSize(ProxyState::property) > offset);

  const ProxyState_Property* prop = &msg->GetExtension(ProxyState::property, offset);
  assert(strcmp(prop->name().c_str(), this->GetXMLName()) == 0);

  const Variant* variant = &prop->value();
  int num_elems = variant->float64_size();
  double* values = new double[num_elems];
  for (int cc = 0; cc < num_elems; cc++)
  {
    values[cc] = variant->float64(cc);
  }
  this->SetElements(values, num_elems);
  delete[] values;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetNumberOfUncheckedElements(unsigned int num)
{
  this->Internals->SetNumberOfUncheckedElements(num);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetNumberOfElements(unsigned int num)
{
  this->Internals->SetNumberOfElements(num);
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleVectorProperty::GetNumberOfUncheckedElements()
{
  return this->Internals->GetNumberOfUncheckedElements();
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleVectorProperty::GetNumberOfElements()
{
  return this->Internals->GetNumberOfElements();
}

//---------------------------------------------------------------------------
double* vtkSMDoubleVectorProperty::GetElements()
{
  return this->Internals->GetElements();
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetElement(unsigned int idx)
{
  return this->Internals->GetElement(idx);
}

//---------------------------------------------------------------------------
double vtkSMDoubleVectorProperty::GetUncheckedElement(unsigned int idx)
{
  return this->Internals->GetUncheckedElement(idx);
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::SetUncheckedElement(unsigned int idx, double value)
{
  this->Internals->SetUncheckedElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElement(unsigned int idx, double value)
{
  return this->Internals->SetElement(idx, value);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements1(double value0)
{
  return this->SetElement(0, value0);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements2(double value0, double value1)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  return (retVal1 && retVal2);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements3(double value0, double value1, double value2)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  return (retVal1 && retVal2 && retVal3);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements4(
  double value0, double value1, double value2, double value3)
{
  int retVal1 = this->SetElement(0, value0);
  int retVal2 = this->SetElement(1, value1);
  int retVal3 = this->SetElement(2, value2);
  int retVal4 = this->SetElement(3, value3);
  return (retVal1 && retVal2 && retVal3 && retVal4);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements(const double* values, unsigned int numValues)
{
  return this->Internals->SetElements(values, numValues);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetElements(const double* values)
{
  return this->Internals->SetElements(values);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetUncheckedElements(const double* values)
{
  return this->Internals->SetUncheckedElements(values);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::SetUncheckedElements(const double* values, unsigned int numValues)
{
  return this->Internals->SetUncheckedElements(values, numValues);
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::ReadXMLAttributes(vtkSMProxy* proxy, vtkPVXMLElement* element)
{
  int retVal;

  retVal = this->Superclass::ReadXMLAttributes(proxy, element);
  if (!retVal)
  {
    return retVal;
  }

  int arg_is_array;
  retVal = element->GetScalarAttribute("argument_is_array", &arg_is_array);
  if (retVal)
  {
    this->SetArgumentIsArray(arg_is_array);
  }

  int numElems = this->GetNumberOfElements();
  if (numElems > 0)
  {
    if (element->GetAttribute("default_values") &&
      strcmp("none", element->GetAttribute("default_values")) == 0)
    {
      this->Internals->Initialized = false;
    }
    else
    {
      double* initVal = new double[numElems];
      int numRead = element->GetVectorAttribute("default_values", numElems, initVal);

      if (numRead > 0)
      {
        if (numRead != numElems)
        {
          vtkErrorMacro("The number of default values does not match the number "
                        "of elements. Initialization failed.");
          delete[] initVal;
          return 0;
        }
        this->SetElements(initVal);
        this->Internals->UpdateDefaultValues();
      }
      else if (!this->Internals->Initialized)
      {
        vtkErrorMacro("No default value is specified for property: "
          << this->GetXMLName() << ". This might lead to stability problems");
      }
      delete[] initVal;
    }
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::Copy(vtkSMProperty* src)
{
  this->Superclass::Copy(src);

  vtkSMDoubleVectorProperty* dsrc = vtkSMDoubleVectorProperty::SafeDownCast(src);
  if (dsrc)
  {
    this->Internals->Copy(dsrc->Internals);
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::ClearUncheckedElements()
{
  this->Internals->ClearUncheckedElements();
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::ResetToXMLDefaults()
{
  this->Internals->ResetToXMLDefaults();
}

//---------------------------------------------------------------------------
void vtkSMDoubleVectorProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ArgumentIsArray: " << this->ArgumentIsArray << endl;
  os << indent << "Values: ";
  for (unsigned int i = 0; i < this->GetNumberOfElements(); i++)
  {
    os << this->GetElement(i) << " ";
  }
  os << endl;
}

//---------------------------------------------------------------------------
int vtkSMDoubleVectorProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
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
void vtkSMDoubleVectorProperty::SaveStateValues(vtkPVXMLElement* propElement)
{
  this->Internals->SaveStateValues(propElement);
}

//---------------------------------------------------------------------------
bool vtkSMDoubleVectorProperty::IsValueDefault()
{
  return this->Internals->IsValueDefault();
}
