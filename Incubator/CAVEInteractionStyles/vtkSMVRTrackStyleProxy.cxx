// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRTrackStyleProxy.h"

#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRTrackStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRTrackStyleProxy::vtkSMVRTrackStyleProxy()
  : Superclass()
{
  this->AddTrackerRole("Tracker");
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
bool vtkSMVRTrackStyleProxy::Update()
{
  if (this->ControlledProxy != nullptr && this->ControlledPropertyName != nullptr &&
    this->ControlledPropertyName[0] != '\0')
  {
    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(*this->TrackerMatrix->Element, 16);
  }

  this->ControlledProxy->UpdateVTKObjects();

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRTrackStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role == "Tracker")
  {
    this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
  }
}
