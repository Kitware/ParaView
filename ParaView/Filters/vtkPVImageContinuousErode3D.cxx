/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImageContinuousErode3D.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVImageContinuousErode3D.h"

#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkPVImageContinuousErode3D, "1.2");
vtkStandardNewMacro(vtkPVImageContinuousErode3D);

//----------------------------------------------------------------------------
vtkPVImageContinuousErode3D::vtkPVImageContinuousErode3D()
{
}


//----------------------------------------------------------------------------
vtkPVImageContinuousErode3D::~vtkPVImageContinuousErode3D()
{
}


//----------------------------------------------------------------------------
void vtkPVImageContinuousErode3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputScalarsSelection: " 
     << (this->InputScalarsSelection ? this->InputScalarsSelection : "(none)")
     << endl;
}
