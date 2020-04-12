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

// -----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRGrabWorldStyle)

  // -----------------------------------------------------------------------------
  vtkVRGrabWorldStyle::vtkVRGrabWorldStyle()
  : Superclass()
{
  this->AddButtonRole("Rotate world");
  this->AddButtonRole("Translate world");
  this->AddButtonRole("Reset world");
  this->EnableTranslate = false;
  this->EnableRotate = false;
  this->IsInitialTransRecorded = false;
  this->IsInitialRotRecorded = false;
  this->CachedTransMatrix->Identity();
  this->CachedRotMatrix->Identity();
}

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::~vtkVRGrabWorldStyle()
{
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::HandleButton(const vtkVREventData& data)
{
  std::string role = this->GetButtonRole(data.name);
  if (role == "Translate world")
  {
    this->EnableTranslate = data.data.button.state;
  }
  else if (role == "Rotate world")
  {
    this->EnableRotate = data.data.button.state;
  }
  else if (data.data.button.state && role == "Reset world")
  {
    this->CachedTransMatrix->Identity();
    this->CachedRotMatrix->Identity();

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(&this->CachedTransMatrix->Element[0][0], 16);

    vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
      .Set(&this->CachedRotMatrix->Element[0][0], 16);

    this->IsInitialTransRecorded = false;
    this->IsInitialRotRecorded = false;
  }
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::HandleTracker(const vtkVREventData& data)
{
  std::string role = this->GetTrackerRole(data.name);
  if (role == "Tracker")
  {
    if (this->EnableTranslate)
    {
      this->IsInitialRotRecorded = false;

      vtkNew<vtkMatrix4x4> rotMatrix;
      vtkCamera* camera = 0;
      pqActiveObjects& activeObjs = pqActiveObjects::instance();
      if (pqView* pqview = activeObjs.activeView())
      {
        if (pqRenderView* rview = qobject_cast<pqRenderView*>(pqview))
        {
          if (vtkSMRenderViewProxy* rviewPxy =
                vtkSMRenderViewProxy::SafeDownCast(rview->getProxy()))
          {
            if ((camera = rviewPxy->GetActiveCamera()))
            {
              // vtkCamera API is misleading...view transform == modelview
              rotMatrix->DeepCopy(camera->GetModelViewTransformMatrix());
              rotMatrix->SetElement(0, 3, 0.0);
              rotMatrix->SetElement(1, 3, 0.0);
              rotMatrix->SetElement(2, 3, 0.0);
            }
          }
        }
      }
      if (!camera)
      {
        vtkWarningMacro(<< "Cannot grab active camera.");
        return;
      }

      double speed = GetSpeedFactor(camera);

      if (!this->IsInitialTransRecorded)
      {
        this->InverseInitialTransMatrix->Identity();
        this->InverseInitialTransMatrix->SetElement(0, 3, data.data.tracker.matrix[3] * speed);
        this->InverseInitialTransMatrix->SetElement(1, 3, data.data.tracker.matrix[7] * speed);
        this->InverseInitialTransMatrix->SetElement(2, 3, data.data.tracker.matrix[11] * speed);
        this->InverseInitialTransMatrix->Invert();
        this->IsInitialTransRecorded = true;
      }
      else
      {
        // Get the current tracker matrix, update with initial data
        vtkNew<vtkMatrix4x4> transformMatrix;
        transformMatrix->Identity();
        vtkNew<vtkMatrix4x4> totalTrackerMatrix;
        vtkNew<vtkMatrix4x4> tempMatrix;
        tempMatrix->Identity();

        transformMatrix->SetElement(0, 3, data.data.tracker.matrix[3] * speed);
        transformMatrix->SetElement(1, 3, data.data.tracker.matrix[7] * speed);
        transformMatrix->SetElement(2, 3, data.data.tracker.matrix[11] * speed);
        tempMatrix->DeepCopy(transformMatrix.GetPointer());

        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
          this->InverseInitialTransMatrix.GetPointer(), transformMatrix.GetPointer());

        // Compute all transformations via tracker
        vtkMatrix4x4::Multiply4x4(this->CachedTransMatrix.GetPointer(),
          transformMatrix.GetPointer(), totalTrackerMatrix.GetPointer());

        // Store all transformations
        this->CachedTransMatrix->DeepCopy(totalTrackerMatrix.GetPointer());

        // Apply rotation if exist
        vtkMatrix4x4::Multiply4x4(
          totalTrackerMatrix.GetPointer(), rotMatrix.GetPointer(), totalTrackerMatrix.GetPointer());

        transformMatrix->DeepCopy(totalTrackerMatrix.GetPointer());

        // Set the new matrix for the proxy.
        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Set(&transformMatrix->Element[0][0], 16);
        this->ControlledProxy->UpdateVTKObjects();

        // update the initial matrix to prepare for the next event.
        this->InverseInitialTransMatrix->DeepCopy(tempMatrix.GetPointer());
        this->InverseInitialTransMatrix->Invert();
      }
    }
    else if (this->EnableRotate)
    {
      this->IsInitialTransRecorded = false;

      vtkNew<vtkMatrix4x4> modelViewTransMatrix;
      vtkCamera* camera = 0;

      if (!this->IsInitialRotRecorded)
      {
        this->InverseInitialRotMatrix->Identity();
        this->InverseInitialRotMatrix->DeepCopy(data.data.tracker.matrix);
        this->InverseInitialRotMatrix->SetElement(0, 3, 0);
        this->InverseInitialRotMatrix->SetElement(1, 3, 0);
        this->InverseInitialRotMatrix->SetElement(2, 3, 0);
        this->InverseInitialRotMatrix->Invert();
        this->IsInitialRotRecorded = true;
      }
      else
      {
        bool foundCamera = false;
        pqActiveObjects& activeObjs = pqActiveObjects::instance();
        if (pqView* pqview = activeObjs.activeView())
        {
          if (pqRenderView* rview = qobject_cast<pqRenderView*>(pqview))
          {
            if (vtkSMRenderViewProxy* rviewPxy =
                  vtkSMRenderViewProxy::SafeDownCast(rview->getProxy()))
            {
              if ((camera = rviewPxy->GetActiveCamera()))
              {
                // vtkCamera API is misleading...view transform == modelview
                modelViewTransMatrix->Identity();
                modelViewTransMatrix->SetElement(
                  0, 3, camera->GetModelTransformMatrix()->GetElement(0, 3));
                modelViewTransMatrix->SetElement(
                  1, 3, camera->GetModelTransformMatrix()->GetElement(1, 3));
                modelViewTransMatrix->SetElement(
                  2, 3, camera->GetModelTransformMatrix()->GetElement(2, 3));

                foundCamera = true;

                // Since we are going to reset the camera, preserve the translation
                this->CachedTransMatrix->DeepCopy(modelViewTransMatrix.GetPointer());
              }
            }
          }
        }
        if (!foundCamera)
        {
          vtkWarningMacro(<< "Cannot grab active camera.");
          return;
        }

        // Get the current tracker matrix, update with initial data
        vtkNew<vtkMatrix4x4> transformMatrix;
        transformMatrix->Identity();
        vtkNew<vtkMatrix4x4> totalTrackerMatrix;
        totalTrackerMatrix->Identity();
        vtkNew<vtkMatrix4x4> tempMatrix;
        tempMatrix->Identity();

        transformMatrix->DeepCopy(data.data.tracker.matrix);
        transformMatrix->SetElement(0, 3, 0);
        transformMatrix->SetElement(1, 3, 0);
        transformMatrix->SetElement(2, 3, 0);

        tempMatrix->DeepCopy(transformMatrix.GetPointer());

        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
          this->InverseInitialRotMatrix.GetPointer(), transformMatrix.GetPointer());

        // Compute all transformations via tracker
        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(), this->CachedRotMatrix.GetPointer(),
          totalTrackerMatrix.GetPointer());

        // Compute new model transform matrix
        vtkMatrix4x4::Multiply4x4(modelViewTransMatrix.GetPointer(),
          totalTrackerMatrix.GetPointer(), transformMatrix.GetPointer());

        // Store all transformations
        this->CachedRotMatrix->DeepCopy(totalTrackerMatrix.GetPointer());

        // Set the new matrix for the proxy.
        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Set(&transformMatrix->Element[0][0], 16);

        this->ControlledProxy->UpdateVTKObjects();

        // update the initial matrix to prepare for the next event.
        this->InverseInitialRotMatrix->DeepCopy(tempMatrix.GetPointer());
        this->InverseInitialRotMatrix->Invert();
      }
    }
    else
    {
      this->IsInitialTransRecorded = false;
      this->IsInitialRotRecorded = false;
    }
  }
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableTranslate: " << this->EnableTranslate << endl;
  os << indent << "EnableRotate: " << this->EnableRotate << endl;
  os << indent << "IsInitialTransRecorded: " << this->IsInitialTransRecorded << endl;
  os << indent << "IsInitialRotRecorded: " << this->IsInitialRotRecorded << endl;
  os << indent << "InverseInitialTransMatrix:" << endl;
  this->InverseInitialTransMatrix->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
float vtkVRGrabWorldStyle::GetSpeedFactor(vtkCamera* cam)
{
  // Return the distance between the camera and the focal point.
  double pos[3];
  double foc[3];
  cam->GetPosition(pos);
  cam->GetFocalPoint(foc);
  pos[0] -= foc[0];
  pos[1] -= foc[1];
  pos[2] -= foc[2];
  pos[0] *= pos[0];
  pos[1] *= pos[1];
  pos[2] *= pos[2];
  return sqrt(pos[0] + pos[1] + pos[2]);
}
