/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageContinuousDilate3D.cxx
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
#include "vtkPVImageContinuousDilate3D.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVImageContinuousDilate3D, "1.1");
vtkStandardNewMacro(vtkPVImageContinuousDilate3D);

//----------------------------------------------------------------------------
vtkPVImageContinuousDilate3D::vtkPVImageContinuousDilate3D()
{
}


//----------------------------------------------------------------------------
vtkPVImageContinuousDilate3D::~vtkPVImageContinuousDilate3D()
{
}


//----------------------------------------------------------------------------
void vtkPVImageContinuousDilate3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;
}

