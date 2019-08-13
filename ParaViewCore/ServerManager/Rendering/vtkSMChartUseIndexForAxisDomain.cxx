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
#include "vtkSMChartSeriesListDomain.h"
#include "vtkSMPropertyHelper.h"

vtkStandardNewMacro(vtkSMChartUseIndexForAxisDomain);
//----------------------------------------------------------------------------
vtkSMChartUseIndexForAxisDomain::vtkSMChartUseIndexForAxisDomain()
{
}

//----------------------------------------------------------------------------
vtkSMChartUseIndexForAxisDomain::~vtkSMChartUseIndexForAxisDomain()
{
}

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

    const char* value = helper.GetAsString();
    const char** known_names = vtkSMChartSeriesListDomain::GetKnownSeriesNames();
    for (int cc = 0; known_names[cc] != NULL && value != NULL; cc++)
    {
      if (strstr(value, known_names[cc]) != nullptr)
      {
        helper2.Set(0);
        return 1;
      }
    }
    helper2.Set(1);
    return 1;
  }

  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMChartUseIndexForAxisDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
