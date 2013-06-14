/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCameraAnimationCue.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVCameraAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPVCameraCueManipulator.h"

vtkStandardNewMacro(vtkPVCameraAnimationCue);
vtkCxxSetObjectMacro(vtkPVCameraAnimationCue, View, vtkPVRenderView);
//----------------------------------------------------------------------------
vtkPVCameraAnimationCue::vtkPVCameraAnimationCue()
{
  this->View = 0;
  vtkPVCameraCueManipulator * manip = vtkPVCameraCueManipulator::New();
  this->SetManipulator(manip);
  manip->Delete();
}

//----------------------------------------------------------------------------
vtkPVCameraAnimationCue::~vtkPVCameraAnimationCue()
{
  this->SetView(NULL);
}

//----------------------------------------------------------------------------
vtkCamera* vtkPVCameraAnimationCue::GetCamera()
{
  return this->View? this->View->GetActiveCamera() : NULL;
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::SetMode(int mode)
{
  vtkPVCameraCueManipulator::SafeDownCast(this->Manipulator)->SetMode(mode);
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::EndUpdateAnimationValues()
{
  if (this->View)
    {
    this->View->ResetCameraClippingRange();
    }
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::SetDataSourceProxy(vtkSMProxy *dataSourceProxy)
{
  vtkPVCameraCueManipulator::SafeDownCast(this->Manipulator)->
    SetDataSourceProxy(dataSourceProxy);
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
