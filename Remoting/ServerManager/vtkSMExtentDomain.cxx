/*=========================================================================

  Program:   ParaView
  Module:    vtkSMExtentDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMExtentDomain.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkSMInputProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSourceProxy.h"

#include <cmath>

vtkStandardNewMacro(vtkSMExtentDomain);

//---------------------------------------------------------------------------
vtkSMExtentDomain::vtkSMExtentDomain() = default;

//---------------------------------------------------------------------------
vtkSMExtentDomain::~vtkSMExtentDomain() = default;

//---------------------------------------------------------------------------
void vtkSMExtentDomain::Update(vtkSMProperty*)
{
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(this->GetRequiredProperty("Input"));
  if (pp)
  {
    this->Update(pp);
  }
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::Update(vtkSMProxyProperty* pp)
{
  vtkSMInputProperty* ip = vtkSMInputProperty::SafeDownCast(pp);

  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (unsigned int i = 0; i < numProxs; i++)
  {
    vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
    {
      vtkPVDataInformation* info =
        sp->GetDataInformation((ip ? ip->GetUncheckedOutputPortForConnection(i) : 0));
      if (!info)
      {
        continue;
      }
      int extent[6];
      info->GetExtent(extent);

      std::vector<vtkEntry> entries;
      for (int j = 0; j < 3; j++)
      {
        entries.push_back(vtkEntry(extent[2 * j], extent[2 * j + 1]));
      }
      this->SetEntries(entries);
      return;
    }
  }
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::SetAnimationValue(vtkSMProperty* property, int idx, double value)
{
  int compare;
  int animValue = (int)(floor(value));

  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ivp)
  {
    switch (idx)
    {
      case 0:
      case 2:
      case 4:
        compare = ivp->GetElement(idx + 1);
        if (animValue > compare)
        {
          ivp->SetElement(idx + 1, animValue);
        }
        ivp->SetElement(idx, animValue);
        break;
      case 1:
      case 3:
      case 5:
        compare = ivp->GetElement(idx - 1);
        if (animValue < compare)
        {
          ivp->SetElement(idx - 1, animValue);
        }
        ivp->SetElement(idx, animValue);
        break;
      default:
        vtkErrorMacro("Invalid extent index.");
        break;
    }
  }
}

//---------------------------------------------------------------------------
int vtkSMExtentDomain::SetDefaultValues(vtkSMProperty* property, bool use_unchecked_values)
{
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(property);
  unsigned int numElems = ivp ? ivp->GetNumberOfElements() : 0;
  if (ivp && (numElems % 2 == 0))
  {
    vtkSMPropertyHelper helper(property);
    helper.SetUseUnchecked(use_unchecked_values);
    for (unsigned int cc = 0; cc < numElems / 2; cc++)
    {
      if (this->GetMinimumExists(cc))
      {
        helper.Set(2 * cc, this->GetMinimum(cc));
      }
      if (this->GetMaximumExists(cc))
      {
        helper.Set(2 * cc + 1, this->GetMaximum(cc));
      }
    }
    return 1;
  }
  return this->Superclass::SetDefaultValues(property, use_unchecked_values);
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
