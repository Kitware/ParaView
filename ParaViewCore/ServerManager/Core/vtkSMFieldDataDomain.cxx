/*=========================================================================

  Program:   ParaView
  Module:    vtkSMFieldDataDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFieldDataDomain.h"

#include "vtkDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDomainIterator.h"
#include "vtkSMInputArrayDomain.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkSmartPointer.h"

#include <set>

vtkStandardNewMacro(vtkSMFieldDataDomain);
//---------------------------------------------------------------------------
vtkSMFieldDataDomain::vtkSMFieldDataDomain()
{
  this->EnableFieldDataSelection = false;
}

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::~vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
const char* vtkSMFieldDataDomain::GetAttributeTypeAsString(int attrType)
{
  static const char* const vtkSMFieldDataDomainAttributeTypes[] = { "Point Data", "Cell Data",
    "Field Data", nullptr, "Vertex Data", "Edge Data", "Row Data", nullptr };

  if (attrType >= 0 && attrType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    return vtkSMFieldDataDomainAttributeTypes[attrType];
  }
  return nullptr;
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::ComputeDefaultValue(int currentValue)
{
  auto dataInfo = this->GetInputDataInformation("Input");
  if (!dataInfo)
  {
    return -1;
  }

  // first, find an attribute with non-empty arrays and tuples
  for (unsigned int cc = 0, max = this->GetNumberOfEntries(); cc < max; ++cc)
  {
    const int attrType = this->GetEntryValue((cc + currentValue) % max);
    auto attrInfo = dataInfo->GetAttributeInformation(attrType);
    if (attrInfo && attrInfo->GetNumberOfArrays() > 0 && attrInfo->GetMaximumNumberOfTuples() > 0)
    {
      return attrType;
    }
  }

  // if that fails, find an attribute with non-empty arrays
  for (unsigned int cc = 0, max = this->GetNumberOfEntries(); cc < max; ++cc)
  {
    const int attrType = this->GetEntryValue((cc + currentValue) % max);
    auto attrInfo = dataInfo->GetAttributeInformation(attrType);
    if (attrInfo && attrInfo->GetNumberOfArrays() > 0)
    {
      return attrType;
    }
  }

  return -1;
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  if (vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop))
  {
    int currentValue = ivp->GetElement(0);
    const int defaultValue = this->ComputeDefaultValue(currentValue);
    if (defaultValue != -1)
    {
      if (use_unchecked_values)
      {
        ivp->SetUncheckedElement(0, defaultValue);
      }
      else
      {
        ivp->SetElement(0, defaultValue);
      }
      return 1;
    }
  }
  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  if (vtkSMStringVectorProperty::SafeDownCast(prop))
  {
#if !defined(VTK_LEGACY_SILENT)
    vtkErrorMacro(
      "`vtkSMFieldDataDomain` is being used on a `vtkSMStringVectorProperty`. "
      "This is no longer needed or supported. Simply remove it from your XML configuration "
      "for property "
      << prop->GetXMLName());
#endif

#if defined(VTK_LEGACY_REMOVE)
    return 0;
#else
    return 1;
#endif
  }

#if !defined(VTK_LEGACY_SILENT)
  if (element->GetAttribute("disable_update_domain_entries"))
  {
    vtkErrorMacro("`vtkSMFieldDataDomain` no longer supports or needs "
                  "`disable_update_domain_entries` attribute."
                  "Simply remove it from the XML for property "
      << prop->GetXMLName());
  }

  if (element->GetAttribute("force_point_cell_data"))
  {
    vtkErrorMacro(
      "`vtkSMFieldDataDomain` no longer supports or needs `force_point_cell_data` attribute."
      "Simply remove it from the XML for property "
      << prop->GetXMLName());
  }
#endif

  int enable_field_data = 0;
  if (element->GetScalarAttribute("enable_field_data", &enable_field_data))
  {
    this->EnableFieldDataSelection = (enable_field_data != 0) ? true : false;
  }
  this->Update(prop);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProperty* vtkNotUsed(prop))
{
  // Use data information only with a single input
  vtkPVDataInformation* dataInfo = nullptr;
  if (this->GetNumberOfInputConnections("Input") == 1)
  {
    dataInfo = this->GetInputDataInformation("Input");
  }
  this->RemoveAllEntries();
  for (int idx = 0; idx < vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES; idx++)
  {
    auto label = vtkSMFieldDataDomain::GetAttributeTypeAsString(idx);
    if (!label)
    {
      continue;
    }
    if (idx == vtkDataObject::FIELD && !this->EnableFieldDataSelection)
    {
      continue;
    }
    if (dataInfo && !dataInfo->IsAttributeValid(idx))
    {
      continue;
    }
    this->AddEntry(label, idx);
  }
  this->DomainModified();
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
