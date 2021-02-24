/*=========================================================================

  Program:   ParaView
  Module:    vtkCameraManipulator.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraManipulator.h"

#include "vtkCameraManipulatorGUIHelper.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

vtkStandardNewMacro(vtkCameraManipulator);
vtkCxxSetObjectMacro(vtkCameraManipulator, GUIHelper, vtkCameraManipulatorGUIHelper);

//-------------------------------------------------------------------------
vtkCameraManipulator::vtkCameraManipulator()
{
  this->Button = 1;
  this->Shift = 0;
  this->Control = 0;

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->RotationFactor = 1.0;
  this->DisplayCenter[0] = this->DisplayCenter[1] = 0.0;

  this->ManipulatorName = nullptr;
  this->GUIHelper = nullptr;
}

//-------------------------------------------------------------------------
vtkCameraManipulator::~vtkCameraManipulator()
{
  this->SetManipulatorName(nullptr);
  this->SetGUIHelper(nullptr);
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::StartInteraction()
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::EndInteraction()
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::OnButtonDown(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::OnButtonUp(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::OnMouseMove(int, int, vtkRenderer*, vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::OnKeyUp(vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::OnKeyDown(vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::ComputeDisplayCenter(vtkRenderer* ren)
{
  double* pt;

  // save the center of rotation in screen coordinates
  ren->SetWorldPoint(this->Center[0], this->Center[1], this->Center[2], 1.0);
  ren->WorldToDisplay();
  pt = ren->GetDisplayPoint();
  this->DisplayCenter[0] = pt[0];
  this->DisplayCenter[1] = pt[1];
}

//-------------------------------------------------------------------------
void vtkCameraManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ManipulatorName: " << (this->ManipulatorName ? this->ManipulatorName : "none")
     << endl;
  os << indent << "Button: " << this->Button << endl;
  os << indent << "Shift: " << this->Shift << endl;
  os << indent << "Control: " << this->Control << endl;
  os << indent << "Center: " << this->Center[0] << ", " << this->Center[1] << ", "
     << this->Center[2] << endl;
  os << indent << "RotationFactor: " << this->RotationFactor << endl;
  os << indent << "GUIHelper: " << this->GUIHelper << endl;
}
