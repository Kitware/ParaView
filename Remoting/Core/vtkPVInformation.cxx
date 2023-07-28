// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVInformation.h"

//----------------------------------------------------------------------------
vtkPVInformation::vtkPVInformation()
{
  this->RootOnly = 0;
}

//----------------------------------------------------------------------------
vtkPVInformation::~vtkPVInformation() = default;

//----------------------------------------------------------------------------
void vtkPVInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "RootOnly: " << this->RootOnly << endl;
}

//----------------------------------------------------------------------------
void vtkPVInformation::CopyFromObject(vtkObject*)
{
  vtkErrorMacro("CopyFromObject not implemented.");
}

//----------------------------------------------------------------------------
void vtkPVInformation::AddInformation(vtkPVInformation*)
{
  vtkErrorMacro("AddInformation not implemented.");
}

//----------------------------------------------------------------------------
void vtkPVInformation::CopyFromStream(const vtkClientServerStream*)
{
  vtkErrorMacro("CopyFromStream not implemented.");
}
