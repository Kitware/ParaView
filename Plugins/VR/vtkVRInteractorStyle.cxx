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
#include "vtkVRInteractorStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"

#include <sstream>
#include <algorithm>

// ----------------------------------------------------------------------------
vtkStandardNewMacro(vtkVRInteractorStyle)
vtkCxxSetObjectMacro(vtkVRInteractorStyle, ControlledProxy, vtkSMProxy)

// ----------------------------------------------------------------------------
vtkVRInteractorStyle::vtkVRInteractorStyle()
  : Superclass(), ControlledProxy(NULL), ControlledPropertyName(NULL),
    NeedsAnalog(false), NeedsButton(false), NeedsTracker(false),
    AnalogName(NULL), ButtonName(NULL), TrackerName(NULL)
{
}

// ----------------------------------------------------------------------------
vtkVRInteractorStyle::~vtkVRInteractorStyle()
{
  this->SetControlledProxy(NULL);
  this->SetControlledPropertyName(NULL);
  this->SetAnalogName(NULL);
  this->SetButtonName(NULL);
  this->SetTrackerName(NULL);
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::Configure(vtkPVXMLElement *child,
                                     vtkSMProxyLocator *locator)
{
  if (!child->GetName() || strcmp(child->GetName(),"Style") != 0 ||
      strcmp(this->GetClassName(), child->GetAttributeOrEmpty("class")) != 0)
    {
    return false;
    }

  int id;
  if (child->GetScalarAttribute("proxy", &id) == 0)
    {
    return false;
    }

  if (child->GetAttribute("property") == NULL)
    {
    return false;
    }


  for (unsigned int cc=0; cc < child->GetNumberOfNestedElements(); cc++)
    {
    vtkPVXMLElement* element = child->GetNestedElement(cc);
    if (element && element->GetName())
      {
      if (strcmp(element->GetName(), "Analog") == 0)
        {
        this->SetAnalogName(element->GetAttributeOrEmpty("name"));
        }
      else if (strcmp(element->GetName(), "Button") == 0)
        {
        this->SetButtonName(element->GetAttributeOrEmpty("name"));
        }
      else if (strcmp(element->GetName(), "Tracker") == 0)
        {
        this->SetTrackerName(element->GetAttributeOrEmpty("name"));
        }
      }
    }

  this->ControlledProxy = locator->LocateProxy(id);
  this->SetControlledPropertyName(child->GetAttribute("property"));

  return true;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRInteractorStyle::SaveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class",this->GetClassName());
  child->AddAttribute("proxy",
    this->ControlledProxy?
    this->ControlledProxy->GetGlobalIDAsString() : "0");
  if (this->ControlledPropertyName != NULL &&
      this->ControlledPropertyName[0] != '\0')
    {
    child->AddAttribute("property",
      this->ControlledPropertyName);
    }

  if (this->NeedsAnalog && this->AnalogName)
    {
    vtkPVXMLElement *analog = vtkPVXMLElement::New();
    analog->SetName("Analog");
    analog->AddAttribute("name", this->AnalogName);
    child->AddNestedElement(analog);
    analog->FastDelete();
    }
  if (this->NeedsButton && this->ButtonName)
    {
    vtkPVXMLElement *button = vtkPVXMLElement::New();
    button->SetName("Button");
    button->AddAttribute("name", this->ButtonName);
    child->AddNestedElement(button);
    button->FastDelete();
    }
  if (this->NeedsTracker && this->TrackerName)
    {
    vtkPVXMLElement* tracker = vtkPVXMLElement::New();
    tracker->SetName("Tracker");
    tracker->AddAttribute("name", this->TrackerName);
    child->AddNestedElement(tracker);
    tracker->FastDelete();
    }

  return child;
}

// ----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyle::Tokenize( std::string input)
{
  std::replace( input.begin(), input.end(), '.', ' ' );
  std::istringstream stm( input );
  std::vector<std::string> token;
  for (;;)
    {
    std::string word;
    if (!(stm >> word)) break;
    token.push_back(word);
    }
  return token;
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::HandleEvent(const vtkVREventData& data)
{
  switch( data.eventType )
    {
    case BUTTON_EVENT:
      this->HandleButton( data );
      break;
    case ANALOG_EVENT:
      this->HandleAnalog( data );
      break;
    case TRACKER_EVENT:
      this->HandleTracker( data );
      break;
    }
  return false;
}

// -----------------------------------------------------------------------------
bool vtkVRInteractorStyle::Update()
{
  return true;
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::HandleButton( const vtkVREventData& vtkNotUsed( data ) )
{
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::HandleAnalog( const vtkVREventData& vtkNotUsed( data ) )
{
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::HandleTracker( const vtkVREventData& vtkNotUsed( data ) )
{
}

// ----------------------------------------------------------------------------
void vtkVRInteractorStyle::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
  os << indent << "NeedsAnalog: " << this->NeedsAnalog << endl;
  os << indent << "AnalogName: " << this->AnalogName << endl;
  os << indent << "NeedsButton: " << this->NeedsButton << endl;
  os << indent << "ButtonName: " << this->ButtonName << endl;
  os << indent << "NeedsTracker: " << this->NeedsTracker << endl;
  os << indent << "TrackerName: " << this->TrackerName << endl;
}
