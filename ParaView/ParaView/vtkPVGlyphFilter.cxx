/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVGlyphFilter.cxx
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
#include "vtkPVGlyphFilter.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVGlyphFilter, "1.1");
vtkStandardNewMacro(vtkPVGlyphFilter);

vtkPVGlyphFilter::vtkPVGlyphFilter()
{
}

vtkPVGlyphFilter::~vtkPVGlyphFilter()
{
}

void vtkPVGlyphFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;

  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;

  os << indent << "InputNormalsSelection: " 
     << (this->InputNormalsSelection ? this->InputNormalsSelection : "(none)")
     << endl;
}
