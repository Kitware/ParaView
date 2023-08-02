// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRControlSlicePositionStyleProxy.h"

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqView.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRControlSlicePositionStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRControlSlicePositionStyleProxy::vtkSMVRControlSlicePositionStyleProxy()
{
  this->Enabled = false;
  this->InitialPositionRecorded = false;
  this->AddButtonRole("Grab slice");
  this->AddTrackerRole("Slice position");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkSMVRControlSlicePositionStyleProxy::~vtkSMVRControlSlicePositionStyleProxy() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRControlSlicePositionStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialPositionRecorded: " << this->InitialPositionRecorded << endl;
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << " " << this->Origin[3] << endl;
  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
// Update() method
// WRS-TODO: Explain what this->ControlledProxy indicates and why it's important for Update()
bool vtkSMVRControlSlicePositionStyleProxy::Update()
{
  if (!this->ControlledProxy)
  {
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkSMVRControlSlicePositionStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == "Grab slice")
  {
    if (this->Enabled && event.data.button.state == 0)
    {
      this->ControlledProxy->UpdateVTKObjects();
      this->InitialPositionRecorded = false;
    }

    this->Enabled = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkSMVRControlSlicePositionStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);
  if (role != "Slice position")
  {
    return;
  }

  if (!this->Enabled)
  {
    this->InitialPositionRecorded = false;
    return;
  }

  vtkSMRenderViewProxy* proxy = 0;
  vtkSMDoubleVectorProperty* prop = 0;
  pqView* view = pqActiveObjects::instance().activeView();
  if (view)
  {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("ModelTransformMatrix"));
      if (prop)
      {
        if (!this->InitialPositionRecorded)
        {
          // Copy the data into matrix
          this->InitialInvertedPose->Identity();
          this->InitialInvertedPose->SetElement(0, 3, event.data.tracker.matrix[3]);
          this->InitialInvertedPose->SetElement(1, 3, event.data.tracker.matrix[7]);
          this->InitialInvertedPose->SetElement(2, 3, event.data.tracker.matrix[11]);

          // Invert the matrix
          this->InitialInvertedPose->Invert();

          vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
            .Get(this->Origin, 3);

          this->Origin[3] = 1;
          this->InitialPositionRecorded = true;
        }
        else
        {
          double origin[4];
          vtkNew<vtkMatrix4x4> transformMatrix;
          transformMatrix->Identity();
          transformMatrix->SetElement(0, 3, event.data.tracker.matrix[3]);
          transformMatrix->SetElement(1, 3, event.data.tracker.matrix[7]);
          transformMatrix->SetElement(2, 3, event.data.tracker.matrix[11]);

          vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
            this->InitialInvertedPose.GetPointer(), transformMatrix.GetPointer());

          // Get the current model transform matrix to get the orientation
          double matrix[16];
          vtkNew<vtkMatrix4x4> modelTransformMatrix;
          vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(matrix, 16);
          modelTransformMatrix->DeepCopy(matrix);

          // We need only the rotation component
          modelTransformMatrix->SetElement(0, 3, 0.0);
          modelTransformMatrix->SetElement(1, 3, 0.0);
          modelTransformMatrix->SetElement(2, 3, 0.0);

          // Now put the transform in new coordinate frame
          modelTransformMatrix->Invert();
          vtkMatrix4x4::Multiply4x4(modelTransformMatrix.GetPointer(), transformMatrix.GetPointer(),
            transformMatrix.GetPointer());

          double* transformedPoints = transformMatrix->MultiplyDoublePoint(this->Origin);
          origin[0] = transformedPoints[0] / transformedPoints[3];
          origin[1] = transformedPoints[1] / transformedPoints[3];
          origin[2] = transformedPoints[2] / transformedPoints[3];
          origin[3] = 1;

          vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(origin, 3);
          // this->ControlledProxy->UpdateVTKObjects();
        }
      }
    }
  }
  else
  {
    // If the button is released then
    this->InitialPositionRecorded = false;
  }
}
