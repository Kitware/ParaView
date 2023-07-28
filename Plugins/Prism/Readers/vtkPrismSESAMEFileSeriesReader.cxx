// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPrismSESAMEFileSeriesReader.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPrismSESAMEFileSeriesReader);

//----------------------------------------------------------------------------
vtkPrismSESAMEFileSeriesReader::vtkPrismSESAMEFileSeriesReader()
{
  this->SetNumberOfOutputPorts(2);
}

//----------------------------------------------------------------------------
vtkPrismSESAMEFileSeriesReader::~vtkPrismSESAMEFileSeriesReader() = default;

//----------------------------------------------------------------------------
void vtkPrismSESAMEFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
