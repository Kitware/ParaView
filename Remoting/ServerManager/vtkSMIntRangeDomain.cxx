/*=========================================================================

  Program:   ParaView
  Module:    vtkSMIntRangeDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Instantiate superclass first to give the template a DLL interface.
#include "vtkSMRangeDomainTemplate.txx"
VTK_SM_RANGE_DOMAIN_TEMPLATE_INSTANTIATE(int);

#define vtkSMIntRangeDomain_cxx
#include "vtkSMIntRangeDomain.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMIntRangeDomain);
//---------------------------------------------------------------------------
vtkSMIntRangeDomain::vtkSMIntRangeDomain()
{
}

//---------------------------------------------------------------------------
vtkSMIntRangeDomain::~vtkSMIntRangeDomain()
{
}

////---------------------------------------------------------------------------
// int vtkSMIntRangeDomain::SetDefaultValues(vtkSMProperty* prop)
//{
//  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(prop);
//  if (!ivp || this->GetNumberOfRequiredProperties() == 0)
//    {
//    // If number of required properties is 0, this domain
//    // does not change based on values for some other property
//    // in that case, the property default does not depend on the domain.
//    return this->Superclass::SetDefaultValues(prop);
//    }
//  int updated = 0;
//  unsigned int numEls = ivp->GetNumberOfElements();
//  for (unsigned int i=0; i<numEls; i++)
//    {
//    if ( i % 2 == 0)
//      {
//      if (this->GetMinimumExists(i/2))
//        {
//        ivp->SetElement(i, this->GetMinimum(i/2));
//        updated = 1;
//        }
//      }
//    else
//      {
//      if (this->GetMaximumExists(i/2))
//        {
//        ivp->SetElement(i, this->GetMaximum(i/2));
//        updated = 1;
//        }
//      }
//    }
//  return updated;
//}

//---------------------------------------------------------------------------
void vtkSMIntRangeDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
