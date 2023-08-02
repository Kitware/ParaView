// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCPFieldBuilder.h"

#include "vtkCPTensorFieldFunction.h"

vtkCxxSetObjectMacro(vtkCPFieldBuilder, TensorFieldFunction, vtkCPTensorFieldFunction);

//----------------------------------------------------------------------------
vtkCPFieldBuilder::vtkCPFieldBuilder()
{
  this->ArrayName = nullptr;
  this->TensorFieldFunction = nullptr;
}

//----------------------------------------------------------------------------
vtkCPFieldBuilder::~vtkCPFieldBuilder()
{
  this->SetArrayName(nullptr);
  this->SetTensorFieldFunction(nullptr);
}

//----------------------------------------------------------------------------
vtkCPTensorFieldFunction* vtkCPFieldBuilder::GetTensorFieldFunction()
{
  return this->TensorFieldFunction;
}

//----------------------------------------------------------------------------
void vtkCPFieldBuilder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ArrayName: " << this->ArrayName << endl;
  os << indent << "TensorFieldFunction: " << this->TensorFieldFunction << endl;
}
