/*=========================================================================

  Program:   ParaView
  Module:    vtkSMDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMDomain.h"

vtkCxxRevisionMacro(vtkSMDomain, "1.2");

//---------------------------------------------------------------------------
vtkSMDomain::vtkSMDomain()
{
  this->XMLName = 0;
}

//---------------------------------------------------------------------------
vtkSMDomain::~vtkSMDomain()
{
  this->SetXMLName(0);
}

//---------------------------------------------------------------------------
void vtkSMDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
