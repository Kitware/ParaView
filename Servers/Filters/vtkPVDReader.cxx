/*=========================================================================

  Program:   ParaView
  Module:    vtkPVDReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVDReader.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVDReader, "1.1");
vtkStandardNewMacro(vtkPVDReader);

//----------------------------------------------------------------------------
vtkPVDReader::vtkPVDReader()
{  
}

//----------------------------------------------------------------------------
vtkPVDReader::~vtkPVDReader()
{
}

//----------------------------------------------------------------------------
void vtkPVDReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkPVDReader::SetTimestepAsIndex(int index)
{
  this->SetRestrictionAsIndex("timestep", index);
}

//----------------------------------------------------------------------------
int vtkPVDReader::GetTimestepAsIndex()
{
  return this->GetRestrictionAsIndex("timestep");
}
