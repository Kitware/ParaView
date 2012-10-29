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
#include "vtkVRControlSlicePositionStyle.h"

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
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRControlSlicePositionStyle)

// ----------------------------------------------------------------------------
vtkVRControlSlicePositionStyle::vtkVRControlSlicePositionStyle()
{
  this->Enabled = false;
  this->InitialPositionRecorded = false;
  this->NeedsButton = true;
  this->NeedsTracker = true;
}

// ----------------------------------------------------------------------------
void vtkVRControlSlicePositionStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Enabled: " << this->Enabled << endl;
  os << indent << "InitialPositionRecorded: " << this->InitialPositionRecorded
     << endl;
  os << indent << "InitialPos: " << this->InitialPos[0] << " "
     << this->InitialPos[1] << " " << this->InitialPos[2] << endl;
  os << indent << "Origin: " << this->Origin[0] << " "
     << this->Origin[1] << " " << this->Origin[2] << " " << this->Origin[3]
     << endl;

  os << indent << "Old:" << endl;
  this->Old->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Tx:" << endl;
  this->Tx->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Neo:" << endl;
  this->Neo->PrintSelf(os, indent.GetNextIndent());
  os << indent << "InitialInvertedPose:" << endl;
  this->InitialInvertedPose->PrintSelf(os, indent.GetNextIndent());
}

// ----------------------------------------------------------------------------
vtkVRControlSlicePositionStyle::~vtkVRControlSlicePositionStyle()
{
}

// ----------------------------------------------------------------------------
bool vtkVRControlSlicePositionStyle::Configure(vtkPVXMLElement *child,
                                               vtkSMProxyLocator *locator)
{
  if (!this->Superclass::Configure(child, locator))
    {
    return false;
    }

  if (!this->ButtonName)
    {
    std::cerr << "vtkVRControlSlicePositionStyle::Configure(): "
              << "Button event has to be specified" << std::endl
              << "<Button name=\"buttonEventName\"/>"
              << std::endl;
    return false;
    }

  if (!this->TrackerName)
    {
    std::cerr << "vtkVRControlSlicePositionStyle::Configure(): "
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
vtkPVXMLElement* vtkVRControlSlicePositionStyle::SaveConfiguration() const
{
  return this->Superclass::SaveConfiguration();
}

// ----------------------------------------------------------------------------
bool vtkVRControlSlicePositionStyle::Update()
{
  if (!this->ControlledProxy)
    {
    return false;
    }

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
      return true;
      }
    }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRControlSlicePositionStyle::HandleButton(const vtkVREventData& data)
{
  if (data.name.compare(this->ButtonName) == 0)
    {
    this->Enabled = data.data.button.state;
    }
}

// ----------------------------------------------------------------------------
void vtkVRControlSlicePositionStyle::HandleTracker( const vtkVREventData& data )
{
  if (data.name.compare(this->TrackerName) != 0)
    {
    return;
    }

  if (!this->Enabled)
    {
    this->InitialPositionRecorded = false;
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
        if (!this->InitialPositionRecorded)
          {
          this->RecordCurrentPosition(data);
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
                              this->ControlledPropertyName).Get(this->Origin,
                                                                3);
          this->Origin[3] = 1;
          this->InitialPositionRecorded = true;
          }
        else
          {
          double modelTransformMatrix[16];
          double origin[4];
          vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
                                    &this->InitialInvertedPose->Element[0][0],
                                    modelTransformMatrix);
          vtkMatrix4x4::MultiplyPoint(modelTransformMatrix, this->Origin,
                                      origin);
          vtkSMPropertyHelper(this->ControlledProxy,
                              this->ControlledPropertyName).Set(origin, 3);
          }

        // // Calculate the delta between the old rcorded value and new value
        // double deltaPos[3];
        // deltaPos[0] = data.data.tracker.matrix[3]  - InitialPos[0];
        // deltaPos[1] = data.data.tracker.matrix[7]  - InitialPos[1];
        // deltaPos[2] = data.data.tracker.matrix[11] - InitialPos[2];
        // this->RecordCurrentPosition(data);

        // double origin[3];
        // vtkSMPropertyHelper( this->Proxy, "Origin" ).Get( origin, 3 );
        // for ( int i=0;i<3;i++ )
        //   origin[i] += deltaPos[i];
        // vtkSMPropertyHelper( this->Proxy, "Origin" ).Set( origin, 3 );
        }
      }
    }
  else
    {
    // If the button is released then
    this->InitialPositionRecorded = false;
    }

}

// ----------------------------------------------------------------------------
void vtkVRControlSlicePositionStyle::RecordCurrentPosition(
    const vtkVREventData& data)
{
  this->InitialPos[0] = data.data.tracker.matrix[3];
  this->InitialPos[1] = data.data.tracker.matrix[7];
  this->InitialPos[2] = data.data.tracker.matrix[11];
}
