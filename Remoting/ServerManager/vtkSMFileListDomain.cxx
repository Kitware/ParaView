// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMFileListDomain.h"

#include "vtkObjectFactory.h"
#include "vtkSMStringVectorProperty.h"

vtkStandardNewMacro(vtkSMFileListDomain);

//---------------------------------------------------------------------------
vtkSMFileListDomain::vtkSMFileListDomain() = default;

//---------------------------------------------------------------------------
vtkSMFileListDomain::~vtkSMFileListDomain() = default;

//---------------------------------------------------------------------------
void vtkSMFileListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
