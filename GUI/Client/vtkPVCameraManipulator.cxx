/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraManipulator.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"

vtkCxxRevisionMacro(vtkPVCameraManipulator, "1.12");
vtkStandardNewMacro(vtkPVCameraManipulator);

vtkCxxSetObjectMacro(vtkPVCameraManipulator,Application,vtkPVApplication);

//-------------------------------------------------------------------------
vtkPVCameraManipulator::vtkPVCameraManipulator()
{
  this->Application = 0;
}

//-------------------------------------------------------------------------
vtkPVCameraManipulator::~vtkPVCameraManipulator()
{
  this->SetApplication(0);
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Application: " << this->GetApplication() << endl;
}


