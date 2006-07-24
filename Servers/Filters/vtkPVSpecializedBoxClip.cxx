/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSpecializedBoxClip.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVSpecializedBoxClip.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVSpecializedBoxClip, "1.1");
vtkStandardNewMacro(vtkPVSpecializedBoxClip);

//----------------------------------------------------------------------------
vtkPVSpecializedBoxClip::vtkPVSpecializedBoxClip(vtkImplicitFunction *vtkNotUsed(cf))
{
  // setting NumberOfOutputPorts to 1 because ParaView does not allow you to
  // generate the clipped output
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkPVSpecializedBoxClip::~vtkPVSpecializedBoxClip()
{
}

//----------------------------------------------------------------------------
void vtkPVSpecializedBoxClip::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
