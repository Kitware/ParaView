/*=========================================================================

  Program:   ParaView
  Module:    vtkPVRibbonFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVRibbonFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVRibbonFilter, "1.2");
vtkStandardNewMacro(vtkPVRibbonFilter);

vtkPVRibbonFilter::vtkPVRibbonFilter()
{
}

vtkPVRibbonFilter::~vtkPVRibbonFilter()
{
}


void vtkPVRibbonFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;
}

