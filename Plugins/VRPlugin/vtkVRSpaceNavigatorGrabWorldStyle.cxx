/*=========================================================================

   Program: ParaView
   Module:  vtkVRSpaceNavigatorGrabWorldStyle.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
#include "vtkVRSpaceNavigatorGrabWorldStyle.h"

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
vtkStandardNewMacro(vtkVRSpaceNavigatorGrabWorldStyle);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRSpaceNavigatorGrabWorldStyle::vtkVRSpaceNavigatorGrabWorldStyle()
  : Superclass()
{
  this->AddAnalogRole("Move");
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRSpaceNavigatorGrabWorldStyle::~vtkVRSpaceNavigatorGrabWorldStyle()
{
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRSpaceNavigatorGrabWorldStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// HandleAnalog() method
void vtkVRSpaceNavigatorGrabWorldStyle::HandleAnalog(const vtkVREvent& event)
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
      camera->Dolly(
        pow(1.01, analog_input[1])); /* WRS: why the use of pow()?  And why doesn't this use the
                                        forward_vector? */
      camera->Elevation(1.0 * analog_input[3]);
      camera->Azimuth(1.0 * analog_input[5]);
      camera->Roll(1.0 * analog_input[4]);
    }
  }
}
