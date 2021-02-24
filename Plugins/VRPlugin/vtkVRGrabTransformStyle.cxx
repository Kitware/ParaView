/*=========================================================================

   Program: ParaView
   Module:  vtkVRGrabTransformStyle.cxx

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
#include "vtkVRGrabTransformStyle.h"

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
#include <cmath>
#include <sstream>

// WRS: This version of "vtkVRGrabTransformStyle.cxx" contains code that will
//   output information into a file (or pipe), that allows an helper tool to
//   visualize the interactions that are taking place.  The purpose of this
//   is to help debug the order of transformations performed by this style.

#define OUTPUT_DATA_FOR_DEBUGGING
#ifdef OUTPUT_DATA_FOR_DEBUGGING
// ----------------------------------------------------------------------------
// sendMatrixToPVtest(): Output a 4x4 matrix to produce a graphical representation
// Parameters:
//   freq -- whether or not to output frequently (all), or infrequently, of this marker (to help
//   with data flow)
void sendMatrixToPVtest(int marker, int freq, vtkMatrix4x4* mat)
{
  static int counter = 0;

  if (!freq)
  {
    counter++;
    if (counter == (53 - 1))
    {
      counter = 1;
    }
    else
    {
      return;
    }
  }
  printf("pos %d %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n", marker, mat->Element[0][0],
    mat->Element[1][0], mat->Element[2][0], mat->Element[3][0], mat->Element[0][1],
    mat->Element[1][1], mat->Element[2][1], mat->Element[3][1], mat->Element[0][2],
    mat->Element[1][2], mat->Element[2][2], mat->Element[3][2], mat->Element[0][3],
    mat->Element[1][3], mat->Element[2][3], mat->Element[3][3]);
  fflush(stdout);
}

// ----------------------------------------------------------------------------
// sendLocationToPVtest(): Output a 3 element array to produce a graphical representation
//   freq -- whether or not to output frequently (all), or infrequently, of this marker (to help
//   with data flow)
void sendLocationToPVtest(int marker, int freq, float x, float y, float z)
{
  static int counter = 0;

  if (!freq)
  {
    counter++;
    if (counter == (53 - 1))
    {
      counter = 1;
    }
    else
    {
      return;
    }
  }
  printf("loc %d %f %f %f\n", marker, x, y, z);
  fflush(stdout);
}
#endif /* OUTPUT_DATA_FOR_DEBUGGING */

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRGrabTransformStyle);

// ----------------------------------------------------------------------------
// Constructor method
vtkVRGrabTransformStyle::vtkVRGrabTransformStyle()
  : Superclass()
{
  this->AddButtonRole("Navigate world");
  this->AddButtonRole("Reset world");
  this->EnableNavigate = false;
  this->IsInitialRecorded = false;
}

// ----------------------------------------------------------------------------
// Destructor method
vtkVRGrabTransformStyle::~vtkVRGrabTransformStyle()
{
}

// ----------------------------------------------------------------------------
// PrintSelf() method
void vtkVRGrabTransformStyle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "EnableNavigate: " << this->EnableNavigate << endl;
  os << indent << "IsInitialRecorded: " << this->IsInitialRecorded << endl;
}

// ----------------------------------------------------------------------------
// GetCamera() method
//
// NOTE: in the vtkVRSpaceNavigatorGrabWorldStyle.cxx code, the multiple steps used to
//   get the "rview->getProxy()" value is simply replaced with "this->ControlledProxy".
//   I would like to get an explanation so we know if we can do the same here.
vtkCamera* vtkVRGrabTransformStyle::GetCamera()
{
  vtkCamera* camera = nullptr;
  pqActiveObjects& activeObjs = pqActiveObjects::instance();

  /* Alert!  The following conditional uses a lazy assignment/evaluation -- the "=" is not a bug
   * (BS: I assume) */
  if (pqView* pqview = activeObjs.activeView())
  {
    /* Alert!  The following conditional uses a lazy assignment/evaluation -- the "=" is not a bug
     * (BS: I assume) */
    if (pqRenderView* rview = qobject_cast<pqRenderView*>(pqview))
    {
      /* Alert!  The following conditional uses a lazy assignment/evaluation -- the "=" is not a bug
       * (BS: I assume) */
      if (vtkSMRenderViewProxy* rviewPxy = vtkSMRenderViewProxy::SafeDownCast(rview->getProxy()))
      {
        camera = rviewPxy->GetActiveCamera();
      }
    }
  }
  if (!camera)
  {
    vtkWarningMacro(<< "Cannot grab active camera.");
  }

  return camera;
}

// ----------------------------------------------------------------------------
// HandleButton() method
void vtkVRGrabTransformStyle::HandleButton(const vtkVREvent& event)
{
  std::string role = this->GetButtonRole(event.name);
  if (role == "Navigate world")
  {
    vtkCamera* camera = nullptr;
    this->EnableNavigate = event.data.button.state;
    if (!this->EnableNavigate)
    {
      this->IsInitialRecorded = false;
    }
  }
  else if (event.data.button.state && role == "Reset world")
  {
    vtkNew<vtkMatrix4x4> transformMatrix;
    if (this->ControlledProxy != nullptr)
    {
      vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
        .Set(&transformMatrix.GetPointer()->Element[0][0], 16);
    }
    else
    {
      cout << "ERROR: this->ControlledProxy is NULL.  This should not happen.\n";
    }
    this->IsInitialRecorded = false;
  }
}

// ----------------------------------------------------------------------------
// HandleTracker() method
void vtkVRGrabTransformStyle::HandleTracker(const vtkVREvent& event)
{
  static double lastspeed = -1.0;

  vtkCamera* camera = nullptr;
  std::string role = this->GetTrackerRole(event.name);
  if (role != "Tracker")
    return;
  if (this->EnableNavigate)
  {
    camera = vtkVRGrabTransformStyle::GetCamera();
    if (!camera)
    {
      vtkWarningMacro(<< " HandleTracker: Cannot grab active camera.");
      return;
    }
    double speed = GetSpeedFactor(camera, camera->GetModelViewTransformMatrix());
#ifdef OUTPUT_DATA_FOR_DEBUGGING
    sendLocationToPVtest(5, 1, -3.0, speed * 3,
      0.0); /* exaggerate it a bit to be able to see what's happening better */
    sendMatrixToPVtest(1, 0, camera->GetModelViewTransformMatrix());
#endif
#if 1 /* fix the "speed" factor at 1.0 */
    speed = 1.0;
#endif

    if (!this->IsInitialRecorded)
    {
      // save the ModelView Matrix the Wand Matrix
      this->SavedModelViewMatrix->DeepCopy(
        camera->GetModelViewTransformMatrix()); /* WRS: model-view instead */
#ifdef OUTPUT_DATA_FOR_DEBUGGING
      sendMatrixToPVtest(4, 1, this->SavedModelViewMatrix.GetPointer());
#endif

      this->SavedModelViewMatrix->DeepCopy(camera->GetModelTransformMatrix());
      /* WRS: Nikhil's original */ /* WRS: NOTE: this should really make use of the Proxy/Property
                                      value since that's where we're going to write the altered
                                      value back to */
#ifdef OUTPUT_DATA_FOR_DEBUGGING
      sendMatrixToPVtest(3, 1, this->SavedModelViewMatrix.GetPointer());
#endif
      this->SavedInverseWandMatrix->DeepCopy(event.data.tracker.matrix);

#if 1
      if (isnan(speed))
      {
        fprintf(stderr, "Hey, 'speed' is not-a-number! -- last value was %f\n", lastspeed);
      }
      else
      {
        this->SavedInverseWandMatrix->SetElement(0, 3, event.data.tracker.matrix[3] * speed);
        this->SavedInverseWandMatrix->SetElement(1, 3, event.data.tracker.matrix[7] * speed);
        this->SavedInverseWandMatrix->SetElement(2, 3, event.data.tracker.matrix[11] * speed);
        lastspeed = speed;
#ifdef OUTPUT_DATA_FOR_DEBUGGING
        sendMatrixToPVtest(6, 1,
          this->SavedInverseWandMatrix.GetPointer()); /* send a copy of matrix prior to inversion */
      }
#endif
#endif
      vtkMatrix4x4::Invert(
        this->SavedInverseWandMatrix.GetPointer(), this->SavedInverseWandMatrix.GetPointer());
      this->IsInitialRecorded = true;
#ifdef OUTPUT_DATA_FOR_DEBUGGING
      sendMatrixToPVtest(2, 1, this->SavedInverseWandMatrix.GetPointer());
#endif
      // vtkWarningMacro(<< "button is pressed");
    }
    else
    {
      // Apply the transformation and get the new transformation matrix
      vtkNew<vtkMatrix4x4> transformMatrix;
      transformMatrix->DeepCopy(event.data.tracker.matrix);
#if 1
      if (isnan(speed))
      {
        fprintf(stderr, "Hey, 'speed' is not-a-number! -- last value was %f\n", lastspeed);
      }
      else
      {
        transformMatrix->SetElement(0, 3, event.data.tracker.matrix[3] * speed);
        transformMatrix->SetElement(1, 3, event.data.tracker.matrix[7] * speed);
        transformMatrix->SetElement(2, 3, event.data.tracker.matrix[11] * speed);
        lastspeed = speed;
      }
#endif

      /* swapping order of both multiplies */
      vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
        this->SavedInverseWandMatrix.GetPointer(), transformMatrix.GetPointer());
      vtkMatrix4x4::Multiply4x4(transformMatrix.GetPointer(),
        this->SavedModelViewMatrix.GetPointer(), transformMatrix.GetPointer());
#ifdef OUTPUT_DATA_FOR_DEBUGGING
      sendMatrixToPVtest(0, 0, transformMatrix.GetPointer());
#endif

      // Set the new matrix for the proxy.
      if (this->ControlledProxy != nullptr)
      {
        vtkSMPropertyHelper(this->ControlledProxy, this->ControlledPropertyName)
          .Set(&transformMatrix->Element[0][0], 16);
        this->ControlledProxy->UpdateVTKObjects();
        // vtkWarningMacro(<< "button is continuously pressed");
      }
      else
      {
        cout << "ERROR: this->ControlledProxy is NULL.  This shouldn't happen.\n";
      }
    }
  }
}

// ----------------------------------------------------------------------------
float vtkVRGrabTransformStyle::GetSpeedFactor(vtkCamera* cam, vtkMatrix4x4* mvmatrix)
{
  // Return the distance between the camera and the focal point.
  // WRS: and the distance between the camera and the focal point is a speed factor???
  double pos[3];
  double foc[3];
  float result;

#if 1 /* Aashish's method from SC13  (if set to "1") */
  cam->GetEyePosition(pos);
  foc[0] = mvmatrix->GetElement(0, 3);
  foc[1] = mvmatrix->GetElement(1, 3);
  foc[2] = mvmatrix->GetElement(2, 3);
  result =
    std::log(sqrt((pos[0] - foc[0]) * (pos[0] - foc[0]) + (pos[1] - foc[1]) * (pos[1] - foc[1]) +
      (pos[2] - foc[2]) *
        (pos[2] - foc[2]))); /* WRS: I added the squaring of each component -- otherwise nan-city */
#else                        /* pre-SC13 method */
  /* WRS: Note the "Grab-Transform" version of this used to not square each component, resulting in
   * many nan results */
  cam->GetPosition(pos);
  cam->GetFocalPoint(foc);
  pos[0] -= foc[0];
  pos[1] -= foc[1];
  pos[2] -= foc[2];
  pos[0] *= pos[0];
  pos[1] *= pos[1];
  pos[2] *= pos[2];
  result = sqrt(pos[0] + pos[1] + pos[2]);
#endif

  if (isnan(result))
  {
    fprintf(stderr, "GetSpeedFactor -- nan -- pos = (%f %f %f), foc = (%f %f %f)\n", pos[0], pos[1],
      pos[2], foc[0], foc[1], foc[2]);
    return 1.0;
  }

  return result;
}
