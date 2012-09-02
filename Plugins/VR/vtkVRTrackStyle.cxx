/*=========================================================================

   Program: ParaView
   Module:    vtkVRTrackStyle.cxx

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
#include "vtkVRTrackStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"

#include <sstream>
#include <algorithm>
#include <QtDebug>

// ----------------------------------------------------------------------------
vtkVRTrackStyle::vtkVRTrackStyle(QObject* parentObject) :
  Superclass(parentObject)
{
}

// ----------------------------------------------------------------------------
vtkVRTrackStyle::~vtkVRTrackStyle()
{
}

//-----------------------------------------------------------------------------
void vtkVRTrackStyle::setControlledProxy(vtkSMProxy* proxy)
{
  this->ControlledProxy = proxy;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkVRTrackStyle::controlledProxy() const
{
  return this->ControlledProxy;
}

//-----------------------------------------------------------------------------
bool vtkVRTrackStyle::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  int id;
  if (!this->Superclass::configure(child, locator))
    {
    return false;
    }

  if (child->GetScalarAttribute("proxy", &id) == 0)
    {
    return false;
    }

   if (child->GetAttribute("property") == NULL)
    {
    return false;
    }

  this->ControlledProxy = locator->LocateProxy(id);
  this->ControlledPropertyName = child->GetAttribute("property");

  for (unsigned int cc=0; cc < child->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* tracker = child->GetNestedElement(cc);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Tracker")==0)
      {
      this->TrackerName = tracker->GetAttributeOrEmpty("name");
      }
    }

  if (this->TrackerName.isEmpty() || !this->ControlledProxy ||
    this->ControlledPropertyName.isEmpty())
    {
    qCritical() << "Incorrect state for vtkVRTrackStyle";
    return false;
    }

  return true;
}

// -----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRTrackStyle::saveConfiguration() const
{
  vtkPVXMLElement* child = this->Superclass::saveConfiguration();
  child->AddAttribute("proxy",
    this->ControlledProxy?
    this->ControlledProxy->GetGlobalIDAsString() : "0");
  if (!this->ControlledPropertyName.isEmpty())
    {
    child->AddAttribute("property",
      this->ControlledPropertyName.toAscii().data());
    }
  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name", this->TrackerName.toAscii().data() );
  child->AddNestedElement(tracker);
  tracker->FastDelete();
  return child;
}

// ----------------------------------------------------------------------------
void vtkVRTrackStyle::handleTracker( const vtkVREventData& data )
{
  if ( this->TrackerName == QString(data.name.c_str()))
    {
    if (this->ControlledProxy && !this->ControlledPropertyName.isEmpty())
      {
      vtkSMPropertyHelper(this->ControlledProxy,
        this->ControlledPropertyName.toAscii().data()).Set(
        data.data.tracker.matrix, 16);
      this->ControlledProxy->UpdateVTKObjects();
      }
    }
}
