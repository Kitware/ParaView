// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkANARIHidingDecorator.h"
#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkANARIHidingDecorator);

//-----------------------------------------------------------------------------
void vtkANARIHidingDecorator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool vtkANARIHidingDecorator::CanShow([[maybe_unused]] bool showAdvanced) const
{
#if VTK_MODULE_ENABLE_VTK_RenderingAnari
  return this->Superclass::CanShow(showAdvanced);
#else
  return false;
#endif
}
