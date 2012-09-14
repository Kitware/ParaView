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

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRTrackStyle)
vtkCxxSetObjectMacro(vtkVRTrackStyle, ControlledProxy, vtkSMProxy)

// ----------------------------------------------------------------------------
vtkVRTrackStyle::vtkVRTrackStyle() :
  Superclass()
{
  this->TrackerName = NULL;
  this->ControlledProxy = NULL;
  this->ControlledPropertyName = NULL;
}

// ----------------------------------------------------------------------------
vtkVRTrackStyle::~vtkVRTrackStyle()
{
  this->SetTrackerName(NULL);
  this->SetControlledProxy(NULL);
  this->SetControlledPropertyName(NULL);
}

//-----------------------------------------------------------------------------
bool vtkVRTrackStyle::Configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  int id;
  if (!this->Superclass::Configure(child, locator))
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
  this->SetControlledPropertyName(child->GetAttribute("property"));

  for (unsigned int cc=0; cc < child->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* tracker = child->GetNestedElement(cc);
    if (tracker && tracker->GetName() &&
        strcmp(tracker->GetName(), "Tracker") == 0)
      {
      this->SetTrackerName(tracker->GetAttributeOrEmpty("name"));
      }
    }

  if (this->TrackerName == NULL || this->TrackerName[0] == '\0' ||
      this->ControlledPropertyName == NULL ||
      this->ControlledPropertyName[0] == '\0' ||
      !this->ControlledProxy)
    {
    vtkErrorMacro(<< "Incorrect state for vtkVRTrackStyle");
    return false;
    }

  return true;
}

// -----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRTrackStyle::SaveConfiguration() const
{
  vtkPVXMLElement* child = this->Superclass::SaveConfiguration();
  child->AddAttribute("proxy",
    this->ControlledProxy?
    this->ControlledProxy->GetGlobalIDAsString() : "0");
  if (this->ControlledPropertyName != NULL &&
      this->ControlledPropertyName[0] != '\0')
    {
    child->AddAttribute("property",
      this->ControlledPropertyName);
    }
  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name", this->TrackerName);
  child->AddNestedElement(tracker);
  tracker->FastDelete();
  return child;
}

// ----------------------------------------------------------------------------
void vtkVRTrackStyle::HandleTracker( const vtkVREventData& data )
{
  if (data.name.size() > 0)
    {
    this->SetTrackerName(data.name.c_str());
    if (this->ControlledProxy && this->ControlledPropertyName != NULL &&
        this->ControlledPropertyName[0] != '\0')
      {
      vtkSMPropertyHelper(this->ControlledProxy,
                          this->ControlledPropertyName).Set(
            data.data.tracker.matrix, 16);
      this->ControlledProxy->UpdateVTKObjects();
      }
    }
}

void vtkVRTrackStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "TrackerName: " << this->TrackerName << endl;
  os << indent << "ControlledPropertyName: "
     << this->ControlledPropertyName << endl;
  if (this->ControlledProxy)
    {
    os << indent << "ControlledProxy:" << endl;
    this->ControlledProxy->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << indent << "ControlledProxy: (None)" << endl;
    }
}
