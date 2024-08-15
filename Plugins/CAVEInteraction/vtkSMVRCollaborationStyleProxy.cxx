// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRCollaborationStyleProxy.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkEventData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkVRCollaborationClient.h"

#include <QString>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRCollaborationStyleProxy);

// ----------------------------------------------------------------------------
class vtkSMVRCollaborationStyleProxy::vtkInternals
{
public:
  /*
   * Event handler which responds to navigation events by sending updated
   * avatar torso up vector message to collaborators.
   */
  class vtkNavigationObserver : public vtkCommand
  {
  public:
    static vtkNavigationObserver* New() { return new vtkNavigationObserver; }

    void Execute(
      vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), void* calldata) override
    {
      if (this->Sharing && this->CollaborationClient)
      {
        // Send a separate message to communicate to collaborators about
        // our avatars new torso up vector. The torso up vector is derived
        // only from our current navigation (nothing to do with our current
        // head orientation, e.g.)
        vtkMatrix4x4* mat = static_cast<vtkMatrix4x4*>(calldata);
        if (mat)
        {
          // "mat" is given to us by the event invoker already inverted (i.e.
          // equivalent to "invNavMatrix" in HandleTracker() below), so that
          // we do not need to invert it here.
          double avatarUp[4];
          this->CollaborationClient->GetAvatarInitialUpVector(avatarUp);
          avatarUp[3] = 0;
          mat->MultiplyPoint(avatarUp, avatarUp);

          std::vector<vtkVRCollaborationClient::Argument> args;
          args.resize(1);
          args[0].SetDoubleVector(&avatarUp[0], 3);

          this->CollaborationClient->SendAMessage("AUV", args);
        }
      }
    }

    vtkVRCollaborationClient* CollaborationClient;
    bool Sharing;

  protected:
    vtkNavigationObserver()
      : CollaborationClient(nullptr)
      , Sharing(false)
    {
    }
    ~vtkNavigationObserver() override {}
  };

  vtkNew<vtkNavigationObserver> NavigationObserver;
  bool ShareNavigation;
  vtkVRCollaborationClient* CollaborationClient;
  QString* HeadEventName;
  QString* LeftHandEventName;
  QString* RightHandEventName;
};

// ----------------------------------------------------------------------------
vtkSMVRCollaborationStyleProxy::vtkSMVRCollaborationStyleProxy()
  : Superclass()
  , Internal(new vtkSMVRCollaborationStyleProxy::vtkInternals())
{
  this->SetNavigationSharing(false);
  this->Internal->CollaborationClient = nullptr;
}

// ----------------------------------------------------------------------------
vtkSMVRCollaborationStyleProxy::~vtkSMVRCollaborationStyleProxy() = default;

// ----------------------------------------------------------------------------
vtkCommand* vtkSMVRCollaborationStyleProxy::GetNavigationObserver()
{
  return this->Internal->NavigationObserver;
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "HeadEventName: (" << this->Internal->HeadEventName << "\n";
  os << indent << "LeftHandEventName: (" << this->Internal->LeftHandEventName << "\n";
  os << indent << "RightHandEventName: (" << this->Internal->RightHandEventName << "\n";
  os << indent << "ShareNavigation: (" << this->Internal->ShareNavigation << "\n";
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
void vtkSMVRCollaborationStyleProxy::SetNavigationSharing(bool enabled)
{
  this->Internal->ShareNavigation = enabled;
  this->Internal->NavigationObserver->Sharing = enabled;
}

// ----------------------------------------------------------------------------
bool vtkSMVRCollaborationStyleProxy::GetNavigationSharing()
{
  return this->Internal->ShareNavigation;
}

// ----------------------------------------------------------------------------
void vtkSMVRCollaborationStyleProxy::SetCollaborationClient(vtkVRCollaborationClient* client)
{
  this->Internal->CollaborationClient = client;
  this->Internal->NavigationObserver->CollaborationClient = client;
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

  if (this->GetNavigationSharing())
  {
    vtkMatrix4x4* navMatrix = this->GetNavigationMatrix();
    if (navMatrix)
    {
      // If we're sharing navigation (in addition to just tracker
      // orientations), we need to multiply the navigation and
      // and tracker matrices to get the position/orientation we
      // share with collaborators.
      vtkNew<vtkMatrix4x4> invNavMatrix;
      invNavMatrix->DeepCopy(navMatrix);
      invNavMatrix->Invert();
      vtkNew<vtkMatrix4x4> trackerMatrix;
      trackerMatrix->DeepCopy(event.data.tracker.matrix);
      vtkMatrix4x4::Multiply4x4(invNavMatrix, trackerMatrix, trackerMatrix);

      // Update position and orientation to include navigation
      wpos[0] = trackerMatrix->GetElement(0, 3);
      wpos[1] = trackerMatrix->GetElement(1, 3);
      wpos[2] = trackerMatrix->GetElement(2, 3);

      ortho[0][0] = trackerMatrix->GetElement(0, 0); // [0];
      ortho[1][0] = trackerMatrix->GetElement(1, 0); // [4];
      ortho[2][0] = trackerMatrix->GetElement(2, 0); // [8];

      ortho[0][1] = trackerMatrix->GetElement(0, 1); // [1];
      ortho[1][1] = trackerMatrix->GetElement(1, 1); // [5];
      ortho[2][1] = trackerMatrix->GetElement(2, 1); // [9];

      ortho[0][2] = trackerMatrix->GetElement(0, 2); // [2];
      ortho[1][2] = trackerMatrix->GetElement(1, 2); // [6];
      ortho[2][2] = trackerMatrix->GetElement(2, 2); // [10];
    }
  }

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
