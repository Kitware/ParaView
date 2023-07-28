// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkUndoElement.h"

#include "vtkObjectFactory.h"

//-----------------------------------------------------------------------------
vtkUndoElement::vtkUndoElement()
{
  this->Mergeable = false;
}

//-----------------------------------------------------------------------------
vtkUndoElement::~vtkUndoElement() = default;

//-----------------------------------------------------------------------------
void vtkUndoElement::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mergeable: " << this->Mergeable << endl;
}
