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
#include "vtkSMCustomBoundsDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkPVDataInformation.h"

vtkStandardNewMacro(vtkSMCustomBoundsDomain);
//----------------------------------------------------------------------------
vtkSMCustomBoundsDomain::vtkSMCustomBoundsDomain()
{
}

//----------------------------------------------------------------------------
vtkSMCustomBoundsDomain::~vtkSMCustomBoundsDomain()
{
}

//----------------------------------------------------------------------------
void vtkSMCustomBoundsDomain::UpdateFromInformation(vtkPVDataInformation* info)
{
  if (!info)
    {
    return;
    }
  vtkIdType npts = info->GetNumberOfPoints();
  if (npts == 0)
    {
    npts = 1;
    }
  double bounds[6];
  info->GetBounds(bounds);

  double diag = sqrt(((bounds[1] - bounds[0]) * (bounds[1] - bounds[0])
      + (bounds[3] - bounds[2]) * (bounds[3] - bounds[2]) + (bounds[5]
        - bounds[4]) * (bounds[5] - bounds[4])) / 3.0);

  double nn = pow(static_cast<double>(npts), 1.0 / 3.0) - 1.0;
  if (nn < 1.0)
    {
    nn = 1.0;
    }
  this->AddMinimum(0, 0);
  this->AddMaximum(0, diag / nn / 2.0);
}

//----------------------------------------------------------------------------
int vtkSMCustomBoundsDomain::SetDefaultValues(vtkSMProperty* prop)
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(prop);
  if (!dvp)
    {
    vtkErrorMacro("vtkSMBoundsDomain only works on vtkSMDoubleVectorProperty.");
    return 0;
    }

  if (this->GetMaximumExists(0) && this->GetMinimumExists(0))
    {
    double min = this->GetMinimum(0);
    double max = this->GetMaximum(0);

    if (dvp->GetNumberOfElements() == 2)
      {
      dvp->SetElement(0, min);
      dvp->SetElement(1, max);
      return 1;
      }
    else if(dvp->GetNumberOfElements() == 1)
      {
      dvp->SetElement(0, max);
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSMCustomBoundsDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
