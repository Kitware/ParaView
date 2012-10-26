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
#include "vtkVRControlSliceOrientationStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include "pqView.h"
#include "pqActiveObjects.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRControlSliceOrientationStyle)

// ----------------------------------------------------------------------------
vtkVRControlSliceOrientationStyle::vtkVRControlSliceOrientationStyle()
{
  this->Enabled = false;
  this->InitialOrientationRecorded = false;
  this->NeedsButton = true;
  this->NeedsTracker = true;
}

// ----------------------------------------------------------------------------
void vtkVRControlSliceOrientationStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialOrientationRecorded: "
     << this->InitialOrientationRecorded << endl;
  os << indent << "InitialQuat: " << this->InitialQuat[0] << " "
     << this->InitialQuat[1] << " " << this->InitialQuat[2] << " "
     << this->InitialQuat[3] << endl;
  os << indent << "InitialTrackerQuat: " << this->InitialTrackerQuat[0] << " "
     << this->InitialTrackerQuat[1] << " " << this->InitialTrackerQuat[2] << " "
     << this->InitialTrackerQuat[3] << endl;
  os << indent << "UpdatedQuat: " << this->UpdatedQuat[0] << " "
     << this->UpdatedQuat[1] << " " << this->UpdatedQuat[2] << " "
     << this->UpdatedQuat[3] << endl;
  os << indent << "Normal: " << this->Normal[0] << " " << this->Normal[1] << " "
     << this->Normal[2] << " " << this->Normal[3] << endl;

  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
vtkVRControlSliceOrientationStyle::~vtkVRControlSliceOrientationStyle()
{
}

// ----------------------------------------------------------------------------
bool vtkVRControlSliceOrientationStyle::Configure(vtkPVXMLElement* child,
                                                  vtkSMProxyLocator* locator)
{
  if (!this->Superclass::Configure(child, locator))
    {
    return false;
    }

  if (!this->ButtonName)
    {
    std::cerr << "vtkVRControlSliceOrientationStyle::Configure(): "
              << "Button event has to be specified" << std::endl
              << "<Button name=\"buttonEventName\"/>"
              << std::endl;
    return false;
    }

  if (!this->TrackerName)
    {
    std::cerr << "vtkVRControlSliceOrientationStyle::Configure(): "
              << "Please Specify Tracker event" << std::endl
              << "<Tracker name=\"TrackerEventName\"/>"
              << std::endl;
    return false;
    }

  if (this->ControlledProxy == NULL ||
      this->ControlledPropertyName == NULL ||
      this->ControlledPropertyName[0] == '\0')
    {
    std::cerr << "vtkVRControlSliceOrientationStyle::Configure(): "
                 "Proxy/property undefined!" << std::endl;
    return false;
    }

  return true;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRControlSliceOrientationStyle::SaveConfiguration() const
{
  return this->Superclass::SaveConfiguration();
}

// ----------------------------------------------------------------------------
bool vtkVRControlSliceOrientationStyle::Update()
{
  pqView *view = 0;
  vtkSMRenderViewProxy *proxy =0;
  view = pqActiveObjects::instance().activeView();
  if (view)
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
      {
      this->ControlledProxy->UpdateVTKObjects();
      proxy->UpdateVTKObjects();
      }
    }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRControlSliceOrientationStyle::HandleButton(const vtkVREventData& data)
{
  if (data.name.compare(this->ButtonName) == 0)
    {
    this->Enabled = data.data.button.state;
    }
}

// ----------------------------------------------------------------------------
void vtkVRControlSliceOrientationStyle::HandleTracker(const vtkVREventData& data)
{
  if (data.name.compare(this->TrackerName) != 0)
    {
    return;
    }

  if (!this->Enabled)
    {
    this->InitialOrientationRecorded = false;
    return;
    }

  vtkSMRenderViewProxy *proxy =0;
  vtkSMDoubleVectorProperty *prop =0;
  pqView *view = pqActiveObjects::instance().activeView();
  if (view)
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
      {
      prop = vtkSMDoubleVectorProperty::SafeDownCast(
            proxy->GetProperty("ModelTransformMatrix"));
      if (prop)
        {
        if (!this->InitialOrientationRecorded)
          {
          // Copy the data into matrix
          for (int i = 0; i < 4; ++i)
            {
            for (int j = 0; j < 4; ++j)
              {
              this->InitialInvertedPose->SetElement(
                    i, j, data.data.tracker.matrix[i*4+j]);
              }
            }
          // invert the matrix
          this->InitialInvertedPose->Invert();
          double modelTransformMatrix[16];
          vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(
                modelTransformMatrix, 16);
          vtkMatrix4x4::Multiply4x4(&this->InitialInvertedPose->Element[0][0],
                                    modelTransformMatrix,
                                    &this->InitialInvertedPose->Element[0][0]);
          vtkSMPropertyHelper(this->ControlledProxy,
                              this->ControlledPropertyName).Get(this->Normal,
                                                                3);
          this->Normal[3] = 0;
          this->InitialOrientationRecorded = true;
          }
        else
          {
          double modelTransformMatrix[16];
          double normal[4];
          vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
                                    &this->InitialInvertedPose->Element[0][0],
                                    modelTransformMatrix);
          vtkMatrix4x4::MultiplyPoint(modelTransformMatrix, this->Normal,
                                      normal);
          vtkSMPropertyHelper(this->ControlledProxy,
                              this->ControlledPropertyName).Set(normal, 3);
          }
        }
      }
    }
}
