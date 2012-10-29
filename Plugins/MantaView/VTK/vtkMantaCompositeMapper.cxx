/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMantaCompositeMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMantaCompositeMapper.h"

#include "vtkMantaPolyDataMapper.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMantaCompositeMapper);

vtkMantaCompositeMapper::vtkMantaCompositeMapper()
{
}

vtkMantaCompositeMapper::~vtkMantaCompositeMapper()
{
}

vtkPolyDataMapper * vtkMantaCompositeMapper::MakeAMapper()
{
  return vtkMantaPolyDataMapper::New();
}

void vtkMantaCompositeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
