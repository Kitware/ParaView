// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMChartSeriesListDomain.h"

#include "vtkDataObject.h"
#include "vtkObjectFactory.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkPVXMLElement.h"
#include "vtkSMArrayListDomain.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMUncheckedPropertyHelper.h"

#include <algorithm>
#include <cassert>
#include <vector>

vtkStandardNewMacro(vtkSMChartSeriesListDomain);
//----------------------------------------------------------------------------
vtkSMChartSeriesListDomain::vtkSMChartSeriesListDomain()
{
  this->HidePartialArrays = true;
}

//----------------------------------------------------------------------------
vtkSMChartSeriesListDomain::~vtkSMChartSeriesListDomain() = default;

//----------------------------------------------------------------------------
void vtkSMChartSeriesListDomain::Update(vtkSMProperty*)
{
  vtkSMProperty* input = this->GetRequiredProperty("Input");
  vtkSMProperty* fieldDataSelection = this->GetRequiredProperty("FieldDataSelection");

  if (!input || !fieldDataSelection)
  {
    vtkWarningMacro("Missing required properties. Update failed.");
    return;
  }

  // build strings based on the current domain.
  vtkPVDataInformation* dataInfo = this->GetInputInformation();
  if (!dataInfo)
  {
    return;
  }

  std::vector<std::string> strings;
  int fieldAssociation = vtkSMUncheckedPropertyHelper(fieldDataSelection).GetAsInt(0);
  vtkPVDataSetAttributesInformation* dsa = dataInfo->GetAttributeInformation(fieldAssociation);

  for (int cc = 0; dsa != nullptr && cc < dsa->GetNumberOfArrays(); cc++)
  {
    this->PopulateArrayComponents(dsa->GetArrayInformation(cc), strings);
  }

  // Process point coordinates array
  if (fieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS)
  {
    this->PopulateArrayComponents(dataInfo->GetPointArrayInformation(), strings);
  }

  this->SetStrings(strings);
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesListDomain::PopulateArrayComponents(
  vtkPVArrayInformation* arrayInfo, std::vector<std::string>& strings)
{
  if (arrayInfo && (this->HidePartialArrays == false || arrayInfo->GetIsPartial() == 0))
  {
    int dataType = arrayInfo->GetDataType();

    if (dataType != VTK_STRING && dataType != VTK_VARIANT)
    {
      if (arrayInfo->GetNumberOfComponents() > 1)
      {
        for (int kk = 0; kk <= arrayInfo->GetNumberOfComponents(); kk++)
        {
          strings.push_back(vtkSMArrayListDomain::CreateMangledName(arrayInfo, kk));
        }
      }
      else
      {
        const char* arrayName = arrayInfo->GetName();
        if (arrayName)
        {
          strings.push_back(arrayName);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMChartSeriesListDomain::GetInputInformation()
{
  vtkSMProperty* inputProperty = this->GetRequiredProperty("Input");
  assert(inputProperty);

  vtkSMUncheckedPropertyHelper helper(inputProperty);
  if (helper.GetNumberOfElements() > 0)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(helper.GetAsProxy(0));
    if (sp)
    {
      return sp->GetDataInformation(helper.GetOutputPort());
    }
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int vtkSMChartSeriesListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  int hide_partial_arrays;
  if (element->GetScalarAttribute("hide_partial_arrays", &hide_partial_arrays))
  {
    this->HidePartialArrays = (hide_partial_arrays == 1);
  }

  if (!this->GetRequiredProperty("Input"))
  {
    vtkWarningMacro("Missing 'Input' property. Domain may not work correctly.");
  }
  if (!this->GetRequiredProperty("FieldDataSelection"))
  {
    vtkWarningMacro("Missing 'FieldDataSelection' property. Domain may not work correctly.");
  }

  return 1;
}

//----------------------------------------------------------------------------
const char** vtkSMChartSeriesListDomain::GetKnownSeriesNames()
{
  static std::vector<const char*> staticStorage;

  // Convert vector of std::string to a const char**
  const auto& knownSeries =
    vtkPVRepresentedArrayListSettings::GetInstance()->GetAllChartsDefaultXAxis();
  staticStorage.resize(knownSeries.size() + 1);
  for (std::size_t i = 0; i < knownSeries.size(); ++i)
  {
    staticStorage[i] = knownSeries[i].c_str();
  }
  staticStorage[knownSeries.size()] = nullptr;

  return staticStorage.data();
}
//----------------------------------------------------------------------------
int vtkSMChartSeriesListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  const auto& strings_to_check =
    vtkPVRepresentedArrayListSettings::GetInstance()->GetAllChartsDefaultXAxis();

  const std::vector<std::string> domain_strings = this->GetStrings();
  for (const std::string& array_name : strings_to_check)
  {
    if (std::find(domain_strings.cbegin(), domain_strings.cend(), array_name) !=
      domain_strings.end())
    {
      vtkSMPropertyHelper helper(prop);
      helper.SetUseUnchecked(use_unchecked_values);
      helper.Set(array_name.c_str());
      return 1;
    }
  }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMChartSeriesListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "HidePartialArrays: " << this->HidePartialArrays << endl;
}
