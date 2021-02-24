/*=========================================================================

   Program: ParaView
   Module:  vtkVRControlSliceOrientationStyle.cxx

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
#include "vtkVRControlSliceOrientationStyle.h"

#include "vtkMath.h"
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
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqView.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRControlSliceOrientationStyle);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRControlSliceOrientationStyle::vtkVRControlSliceOrientationStyle()
{
  this->Enabled = false;
  this->InitialOrientationRecorded = false;
  this->AddButtonRole("Grab slice");
  this->AddTrackerRole("Slice orientation");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRControlSliceOrientationStyle::~vtkVRControlSliceOrientationStyle()
{
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRControlSliceOrientationStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  // this->Superclass::PrintSelf(os, indent);		// WRS-TODO: we never seem to return from
  // this

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialOrientationRecorded: " << this->InitialOrientationRecorded << endl;
  os << indent << "InitialQuat: " << this->InitialQuat[0] << " " << this->InitialQuat[1] << " "
     << this->InitialQuat[2] << " " << this->InitialQuat[3] << endl;
  os << indent << "InitialTrackerQuat: " << this->InitialTrackerQuat[0] << " "
     << this->InitialTrackerQuat[1] << " " << this->InitialTrackerQuat[2] << " "
     << this->InitialTrackerQuat[3] << endl;
  os << indent << "UpdatedQuat: " << this->UpdatedQuat[0] << " " << this->UpdatedQuat[1] << " "
     << this->UpdatedQuat[2] << " " << this->UpdatedQuat[3] << endl;
  os << indent << "Normal: " << this->Normal[0] << " " << this->Normal[1] << " " << this->Normal[2]
     << " " << this->Normal[3] << endl;

  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
// Update() method
bool vtkVRControlSliceOrientationStyle::Update()
{
  if (!this->ControlledProxy)
  {
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkVRControlSliceOrientationStyle::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == "Grab slice")
  {
    if (this->Enabled && event.data.button.state == 0)
    {
      this->ControlledProxy->UpdateVTKObjects();
      this->InitialOrientationRecorded = false;
    }

    this->Enabled = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkVRControlSliceOrientationStyle::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);
  if (role != "Slice orientation")
  {
    return;
  }

  if (!this->Enabled)
  {
    this->InitialOrientationRecorded = false;
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
        if (!this->InitialOrientationRecorded)
        {
          // Copy the data into matrix
          this->InitialInvertedPose->DeepCopy(event.data.tracker.matrix);
          this->InitialInvertedPose->SetElement(0, 3, 0.0);
          this->InitialInvertedPose->SetElement(1, 3, 0.0);
          this->InitialInvertedPose->SetElement(2, 3, 0.0);

          // Invert the matrix
          this->InitialInvertedPose->Invert();

          vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
            .Get(this->Normal, 3);
          this->Normal[3] = 1;
          this->InitialOrientationRecorded = true;
        }
        else
        {
          vtkNew<vtkMatrix4x4> transformMatrix;
          transformMatrix->DeepCopy(event.data.tracker.matrix);
          transformMatrix->SetElement(0, 3, 0.0);
          transformMatrix->SetElement(1, 3, 0.0);
          transformMatrix->SetElement(2, 3, 0.0);

          double normal[4];
          vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
            this->InitialInvertedPose.GetPointer(), transformMatrix.GetPointer());
          double* transformedPoint = transformMatrix->MultiplyDoublePoint(this->Normal);
          normal[0] = transformedPoint[0] / transformedPoint[3];
          normal[1] = transformedPoint[1] / transformedPoint[3];
          normal[2] = transformedPoint[2] / transformedPoint[3];
          normal[3] = 1.0;

          vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(normal, 3);
        }
      }
    }
  }
}
