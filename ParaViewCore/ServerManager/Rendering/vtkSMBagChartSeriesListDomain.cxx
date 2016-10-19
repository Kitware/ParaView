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
#include "vtkSMBagChartSeriesListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkStdString.h"

#include <assert.h>
#include <vtksys/RegularExpression.hxx>

vtkStandardNewMacro(vtkSMBagChartSeriesListDomain);
//----------------------------------------------------------------------------
vtkSMBagChartSeriesListDomain::vtkSMBagChartSeriesListDomain()
{
  this->ArrayType = -1;
}

//----------------------------------------------------------------------------
vtkSMBagChartSeriesListDomain::~vtkSMBagChartSeriesListDomain()
{
}

//----------------------------------------------------------------------------
int vtkSMBagChartSeriesListDomain::ReadXMLAttributes(vtkSMProperty* prop, vtkPVXMLElement* element)
{
  if (!this->Superclass::ReadXMLAttributes(prop, element))
  {
    return 0;
  }

  const char* arrayName = element->GetAttribute("array_type");
  if (arrayName)
  {
    if (!strcmp(arrayName, "x"))
    {
      this->ArrayType = 0;
    }
    if (!strcmp(arrayName, "y"))
    {
      this->ArrayType = 1;
    }
    if (!strcmp(arrayName, "density"))
    {
      this->ArrayType = 2;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkSMBagChartSeriesListDomain::SetDefaultValues(vtkSMProperty* prop, bool use_unchecked_values)
{
  const std::vector<vtkStdString>& strings = this->GetStrings();
  std::vector<vtkStdString>::const_iterator iter = strings.begin();

  // Search for an array with a name like HDR (x,y)
  vtksys::RegularExpression re =
    vtksys::RegularExpression("([a-zA-Z0-9]+)[ ]*\\(([a-zA-Z0-9]+),([a-zA-Z0-9]+)\\)");
  std::string hdr = "";
  while (iter != strings.end())
  {
    if (re.find(*iter))
    {
      hdr = *iter;
      break;
    }
    iter++;
  }
  if (hdr != "")
  {
    std::string x = re.match(3);
    std::string y = re.match(2);

    iter = strings.begin();
    while (iter != strings.end())
    {
      if ((this->ArrayType == 0 && (*iter) == x) || (this->ArrayType == 1 && (*iter) == y) ||
        (this->ArrayType == 2 && (*iter) == hdr))
      {
        vtkSMPropertyHelper helper(prop);
        helper.SetUseUnchecked(use_unchecked_values);
        helper.Set(iter->c_str());
        return 1;
      }
      iter++;
    }
  }

  return this->Superclass::SetDefaultValues(prop, use_unchecked_values);
}

//----------------------------------------------------------------------------
void vtkSMBagChartSeriesListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
