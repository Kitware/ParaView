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
int vtkSMChartUseIndexForAxisDomain::SetDefaultValues(vtkSMProperty* property)
{
  vtkSMProperty* arrayName = this->GetRequiredProperty("ArraySelection");
  if (arrayName)
    {
    vtkSMPropertyHelper helper(arrayName);
    const char* value = helper.GetAsString();
    const char** known_names =
      vtkSMChartSeriesListDomain::GetKnownSeriesNames();
    for (int cc=0; known_names[cc] != NULL && value != NULL; cc++)
      {
      if (strcmp(known_names[cc], value) == 0)
        {
        vtkSMPropertyHelper(property).Set(0);
        return 1;
        }
      }
    vtkSMPropertyHelper(property).Set(1);
    return 1;
    }

  return this->Superclass::SetDefaultValues(property);
}

//----------------------------------------------------------------------------
void vtkSMChartUseIndexForAxisDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
