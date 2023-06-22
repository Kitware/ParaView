/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBumpMapMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBumpMapMapper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLBumpMapMapperDelegator.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkBumpMapMapper);

//-----------------------------------------------------------------------------
void vtkBumpMapMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "BumpMappingFactor: " << this->BumpMappingFactor << endl;
}

//-----------------------------------------------------------------------------
vtkCompositePolyDataMapperDelegator* vtkBumpMapMapper::CreateADelegator()
{
  return vtkOpenGLBumpMapMapperDelegator::New();
}
