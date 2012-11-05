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
  this->IsCacheInitialized = false;
  this->NeedsButton = true;
}

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::~vtkVRGrabWorldStyle()
{
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

void printMatrix(vtkMatrix4x4 *mat, const char *name)
{
#if 0
  vtkOStrStreamWrapper vtkmsg;
  vtkmsg << name << ":\n  Det: " << mat->Determinant() << "\n";
  mat->Print(vtkmsg);
  vtkOutputWindowDisplayWarningText(vtkmsg.str());
#endif
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
        this->InverseInitialMatrix->DeepCopy( data.data.tracker.matrix );
        this->InverseInitialMatrix->Invert();
        this->IsInitialRecorded = true;
        }
      else
        {
        // Try to get the current camera and extract the view matrix
        vtkNew<vtkMatrix4x4> viewMatrix;
        bool foundCamera = false;
        pqActiveObjects &activeObjs = pqActiveObjects::instance();
        if (pqView *pqview = activeObjs.activeView())
          {
          if (pqRenderView *rview = qobject_cast<pqRenderView*>(pqview))
            {
            if (vtkSMRenderViewProxy *rviewPxy =
                vtkSMRenderViewProxy::SafeDownCast(rview->getProxy()))
              {
              if (vtkCamera *camera = rviewPxy->GetActiveCamera())
                {
                // vtkCamera API is misleading...view transform == modelview
                vtkNew<vtkMatrix4x4> modelViewMatrix;
                modelViewMatrix->DeepCopy(camera->GetViewTransformMatrix());

                vtkNew<vtkMatrix4x4> invModelMatrix;
                vtkSMPropertyHelper(rviewPxy, "ModelTransformMatrix").Get(
                      &invModelMatrix->Element[0][0]);
                invModelMatrix->Invert();

                // Calculate the view matrix from what we have
                vtkMatrix4x4::Multiply4x4(modelViewMatrix.GetPointer(),
                                          invModelMatrix.GetPointer(),
                                          viewMatrix.GetPointer());

                foundCamera = true;
                }
              }
            }
          }
        if (!foundCamera)
          {
          vtkWarningMacro(<<"Cannot grab active camera.");
          return;
          }

        // Calculate the inverse view matrix
        printMatrix(viewMatrix.GetPointer(), "View");
        vtkNew<vtkMatrix4x4> invViewMatrix;
        invViewMatrix->DeepCopy(viewMatrix.GetPointer());
        invViewMatrix->Invert();

        // Get the current matrix from the proxy
        vtkNew<vtkMatrix4x4> controlledMatrix;
        vtkSMPropertyHelper(this->ControlledProxy,
                            this->ControlledPropertyName).Get(
              &controlledMatrix->Element[0][0], 16);

//        if (this->IsCacheInitialized)
//          {
//          vtkMatrix4x4::Multiply4x4(controlledMatrix.GetPointer(),
//                                    this->CachedMatrix.GetPointer(),
//                                    controlledMatrix.GetPointer());
//          }

        // Get the current tracker matrix, update with initial data
        vtkNew<vtkMatrix4x4> transformMatrix;
        transformMatrix->DeepCopy(data.data.tracker.matrix);
        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                                  this->InverseInitialMatrix.GetPointer(),
                                  transformMatrix.GetPointer());
        printMatrix(transformMatrix.GetPointer(), "Net transform");

        // Account for camera
//        vtkMatrix4x4::Multiply4x4(invViewMatrix.GetPointer(),
//                                  transformMatrix.GetPointer(),
//                                  transformMatrix.GetPointer());
//        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
//                                  viewMatrix.GetPointer(),
//                                  transformMatrix.GetPointer());

        printMatrix(transformMatrix.GetPointer(), "Net transform w/ camera");

        printMatrix(controlledMatrix.GetPointer(), "Original model");

        // Apply the transformation
        vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
                                  controlledMatrix.GetPointer(),
                                  controlledMatrix.GetPointer());

        // Adjust for camera
//        vtkMatrix4x4::Multiply4x4(controlledMatrix.GetPointer(),
//                                  invViewMatrix.GetPointer(),
//                                  controlledMatrix.GetPointer());

        printMatrix(controlledMatrix.GetPointer(), "New model");

        // Set the new matrix for the proxy.
        vtkSMPropertyHelper(this->ControlledProxy,
                            this->ControlledPropertyName).Set(
              &controlledMatrix->Element[0][0], 16);
        this->ControlledProxy->UpdateVTKObjects();

        // update the initial matrix to prepare for the next event.
        this->InverseInitialMatrix->DeepCopy( data.data.tracker.matrix );
        this->InverseInitialMatrix->Invert();

        this->IsCacheInitialized = true;
        this->CachedMatrix->DeepCopy(viewMatrix.GetPointer());
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
  os << indent << "InverseInitialMatrix:" << endl;
  this->InverseInitialMatrix->PrintSelf(os, indent.GetNextIndent());
}
