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
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMSourceProxy.h"

vtkStandardNewMacro(vtkSMExtentDomain);
vtkCxxRevisionMacro(vtkSMExtentDomain, "1.2.2.1");

//---------------------------------------------------------------------------
vtkSMExtentDomain::vtkSMExtentDomain()
{
}

//---------------------------------------------------------------------------
vtkSMExtentDomain::~vtkSMExtentDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::Update(vtkSMProperty*)
{
  this->RemoveAllMinima();
  this->RemoveAllMaxima();
  
  vtkSMProxyProperty *pp = vtkSMProxyProperty::SafeDownCast(
    this->GetRequiredProperty("Input"));
  if (pp)
    {
    this->Update(pp);
    }
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::Update(vtkSMProxyProperty *pp)
{
  unsigned int i, j;
  unsigned int numProxs = pp->GetNumberOfUncheckedProxies();
  for (i = 0; i < numProxs; i++)
    {
    vtkSMSourceProxy* sp =
      vtkSMSourceProxy::SafeDownCast(pp->GetUncheckedProxy(i));
    if (sp)
      {
      vtkPVDataInformation *info = sp->GetDataInformation();
      if (!info)
        {
        return;
        }
      int extent[6];
      info->GetExtent(extent);
      for (j = 0; j < 3; j++)
        {
        this->AddMinimum(j, extent[2*j]);
        this->AddMaximum(j, extent[2*j+1]);
        }
      return;
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMExtentDomain::SetAnimationValue(vtkSMProperty *property, int idx,
                                          double value)
{
  int compare;
  int animValue = (int)(floor(value));
  
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(property);
  if (ivp)
    {
    switch (idx)
      {
      case 0:
      case 2:
      case 4:
        compare = ivp->GetElement(idx+1);
        if (animValue > compare)
          {
          ivp->SetElement(idx+1, animValue);
          }
        ivp->SetElement(idx, animValue);
        break;
      case 1:
      case 3:
      case 5:
        compare = ivp->GetElement(idx-1);
        if (animValue < compare)
          {
          ivp->SetElement(idx-1, animValue);
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
void vtkSMExtentDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
