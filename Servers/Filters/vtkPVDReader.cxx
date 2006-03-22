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

vtkCxxRevisionMacro(vtkPVDReader, "1.3");
vtkStandardNewMacro(vtkPVDReader);


//----------------------------------------------------------------------------
vtkPVDReader::vtkPVDReader()
{
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = 0;
}

//----------------------------------------------------------------------------
vtkPVDReader::~vtkPVDReader()
{
}

//----------------------------------------------------------------------------
void vtkPVDReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "TimeStepRange: "
     << this->TimeStepRange[0] << " "
     << this->TimeStepRange[1] << "\n";
}

//----------------------------------------------------------------------------
void vtkPVDReader::SetTimeStep(int index)
{
  this->SetRestrictionAsIndex("timestep", index);
}

//----------------------------------------------------------------------------
int vtkPVDReader::GetTimeStep()
{
  return this->GetRestrictionAsIndex("timestep");
}


void vtkPVDReader::SetupOutputInformation(vtkInformation *outInfo)
{
  this->Superclass::SetupOutputInformation(outInfo);

  int index = this->GetAttributeIndex("timestep");
  this->TimeStepRange[0] = 0;
  this->TimeStepRange[1] = this->GetNumberOfAttributeValues(index)-1;
  if (this->TimeStepRange[1] == -1)
    {
    this->TimeStepRange[1] = 0;
    }
}




