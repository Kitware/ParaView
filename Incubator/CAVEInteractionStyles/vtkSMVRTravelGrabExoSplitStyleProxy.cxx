// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRTravelGrabExoSplitStyleProxy.h"

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkStringFormatter.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <cmath>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTravelGrabExoSplitStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRTravelGrabExoSplitStyleProxy::vtkSMVRTravelGrabExoSplitStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Rotate");
  this->AddButtonRole("Translate");
  this->AddButtonRole("Reset");
  this->EnableTranslate = false;
  this->EnableRotate = false;
  this->IsInitialRecorded = false;
  this->CurrentTranslation[0] = 0.0;
  this->CurrentTranslation[1] = 0.0;
  this->CurrentTranslation[2] = 0.0;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabExoSplitStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableTranslate: " << this->EnableTranslate << endl;
  os << indent << "EnableRotate: " << this->EnableRotate << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
  os << indent << "CurrentTranslation: [" << this->CurrentTranslation[0] << ", "
     << this->CurrentTranslation[1] << ", " << this->CurrentTranslation[2] << "]" << endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRTravelGrabExoSplitStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Update(): Missing required proxy or property name!");
    return true;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->EnableTranslate && this->IsInitialRecorded)
  {
    // Get the tracker matrix and "subtract out" the initial wand orientation,
    // so the result is only the diff since the button was pressed.
    vtkMatrix4x4::Multiply4x4(this->TrackerMatrix.GetPointer(),
      this->InverseWandMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    // Create a matrix to represent only the translational portion of the
    // diff since the button was pressed.
    this->TranslateMatrix->Identity();
    this->TranslateMatrix->SetElement(0, 3, this->TransformMatrix->GetElement(0, 3));
    this->TranslateMatrix->SetElement(1, 3, this->TransformMatrix->GetElement(1, 3));
    this->TranslateMatrix->SetElement(2, 3, this->TransformMatrix->GetElement(2, 3));

    // Apply the translational portion of the diff to the target matrix
    // captured when the button was pressed.
    vtkMatrix4x4::Multiply4x4(this->TranslateMatrix.GetPointer(),
      this->SavedPropertyMatrix.GetPointer(), this->TranslateMatrix.GetPointer());

    // Store the target matrix property back on the proxy
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->TranslateMatrix);

    this->ControlledProxy->UpdateVTKObjects();
  }
  else if (this->EnableRotate && this->IsInitialRecorded)
  {
    // To achieve rotation about the world origin, first translate the current
    // property matrix to the origin, then perform the rotation, then translate
    // back

    // Set up the matrix to translate to the origin
    this->TranslateToOrigin->Identity();
    this->TranslateToOrigin->SetElement(0, 3, -this->CurrentTranslation[0]);
    this->TranslateToOrigin->SetElement(1, 3, -this->CurrentTranslation[1]);
    this->TranslateToOrigin->SetElement(2, 3, -this->CurrentTranslation[2]);

    // To compute the transform we want to perform, first remove the translational
    // component, just as when the button was pressed. Then multiply that against
    // the saved inverse, to get only the rotational component of the diff.
    this->TransformMatrix->DeepCopy(this->TrackerMatrix);
    this->TransformMatrix->SetElement(0, 3, 0);
    this->TransformMatrix->SetElement(1, 3, 0);
    this->TransformMatrix->SetElement(2, 3, 0);
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->InverseWandMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    // Set up the matrix to translate back
    this->TranslateBack->Identity();
    this->TranslateBack->SetElement(0, 3, this->CurrentTranslation[0]);
    this->TranslateBack->SetElement(1, 3, this->CurrentTranslation[1]);
    this->TranslateBack->SetElement(2, 3, this->CurrentTranslation[2]);

    // Build the stack of transformations
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->TranslateToOrigin.GetPointer(), this->TransformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(this->TranslateBack.GetPointer(), this->TransformMatrix.GetPointer(),
      this->TransformMatrix.GetPointer());

    // Finally, apply the stack of transformations to the saved property matrix
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->SavedPropertyMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    // Store the target matrix property back on the proxy
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->TransformMatrix);

    this->ControlledProxy->UpdateVTKObjects();
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabExoSplitStyleProxy::HandleButton(const vtkVREvent& event)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("HandleButton(): Missing required proxy or property name!");
    return;
  }

  std::string role = this->GetButtonRole(event.name);

  if (role == "Translate")
  {
    this->EnableTranslate = event.data.button.state;
    this->IsInitialRecorded = false;
  }
  else if (role == "Rotate")
  {
    this->EnableRotate = event.data.button.state;
    this->IsInitialRecorded = false;
  }
  else if (event.data.button.state && role == "Reset")
  {
    // Reset all working matrices to identity
    this->InverseWandMatrix->Identity();
    this->SavedPropertyMatrix->Identity();
    this->TrackerMatrix->Identity();
    this->TransformMatrix->Identity();
    this->TranslateMatrix->Identity();
    this->TranslateToOrigin->Identity();
    this->TranslateBack->Identity();

    // Reset the stored current translation
    for (size_t i = 0; i < 3; ++i)
    {
      this->CurrentTranslation[i] = 0.0;
    }

    // Reset controlled proxy/property
    this->UpdateMatrixProperty(
      this->ControlledProxy, this->ControlledPropertyName, this->TransformMatrix);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRTravelGrabExoSplitStyleProxy::HandleTracker(const vtkVREvent& event)
{

  std::string role = this->GetTrackerRole(event.name);

  if (role != "Tracker")
  {
    return;
  }

  if (this->EnableTranslate)
  {
    // If we just pressed the button, capture initial state
    if (!this->IsInitialRecorded)
    {
      // Capture the wand matrix and invert it. This is so we can
      this->InverseWandMatrix->DeepCopy(event.data.tracker.matrix);
      this->InverseWandMatrix->Invert();

      // Capture the target matrix property
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Get(*this->SavedPropertyMatrix->Element, 16);

      this->IsInitialRecorded = true;
    }

    // Store the latest tracker matrix, and save computation for Update()
    this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
  }
  else if (this->EnableRotate)
  {
    // If we just pressed the button, capture initial state
    if (!this->IsInitialRecorded)
    {
      // Capture the wand matrix, remove the translational component, and then
      // invert the result.
      this->InverseWandMatrix->DeepCopy(event.data.tracker.matrix);
      this->InverseWandMatrix->SetElement(0, 3, 0);
      this->InverseWandMatrix->SetElement(1, 3, 0);
      this->InverseWandMatrix->SetElement(2, 3, 0);
      this->InverseWandMatrix->Invert();

      // Capture the target matrix property
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Get(*this->SavedPropertyMatrix->Element, 16);

      // Store the translation component of the target matrix property
      this->CurrentTranslation[0] = this->SavedPropertyMatrix->GetElement(0, 3);
      this->CurrentTranslation[1] = this->SavedPropertyMatrix->GetElement(1, 3);
      this->CurrentTranslation[2] = this->SavedPropertyMatrix->GetElement(2, 3);

      this->IsInitialRecorded = true;
    }

    // Store the latest tracker matrix, and save computation for Update()
    this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
  }
}
