/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVExtractArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVExtractArraysOverTime.h"

#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVExtractSelection.h"

vtkStandardNewMacro(vtkPVExtractArraysOverTime);

//----------------------------------------------------------------------------
vtkPVExtractArraysOverTime::vtkPVExtractArraysOverTime()
{
  vtkNew<vtkPVExtractSelection> se;
  this->SetSelectionExtractor(se.GetPointer());
}

//----------------------------------------------------------------------------
vtkPVExtractArraysOverTime::~vtkPVExtractArraysOverTime()
{
}

//----------------------------------------------------------------------------
void vtkPVExtractArraysOverTime::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
