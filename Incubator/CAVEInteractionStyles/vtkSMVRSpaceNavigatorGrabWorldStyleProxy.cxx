// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSMVRSpaceNavigatorGrabWorldStyleProxy.h"

#include "vtkCamera.h" /* needed by vtkSMRenderViewProxy.h */
#include "vtkMath.h"   /* needed for Cross product function */
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h" /* for acquiring the active camera */
#include "vtkVRQueue.h"           /* for the vtkVREvent structure */

#include <algorithm>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSMVRSpaceNavigatorGrabWorldStyleProxy);

// ----------------------------------------------------------------------------
// Constructor method
vtkSMVRSpaceNavigatorGrabWorldStyleProxy::vtkSMVRSpaceNavigatorGrabWorldStyleProxy()
  : Superclass()
{
  this->AddAnalogRole("Move");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkSMVRSpaceNavigatorGrabWorldStyleProxy::~vtkSMVRSpaceNavigatorGrabWorldStyleProxy() = default;

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkSMVRSpaceNavigatorGrabWorldStyleProxy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// HandleAnalog() method
void vtkSMVRSpaceNavigatorGrabWorldStyleProxy::HandleAnalog(const vtkVREvent& event)
{
  std::string role = this->GetAnalogRole(event.name);

  if (role == "Move")
  {
    // A Space Navigator will have 6 analog data streams.  Ignore data
    //   if this input does not match this parameter of the Space Navigator.
    if (event.data.analog.num_channels != 6)
    {
      return;
    }

    vtkSMRenderViewProxy* viewProxy = vtkSMRenderViewProxy::SafeDownCast(this->ControlledProxy);
    if (viewProxy)
    {
      vtkCamera* camera;
#define MOVEMENT_FACTOR 0.05
      double camera_location[3];
      double focal_point[3];
      double up_vector[3];
      double forward_vector[3];
      double orient[3];
      const double* analog_input = event.data.analog.channel;

      camera = viewProxy->GetActiveCamera();

      camera->GetPosition(camera_location);
      camera->GetFocalPoint(focal_point);
      camera->GetDirectionOfProjection(forward_vector);
      camera->OrthogonalizeViewUp();
      camera->GetViewUp(up_vector);

      // Apply up-down motion
      for (int i = 0; i < 3; i++)
      {
        double dx = MOVEMENT_FACTOR * analog_input[2] * up_vector[i];

        camera_location[i] += dx;
        focal_point[i] += dx;
      }

      // Apply right-left motion
      double side_vector[3];
      vtkMath::Cross(forward_vector, up_vector, side_vector);

      for (int i = 0; i < 3; i++)
      {
        double dx = -MOVEMENT_FACTOR * analog_input[0] * side_vector[i];

        camera_location[i] += dx;
        focal_point[i] += dx;
      }

      // Set the two calculated camera values
      camera->SetPosition(camera_location);
      camera->SetFocalPoint(focal_point);

      // Set all the other camera values
      /* WRS: why the use of pow()?  And why doesn't this use the forward_vector? */
      camera->Dolly(pow(1.01, analog_input[1]));
      camera->Elevation(1.0 * analog_input[3]);
      camera->Azimuth(1.0 * analog_input[5]);
      camera->Roll(1.0 * analog_input[4]);
    }
  }
}
