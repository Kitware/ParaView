/*=========================================================================

   Program: ParaView
   Module:    vtkVRStyleTracking.cxx

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
#include "vtkVRStyleTracking.h"

#include "pqActiveObjects.h"
#include "pqDataRepresentation.h"
#include "pqView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMRepresentationProxy.h"
#include "vtkPVXMLElement.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"
#include <sstream>
#include <algorithm>

// ----------------------------------------------------------------------------
vtkVRStyleTracking::vtkVRStyleTracking(QObject* parentObject) :
  Superclass(parentObject)
{
  this->OutPose = vtkTransform::New();
}

// ----------------------------------------------------------------------------
vtkVRStyleTracking::~vtkVRStyleTracking()
{
  this->OutPose->Delete();
}

//-----------------------------------------------------------------------------
bool vtkVRStyleTracking::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if ( Superclass::configure( child,locator ) )
    {
    vtkPVXMLElement* tracker = child->GetNestedElement(0);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Tracker")==0)
      {
      this->Tracker = tracker->GetAttributeOrEmpty("name");
      }
    else
      {
      std::cerr << "vtkVRStyleTracking::configure(): "
                << "Please Specify Tracker event" <<std::endl
                << "<Tracker name=\"TrackerEventName\"/>"
                << std::endl;
      return false;
      }
    return true;
    }
  return false;
}

// -----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRStyleTracking::saveConfiguration() const
{
  vtkPVXMLElement* child = Superclass::saveConfiguration();
  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name",this->Tracker.c_str() );
  child->AddNestedElement(tracker);
  tracker->FastDelete();
  return child;
}

// ----------------------------------------------------------------------------
void vtkVRStyleTracking::HandleTracker( const vtkVREventData& data )
{
  if ( this->Tracker == data.name)
    {
    this->OutPose->SetMatrix( data.data.tracker.matrix );
    this->SetProperty( );
    }
}

// ----------------------------------------------------------------------------
void vtkVRStyleTracking::SetProperty()
{
  vtkSMPropertyHelper(this->OutProxy,this->OutPropertyName.c_str())
    .Set(&this->OutPose->GetMatrix()->Element[0][0],16 );
}

// ----------------------------------------------------------------------------
bool vtkVRStyleTracking::update()
{
  Superclass::update();
  return false;
}
