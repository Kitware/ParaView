// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRStylusStyleProxy.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRStylusStyleProxy);

// -----------------------------------------------------------------------------
// Constructor method
vtkSMVRStylusStyleProxy::vtkSMVRStylusStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Rotate world");
  this->AddButtonRole("Translate world");
  this->EnableTranslate = false;
  this->EnableRotate = false;
}

// ----------------------------------------------------------------------------
// Destructor method

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRStylusStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableTranslate: " << this->EnableTranslate << endl;
  os << indent << "EnableRotate: " << this->EnableRotate << endl;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkSMVRStylusStyleProxy::HandleButton(const vtkVREvent& event)
{
  this->PositionRecorded = false;

  std::string role = this->GetButtonRole(event.name);
  if (role == "Translate world")
  {
    this->EnableTranslate = event.data.button.state;
  }
  else if (role == "Rotate world")
  {
    this->EnableRotate = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkSMVRStylusStyleProxy::HandleTracker(const vtkVREvent& event)
{
  if (this->GetTrackerRole(event.name) == "Tracker")
  {
    if (!this->PositionRecorded)
    {
      this->LastRecordedPosition[0] = event.data.tracker.matrix[3];
      this->LastRecordedPosition[1] = event.data.tracker.matrix[7];
      this->LastRecordedPosition[2] = event.data.tracker.matrix[11];
      this->PositionRecorded = true;
      return;
    }

    double diffPosition[3];

    diffPosition[0] = event.data.tracker.matrix[3] - this->LastRecordedPosition[0];
    diffPosition[1] = event.data.tracker.matrix[7] - this->LastRecordedPosition[1];
    diffPosition[2] = event.data.tracker.matrix[11] - this->LastRecordedPosition[2];

    this->LastRecordedPosition[0] = event.data.tracker.matrix[3];
    this->LastRecordedPosition[1] = event.data.tracker.matrix[7];
    this->LastRecordedPosition[2] = event.data.tracker.matrix[11];

    vtkSMRenderViewProxy* rvProxy = vtkSMRenderViewProxy::SafeDownCast(this->ControlledProxy);

    if (rvProxy)
    {
      vtkCamera* camera = rvProxy->GetActiveCamera();

      if (this->EnableTranslate)
      {
        vtkNew<vtkMatrix4x4> modelMatrix;
        vtkNew<vtkMatrix4x4> modelViewMatrix;
        vtkNew<vtkMatrix4x4> invViewMatrix;

        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Get(*modelMatrix->Element, 16);

        modelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());

        // compute inverse view matrix
        vtkMatrix4x4::Invert(modelViewMatrix, invViewMatrix);
        vtkMatrix4x4::Multiply4x4(modelMatrix, invViewMatrix, invViewMatrix);

        double dist = camera->GetDistance();

        modelMatrix->DeepCopy(modelViewMatrix);

        modelMatrix->SetElement(0, 3, modelMatrix->GetElement(0, 3) + diffPosition[0] * dist);
        modelMatrix->SetElement(1, 3, modelMatrix->GetElement(1, 3) + diffPosition[1] * dist);
        modelMatrix->SetElement(2, 3, modelMatrix->GetElement(2, 3) + diffPosition[2] * dist);

        vtkMatrix4x4::Multiply4x4(invViewMatrix, modelMatrix, modelMatrix);

        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Set(*modelMatrix->Element, 16);

        this->ControlledProxy->UpdateVTKObjects();
      }

      if (this->EnableRotate)
      {
        vtkNew<vtkMatrix4x4> modelMatrix;
        vtkNew<vtkMatrix4x4> modelViewMatrix;
        vtkNew<vtkMatrix4x4> invViewMatrix;

        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Get(*modelMatrix->Element, 16);

        modelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());

        // compute inverse view matrix and inverse model/view matrix
        vtkMatrix4x4::Invert(modelViewMatrix, invViewMatrix);
        vtkMatrix4x4::Multiply4x4(modelMatrix, invViewMatrix, invViewMatrix);

        double center[4];
        vtkSMPropertyHelper(this->ControlledProxy, "CenterOfRotation").Get(center, 3);
        center[3] = 1.0;
        modelViewMatrix->MultiplyPoint(center, center);

        // compute the transformation to apply
        vtkNew<vtkMatrix4x4> transfoMatrix;
        transfoMatrix->SetElement(0, 3, -center[0]);
        transfoMatrix->SetElement(1, 3, -center[1]);
        transfoMatrix->SetElement(2, 3, -center[2]);

        vtkNew<vtkTransform> transfo;
        transfo->RotateX(-diffPosition[1] * 300.0);
        transfo->RotateY(diffPosition[0] * 300.0);

        vtkMatrix4x4::Multiply4x4(transfo->GetMatrix(), transfoMatrix, transfoMatrix);

        transfoMatrix->SetElement(0, 3, transfoMatrix->GetElement(0, 3) + center[0]);
        transfoMatrix->SetElement(1, 3, transfoMatrix->GetElement(1, 3) + center[1]);
        transfoMatrix->SetElement(2, 3, transfoMatrix->GetElement(2, 3) + center[2]);

        // apply the transformation
        vtkMatrix4x4::Multiply4x4(transfoMatrix, modelViewMatrix, modelMatrix);
        vtkMatrix4x4::Multiply4x4(invViewMatrix, modelMatrix, modelMatrix);

        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Set(*modelMatrix->Element, 16);
      }
    }
    else
    {
      vtkWarningMacro(
        "This interactor style only works with render view proxy, ignoring interaction...");
    }
  }
}
