/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMFunctionalBagChartSeriesSelectionDomain.h"

#include "vtkObjectFactory.h"
#include "vtkStdString.h"
#include "vtksys/SystemTools.hxx"

#include <vector>

vtkStandardNewMacro(vtkSMFunctionalBagChartSeriesSelectionDomain);

//----------------------------------------------------------------------------
vtkSMFunctionalBagChartSeriesSelectionDomain::vtkSMFunctionalBagChartSeriesSelectionDomain()
{
}

//----------------------------------------------------------------------------
vtkSMFunctionalBagChartSeriesSelectionDomain::~vtkSMFunctionalBagChartSeriesSelectionDomain()
{
}

//----------------------------------------------------------------------------
bool vtkSMFunctionalBagChartSeriesSelectionDomain::GetDefaultSeriesVisibility(const char* name)
{
  return (vtksys::SystemTools::StringStartsWith(name, "Q") ||
    vtksys::SystemTools::StringEndsWith(name, "_outlier"));
}

//----------------------------------------------------------------------------
std::vector<vtkStdString> vtkSMFunctionalBagChartSeriesSelectionDomain::GetDefaultValue(const char* series)
{
  if (this->DefaultMode == LABEL)
    {
    std::vector<vtkStdString> values;
    // Remove _outlier extension in the series label
    std::string name(series);
    if (vtksys::SystemTools::StringEndsWith(series, "_outlier"))
      {
      vtksys::SystemTools::ReplaceString(name, "_outlier", "");
      }
    else if (name == "Q3Points")
      {
      name = "99%";
      }
    else if (name == "QMedPoints")
      {
      name = "50%";
      }
    else if (name == "QMedianLine")
      {
      name = "Median";
      }
    values.push_back(name.c_str());
    return values;
    }
  return this->Superclass::GetDefaultValue(series);
}

//----------------------------------------------------------------------------
void vtkSMFunctionalBagChartSeriesSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
