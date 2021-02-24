/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnvironmentInformationHelper.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnvironmentInformationHelper.h"

#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPVEnvironmentInformationHelper);

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::vtkPVEnvironmentInformationHelper()
{
  this->Variable = nullptr;
}

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::~vtkPVEnvironmentInformationHelper()
{
  this->SetVariable(nullptr);
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Variable: " << (this->Variable ? this->Variable : "(null)") << endl;
}
