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

#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkPVXMLElement.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"

#include <sstream>
#include <algorithm>

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::vtkVRGrabWorldStyle(QObject* parentObject) :
  Superclass(parentObject)
{
  this->Enabled = false;
  this->IsInitialRecorded =false;
  this->InverseInitialMatrix = vtkTransform::New();
}

// -----------------------------------------------------------------------------
vtkVRGrabWorldStyle::~vtkVRGrabWorldStyle()
{
  this->InverseInitialMatrix->Delete();
}

// ----------------------------------------------------------------------------
bool vtkVRGrabWorldStyle::configure(
  vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (!this->Superclass::configure(child, locator))
    {
    return false;
    }

  for (unsigned int cc=0; cc < child->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* button = child->GetNestedElement(cc);
    if (button && button->GetName() && strcmp(button->GetName(), "Button")==0)
      {
      this->ButtonName = button->GetAttributeOrEmpty("name");
      }
    }

  return !this->ButtonName.isEmpty();
}

// -----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRGrabWorldStyle::saveConfiguration() const
{
  vtkPVXMLElement* child = this->Superclass::saveConfiguration();

  vtkPVXMLElement* button = vtkPVXMLElement::New();
  button->SetName("Button");
  button->AddAttribute("name", this->ButtonName.toAscii().data() );
  child->AddNestedElement(button);
  button->FastDelete();
  return child;
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::handleButton( const vtkVREventData& data )
{
  if ( this->ButtonName == QString(data.name.c_str()))
    {
    this->Enabled = data.data.button.state;
    }
}

// ----------------------------------------------------------------------------
void vtkVRGrabWorldStyle::handleTracker( const vtkVREventData& data )
{
  if ( this->TrackerName == data.name.c_str() )
    {
    if ( this->Enabled )
      {
      if ( !this->IsInitialRecorded )
        {
        this->InverseInitialMatrix->SetMatrix( data.data.tracker.matrix );
        this->InverseInitialMatrix->Inverse();
        this->IsInitialRecorded = true;
        }
      else
        {
        vtkNew<vtkTransform> currentTransform;
        currentTransform->SetMatrix(data.data.tracker.matrix );
        currentTransform->Concatenate(this->InverseInitialMatrix);

        double currentValue[16];
        vtkSMPropertyHelper(
          this->ControlledProxy,
          this->ControlledPropertyName.toAscii().data()).Get(currentValue,
          16);
        vtkNew<vtkTransform> currentValueTransform;
        currentValueTransform->SetMatrix(currentValue);
        currentTransform->Concatenate(currentValueTransform.GetPointer());

        vtkSMPropertyHelper(
          this->ControlledProxy,
          this->ControlledPropertyName.toAscii().data()).Set(
          &currentTransform->GetMatrix()->Element[0][0], 16);
        this->ControlledProxy->UpdateVTKObjects();
        }
      }
    else
      {
      this->IsInitialRecorded = false;
      }
    }
}
