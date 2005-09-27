/*=========================================================================

  Program:   ParaView
  Module:    vtkSMArraySelectionDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMArraySelectionDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMArraySelectionDomain);
vtkCxxRevisionMacro(vtkSMArraySelectionDomain, "1.1");

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::vtkSMArraySelectionDomain()
{
}

//---------------------------------------------------------------------------
vtkSMArraySelectionDomain::~vtkSMArraySelectionDomain()
{
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::Update(vtkSMProperty* prop)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(prop);
  if (svp && svp->GetInformationOnly())
    {
    this->RemoveAllStrings();
    this->SetIntDomainMode(vtkSMStringListRangeDomain::BOOLEAN);

    unsigned int numEls = svp->GetNumberOfElements();
    if (numEls % 2 != 0)
      {
      vtkErrorMacro("The required property seems to have wrong number of "
                    "elements. It should be a multiple of 2");
      return;
      }
    for (unsigned int i=0; i<numEls/2; i++)
      {
      this->AddString(svp->GetElement(i*2));
      }
    }
}

//---------------------------------------------------------------------------
void vtkSMArraySelectionDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
