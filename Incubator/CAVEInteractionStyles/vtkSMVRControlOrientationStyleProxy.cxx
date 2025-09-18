// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRControlOrientationStyleProxy.h"

#include "vtkMath.h"
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
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqView.h"

#include <algorithm>
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRControlOrientationStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRControlOrientationStyleProxy::vtkSMVRControlOrientationStyleProxy()
{
  this->Enabled = false;
  this->FirstUpdate = true;
  this->OriginalNormal[0] = 1;
  this->OriginalNormal[1] = 0;
  this->OriginalNormal[2] = 0;
  this->InitialOrientationRecorded = false;
  this->DeferredUpdate = true;
  this->AddButtonRole("Reset");
  this->AddButtonRole("Grab");
  this->AddTrackerRole("Tracker");
}

// ----------------------------------------------------------------------------
void vtkSMVRControlOrientationStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialOrientationRecorded: " << this->InitialOrientationRecorded << endl;
  os << indent << "Normal: " << this->Normal[0] << " " << this->Normal[1] << " " << this->Normal[2]
     << " " << this->Normal[3] << endl;

  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
  os << indent << "DeferredUpdate: " << this->DeferredUpdate << std::endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlOrientationStyleProxy::Update()
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
      .Get(this->OriginalNormal, 3);
    this->FirstUpdate = false;
  }

  // Between pressing the button and updating, we need at least one tracker event
  // to capture initial state

  if (this->Enabled && this->InitialOrientationRecorded)
  {
    this->TransformMatrix->DeepCopy(this->TrackerMatrix);
    this->TransformMatrix->SetElement(0, 3, 0.0);
    this->TransformMatrix->SetElement(1, 3, 0.0);
    this->TransformMatrix->SetElement(2, 3, 0.0);

    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->InitialInvertedPose.GetPointer(), this->TransformMatrix.GetPointer());

    vtkMatrix4x4::Multiply4x4(this->TransformMatrix.GetPointer(),
      this->ModelTransformMatrix.GetPointer(), this->TransformMatrix.GetPointer());
    vtkMatrix4x4::Multiply4x4(this->InverseModelTransformMatrix.GetPointer(),
      this->TransformMatrix.GetPointer(), this->TransformMatrix.GetPointer());

    double* transformedPoint = this->TransformMatrix->MultiplyDoublePoint(this->Normal);

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(transformedPoint, 3);

    if (!this->DeferredUpdate)
    {
      this->ControlledProxy->UpdateVTKObjects();
    }
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRControlOrientationStyleProxy::HandleButton(const vtkVREvent& event)
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

  this->InitialOrientationRecorded = false;

  std::string role = this->GetButtonRole(event.name);

  if (role == "Grab")
  {
    if (this->Enabled && event.data.button.state == 0 && this->DeferredUpdate)
    {
      // This interactor style was originally designed to control a slice
      // plane normal, and since the slice computation could be expensive,
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
    this->TrackerMatrix->Identity();
    this->TransformMatrix->Identity();
    this->ModelTransformMatrix->Identity();
    this->InverseModelTransformMatrix->Identity();

    // Reset stored normal ivar
    for (size_t i = 0; i < 3; ++i)
    {
      this->Normal[i] = this->OriginalNormal[i];
    }

    // Reset controlled proxy/property
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(this->OriginalNormal, 3);
    this->ControlledProxy->UpdateVTKObjects();
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRControlOrientationStyleProxy::HandleTracker(const vtkVREvent& event)
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

  if (!this->InitialOrientationRecorded)
  {
    vtkSMRenderViewProxy* proxy = this->GetActiveViewProxy();
    if (proxy == nullptr)
    {
      return;
    }

    // Copy the tracker matrix, zero the translational component, and invert
    this->InitialInvertedPose->DeepCopy(event.data.tracker.matrix);
    this->InitialInvertedPose->SetElement(0, 3, 0.0);
    this->InitialInvertedPose->SetElement(1, 3, 0.0);
    this->InitialInvertedPose->SetElement(2, 3, 0.0);
    this->InitialInvertedPose->Invert();

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName).Get(this->Normal, 3);
    this->Normal[3] = 1;

    // Save the current model transform matrix (rotation part only) and its inverse
    vtkSMPropertyHelper(proxy, "ModelTransformMatrix")
      .Get(*this->ModelTransformMatrix->Element, 16);
    this->ModelTransformMatrix->SetElement(0, 3, 0.0);
    this->ModelTransformMatrix->SetElement(1, 3, 0.0);
    this->ModelTransformMatrix->SetElement(2, 3, 0.0);
    vtkMatrix4x4::Invert(this->ModelTransformMatrix, this->InverseModelTransformMatrix);

    this->InitialOrientationRecorded = true;
  }

  this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
}

// ----------------------------------------------------------------------------
void vtkSMVRControlOrientationStyleProxy::SetDeferredUpdate(bool deferred)
{
  std::cout << "Setting DeferredUpdate to " << deferred << std::endl;
  this->DeferredUpdate = deferred;
}

// ----------------------------------------------------------------------------
bool vtkSMVRControlOrientationStyleProxy::GetDeferredUpdate()
{
  std::cout << "Getting DeferredUpdate (is is currently " << this->DeferredUpdate << ")"
            << std::endl;
  return this->DeferredUpdate;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkSMVRControlOrientationStyleProxy::SaveConfiguration()
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
bool vtkSMVRControlOrientationStyleProxy::Configure(
  vtkPVXMLElement* child, vtkSMProxyLocator* locator)
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
