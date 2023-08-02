// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVEnvironmentInformationHelper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVEnvironmentInformationHelper);

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::vtkPVEnvironmentInformationHelper()
{
  this->Variable = nullptr;
}

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::~vtkPVEnvironmentInformationHelper()
{
  this->SetVariable(nullptr);
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Variable: " << (this->Variable ? this->Variable : "(null)") << endl;
}
