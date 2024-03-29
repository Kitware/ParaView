// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
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
  this->UseElementTypes = false;
}

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::~vtkSMFieldDataDomain() = default;

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
const char* vtkSMFieldDataDomain::GetElementTypeAsString(int attrType)
{
  static const char* const vtkSMFieldDataDomainAttributeTypes[] = { "Point", "Cell", "Field",
    nullptr, "Vertex", "Edge", "Row", nullptr };

  if (attrType >= 0 && attrType < vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    return vtkSMFieldDataDomainAttributeTypes[attrType];
  }
  return nullptr;
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::ComputeDefaultValue(int originalDefaultValue)
{
  auto dataInfo = this->GetInputDataInformation("Input");
  if (!dataInfo)
  {
    return -1;
  }

  // Find an attribute with non-empty arrays and tuples
  for (unsigned int cc = 0; cc < this->GetNumberOfEntries(); ++cc)
  {
    const int attrType = this->GetEntryValue(cc);
    if (originalDefaultValue == -1 || originalDefaultValue == attrType)
    {
      auto attrInfo = dataInfo->GetAttributeInformation(attrType);
      if (attrInfo && attrInfo->GetNumberOfArrays() > 0 && attrInfo->GetMaximumNumberOfTuples() > 0)
      {
        return attrType;
      }
    }
  }

  // if that fails, find an attribute with non-empty arrays
  for (unsigned int cc = 0; cc < this->GetNumberOfEntries(); ++cc)
  {
    const int attrType = this->GetEntryValue(cc);
    if (originalDefaultValue == -1 || originalDefaultValue == attrType)
    {
      auto attrInfo = dataInfo->GetAttributeInformation(attrType);
      if (attrInfo && attrInfo->GetNumberOfArrays() > 0)
      {
        return attrType;
      }
    }
  }

  return -1;
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  if (vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop))
  {
    // Check provided default_values contains data
    int originalDefaultValue = ivp->GetDefaultValue(0);
    int defaultValue = this->ComputeDefaultValue(originalDefaultValue);
    if (defaultValue == -1)
    {
      // If not, try to find a default_values that contains data
      defaultValue = this->ComputeDefaultValue(-1);
    }

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
    vtkErrorMacro(
      "`vtkSMFieldDataDomain` is being used on a `vtkSMStringVectorProperty`. "
      "This is no longer needed or supported. Simply remove it from your XML configuration "
      "for property "
      << prop->GetXMLName());

    return 0;
  }

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

  int enable_field_data = 0;
  if (element->GetScalarAttribute("enable_field_data", &enable_field_data))
  {
    this->EnableFieldDataSelection = (enable_field_data != 0) ? true : false;
  }

  int use_element_types = 0;
  if (element->GetScalarAttribute("use_element_types", &use_element_types))
  {
    this->UseElementTypes = (use_element_types == 1);
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
    auto label = this->UseElementTypes ? vtkSMFieldDataDomain::GetElementTypeAsString(idx)
                                       : vtkSMFieldDataDomain::GetAttributeTypeAsString(idx);
    if (!label)
    {
      continue;
    }
    if (idx == vtkDataObject::FIELD)
    {
      continue;
    }
    if (dataInfo && !dataInfo->IsAttributeValid(idx))
    {
      continue;
    }
    this->AddEntry(label, idx);
  }

  // FIELD is always considered last
  if (this->EnableFieldDataSelection &&
    (!dataInfo || dataInfo->IsAttributeValid(vtkDataObject::FIELD)))
  {
    auto label = this->UseElementTypes
      ? vtkSMFieldDataDomain::GetElementTypeAsString(vtkDataObject::FIELD)
      : vtkSMFieldDataDomain::GetAttributeTypeAsString(vtkDataObject::FIELD);
    this->AddEntry(label, vtkDataObject::FIELD);
  }

  this->DomainModified();
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
