// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMNullProxy.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSMNullProxy);
//-----------------------------------------------------------------------------
vtkSMNullProxy::vtkSMNullProxy() = default;

//-----------------------------------------------------------------------------
vtkSMNullProxy::~vtkSMNullProxy() = default;

//-----------------------------------------------------------------------------
void vtkSMNullProxy::CreateVTKObjects()
{
  this->SetVTKClassName(nullptr);
  this->Superclass::CreateVTKObjects();
}

//-----------------------------------------------------------------------------
void vtkSMNullProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
