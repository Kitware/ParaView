/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputFixedTypeRequirement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputFixedTypeRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVSource.h"
#include "vtkSMPart.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputFixedTypeRequirement);
vtkCxxRevisionMacro(vtkPVInputFixedTypeRequirement, "1.8");

//----------------------------------------------------------------------------
vtkPVInputFixedTypeRequirement::vtkPVInputFixedTypeRequirement()
{
}

//----------------------------------------------------------------------------
int vtkPVInputFixedTypeRequirement::ReadXMLAttributes(vtkPVXMLElement*,
                                                      vtkPVXMLPackageParser*)
{
  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputFixedTypeRequirement::GetIsValidInput(vtkPVSource* newInput, 
                                                    vtkPVSource* pvs)
{
  vtkPVDataInformation *info1;
  vtkPVDataInformation *info2;
  vtkPVSource* oldInput;
  int idx, num;

  if (newInput == NULL)
    {
    return 0;
    }

  if (pvs->GetNumberOfPVInputs() == 0)
    {
    // Must be a prototype.
    return 1;
    }
  // Only worry about the first input for now.
  // We have no multiple data set to data set filters.
  oldInput = pvs->GetPVInput(0);
  num = oldInput->GetNumberOfParts();

  if (newInput->GetNumberOfParts() != num)
    {
    return 0;
    }
  for (idx = 0; idx < num; ++idx)
    {
    info1 = newInput->GetPart(idx)->GetDataInformation();
    info2 = oldInput->GetPart(idx)->GetDataInformation();
    if (info1->GetDataSetType() != info2->GetDataSetType())
      {
      return 0;
      }
    }
 
  return 1;
}



//----------------------------------------------------------------------------
void vtkPVInputFixedTypeRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



