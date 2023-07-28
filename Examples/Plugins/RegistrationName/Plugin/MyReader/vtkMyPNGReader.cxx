// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkMyPNGReader.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro(vtkMyPNGReader);

//----------------------------------------------------------------------------
vtkMyPNGReader::vtkMyPNGReader() = default;

//----------------------------------------------------------------------------
vtkMyPNGReader::~vtkMyPNGReader() = default;

//----------------------------------------------------------------------------
const char* vtkMyPNGReader::GetRegistrationName()
{
  return "ReaderDefinedName";
}

//----------------------------------------------------------------------------
void vtkMyPNGReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
