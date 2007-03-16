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
vtkCxxRevisionMacro(vtkPVEnvironmentInformationHelper, "1.1");

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::vtkPVEnvironmentInformationHelper()
{
  this->Variable = NULL;
}

//-----------------------------------------------------------------------------
vtkPVEnvironmentInformationHelper::~vtkPVEnvironmentInformationHelper()
{
}

//-----------------------------------------------------------------------------
void vtkPVEnvironmentInformationHelper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Variable: " << (this->Variable? this->Variable : "(null)") << endl;
}

