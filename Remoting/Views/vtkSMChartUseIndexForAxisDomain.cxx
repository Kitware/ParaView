/*=========================================================================

  Program:   ParaView
  Module:    vtkSMChartUseIndexForAxisDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMChartUseIndexForAxisDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMChartUseIndexForAxisDomain);
//----------------------------------------------------------------------------
vtkSMChartUseIndexForAxisDomain::vtkSMChartUseIndexForAxisDomain() = default;

//----------------------------------------------------------------------------
vtkSMChartUseIndexForAxisDomain::~vtkSMChartUseIndexForAxisDomain() = default;

//----------------------------------------------------------------------------
void vtkSMChartUseIndexForAxisDomain::Update(vtkSMProperty* requestingProperty)
{
  this->Superclass::Update(requestingProperty);

  vtkSMProperty* arrayName = this->GetRequiredProperty("ArraySelection");
  if (requestingProperty == arrayName)
  {
    this->DomainModified();
  }
}

//----------------------------------------------------------------------------
int vtkSMChartUseIndexForAxisDomain::SetDefaultValues(
  vtkSMProperty* property, bool use_unchecked_values)
{
  vtkSMProperty* arrayName = this->GetRequiredProperty("ArraySelection");
  if (arrayName)
  {
    vtkSMPropertyHelper helper(arrayName);
    helper.SetUseUnchecked(use_unchecked_values);

    vtkSMPropertyHelper helper2(property);
    helper2.SetUseUnchecked(use_unchecked_values);

    const std::string value = helper.GetAsString();
    if (!value.empty())
    {
      const auto& known_names =
        vtkPVRepresentedArrayListSettings::GetInstance()->GetAllChartsDefaultXAxis();
      for (const std::string& name : known_names)
      {
        if (value.find(name) != std::string::npos)
        {
          helper2.Set(false);
          return 1;
        }
      }
    }
    helper2.Set(true);
    return 1;
  }

  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMChartUseIndexForAxisDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
