/*=========================================================================

   Program: ParaView
   Module:    vtkVRSpaceNavigatorGrabWorldStyle.cxx

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

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRQueue.h"

#include <algorithm>
#include <sstream>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRSpaceNavigatorGrabWorldStyle)

  // ----------------------------------------------------------------------------
  vtkVRSpaceNavigatorGrabWorldStyle::vtkVRSpaceNavigatorGrabWorldStyle()
  : Superclass()
{
  this->AddAnalogRole("Move");
}

// ----------------------------------------------------------------------------
vtkVRSpaceNavigatorGrabWorldStyle::~vtkVRSpaceNavigatorGrabWorldStyle()
{
}

// ----------------------------------------------------------------------------
void vtkVRSpaceNavigatorGrabWorldStyle::HandleAnalog(const vtkVREventData& data)
{
  std::string role = this->GetAnalogRole(data.name);
  if (role == "Move")
  {
    // Values for Space Navigator
    if (data.data.analog.num_channel != 6)
    {
      return;
    }

    vtkSMRenderViewProxy* viewProxy = vtkSMRenderViewProxy::SafeDownCast(this->ControlledProxy);
    if (viewProxy)
    {
      vtkCamera* camera;
      double pos[3], fp[3], up[3], dir[3];
      const double* channel = data.data.analog.channel;

      camera = viewProxy->GetActiveCamera();

      camera->GetPosition(pos);
      camera->GetFocalPoint(fp);
      camera->GetDirectionOfProjection(dir);
      camera->OrthogonalizeViewUp();
      camera->GetViewUp(up);

      // Apply up-down motion
      for (int i = 0; i < 3; i++)
      {
        double dx = 0.05 * channel[2] * up[i];
        pos[i] += dx;
        fp[i] += dx;
      }

      // Apply right-left motion
      double r[3];
      vtkMath::Cross(dir, up, r);

      for (int i = 0; i < 3; i++)
      {
        double dx = -0.05 * channel[0] * r[i];
        pos[i] += dx;
        fp[i] += dx;
      }

      camera->SetPosition(pos);
      camera->SetFocalPoint(fp);

      camera->Dolly(pow(1.01, channel[1]));
      camera->Elevation(1.0 * channel[3]);
      camera->Azimuth(1.0 * channel[5]);
      camera->Roll(1.0 * channel[4]);
    }
  }
}

void vtkVRSpaceNavigatorGrabWorldStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
