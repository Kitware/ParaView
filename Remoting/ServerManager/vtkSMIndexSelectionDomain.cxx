/*==============================================================================

  Program:   Visualization Toolkit
  Module:    vtkSMIndexSelectionDomain.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

==============================================================================*/

#include "vtkSMIndexSelectionDomain.h"

#include "vtkSMStringVectorProperty.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMIndexSelectionDomain)

  //------------------------------------------------------------------------------
  void vtkSMIndexSelectionDomain::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkSMIndexSelectionDomain::IsInDomain(vtkSMProperty* property)
{
  vtkSMStringVectorProperty* svp = vtkSMStringVectorProperty::SafeDownCast(property);
  if (!svp)
  {
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
int vtkSMIndexSelectionDomain::SetDefaultValues(vtkSMProperty* property, bool useUnchecked)
{
  vtkSMStringVectorProperty* svpInfo =
    vtkSMStringVectorProperty::SafeDownCast(this->GetInfoProperty());
  vtkSMStringVectorProperty* svpOut = vtkSMStringVectorProperty::SafeDownCast(property);

  if (svpOut && svpInfo && svpInfo->GetNumberOfElements() % 3 == 0)
  {
    svpOut->SetNumberOfElements(svpInfo->GetNumberOfElements() / 3 * 2);
    for (unsigned int infoIdx = 0, outIdx = 0; infoIdx < svpInfo->GetNumberOfElements();
         infoIdx += 3, outIdx += 2)
    {
      // Copy the dimension name
      svpOut->SetElement(outIdx, svpInfo->GetElement(infoIdx));
      // And the current value
      svpOut->SetElement(outIdx + 1, svpInfo->GetElement(infoIdx + 1));
      // Omit the max size.
    }

    return 1;
  }

  return this->Superclass::SetDefaultValues(property, useUnchecked);
}

//------------------------------------------------------------------------------
vtkSMIndexSelectionDomain::vtkSMIndexSelectionDomain()
{
}

//------------------------------------------------------------------------------
vtkSMIndexSelectionDomain::~vtkSMIndexSelectionDomain()
{
}
