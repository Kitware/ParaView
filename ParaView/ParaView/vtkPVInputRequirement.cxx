/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInputRequirement.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVInputRequirement.h"

#include "vtkObjectFactory.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVDataSetAttributesInformation.h"
#include "vtkPVArrayInformation.h"
#include "vtkDataSet.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVInputRequirement);
vtkCxxRevisionMacro(vtkPVInputRequirement, "1.5");


//----------------------------------------------------------------------------
int vtkPVInputRequirement::GetIsValidInput(vtkPVSource*, vtkPVSource*)
{
  vtkErrorMacro("Requirment class did not supply a 'GetIsValidInput' method.");

  return 0;
}

//----------------------------------------------------------------------------
int vtkPVInputRequirement::GetIsValidField(int, 
                                           vtkPVDataSetAttributesInformation*)
{
  // Assume most requirements do not concern arrays.
  return 1;
}



//----------------------------------------------------------------------------
int vtkPVInputRequirement::ReadXMLAttributes(vtkPVXMLElement*,
                                             vtkPVXMLPackageParser*)
{
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVInputRequirement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}


  



