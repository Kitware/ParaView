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
  return vtksys::SystemTools::StringStartsWith(name, "Q3Points") ||
    vtksys::SystemTools::StringStartsWith(name, "QMedPoints") ||
    vtksys::SystemTools::StringEndsWith(name, "_outlier") ||
    vtksys::SystemTools::StringEndsWith(name, "_median");
}

//----------------------------------------------------------------------------
std::vector<std::string> vtkSMFunctionalBagChartSeriesSelectionDomain::GetDefaultValue(
  const char* series)
{
  std::vector<std::string> values;
  std::string name(series);
  if (this->DefaultMode == LABEL)
  {
    // Remove _outlier extension in the series label
    if (vtksys::SystemTools::StringEndsWith(series, "_outlier"))
    {
      vtksys::SystemTools::ReplaceString(name, "_outlier", "");
    }
    else if (vtksys::SystemTools::StringStartsWith(name, "Q3Points"))
    {
      // Fetch the ratio after the "Q3Points" string
      name = name.substr(8) + std::string("%");
    }
    else if (name == "QMedPoints")
    {
      name = "50%";
    }
    else if (vtksys::SystemTools::StringEndsWith(name, "_median"))
    {
      vtksys::SystemTools::ReplaceString(name, "_median", "");
      name = "Median (" + name + ")";
    }
    values.push_back(name.c_str());
    return values;
  }
  else if (this->DefaultMode == COLOR)
  {
    if (vtksys::SystemTools::StringStartsWith(name, "Q3Points"))
    {
      values.push_back("0.50");
      values.push_back("0.00");
      values.push_back("0.00");
      return values;
    }
    else if (name == "QMedPoints")
    {
      values.push_back("0.75");
      values.push_back("0.00");
      values.push_back("0.00");
      return values;
    }
    else if (vtksys::SystemTools::StringEndsWith(name, "_median"))
    {
      values.push_back("0.00");
      values.push_back("0.00");
      values.push_back("0.00");
      return values;
    }
  }
  return this->Superclass::GetDefaultValue(series);
}

//----------------------------------------------------------------------------
void vtkSMFunctionalBagChartSeriesSelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
