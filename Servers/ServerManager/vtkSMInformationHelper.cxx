/*=========================================================================

  Program:   ParaView
  Module:    vtkSMInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMInformationHelper.h"

vtkCxxRevisionMacro(vtkSMInformationHelper, "1.1");

//---------------------------------------------------------------------------
vtkSMInformationHelper::vtkSMInformationHelper()
{
}

//---------------------------------------------------------------------------
vtkSMInformationHelper::~vtkSMInformationHelper()
{
}

//---------------------------------------------------------------------------
void vtkSMInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
