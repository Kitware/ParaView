/*=========================================================================

   Program: ParaView
   Module:    vtkVRMovePointStyle.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "vtkVRMovePointStyle.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRMovePointStyle);

// -----------------------------------------------------------------------------
vtkVRMovePointStyle::vtkVRMovePointStyle()
  : Superclass()
{
  this->AddButtonRole("Move");
  this->EnableMovePoint = false;
}

// ----------------------------------------------------------------------------
void vtkVRMovePointStyle::HandleButton(const vtkVREventData& data)
{
  this->PositionRecorded = false;

  std::string role = this->GetButtonRole(data.name);
  if (role == "Move")
  {
    this->EnableMovePoint = data.data.button.state;
  }
}

// ----------------------------------------------------------------------------
void vtkVRMovePointStyle::HandleTracker(const vtkVREventData& data)
{
  if (this->GetTrackerRole(data.name) != "Tracker")
  {
    return;
  }

  if (!this->PositionRecorded)
  {
    this->LastRecordedPosition[0] = data.data.tracker.matrix[3];
    this->LastRecordedPosition[1] = data.data.tracker.matrix[7];
    this->LastRecordedPosition[2] = data.data.tracker.matrix[11];
    this->PositionRecorded = true;
    return;
  }

  double diffPosition[3];

  diffPosition[0] = data.data.tracker.matrix[3] - this->LastRecordedPosition[0];
  diffPosition[1] = data.data.tracker.matrix[7] - this->LastRecordedPosition[1];
  diffPosition[2] = data.data.tracker.matrix[11] - this->LastRecordedPosition[2];

  this->LastRecordedPosition[0] = data.data.tracker.matrix[3];
  this->LastRecordedPosition[1] = data.data.tracker.matrix[7];
  this->LastRecordedPosition[2] = data.data.tracker.matrix[11];

  if (this->EnableMovePoint)
  {
    vtkCamera* camera = nullptr;
    pqActiveObjects& activeObjs = pqActiveObjects::instance();
    pqView* view = activeObjs.activeView();
    if (view)
    {
      pqRenderView* rview = qobject_cast<pqRenderView*>(view);
      if (rview)
      {
        vtkSMRenderViewProxy* rProxy = vtkSMRenderViewProxy::SafeDownCast(rview->getProxy());
        if (rProxy)
        {
          camera = rProxy->GetActiveCamera();
        }
      }
    }
    if (!camera)
    {
      vtkWarningMacro("Cannot get active camera.");
      return;
    }

    double point[4];
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(point, 3);
    point[3] = 1.0;

    vtkNew<vtkMatrix4x4> modelViewMatrix;
    vtkNew<vtkMatrix4x4> invModelViewMatrix;
    modelViewMatrix->DeepCopy(camera->GetModelViewTransformMatrix());
    vtkMatrix4x4::Invert(modelViewMatrix, invModelViewMatrix);

    modelViewMatrix->MultiplyPoint(point, point);

    // the translation is scaled by the depth distance
    point[0] -= diffPosition[0] * point[2];
    point[1] -= diffPosition[1] * point[2];
    point[2] -= diffPosition[2] * point[2];

    invModelViewMatrix->MultiplyPoint(point, point);

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(point, 3);

    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkVRMovePointStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableMovePoint: " << this->EnableMovePoint << endl;
}
