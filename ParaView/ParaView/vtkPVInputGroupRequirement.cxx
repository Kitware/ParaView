/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputGroupRequirement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputGroupRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVSource.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkDataSet.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputGroupRequirement);
vtkCxxRevisionMacro(vtkPVInputGroupRequirement, "1.5");

//----------------------------------------------------------------------------
vtkPVInputGroupRequirement::vtkPVInputGroupRequirement()
{
  this->Quantity = 1;
}

//----------------------------------------------------------------------------
int vtkPVInputGroupRequirement::ReadXMLAttributes(vtkPVXMLElement* element,
                                                  vtkPVXMLPackageParser*)
{
  const char* rAttr;

  rAttr = element->GetAttribute("quantity");
  if (rAttr) 
    {
    if (strcmp(rAttr, "Multiple") == 0)
      {
      this->Quantity = -1;
      }
    else if (strcmp(rAttr, "Single") == 0)
      {
      this->Quantity = 1;
      }
    else 
      {
      this->Quantity = atoi(rAttr);
      }
    } 

  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputGroupRequirement::GetIsValidInput(vtkPVSource* input, vtkPVSource*)
{
  int num = input->GetNumberOfParts();
  if (this->Quantity == -1 && num > 1)
    {
    return 1;
    }
  if (this->Quantity > 0 && this->Quantity == num)
    {
    return 1;
    }

  return 0;
}



//----------------------------------------------------------------------------
void vtkPVInputGroupRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if (this->Quantity == -1)
    {
    os << indent << "Quantity: Multiple" << endl;
    }
  else
    {  
    os << indent << "Quantity: " << this->Quantity << endl;
    }
}


  



