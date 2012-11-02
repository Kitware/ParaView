/*=========================================================================

   Program: ParaView
   Module:    vtkVRGrabWorldStyle.cxx

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
#include "vtkVRGrabWorldStyle.h"

#include "vtkCamera.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqView.h"

#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRGrabWorldStyle)

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::vtkVRGrabWorldStyle() :
  Superclass()
{
  this->Enabled = false;
  this->IsInitialRecorded =false;
  this->InverseInitialTransform = vtkTransform::New();
  this->NeedsButton = true;
}

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::~vtkVRGrabWorldStyle()
{
  this->InverseInitialTransform->Delete();
}

// ----------------------------------------------------------------------------
bool vtkVRGrabWorldStyle::Configure(
  vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::Configure(child, locator))
    {
    return false;
    }

  if (this->ButtonName == NULL || this->ButtonName[0] == '\0')
    {
    vtkErrorMacro(<<"Incorrect state for vtkVRGrabWorldStyle");
    return false;
    }

  return true;
}

// -----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRGrabWorldStyle::SaveConfiguration() const
{
  return this->Superclass::SaveConfiguration();
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::HandleButton( const vtkVREventData& data )
{
  if (data.name == std::string(this->ButtonName))
    {
    this->Enabled = data.data.button.state;
    }
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::HandleTracker( const vtkVREventData& data )
{
  if (data.name == std::string(this->TrackerName))
    {
    if ( this->Enabled )
      {
      if ( !this->IsInitialRecorded )
        {
        this->InverseInitialTransform->SetMatrix( data.data.tracker.matrix );
        this->InverseInitialTransform->Inverse();
        this->IsInitialRecorded = true;
        }
      else
        {
        // Try to get the current camera...
        vtkCamera *camera = NULL;
        pqActiveObjects &activeObjs = pqActiveObjects::instance();
        if (pqView *pqview = activeObjs.activeView())
          {
          if (pqRenderView *rview = qobject_cast<pqRenderView*>(pqview))
            {
            if (vtkSMRenderViewProxy *rviewPxy =
                vtkSMRenderViewProxy::SafeDownCast(rview->getProxy()))
              {
              camera = rviewPxy->GetActiveCamera();
              }
            }
          }
        if (camera == NULL)
          {
          vtkWarningMacro(<<"Cannot grab active camera.");
          return;
          }

        // Get the camera matrix and it's inverse
        vtkNew<vtkMatrix4x4> cameraMatrix;
        vtkNew<vtkMatrix4x4> invCameraMatrix;
        cameraMatrix->DeepCopy(camera->GetViewTransformMatrix());
        invCameraMatrix->DeepCopy(cameraMatrix.GetPointer());
        invCameraMatrix->Invert();

        // TODO I'm pretty sure that the multiplication order below is wrong...
        // Get the current tracker matrix
        vtkNew<vtkMatrix4x4> trackerMatrix;
        trackerMatrix->DeepCopy(data.data.tracker.matrix);

        // Calculate the full transform considering the camera offset
        vtkMatrix4x4::Multiply4x4(invCameraMatrix.GetPointer(),
                                  trackerMatrix.GetPointer(),
                                  trackerMatrix.GetPointer());
        vtkMatrix4x4::Multiply4x4(this->InverseInitialTransform->GetMatrix(),
                                  trackerMatrix.GetPointer(),
                                  trackerMatrix.GetPointer());
        vtkMatrix4x4::Multiply4x4(cameraMatrix.GetPointer(),
                                  trackerMatrix.GetPointer(),
                                  trackerMatrix.GetPointer());

        // Update the proxy.
        vtkNew<vtkMatrix4x4> controlledMatrix;
        vtkSMPropertyHelper(this->ControlledProxy,
                            this->ControlledPropertyName).Get(
              &controlledMatrix->Element[0][0], 16);

        vtkMatrix4x4::Multiply4x4(trackerMatrix.GetPointer(),
                                  controlledMatrix.GetPointer(),
                                  controlledMatrix.GetPointer());

        vtkSMPropertyHelper(this->ControlledProxy,
                            this->ControlledPropertyName).Set(
              &controlledMatrix->Element[0][0], 16);
        this->ControlledProxy->UpdateVTKObjects();
        }
      }
    else
      {
      this->IsInitialRecorded = false;
      }
    }
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
  if (this->InverseInitialTransform)
    {
    os << indent << "InverseInitialMatrix:" << endl;
    this->InverseInitialTransform->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "InverseInitialMatrix: (None)" << endl;
    }
}
