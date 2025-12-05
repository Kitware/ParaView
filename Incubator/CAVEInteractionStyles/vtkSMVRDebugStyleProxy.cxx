// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRDebugStyleProxy.h"

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
#include <iostream>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRDebugStyleProxy);

// ----------------------------------------------------------------------------
vtkSMVRDebugStyleProxy::vtkSMVRDebugStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Report Tracker");
  this->AddButtonRole("Report Self");
  this->AddValuatorRole("X");
  this->AddTrackerRole("Tracker");
  this->EnableReport = false;
}

// ----------------------------------------------------------------------------
void vtkSMVRDebugStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableReport: " << this->EnableReport << endl;
}

// ----------------------------------------------------------------------------
bool vtkSMVRDebugStyleProxy::Update()
{
  // Any computationally expensive operations (e.g. UpdateVTKObjects()) belong
  // here, in the Update() method.
  if (this->EnableReport)
  {
    std::cout << "Tracker matrix\n";
    this->TrackerMatrix->PrintSelf(std::cout, vtkIndent(0));
  }

  return true;
}

// ----------------------------------------------------------------------------
void vtkSMVRDebugStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);

  if (event.data.button.state == 1)
  {
    std::cout << "Button " << event.data.button.button << "is pressed\n";
  }
  if (role == "Report Self")
  {
    if (event.data.button.state == 1)
    {
      std::cout << "Reporting on myself\n";
      this->PrintSelf(std::cout, vtkIndent(0));
    }
  }
  if (role == "Report Tracker")
  {
    this->EnableReport = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
void vtkSMVRDebugStyleProxy::HandleValuator(const vtkVREvent& event)
{
  unsigned int xIdx = this->GetChannelIndexForValuatorRole("X");
  std::cout << "Got a value for 'X' of " << event.data.valuator.channel[xIdx] << std::endl;
}

// ----------------------------------------------------------------------------
void vtkSMVRDebugStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role == "Tracker" && this->EnableReport)
  {
    // Best practice is to do the least amount of work here, saving any work for Update()
    this->TrackerMatrix->DeepCopy(event.data.tracker.matrix);
  }
}
