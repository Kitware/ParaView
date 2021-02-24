/*=========================================================================

   Program: ParaView
   Module:    vtkVRControlSlicePositionStyle.cxx

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
#include "vtkVRControlSlicePositionStyle.h"

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
vtkStandardNewMacro(vtkVRControlSlicePositionStyle);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRControlSlicePositionStyle::vtkVRControlSlicePositionStyle()
{
  this->Enabled = false;
  this->InitialPositionRecorded = false;
  this->AddButtonRole("Grab slice");
  this->AddTrackerRole("Slice position");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRControlSlicePositionStyle::~vtkVRControlSlicePositionStyle()
{
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRControlSlicePositionStyle::PrintSelf(ostream& os, vtkIndent indent)
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
bool vtkVRControlSlicePositionStyle::Update()
{
  if (!this->ControlledProxy)
  {
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkVRControlSlicePositionStyle::HandleButton(const vtkVREvent& event)
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
void vtkVRControlSlicePositionStyle::HandleTracker(const vtkVREvent& event)
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
