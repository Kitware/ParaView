/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDiscreteDoubleDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDiscreteDoubleDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"

#include <algorithm>

vtkStandardNewMacro(vtkSMDiscreteDoubleDomain);

std::vector<double> vtkSMDiscreteDoubleDomain::GetValues()
{
  return this->Values;
}

//---------------------------------------------------------------------------
bool vtkSMDiscreteDoubleDomain::GetValuesExists()
{
  return this->Values.size() > 0;
}

//---------------------------------------------------------------------------
void vtkSMDiscreteDoubleDomain::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of values: " << this->Values.size() << std::endl;
}

//---------------------------------------------------------------------------
void vtkSMDiscreteDoubleDomain::Update(vtkSMProperty* property)
{
  vtkSMDoubleVectorProperty* doubleProperty = vtkSMDoubleVectorProperty::SafeDownCast(property);
  if (doubleProperty && doubleProperty->GetInformationOnly())
  {
    this->Values.clear();
    for (unsigned int cc = 0; cc < doubleProperty->GetNumberOfElements(); ++cc)
    {
      this->Values.push_back(doubleProperty->GetElement(cc));
    }
    this->InvokeModified();
  }
}

//---------------------------------------------------------------------------
int vtkSMDiscreteDoubleDomain::IsInDomain(vtkSMProperty* property)
{
  if (this->IsOptional)
  {
    return 1;
  }

  vtkSMDoubleVectorProperty* doubleProperty = vtkSMDoubleVectorProperty::SafeDownCast(property);
  if (!doubleProperty)
  {
    return 0;
  }
  unsigned int numElems = doubleProperty->GetNumberOfUncheckedElements();
  for (unsigned int i = 0; i < numElems; i++)
  {
    if (std::find(this->Values.begin(), this->Values.end(),
          doubleProperty->GetUncheckedElement(i)) == this->Values.end())
    {
      return 0;
    }
  }
  return 1;
}

//---------------------------------------------------------------------------
vtkSMDiscreteDoubleDomain::vtkSMDiscreteDoubleDomain() = default;

//---------------------------------------------------------------------------
vtkSMDiscreteDoubleDomain::~vtkSMDiscreteDoubleDomain() = default;

//---------------------------------------------------------------------------
int vtkSMDiscreteDoubleDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const int MAX_NUM = 256;
  this->Values.resize(MAX_NUM);
  int numRead = element->GetVectorAttribute("values", MAX_NUM, this->Values.data());
  this->Values.resize(numRead);

  if (numRead <= 0)
  {
    vtkErrorMacro(<< "Can not find required attribute: values. "
                  << "Can not parse domain xml.");
    return 0;
  }

  return 1;
}
