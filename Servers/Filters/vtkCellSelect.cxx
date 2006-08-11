/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellSelect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellSelect.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkCellSelect, "1.2");
vtkStandardNewMacro(vtkCellSelect);

//----------------------------------------------------------------------------
vtkCellSelect::vtkCellSelect()
{
}

//----------------------------------------------------------------------------
vtkCellSelect::~vtkCellSelect()
{
}

//----------------------------------------------------------------------------
int vtkCellSelect::RequestData(
  vtkInformation *vtkNotUsed(r),
  vtkInformationVector **vtkNotUsed(iv),
  vtkInformationVector *vtkNotUsed(ov))
{ 
  //This is just a dummy filter for testing purposes.
  return 1;
}


//----------------------------------------------------------------------------
void vtkCellSelect::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

