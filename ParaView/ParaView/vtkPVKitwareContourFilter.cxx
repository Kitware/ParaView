/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVKitwareContourFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVKitwareContourFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVKitwareContourFilter, "1.1");
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

