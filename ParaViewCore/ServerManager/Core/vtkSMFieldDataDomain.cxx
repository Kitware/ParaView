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
#include "vtkSmartPointer.h"

#include <set>

vtkStandardNewMacro(vtkSMFieldDataDomain);
//---------------------------------------------------------------------------
static const char* const vtkSMFieldDataDomainAttributeTypes[] = { "Point Data", "Cell Data",
  "Field Data", "(invalid)", "Vertex Data", "Edge Data", "Row Data", NULL };
//---------------------------------------------------------------------------
vtkSMFieldDataDomain::vtkSMFieldDataDomain()
{
  this->EnableFieldDataSelection = false;
  this->DisableUpdateDomainEntries = false;
  this->ForcePointAndCellDataSelection = false;
  this->DefaultValue = -1;
}

//---------------------------------------------------------------------------
vtkSMFieldDataDomain::~vtkSMFieldDataDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::Update(vtkSMProperty*)
{
  // We don't return here, since even if updating the domain values may be
  // disbaled, we still would want to pick a good default for this domain.
  // if (this->DisableUpdateDomainEntries)
  //  {
  //  return;
  //  }

  vtkPVDataInformation* dataInfo = this->GetInputDataInformation("Input");
  vtkSMProperty* pp = this->GetRequiredProperty("Input");
  vtkSMInputArrayDomain* iad =
    pp ? vtkSMInputArrayDomain::SafeDownCast(pp->FindDomain("vtkSMInputArrayDomain")) : NULL;

  this->UpdateDomainEntries(iad ? iad->GetAttributeType() : vtkSMInputArrayDomain::ANY, dataInfo);
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::UpdateDomainEntries(
  int acceptable_association, vtkPVDataInformation* dataInfo)
{
  std::set<int> accepted_associations;

  // iterate over all attribute types and add the "acceptable" attribute types
  // to this domain.
  for (int idx = vtkSMInputArrayDomain::POINT;
       idx < vtkSMInputArrayDomain::NUMBER_OF_ATTRIBUTE_TYPES; idx++)
  {
    if (idx == vtkSMInputArrayDomain::ANY ||
      !vtkSMInputArrayDomain::IsAttributeTypeAcceptable(acceptable_association, idx, NULL))
    {
      continue;
    }

    // Add field-data if ...
    if (
      // ... domain updates are disabled
      this->DisableUpdateDomainEntries ||
      // ... point/cell if forced.
      (this->ForcePointAndCellDataSelection &&
        (idx == vtkSMInputArrayDomain::POINT || idx == vtkSMInputArrayDomain::CELL)))
    {
      accepted_associations.insert(idx);
      continue;
    }

    // add the idx is it has some arrays.
    if (dataInfo == NULL || dataInfo->GetAttributeInformation(idx) == NULL ||
      dataInfo->GetAttributeInformation(idx)->GetMaximumNumberOfTuples() == 0)
    {
      continue;
    }
    accepted_associations.insert(idx);
  }

  // field data is added only if this->EnableFieldDataSelection is true.
  if (this->EnableFieldDataSelection)
  {
    accepted_associations.insert(vtkSMInputArrayDomain::FIELD);
  }

  if (accepted_associations.size() > 0)
  {
    // to pick a good default, find the first non-empty acceptable attribute.
    this->DefaultValue = (*accepted_associations.begin());
    for (std::set<int>::const_iterator iter = accepted_associations.begin();
         iter != accepted_associations.end(); ++iter)
    {
      vtkPVDataSetAttributesInformation* attrInfo =
        dataInfo ? dataInfo->GetAttributeInformation(*iter) : NULL;
      if (attrInfo && attrInfo->GetNumberOfArrays() > 0)
      {
        this->DefaultValue = *iter;
        break;
      }
    }
  }
  else
  {
    this->DefaultValue = -1;
  }

  // FIXME: Add ability to not modify the domain unless changed for real.
  this->RemoveAllEntries();
  for (std::set<int>::const_iterator iter = accepted_associations.begin();
       iter != accepted_associations.end(); iter++)
  {
    this->AddEntry(vtkSMFieldDataDomainAttributeTypes[*iter], *iter);
  }
}

//---------------------------------------------------------------------------
int vtkSMFieldDataDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
  if (ivp && this->DefaultValue != -1)
  {
    if (use_unchecked_values)
    {
      ivp->SetUncheckedElement(0, this->DefaultValue);
    }
    else
    {
      ivp->SetElement(0, this->DefaultValue);
    }
    return 1;
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

  int enable_field_data = 0;
  if (element->GetScalarAttribute("enable_field_data", &enable_field_data))
  {
    this->EnableFieldDataSelection = (enable_field_data != 0) ? true : false;
  }
  int disable_update_domain_entries = 0;
  if (element->GetScalarAttribute("disable_update_domain_entries", &disable_update_domain_entries))
  {
    this->DisableUpdateDomainEntries = (disable_update_domain_entries != 0) ? true : false;
  }

  int force_point_cell_data = 0;
  if (element->GetScalarAttribute("force_point_cell_data", &force_point_cell_data))
  {
    this->ForcePointAndCellDataSelection = (force_point_cell_data != 0) ? true : false;
  }

  this->UpdateDomainEntries(vtkSMInputArrayDomain::ANY, NULL);
  return 1;
}

//---------------------------------------------------------------------------
void vtkSMFieldDataDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
