/*=========================================================================

   Program: ParaView
   Module:    $RCSfile$

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
#include "vtkVRStyleGrabNRotateWorld.h"

#include "pqActiveObjects.h"
#include "pqView.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVRQueue.h"
#include <algorithm>
#include <iostream>
#include <sstream>

//------------------------------------------------------------------------cnstr
vtkVRStyleGrabNRotateWorld::vtkVRStyleGrabNRotateWorld(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Enabled = false;
  this->InitialOrientationRecored = false;
  this->InitialInvertedPose = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------destr
vtkVRStyleGrabNRotateWorld::~vtkVRStyleGrabNRotateWorld()
{
  this->InitialInvertedPose->Delete();
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (child->GetName() && strcmp(child->GetName(), "Style") == 0 &&
    strcmp(this->metaObject()->className(), child->GetAttributeOrEmpty("class")) == 0)
  {
    if (child->GetNumberOfNestedElements() != 2)
    {
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
                << "There has to be only 2 elements present " << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl
                << "<Tracker name=\"trackerEventName\"/>" << std::endl;
    }
    vtkPVXMLElement* button = child->GetNestedElement(0);
    if (button && button->GetName() && strcmp(button->GetName(), "Button") == 0)
    {
      this->Button = button->GetAttributeOrEmpty("name");
    }
    else
    {
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
                << "Button event has to be specified" << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl;
      return false;
    }
    vtkPVXMLElement* tracker = child->GetNestedElement(1);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Tracker") == 0)
    {
      this->Tracker = tracker->GetAttributeOrEmpty("name");
    }
    else
    {
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
                << "Please Specify Tracker event" << std::endl
                << "<Tracker name=\"TrackerEventName\"/>" << std::endl;
      return false;
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------public
vtkPVXMLElement* vtkVRStyleGrabNRotateWorld::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class", this->metaObject()->className());

  vtkPVXMLElement* button = vtkPVXMLElement::New();
  button->SetName("Button");
  button->AddAttribute("name", this->Button.c_str());
  child->AddNestedElement(button);
  button->FastDelete();

  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name", this->Tracker.c_str());
  child->AddNestedElement(tracker);
  tracker->FastDelete();

  return child;
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::handleEvent(const vtkVREventData& data)
{
  switch (data.eventType)
  {
    case BUTTON_EVENT:
      if (this->Button == data.name)
      {
        this->HandleButton(data);
      }
      break;
    case ANALOG_EVENT:
      this->HandleAnalog(data);
      break;
    case TRACKER_EVENT:
      if (this->Tracker == data.name)
      {
        this->HandleTracker(data);
      }
      break;
  }
  return false;
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::update()
{
  pqView* view = 0;
  vtkSMRenderViewProxy* proxy = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      proxy->UpdateVTKObjects();
      proxy->StillRender();
    }
  }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleButton(const vtkVREventData& data)
{
  std::cout << data.name << std::endl;
  this->Enabled = data.data.button.state;
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleAnalog(const vtkVREventData& data)
{
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleTracker(const vtkVREventData& data)
{

  // check if the button is clicked
  // if it is then get the active proxy and property
  // check if the proxy and property are successfull obtained
  //
  if (this->Enabled)
  {
    std::cout << "its time to rotate " << std::endl;
    vtkSMRenderViewProxy* proxy = 0;
    vtkSMDoubleVectorProperty* prop = 0;

    pqView* view = 0;
    view = pqActiveObjects::instance().activeView();
    if (view)
    {
      proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
      if (proxy)
      {
        prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("ModelTransformMatrix"));
        if (prop)
        {
#define FANCY_LOGIC 0
#define VRUI_LOGIC 1
#if FANCY_LOGIC
          if (!this->InitialOrientationRecored)
          {
            this->RecordOrientation(proxy, data);
            this->InitialOrientationRecored = true;
          }
          else
          {
            this->UpdateOrientation(data);

            // Transform the new quaternion to matrix
            double newMat[3][3];
            vtkMath::QuaternionToMatrix3x3(this->UpdatedQuat, newMat);

            // Update the property
            prop->SetElement(0, newMat[0][0]);
            prop->SetElement(1, newMat[0][1]);
            prop->SetElement(2, newMat[0][2]);

            prop->SetElement(4, newMat[1][0]);
            prop->SetElement(5, newMat[1][1]);
            prop->SetElement(6, newMat[1][2]);

            prop->SetElement(8, newMat[2][0]);
            prop->SetElement(9, newMat[2][1]);
            prop->SetElement(10, newMat[2][2]);
            // this->RecordOrientation( proxy , data);
          }
#elif VRUI_LOGIC
          if (!this->InitialOrientationRecored)
          {
            // Copy the data into matrix
            for (int i = 0; i < 4; ++i)
            {
              for (int j = 0; j < 4; ++j)
              {
                this->InitialInvertedPose->SetElement(i, j, data.data.tracker.matrix[i * 4 + j]);
              }
            }
            // invert the matrix
            vtkMatrix4x4::Invert(this->InitialInvertedPose, this->InitialInvertedPose);
            double modelTransformMatrix[16];
            vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(modelTransformMatrix, 16);
            vtkMatrix4x4::Multiply4x4(&this->InitialInvertedPose->Element[0][0],
              modelTransformMatrix, &this->InitialInvertedPose->Element[0][0]);
            this->InitialOrientationRecored = true;
          }
          else
          {
            double modelTransformMatrix[16];
            vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
              &this->InitialInvertedPose->Element[0][0], modelTransformMatrix);
            // // Copy the data into matrix
            // for (int i = 0; i < 4; ++i)
            //   {
            //   for (int j = 0; j < j; ++j)
            //  {
            //  this->InitialInvertedPose->SetElement( i,j,data.data.tracker.matrix[i*4+j] );
            //  }
            //   }
            // // invert the matrix
            // vtkMatrix4x4::Invert( this->InitialInvertedPose, this->InitialInvertedPose );
            vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Set(modelTransformMatrix, 16);
          }
#else
          prop->SetElement(0, data.data.tracker.matrix[0]);
          prop->SetElement(1, data.data.tracker.matrix[1]);
          prop->SetElement(2, data.data.tracker.matrix[2]);

          prop->SetElement(4, data.data.tracker.matrix[4]);
          prop->SetElement(5, data.data.tracker.matrix[5]);
          prop->SetElement(6, data.data.tracker.matrix[6]);

          prop->SetElement(8, data.data.tracker.matrix[8]);
          prop->SetElement(9, data.data.tracker.matrix[9]);
          prop->SetElement(10, data.data.tracker.matrix[10]);
#endif
        }
      }
    }
  }
  else
  {
    this->InitialOrientationRecored = false;
  }
}

//----------------------------------------------------------------------private
std::vector<std::string> vtkVRStyleGrabNRotateWorld::tokenize(std::string input)
{
  std::replace(input.begin(), input.end(), '.', ' ');
  std::istringstream stm(input);
  std::vector<std::string> token;
  for (;;)
  {
    std::string word;
    if (!(stm >> word))
      break;
    token.push_back(word);
  }
  return token;
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::RecordOrientation(
  vtkSMRenderViewProxy* proxy, const vtkVREventData& data)
{
  double mat[3][3];

  // Collect the initial rotation matrix
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      mat[i][j] = data.data.tracker.matrix[i * 4 + j];
    }
  }

  vtkMath::Matrix3x3ToQuaternion(mat, this->InitialTrackerQuat);

  // Collect the current rotation matrix
  double old[16];
  double oldMat[3][3];
  vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(&old[0], 16);
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      oldMat[i][j] = old[i * 4 + j];
    }
  }
  // Convert rotation matrix to quaternion
  vtkMath::Matrix3x3ToQuaternion(oldMat, this->InitialQuat);
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::UpdateOrientation(const vtkVREventData& data)
{
  double mat[3][3];
  double quat[4];
  double deltaQuat[4];
  // Collect the initial rotation matrix
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      mat[i][j] = data.data.tracker.matrix[i * 4 + j];
    }
  }

  // Make quaternion
  vtkMath::Matrix3x3ToQuaternion(mat, quat);

  // Get the delta rotation
  deltaQuat[0] = quat[0] - this->InitialTrackerQuat[0];
  deltaQuat[1] = quat[1] - this->InitialTrackerQuat[1];
  deltaQuat[2] = quat[2] - this->InitialTrackerQuat[2];
  deltaQuat[3] = quat[3] - this->InitialTrackerQuat[3];

  // if ( fabs( deltaQuat[0] ) > 0 &&
  //      ( fabs( deltaQuat[1] ) > 0 ||
  //      fabs( deltaQuat[2] ) > 0 ||
  //      fabs( deltaQuat[3] ) > 0 ) )
  //   {
  // Multiply new quaternion into inital quaternion

  // double mag = fabs( sqrt ( deltaQuat[0]*deltaQuat[0] +
  //                        deltaQuat[1]*deltaQuat[1] +
  //                        deltaQuat[2]*deltaQuat[2] +
  //                        deltaQuat[3]*deltaQuat[3] ) );
  // if ( mag > 0 )
  // deltaQuat[0] = deltaQuat[0]/mag;
  // deltaQuat[1] = deltaQuat[1]/mag;
  // deltaQuat[2] = deltaQuat[2]/mag;
  // deltaQuat[3] = deltaQuat[3]/mag;
  vtkMath::MultiplyQuaternion(deltaQuat, this->InitialQuat, this->UpdatedQuat);
  // vtkMath::MultiplyQuaternion( quat,  this->InitialQuat,  this->UpdatedQuat );
  // std::cout << "deltaQuat : ["
  //           << deltaQuat[0] << " "
  //           << deltaQuat[1] << " "
  //           << deltaQuat[2] << " "
  //           << deltaQuat[3] << "]" << std::endl;
  // this->InitialTrackerQuat[0] = quat[0];
  // this->InitialTrackerQuat[1] = quat[1];
  // this->InitialTrackerQuat[2] = quat[2];
  // this->InitialTrackerQuat[3] = quat[3];
  // }
}
