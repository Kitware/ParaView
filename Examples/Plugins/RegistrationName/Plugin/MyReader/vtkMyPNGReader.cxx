/*=========================================================================

  Program:   ParaView
  Module:    vtkMyPNGReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
