// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSMVRVirtualHandStyleProxy.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqView.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRVirtualHandStyleProxy);

// -----------------------------------------------------------------------------
// Constructor method
vtkSMVRVirtualHandStyleProxy::vtkSMVRVirtualHandStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Grab world");

  this->CurrentButton = false;
  this->PrevButton = false;

  this->EventPress = false;
  this->EventRelease = false;

  this->InverseTrackerMatrix->Identity();
  this->CachedModelMatrix->Identity();
  this->CurrentTrackerMatrix->Identity();
}

// -----------------------------------------------------------------------------
// Destructor method
vtkSMVRVirtualHandStyleProxy::~vtkSMVRVirtualHandStyleProxy() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRVirtualHandStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "CurrentButton: " << this->CurrentButton << endl;
  os << indent << "PrevButton: " << this->PrevButton << endl;
  os << indent << "EventPress: " << this->EventPress << endl;
  os << indent << "EventRelease: " << this->EventPress << endl;

  os << indent << "CurrentTrackerMatrix:" << endl;
  this->CurrentTrackerMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "InverseTrackerMatrix:" << endl;
  this->InverseTrackerMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "CachedModelMatrix:" << endl;
  this->CachedModelMatrix->PrintSelf(os, indent.GetNextIndent());

  os << indent << "NewModelMatrix:" << endl;
  this->NewModelMatrix->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkSMVRVirtualHandStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == "Grab world")
  {

    this->CurrentButton = event.data.button.state;

    if (this->CurrentButton == true && this->PrevButton == false)
    {
      this->EventPress = true;
    }
    if (this->CurrentButton == false && this->PrevButton == true)
    {
      this->EventRelease = true;
    }

    this->PrevButton = this->CurrentButton;
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkSMVRVirtualHandStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);
  if (role == "Tracker")
  {
    this->CurrentTrackerMatrix->DeepCopy(event.data.tracker.matrix);

    if (this->EventPress)
    {
      double matrix_vals[16];
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(matrix_vals, 16);
      this->CachedModelMatrix->DeepCopy(matrix_vals);

      this->InverseTrackerMatrix->DeepCopy(this->CurrentTrackerMatrix.GetPointer());
      this->InverseTrackerMatrix->Invert();

      this->EventPress = false;
    }

    if (CurrentButton || EventRelease) // if grabbing or if we just let go
    {
      // Goal is:
      // result = tracker * tracker_inverse_at_start * original_model

      vtkNew<vtkMatrix4x4> tempMatrix;

      // TODO: could reduce to 1 matrix multiplication (pre-calc inv*model)
      vtkMatrix4x4::Multiply4x4(this->CurrentTrackerMatrix.GetPointer(),
        this->InverseTrackerMatrix.GetPointer(), tempMatrix.GetPointer());
      vtkMatrix4x4::Multiply4x4(tempMatrix.GetPointer(), this->CachedModelMatrix.GetPointer(),
        this->NewModelMatrix.GetPointer());

      // Set the new matrix for the proxy.
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Set(&this->NewModelMatrix->Element[0][0], 16);
      this->ControlledProxy->UpdateVTKObjects();

      this->EventRelease = false;
    }
  }
}
