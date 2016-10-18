/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetToPiston.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDataSetToPiston.h"

#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVDataSetToPiston);

//----------------------------------------------------------------------------
vtkPVDataSetToPiston::vtkPVDataSetToPiston()
{
  vtkPVDataInformation::RegisterHelper("vtkPistonDataObject", "vtkPistonInformationHelper");
}

//----------------------------------------------------------------------------
vtkPVDataSetToPiston::~vtkPVDataSetToPiston()
{
}

//----------------------------------------------------------------------------
void vtkPVDataSetToPiston::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
