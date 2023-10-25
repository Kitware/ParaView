// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRSkeletonStyleProxy.h"

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

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRSkeletonStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRSkeletonStyleProxy::vtkSMVRSkeletonStyleProxy()
  : Superclass()
{
  this->AddButtonRole("Rotate Tracker");
  this->AddButtonRole("Report Self");
  this->AddValuatorRole("X");
  this->AddTrackerRole("Tracker");
  this->EnableReport = false;

  // leftover stuff:
  this->IsInitialTransRecorded = false;
  this->IsInitialRotRecorded = false;
  this->CachedTransMatrix->Identity();
  this->CachedRotMatrix->Identity();
}

// ----------------------------------------------------------------------------
// Destructor method
vtkSMVRSkeletonStyleProxy::~vtkSMVRSkeletonStyleProxy() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRSkeletonStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableReport: " << this->EnableReport << endl;
  os << indent << "IsInitialTransRecorded: " << this->IsInitialTransRecorded << endl;
  os << indent << "IsInitialRotRecorded: " << this->IsInitialRotRecorded << endl;
  os << indent << "InverseInitialTransMatrix:" << endl;
  this->InverseInitialTransMatrix->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkSMVRSkeletonStyleProxy::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);

  if (event.data.button.state == 1)
  {
    cout << "Button " << event.data.button.button << "is pressed\n";
  }
  if (role == "Report Self")
  {
    if (event.data.button.state == 1)
    {
      cout << "Reporting on myself\n";
      this->PrintSelf(cout, vtkIndent(0));
    }
  }
  if (role == "Report Tracker")
  {
    this->EnableReport = event.data.button.state;
  }
}

// ----------------------------------------------------------------------------
// HandleValuator() method
void vtkSMVRSkeletonStyleProxy::HandleValuator(const vtkVREvent& event)
{
  std::string role = this->GetValuatorRole(event.name);

  if (role == "X")
  {
    cout << "Got a value for 'X' of " << event.data.valuator.num_channels << " : "
         << event.data.valuator.channel[0] << " " << event.data.valuator.channel[1] << "\n";
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkSMVRSkeletonStyleProxy::HandleTracker(const vtkVREvent& event)
{
  std::string role = this->GetTrackerRole(event.name);

  if (role == "Tracker")
  {
    if (this->EnableReport)
    {
      // do something interesting here
      cout << "Do a tracker report\n";
    }
  }
}
