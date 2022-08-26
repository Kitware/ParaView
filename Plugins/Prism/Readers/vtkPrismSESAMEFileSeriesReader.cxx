/*=========================================================================

  Program:   ParaView
  Module:    vtkPrismSESAMEFileSeriesReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
