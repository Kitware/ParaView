// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVCameraAnimationCue.h"

#include "vtkObjectFactory.h"
#include "vtkPVCameraCueManipulator.h"
#include "vtkPVRenderView.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkPVCameraAnimationCue);
vtkCxxSetObjectMacro(vtkPVCameraAnimationCue, View, vtkPVRenderView);
vtkCxxSetObjectMacro(vtkPVCameraAnimationCue, TimeKeeper, vtkSMProxy);
//----------------------------------------------------------------------------
vtkPVCameraAnimationCue::vtkPVCameraAnimationCue()
{
  this->View = nullptr;
  this->TimeKeeper = nullptr;
  vtkPVCameraCueManipulator* manip = vtkPVCameraCueManipulator::New();
  this->SetManipulator(manip);
  manip->Delete();
}

//----------------------------------------------------------------------------
vtkPVCameraAnimationCue::~vtkPVCameraAnimationCue()
{
  this->SetView(nullptr);
  this->SetTimeKeeper(nullptr);
}

//----------------------------------------------------------------------------
vtkCamera* vtkPVCameraAnimationCue::GetCamera()
{
  return this->View ? this->View->GetActiveCamera() : nullptr;
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::SetMode(int mode)
{
  vtkPVCameraCueManipulator::SafeDownCast(this->Manipulator)->SetMode(mode);
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::SetInterpolationMode(int mode)
{
  vtkPVCameraCueManipulator::SafeDownCast(this->Manipulator)->SetInterpolationMode(mode);
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
void vtkPVCameraAnimationCue::SetDataSourceProxy(vtkSMProxy* dataSourceProxy)
{
  vtkPVCameraCueManipulator::SafeDownCast(this->Manipulator)->SetDataSourceProxy(dataSourceProxy);
}

//----------------------------------------------------------------------------
void vtkPVCameraAnimationCue::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
