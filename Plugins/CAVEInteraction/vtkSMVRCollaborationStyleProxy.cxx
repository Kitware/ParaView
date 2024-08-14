// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRCollaborationStyleProxy.h"

#include "vtkCommand.h"
#include "vtkEventData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <QString>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRCollaborationStyleProxy);

// ----------------------------------------------------------------------------
class vtkSMVRCollaborationStyleProxy::vtkInternals
{
public:
  QString* HeadEventName;
  QString* LeftHandEventName;
  QString* RightHandEventName;
};

// ----------------------------------------------------------------------------
vtkSMVRCollaborationStyleProxy::vtkSMVRCollaborationStyleProxy()
  : Superclass()
  , Internal(new vtkSMVRCollaborationStyleProxy::vtkInternals())
{
}

// ----------------------------------------------------------------------------
vtkSMVRCollaborationStyleProxy::~vtkSMVRCollaborationStyleProxy() = default;

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "HeadEventName: (" << this->Internal->HeadEventName << "\n";
  os << indent << "LeftHandEventName: (" << this->Internal->LeftHandEventName << "\n";
  os << indent << "RightHandEventName: (" << this->Internal->RightHandEventName << "\n";
  os << indent << ")\n";
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::SetHeadEventName(QString* eventName)
{
  this->Internal->HeadEventName = eventName;
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::SetLeftHandEventName(QString* eventName)
{
  this->Internal->LeftHandEventName = eventName;
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::SetRightHandEventName(QString* eventName)
{
  this->Internal->RightHandEventName = eventName;
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::HandleTracker(const vtkVREvent& event)
{
  vtkNew<vtkEventDataDevice3D> edd;

  // Set the device based on the event name, filter any events
  // the user didn't configure.
  if (this->Internal->HeadEventName->compare("") != 0 &&
    event.name.compare(this->Internal->HeadEventName->toUtf8()) == 0)
  {
    edd->SetDevice(vtkEventDataDevice::HeadMountedDisplay);
  }
  else if (this->Internal->LeftHandEventName->compare("") != 0 &&
    event.name.compare(this->Internal->LeftHandEventName->toUtf8()) == 0)
  {
    edd->SetDevice(vtkEventDataDevice::LeftController);
  }
  else if (this->Internal->RightHandEventName->compare("") != 0 &&
    event.name.compare(this->Internal->RightHandEventName->toUtf8()) == 0)
  {
    edd->SetDevice(vtkEventDataDevice::RightController);
  }
  else
  {
    return;
  }

  double wpos[3] = { event.data.tracker.matrix[3], event.data.tracker.matrix[7],
    event.data.tracker.matrix[11] };

  double ortho[3][3];

  ortho[0][0] = event.data.tracker.matrix[0];
  ortho[1][0] = event.data.tracker.matrix[4];
  ortho[2][0] = event.data.tracker.matrix[8];

  ortho[0][1] = event.data.tracker.matrix[1];
  ortho[1][1] = event.data.tracker.matrix[5];
  ortho[2][1] = event.data.tracker.matrix[9];

  ortho[0][2] = event.data.tracker.matrix[2];
  ortho[1][2] = event.data.tracker.matrix[6];
  ortho[2][2] = event.data.tracker.matrix[10];

  double wxyz[4] = { 0.0, 0.0, 0.0, 1.0 };

  vtkMath::Matrix3x3ToQuaternion(ortho, wxyz);

  // Compute the return value wxyz
  double mag = sqrt(wxyz[1] * wxyz[1] + wxyz[2] * wxyz[2] + wxyz[3] * wxyz[3]);

  if (mag != 0.0)
  {
    wxyz[0] = 2.0 * vtkMath::DegreesFromRadians(atan2(mag, wxyz[0]));
    wxyz[1] /= mag;
    wxyz[2] /= mag;
    wxyz[3] /= mag;
  }
  else
  {
    wxyz[0] = 0.0;
    wxyz[1] = 0.0;
    wxyz[2] = 0.0;
    wxyz[3] = 1.0;
  }

  edd->SetWorldPosition(wpos);
  edd->SetWorldOrientation(wxyz);
  edd->SetType(vtkCommand::Move3DEvent);

  this->InvokeEvent(vtkCommand::Move3DEvent, edd);
}
