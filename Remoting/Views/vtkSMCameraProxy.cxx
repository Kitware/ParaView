// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMCameraProxy.h"

#include "vtkCamera.h"
#include "vtkObjectFactory.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"

vtkStandardNewMacro(vtkSMCameraProxy);
//-----------------------------------------------------------------------------
vtkSMCameraProxy::vtkSMCameraProxy() = default;

//-----------------------------------------------------------------------------
vtkSMCameraProxy::~vtkSMCameraProxy() = default;

//-----------------------------------------------------------------------------
void vtkSMCameraProxy::UpdatePropertyInformation()
{
  if (this->InUpdateVTKObjects)
  {
    return;
  }

  vtkCamera* camera = vtkCamera::SafeDownCast(this->GetClientSideObject());
  if (!camera)
  {
    this->Superclass::UpdatePropertyInformation();
    return;
  }

  vtkSMDoubleVectorProperty* dvp;
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraPositionInfo"));
  dvp->SetElements(camera->GetPosition());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraFocalPointInfo"));
  dvp->SetElements(camera->GetFocalPoint());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraViewUpInfo"));
  dvp->SetElements(camera->GetViewUp());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraParallelScaleInfo"));
  dvp->SetElement(0, camera->GetParallelScale());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraViewAngleInfo"));
  dvp->SetElement(0, camera->GetViewAngle());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraFocalDiskInfo"));
  dvp->SetElement(0, camera->GetFocalDisk());

  dvp = vtkSMDoubleVectorProperty::SafeDownCast(this->GetProperty("CameraFocalDistanceInfo"));
  dvp->SetElement(0, camera->GetFocalDistance());
}

//-----------------------------------------------------------------------------
void vtkSMCameraProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
