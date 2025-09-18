// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRControlLocationStyleProxy.h"

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkStringScanner.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqView.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRControlLocationStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRControlLocationStyleProxy::vtkSMVRControlLocationStyleProxy()
{
  this->Enabled = false;
  this->FirstUpdate = true;
  this->OriginalOrigin[0] = 0;
  this->OriginalOrigin[1] = 0;
  this->OriginalOrigin[2] = 0;
  this->InitialPositionRecorded = false;
  this->DeferredUpdate = true;
  this->AddButtonRole("Reset");
  this->AddButtonRole("Grab");
  this->AddTrackerRole("Tracker");
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialPositionRecorded: " << this->InitialPositionRecorded << endl;
  os << indent << "Origin: " << this->Origin[0] << " " << this->Origin[1] << " " << this->Origin[2]
     << " " << this->Origin[3] << endl;
  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
  os << indent << "DeferredUpdate: " << this->DeferredUpdate << std::endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlLocationStyleProxy::Update()
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return true;
  }

  // Save the original property value to be used as the "Reset" value
  if (this->FirstUpdate)
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Get(this->OriginalOrigin, 3);
    this->FirstUpdate = false;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->Enabled && this->InitialPositionRecorded)
  {
    // Get the translation portion of the latest tracker matrix
    this->TransformMatrix->Identity();
    this->TransformMatrix->SetElement(0, 3, this->TrackerMatrix->GetElement(0, 3));
    this->TransformMatrix->SetElement(1, 3, this->TrackerMatrix->GetElement(1, 3));
    this->TransformMatrix->SetElement(2, 3, this->TrackerMatrix->GetElement(2, 3));

    // Multiply by the initial inverted pose to get just the delta since
    // the button was pushed
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->InitialInvertedPose.GetPointer(), this->TransformMatrix.GetPointer());

    // To move the property value: 1) convert into transformed model coordinates,
    // 2) perform the translation, 3) convert back into world coordinates.
    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->ModelTransformMatrix.GetPointer(), this->TransformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(this->InverseModelTransformMatrix.GetPointer(),
      this->TransformMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    double* transformedPoints = this->TransformMatrix->MultiplyDoublePoint(this->Origin);

    double origin[3] = { transformedPoints[0], transformedPoints[1], transformedPoints[2] };

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Set(origin, 3);

    if (!this->DeferredUpdate)
    {
      this->ControlledProxy->UpdateVTKObjects();
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationStyleProxy::HandleButton(const vtkVREvent& event)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return;
  }

  vtkSMRenderViewProxy* proxy = this->GetActiveViewProxy();
  if (proxy == nullptr)
  {
    vtkWarningMacro("This interactor style requires an active view proxy");
    return;
  }

  this->InitialPositionRecorded = false;

  std::string role = this->GetButtonRole(event.name);

  if (role == "Grab")
  {
    if (this->Enabled && event.data.button.state == 0 && this->DeferredUpdate)
    {
      // This interactor style was originally designed to control a slice
      // plane origin, and since the slice computation could be expensive,
      // the update of vtk objects was saved until the button was released,
      // rather than being done before every render, in the Update() method.
      this->ControlledProxy->UpdateVTKObjects();
    }

    this->Enabled = event.data.button.state;

    if (event.data.button.state == 1)
    {
      vtkSMIntVectorProperty* ivp;
      ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("DeferredUpdate"));
      int value = ivp->GetElement(0);
      this->DeferredUpdate = value == 0 ? false : true;
    }
  }
  else if (role == "Reset" && event.data.button.state == 1)
  {
    // Reset all working matrices to identity
    this->InitialInvertedPose->Identity();
    this->ModelTransformMatrix->Identity();
    this->InverseModelTransformMatrix->Identity();
    this->TrackerMatrix->Identity();
    this->TransformMatrix->Identity();

    // Reset stored origin ivar
    for (size_t i = 0; i < 3; ++i)
    {
      this->Origin[i] = this->OriginalOrigin[i];
    }

    // Reset controlled proxy/property and update vtk objects
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(this->OriginalOrigin, 3);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationStyleProxy::HandleTracker(const vtkVREvent& event)
{
  if (this->ControlledProxy == nullptr || this->ControlledPropertyName == nullptr)
  {
    vtkWarningMacro("Missing required proxy or property name");
    return;
  }

  std::string role = this->GetTrackerRole(event.name);

  if (role != "Tracker" || !this->Enabled)
  {
    return;
  }

  // Capture some state at the moment of button down
  if (!this->InitialPositionRecorded)
  {
    vtkSMRenderViewProxy* proxy = this->GetActiveViewProxy();
    if (proxy == nullptr)
    {
      return;
    }

    // Capture the inverted initial pose matrix (translation only)
    this->InitialInvertedPose->Identity();
    this->InitialInvertedPose->SetElement(0, 3, event.data.tracker.matrix[3]);
    this->InitialInvertedPose->SetElement(1, 3, event.data.tracker.matrix[7]);
    this->InitialInvertedPose->SetElement(2, 3, event.data.tracker.matrix[11]);
    this->InitialInvertedPose->Invert();

    // Capture the controlled proxy property
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(this->Origin, 3);
    this->Origin[3] = 1;

    // Save the current model transform matrix and its inverse
    vtkSMPropertyHelper(proxy, "ModelTransformMatrix")
      .Get(*this->ModelTransformMatrix->Element, 16);
    vtkMatrix4x4::Invert(this->ModelTransformMatrix, this->InverseModelTransformMatrix);

    this->InitialPositionRecorded = true;
  }

  this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}

// ----------------------------------------------------------------------------
void vtkSMVRControlLocationStyleProxy::SetDeferredUpdate(bool deferred)
{
  this->DeferredUpdate = deferred;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlLocationStyleProxy::GetDeferredUpdate()
{
  return this->DeferredUpdate;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMVRControlLocationStyleProxy::SaveConfiguration()
{
  vtkPVXMLElement* elt = Superclass::SaveConfiguration();

  vtkSMIntVectorProperty* ivp;

  // Save the DeferredUpdate flag
  ivp = vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("DeferredUpdate"));
  int value = ivp->GetElement(0);

  vtkPVXMLElement* deferredUpdateElt = vtkPVXMLElement::New();
  deferredUpdateElt->SetName("DeferredUpdate");
  deferredUpdateElt->AddAttribute("value", value);
  elt->AddNestedElement(deferredUpdateElt);
  deferredUpdateElt->FastDelete();

  return elt;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlLocationStyleProxy::Configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  bool result = true;

  for (unsigned int neCount = 0; neCount < child->GetNumberOfNestedElements(); neCount++)
  {
    vtkPVXMLElement* element = child->GetNestedElement(neCount);
    if (element && element->GetName())
    {
      if (strcmp(element->GetName(), "DeferredUpdate") == 0)
      {
        const char* value = element->GetAttributeOrDefault("value", nullptr);
        if (value && *value)
        {
          vtkSMIntVectorProperty* ivp =
            vtkSMIntVectorProperty::SafeDownCast(this->GetProperty("DeferredUpdate"));
          int deferred;
          VTK_FROM_CHARS_IF_ERROR_RETURN(value, deferred, 1);
          if (ivp->SetElement(0, deferred) == 0)
          {
            vtkWarningMacro(<< "Invalid DeferredUpdate property value: " << deferred);
            result = false;
          }
        }
      }
    }
  }

  if (result)
  {
    this->UpdateVTKObjects();
    result = Superclass::Configure(child, locator);
  }

  return result;
}
