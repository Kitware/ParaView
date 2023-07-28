// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPGridBuilder.h"

#include "vtkCPBaseFieldBuilder.h"

vtkCxxSetObjectMacro(vtkCPGridBuilder, FieldBuilder, vtkCPBaseFieldBuilder);

//----------------------------------------------------------------------------
vtkCPGridBuilder::vtkCPGridBuilder()
{
  this->FieldBuilder = nullptr;
}

//----------------------------------------------------------------------------
vtkCPGridBuilder::~vtkCPGridBuilder()
{
  this->SetFieldBuilder(nullptr);
}

//----------------------------------------------------------------------------
vtkCPBaseFieldBuilder* vtkCPGridBuilder::GetFieldBuilder()
{
  return this->FieldBuilder;
}

//----------------------------------------------------------------------------
void vtkCPGridBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldBuilder: " << this->FieldBuilder << endl;
}
