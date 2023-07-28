// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMySpecialPolyDataMapper.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLMySpecialPolyDataMapperDelegator.h"

vtkStandardNewMacro(vtkMySpecialPolyDataMapper);
//----------------------------------------------------------------------------
vtkMySpecialPolyDataMapper::vtkMySpecialPolyDataMapper() = default;

//----------------------------------------------------------------------------
vtkMySpecialPolyDataMapper::~vtkMySpecialPolyDataMapper() = default;

//----------------------------------------------------------------------------
void vtkMySpecialPolyDataMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkCompositePolyDataMapperDelegator* vtkMySpecialPolyDataMapper::CreateADelegator()
{
  return vtkOpenGLMySpecialPolyDataMapperDelegator::New();
}
