/*=========================================================================

  Program:   ParaView
  Module:    vtkPVKitwareContourFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVKitwareContourFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVKitwareContourFilter, "1.2");
vtkStandardNewMacro(vtkPVKitwareContourFilter);

vtkPVKitwareContourFilter::vtkPVKitwareContourFilter()
{
}

vtkPVKitwareContourFilter::~vtkPVKitwareContourFilter()
{
}
void vtkPVKitwareContourFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;
}

