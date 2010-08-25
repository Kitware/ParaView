/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkPVPostFilterExecutive.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPostFilterExecutive.h"

#include "vtkCommand.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVPostFilterExecutive);

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::vtkPVPostFilterExecutive()
{

}

//----------------------------------------------------------------------------
vtkPVPostFilterExecutive::~vtkPVPostFilterExecutive()
{

}

//----------------------------------------------------------------------------
int vtkPVPostFilterExecutive::NeedToExecuteData(
  int outputPort,
  vtkInformationVector** inInfoVec,
  vtkInformationVector* outInfoVec)
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPostFilterExecutive::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
