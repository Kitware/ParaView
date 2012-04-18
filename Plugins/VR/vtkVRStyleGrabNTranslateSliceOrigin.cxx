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
#include "vtkVRStyleGrabNTranslateSliceOrigin.h"

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
vtkVRStyleGrabNTranslateSliceOrigin::vtkVRStyleGrabNTranslateSliceOrigin(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Enabled = false;
  this->Old = vtkTransform::New();
  this->Tx =  vtkTransform::New();
  this->Neo = vtkTransform::New();
  this->InitialInvertedPose = vtkMatrix4x4::New();
}

// ----------------------------------------------------------------------------
vtkVRStyleGrabNTranslateSliceOrigin::~vtkVRStyleGrabNTranslateSliceOrigin()
{
  // Delete the assigned matix
  this->InitialInvertedPose->Delete();
  Neo->Delete();
  Tx->Delete();
  Old->Delete();
}

// ----------------------------------------------------------------------------
bool vtkVRStyleGrabNTranslateSliceOrigin::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* vtkNotUsed( locator ))
{
  if (child->GetName() && strcmp(child->GetName(),"Style") == 0 &&
    strcmp(this->metaObject()->className(),
      child->GetAttributeOrEmpty("class")) == 0)
    {
    this->OriginPropStr = child->GetAttributeOrEmpty( "origin" );
    if ( !this->OriginPropStr.size() )
      {
      std::cerr << "vtkVRStyleGrabNTranslateSliceOrigin::configure(): "
                << "Origin property not specified " << std::endl
                << "<Style class=\"vtkVRStyleGrabNTranslateSliceOrigin\" origin=\"buttonEventName\"/>" << std::endl
                << std::endl;
      return false;
      }
    std::vector<std::string> token = this->tokenize( this->OriginPropStr );
    if ( token.size()!=2 )
      {
      std::cerr << "Expected \"origin\" Format:  Proxy.Property" << std::endl;
      }
    this->Proxy = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetProxy( token[0].c_str() );
    if( this->Proxy )
      {
      this->Property = vtkSMDoubleVectorProperty::SafeDownCast( this->Proxy->GetProperty( token[1].c_str()) );
      if ( !this->Property )
        {
        std::cout<< "Property ( " << token[1] << ") :Not Found" <<std::endl;
        return false;
        }
      }
    else
      {
      std::cout<< "Proxy ( " << token[0] << ") :Not Found" << std::endl;
      return false;
      }

    if ( child->GetNumberOfNestedElements() !=2 )
      {
      std::cerr << "vtkVRStyleGrabNTranslateSliceOrigin::configure(): "
                << "There has to be only 2 elements present " << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl
                << "<Tracker name=\"trackerEventName\"/>"
                << std::endl;
      return false;
      }
    vtkPVXMLElement* button = child->GetNestedElement(0);
    if (button && button->GetName() && strcmp(button->GetName(), "Button")==0)
      {
      this->Button = button->GetAttributeOrEmpty("name");
      }
    else
      {
      std::cerr << "vtkVRStyleGrabNTranslateSliceOrigin::configure(): "
                << "Button event has to be specified" << std::endl
                << "<Button name=\"buttonEventName\"/>"
                << std::endl;
      return false;
      }
    vtkPVXMLElement* tracker = child->GetNestedElement(1);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Tracker")==0)
      {
      this->Tracker = tracker->GetAttributeOrEmpty("name");
      }
    else
      {
      std::cerr << "vtkVRStyleGrabNTranslateSliceOrigin::configure(): "
                << "Please Specify Tracker event" <<std::endl
                << "<Tracker name=\"TrackerEventName\"/>"
                << std::endl;
      return false;
      }
    return true;
    }
  return false;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRStyleGrabNTranslateSliceOrigin::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName( "Style" );
  child->AddAttribute("class", this->metaObject()->className());
  child->AddAttribute( "origin", this->OriginPropStr.c_str() );

  vtkPVXMLElement* button = vtkPVXMLElement::New();
  button->SetName("Button");
  button->AddAttribute("name", this->Button.c_str() );
  child->AddNestedElement(button);
  button->FastDelete();

  vtkPVXMLElement* tracker = vtkPVXMLElement::New();
  tracker->SetName("Tracker");
  tracker->AddAttribute("name",this->Tracker.c_str() );
  child->AddNestedElement(tracker);
  tracker->FastDelete();

  return child;
}

// ----------------------------------------------------------------------------
bool vtkVRStyleGrabNTranslateSliceOrigin::handleEvent(const vtkVREventData& data)
{
  switch( data.eventType )
    {
    case BUTTON_EVENT:
      if ( this->Button == data.name )
        {
        this->HandleButton( data );
        }
      break;
    case ANALOG_EVENT:
      this->HandleAnalog( data );
      break;
    case TRACKER_EVENT:
      if ( this->Tracker == data.name )
        {
        this->HandleTracker( data );
        }
      break;
    }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRStyleGrabNTranslateSliceOrigin::update()
{
  pqView *view = 0;
  vtkSMRenderViewProxy *proxy =0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( proxy )
      {
      this->Proxy->UpdateVTKObjects();
      proxy->UpdateVTKObjects();
      }
    }
  return false;
}

// ----------------------------------------------------------------------------
void vtkVRStyleGrabNTranslateSliceOrigin::HandleButton( const vtkVREventData& data )
{
    this->Enabled = data.data.button.state;
}

// ----------------------------------------------------------------------------
void vtkVRStyleGrabNTranslateSliceOrigin::HandleTracker( const vtkVREventData& data )
{
  // If the button is pressed then record the current position is tracking space
  if ( this->Enabled )
    {
    vtkSMRenderViewProxy *proxy =0;
    vtkSMDoubleVectorProperty *prop =0;

    pqView *view = 0;
    view = pqActiveObjects::instance().activeView();
    if ( view )
      {
      proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
      if ( proxy )
        {
        prop =
          vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "WandPose" ) );
        if ( prop )
          {
          if ( !this->InitialPositionRecorded )
            {
            this->RecordCurrentPosition( data );
            // Copy the data into matrix
            for (int i = 0; i < 4; ++i)
              {
              for (int j = 0; j < 4; ++j)
                {
                this->InitialInvertedPose->SetElement( i,j,data.data.tracker.matrix[i*4+j] );
                }
              }
            // invert the matrix
            vtkMatrix4x4::Invert( this->InitialInvertedPose, this->InitialInvertedPose );
            double wandPose[16];
            vtkSMPropertyHelper( proxy, "WandPose" ).Get( wandPose, 16 );
            vtkMatrix4x4::Multiply4x4(&this->InitialInvertedPose->Element[0][0],
                                      wandPose,
                                      &this->InitialInvertedPose->Element[0][0]);
             vtkSMPropertyHelper( this->Proxy, "Origin" ).Get(this->Origin, 3 );
             this->Origin[3] =1;
            this->InitialPositionRecorded = true;
            }
          else
            {
            double wandPose[16];
            double origin[4];
            vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
                                      &this->InitialInvertedPose->Element[0][0],
                                      wandPose);
            vtkMatrix4x4::MultiplyPoint( wandPose, this->Origin, origin );
            vtkSMPropertyHelper( this->Proxy, "Origin" ).Set( origin, 3 );
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
    }
  else
    {
    // If the button is released then
    this->InitialPositionRecorded = false;
    }
}

// ----------------------------------------------------------------------------
std::vector<std::string> vtkVRStyleGrabNTranslateSliceOrigin::tokenize( std::string input)
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
void vtkVRStyleGrabNTranslateSliceOrigin::RecordCurrentPosition(const vtkVREventData& data)
{
  this->InitialPos[0] = data.data.tracker.matrix[3];
  this->InitialPos[1] = data.data.tracker.matrix[7];
  this->InitialPos[2] = data.data.tracker.matrix[11];
}
