/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDoubleMapProperty.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSMDoubleMapProperty.h"

#include <map>
#include <sstream>
#include <vector>

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleMapPropertyIterator.h"
#include "vtkSMMessage.h"

//---------------------------------------------------------------------------
class vtkSMDoubleMapPropertyPrivate
{
public:
  std::map<vtkIdType, std::vector<double> > Map;
  unsigned int NumberOfComponents;

  std::vector<double>& GetVector(vtkIdType id)
  {
    std::vector<double>& v = this->Map[id];
    if (v.size() != this->NumberOfComponents)
    {
      v.resize(this->NumberOfComponents);
    }
    return v;
  }
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMDoubleMapProperty);

//---------------------------------------------------------------------------
vtkSMDoubleMapProperty::vtkSMDoubleMapProperty()
{
  this->Private = new vtkSMDoubleMapPropertyPrivate;
  this->Private->NumberOfComponents = 1;
}

//---------------------------------------------------------------------------
vtkSMDoubleMapProperty::~vtkSMDoubleMapProperty()
{
  delete this->Private;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SetNumberOfComponents(unsigned int components)
{
  this->Private->NumberOfComponents = components;
  this->Modified();
}

//---------------------------------------------------------------------------
unsigned int vtkSMDoubleMapProperty::GetNumberOfComponents()
{
  return this->Private->NumberOfComponents;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SetElement(vtkIdType index, double value)
{
  std::vector<double>& vector = this->Private->GetVector(index);

  if (vector[0] != value)
  {
    vector[0] = value;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SetElements(vtkIdType index, const double* values)
{
  std::vector<double>& vector = this->Private->GetVector(index);

  if (!std::equal(vector.begin(), vector.end(), values))
  {
    vector.assign(values, values + this->Private->NumberOfComponents);
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SetElements(
  vtkIdType index, const double* values, unsigned int numValues)
{
  std::vector<double>& vector = this->Private->GetVector(index);

  if (!std::equal(vector.begin(), vector.begin() + numValues, values))
  {
    vector.assign(values, values + numValues);
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SetElementComponent(
  vtkIdType index, unsigned int component, double value)
{
  std::vector<double>& vector = this->Private->GetVector(index);

  if (vector[component] != value)
  {
    vector[component] = value;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
double vtkSMDoubleMapProperty::GetElement(vtkIdType index)
{
  std::vector<double>& vector = this->Private->GetVector(index);
  return vector[0];
}

//---------------------------------------------------------------------------
double* vtkSMDoubleMapProperty::GetElements(vtkIdType index)
{
  std::vector<double>& vector = this->Private->GetVector(index);
  return &vector[0];
}

//---------------------------------------------------------------------------
double vtkSMDoubleMapProperty::GetElementComponent(vtkIdType index, vtkIdType component)
{
  std::vector<double>& vector = this->Private->GetVector(index);
  return vector[component];
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::RemoveElement(vtkIdType index)
{
  if (this->Private->Map.count(index) != 0)
  {
    this->Private->Map.erase(index);
    this->Modified();
  }
}

//---------------------------------------------------------------------------
vtkIdType vtkSMDoubleMapProperty::GetNumberOfElements()
{
  return this->Private->Map.size();
}

//---------------------------------------------------------------------------
bool vtkSMDoubleMapProperty::HasElement(vtkIdType index)
{
  return this->Private->Map.find(index) != this->Private->Map.end();
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::ClearElements()
{
  if (!this->Private->Map.empty())
  {
    this->Private->Map.clear();
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::WriteTo(vtkSMMessage* msg)
{
  ProxyState_Property* prop = msg->AddExtension(ProxyState::property);
  prop->set_name(this->GetXMLName());
  Variant* variant = prop->mutable_value();
  variant->set_type(Variant::MAP);

  std::map<vtkIdType, std::vector<double> >::iterator iter;
  for (iter = this->Private->Map.begin(); iter != this->Private->Map.end(); iter++)
  {
    variant->add_idtype(iter->first);

    std::vector<double>::iterator double_iter;
    for (double_iter = iter->second.begin(); double_iter != iter->second.end(); double_iter++)
    {
      variant->add_float64(*double_iter);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::ReadFrom(
  const vtkSMMessage* msg, int offset, vtkSMProxyLocator* locator)
{
  (void)locator;

  const ProxyState_Property* prop = &msg->GetExtension(ProxyState::property, offset);
  assert(prop->name() == this->GetXMLName());

  const Variant* variant = &prop->value();
  unsigned int num_elems = variant->idtype_size();

  this->ClearElements();

  for (unsigned int i = 0; i < num_elems; i++)
  {
    std::vector<double> values;
    for (unsigned int j = 0; j < this->GetNumberOfComponents(); j++)
    {
      values.push_back(variant->float64(i * this->GetNumberOfComponents() + j));
    }
    this->SetElements(variant->idtype(i), &values[0]);
  }
}

//---------------------------------------------------------------------------
int vtkSMDoubleMapProperty::ReadXMLAttributes(vtkSMProxy* parent, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(parent, element))
  {
    return 0;
  }

  int number_of_components = 0;
  if (element->GetScalarAttribute("number_of_components", &number_of_components))
  {
    this->SetNumberOfComponents(number_of_components);
  }

  return 1;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::SaveStateValues(vtkPVXMLElement* propertyElement)
{
  if (!this->Private->Map.empty())
  {
    propertyElement->AddAttribute("number_of_elements", vtkIdType(this->Private->Map.size()));
    propertyElement->AddAttribute("number_of_components", this->Private->NumberOfComponents);
  }

  typedef std::map<vtkIdType, std::vector<double> >::const_iterator iter;

  for (iter i = this->Private->Map.begin(); i != this->Private->Map.end(); i++)
  {
    vtkPVXMLElement* elementElement = vtkPVXMLElement::New();
    elementElement->SetName("Element");
    elementElement->AddAttribute("index", i->first);

    const std::vector<double>& values = i->second;

    for (size_t j = 0; j < values.size(); j++)
    {
      std::stringstream stream;
      stream << values[j];

      vtkPVXMLElement* vectorElement = vtkPVXMLElement::New();
      vectorElement->SetName("Value");
      vectorElement->AddAttribute("index", vtkIdType(j));
      vectorElement->AddAttribute("value", stream.str().c_str());
      elementElement->AddNestedElement(vectorElement);
      vectorElement->Delete();
    }

    propertyElement->AddNestedElement(elementElement);
    elementElement->Delete();
  }
}

//---------------------------------------------------------------------------
int vtkSMDoubleMapProperty::LoadState(vtkPVXMLElement* element, vtkSMProxyLocator* loader)
{
  if (!this->Superclass::LoadState(element, loader))
  {
    return 0;
  }

  vtkIdType number_of_components = 0;
  element->GetScalarAttribute("number_of_components", &number_of_components);

  for (unsigned int i = 0; i < element->GetNumberOfNestedElements(); i++)
  {
    vtkPVXMLElement* currentElement = element->GetNestedElement(i);
    if (currentElement && strcmp(currentElement->GetName(), "Element") == 0)
    {
      vtkIdType index;

      // read values vector
      std::vector<double> values(number_of_components);
      if (currentElement->GetScalarAttribute("index", &index))
      {
        for (vtkIdType j = 0; j < number_of_components; j++)
        {
          vtkPVXMLElement* valueElement = currentElement->GetNestedElement(j);
          if (!valueElement)
          {
            continue;
          }

          double value;
          valueElement->GetScalarAttribute("value", &value);
          values[j] = value;
        }
      }

      // store values in map
      this->Private->Map[index] = values;
    }
  }

  this->Modified();

  return 1;
}

//---------------------------------------------------------------------------
vtkSMDoubleMapPropertyIterator* vtkSMDoubleMapProperty::NewIterator()
{
  vtkSMDoubleMapPropertyIterator* iter = vtkSMDoubleMapPropertyIterator::New();
  iter->SetProperty(this);
  return iter;
}

//---------------------------------------------------------------------------
void* vtkSMDoubleMapProperty::GetMapPointer()
{
  return &this->Private->Map;
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::Copy(vtkSMProperty* src)
{
  vtkSMDoubleMapProperty* other = vtkSMDoubleMapProperty::SafeDownCast(src);
  if (other && (other->Private->NumberOfComponents != this->Private->NumberOfComponents ||
                 other->Private->Map.size() != this->Private->Map.size() ||
                 !std::equal(other->Private->Map.begin(), other->Private->Map.end(),
                   this->Private->Map.begin())))
  {
    this->Private->Map = other->Private->Map;
    this->Private->NumberOfComponents = other->Private->NumberOfComponents;
    this->Modified();
  }

  this->Superclass::Copy(src);
}

//---------------------------------------------------------------------------
void vtkSMDoubleMapProperty::ResetToXMLDefaults()
{
  // map properties are always empty by default
  this->ClearElements();
}
