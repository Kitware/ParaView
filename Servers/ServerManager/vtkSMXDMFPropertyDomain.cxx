/*=========================================================================

  Program:   ParaView
  Module:    vtkSMXDMFPropertyDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMXDMFPropertyDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMXDMFPropertyDomain);
vtkCxxRevisionMacro(vtkSMXDMFPropertyDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMXDMFPropertyDomain::vtkSMXDMFPropertyDomain()
{
}

//---------------------------------------------------------------------------
vtkSMXDMFPropertyDomain::~vtkSMXDMFPropertyDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMXDMFPropertyDomain::Update(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    this->RemoveAllStrings();
    this->RemoveAllMinima();
    this->RemoveAllMaxima();

    unsigned int numEls = svp->GetNumberOfElements();
    if (numEls % 5 != 0)
      {
      vtkErrorMacro("The required property seems to have wrong number of "
                    "elements. It should be a multiple of 5");
      return;
      }
    for (unsigned int i=0; i<numEls/5; i++)
      {
      this->AddString(svp->GetElement(i*5));
      int min = atoi(svp->GetElement(i*5+2));
      this->AddMinimum(i, min);
      int max = min + atoi(svp->GetElement(i*5+4)) - 1;
      this->AddMaximum(i, max);
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMXDMFPropertyDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
