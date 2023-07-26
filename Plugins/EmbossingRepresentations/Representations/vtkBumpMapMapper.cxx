// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-CLAUSE
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
