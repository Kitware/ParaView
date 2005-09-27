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

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkLight.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkPVCameraManipulator, "1.11");
vtkStandardNewMacro(vtkPVCameraManipulator);

vtkCxxSetObjectMacro(vtkPVCameraManipulator,Application,vtkPVApplication);

//-------------------------------------------------------------------------
vtkPVCameraManipulator::vtkPVCameraManipulator()
{
  this->Button = 1;
  this->Shift = 0;
  this->Control = 0;

  this->Center[0] = this->Center[1] = this->Center[2] = 0.0;
  this->DisplayCenter[0] = this->DisplayCenter[1] = 0.0;
  this->Application = 0;

  this->ManipulatorName = 0;
}

//-------------------------------------------------------------------------
vtkPVCameraManipulator::~vtkPVCameraManipulator()
{
  this->SetApplication(0);
  this->SetManipulatorName(0);
}

void vtkPVCameraManipulator::StartInteraction()
{
  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOn();
}

void vtkPVCameraManipulator::EndInteraction()
{
  this->GetApplication()->GetMainWindow()->InteractiveRenderEnabledOff();
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnButtonDown(int, int, vtkRenderer*,
                                          vtkRenderWindowInteractor*)
{
}


//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnButtonUp(int, int, vtkRenderer*,
                                        vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::OnMouseMove(int, int, vtkRenderer*,
                                         vtkRenderWindowInteractor*)
{
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::ComputeDisplayCenter(vtkRenderer *ren)
{
  double *pt;

  // save the center of rotation in screen coordinates
  ren->SetWorldPoint(this->Center[0],
                     this->Center[1],
                     this->Center[2], 1.0);
  ren->WorldToDisplay();
  pt = ren->GetDisplayPoint();
  this->DisplayCenter[0] = pt[0];
  this->DisplayCenter[1] = pt[1];
}

//-------------------------------------------------------------------------
void vtkPVCameraManipulator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ManipulatorName: " 
     << (this->ManipulatorName?this->ManipulatorName:"none") << endl;
  os << indent << "Button: " << this->Button << endl;
  os << indent << "Shift: " << this->Shift << endl;
  os << indent << "Control: " << this->Control << endl;
  
  os << indent << "Center: " << this->Center[0] << ", " 
     << this->Center[1] << ", " << this->Center[2] << endl;
  os << indent << "Application: " << this->GetApplication() << endl;
}






