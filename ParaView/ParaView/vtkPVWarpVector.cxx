/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVWarpVector.cxx
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
#include "vtkPVWarpVector.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVWarpVector, "1.1");
vtkStandardNewMacro(vtkPVWarpVector);

vtkPVWarpVector::vtkPVWarpVector()
{
}

vtkPVWarpVector::~vtkPVWarpVector()
{
}

void vtkPVWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InputVectorsSelection: " 
     << (this->InputVectorsSelection ? this->InputVectorsSelection : "(none)")
     << endl;
}
