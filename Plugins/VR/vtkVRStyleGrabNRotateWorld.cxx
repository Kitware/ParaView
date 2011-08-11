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

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include "pqView.h"
#include "pqActiveObjects.h"
#include "vtkSMRenderViewProxy.h"

//------------------------------------------------------------------------cnstr
vtkVRStyleGrabNRotateWorld::vtkVRStyleGrabNRotateWorld(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Enabled = false;
}

//------------------------------------------------------------------------destr
vtkVRStyleGrabNRotateWorld::~vtkVRStyleGrabNRotateWorld()
{
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if (child->GetName() && strcmp(child->GetName(),"Style") == 0 &&
    strcmp(this->metaObject()->className(),
      child->GetAttributeOrEmpty("class")) == 0)
    {
    if ( child->GetNumberOfNestedElements() !=2 )
      {
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
                << "There has to be only 2 elements present " << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl
                << "<Tracker name=\"trackerEventName\"/>"
                << std::endl;
      }
    vtkPVXMLElement* button = child->GetNestedElement(0);
    if (button && button->GetName() && strcmp(button->GetName(), "Button")==0)
      {
      this->Button = button->GetAttributeOrEmpty("name");
      }
    else
      {
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
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
      std::cerr << "vtkVRStyleGrabNRotateWorld::configure(): "
                << "Please Specify Tracker event" <<std::endl
                << "<Tracker name=\"TrackerEventName\"/>"
                << std::endl;
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
  child->SetName( "Style" );
  child->AddAttribute("class", this->metaObject()->className());

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
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::handleEvent(const vtkVREventData& data)
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

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateWorld::update()
{
  pqView *view = 0;
  vtkSMRenderViewProxy *proxy =0;
  view = pqActiveObjects::instance().activeView();
  if ( view )
    {
    proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
    if ( proxy )
      {
      proxy->UpdateVTKObjects();
      proxy->StillRender();
      }
    }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleButton( const vtkVREventData& data )
{
  if ( this->Button == data.name )
    {
    std::cout << data.name << std::endl;
    this->Enabled = data.data.button.state;
    }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleAnalog( const vtkVREventData& data )
{
  // std::vector<std::string>token = this->tokenize(data.name );

  // // check for events of types device.name (vector events)
  // std::stringstream event;
  // event << token[0]<<"."<<token[1];
  // if ( this->Map.find(event.str() )!= this->Map.end() )
  //   {
  //   std::cout << event.str() << std::endl;
  //   SetAnalogVectorValue(this->Map[event.str()],
  //                         data.data.analog.channel,
  //                         data.data.analog.num_channel );
  //   }

  // // check for events of type device.name.index (scalar events)
  // for (int i = 0; i < data.data.analog.num_channel; ++i)
  //   {
  //   std::stringstream event;
  //   event << data.name<<"."<<i;
  //   if ( this->Map.find(event.str() )!= this->Map.end() )
  //     {
  //     std::cout << event.str() <<" = "<< data.data.analog.channel[i] << std::endl;
  //     SetAnalogValue( this->Map[event.str()], data.data.analog.channel[i] );
  //     }
  //   }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::HandleTracker( const vtkVREventData& data )
{
  if ( this->Enabled && ( this->Tracker == data.name ))
    {
    std::cout << "its time to rotate " << std::endl;
    }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::SetButtonValue( std::string dest, int value )
{
  // std::vector<std::string>token = this->tokenize( dest );
  // if ( token.size()==2 || token.size()==3 )
  //   {
  //   std::cerr << "Expected \"set_value\" Format:  Proxy.Property[.index]" <<std::endl;
  //   }
  // vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->GetProxy( token[0].c_str() );
  //  if( proxy )
  //   {
  //   property =  proxy->GetProperty( token[1].c_str());
  //   if ( property )
  //     {
  //     property->SetElement( value );
  //     proxy->UpdateVTKObjects();
  //     }
  //   else
  //     {
  //     std::cout<< "Property ( " << token[1] << ") :Not Found" <<std::endl;
  //     return;
  //     }
  //   }
  // else
  //   {
  //   std::cout<< "Proxy ( " << token[1] << ") :Not Found" << std::endl;
  //   return;
  //   }
  // vtkSMProperty *property = proxy->GetProperty( token[0].c_str());
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::SetAnalogValue( std::string dest, double value )
{
  // std::vector<std::string>token = this->tokenize( dest );
  // if ( token.size()!=3 )
  //   {
  //   std::cerr << "Expected \"set_value\" Format:  Proxy.Property.index" << std::endl;
  //   }
  // vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->GetProxy( token[0].c_str() );
  // vtkSMDoubleVectorProperty* property;
  // if( proxy )
  //   {
  //   property = vtkSMDoubleVectorProperty::SafeDownCast( proxy->GetProperty( token[1].c_str()) );
  //   if ( property )
  //     {
  //     property->SetElement( atoi(token[2].c_str() ), value );
  //     proxy->UpdateVTKObjects();
  //     }
  //   else
  //     {
  //     std::cout<< "Property ( " << token[1] << ") :Not Found" <<std::endl;
  //     return;
  //     }
  //   }
  // else
  //   {
  //   std::cout<< "Proxy ( " << token[1] << ") :Not Found" << std::endl;
  //   return;
  //   }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::SetAnalogVectorValue( std::string dest,
                                               const double* value,
                                               unsigned int total)
{
  // std::vector<std::string>token = this->tokenize( dest );
  // if ( token.size()!=2)
  //   {
  //   std::cerr  << "Expected \"set_value\" Format:  Proxy.Property" << std::endl;
  //   }

  // vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->GetProxy( token[0].c_str() );
  // vtkSMProperty* property;
  // if( proxy )
  //   {
  //   property = proxy->GetProperty( token[1].c_str());
  //   }
  // else
  //   {
  //   return;
  //   }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::SetTrackerValue( std::string dest, double value )
{
  // std::vector<std::string>token = this->tokenize( dest );
  // if ( token.size()==2 || token.size()==3 )
  //   {
  //   std::cerr  << "Expected \"set_value\" Format:  Proxy.Property[.index]" << std::endl;
  //   }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateWorld::SetTrackerVectorValue( std::string dest,
                                                const double value[16] )
{
  // std::vector<std::string>token = this->tokenize( dest );
  // if ( token.size()==2 || token.size()==3 )
  //   {
  //   std::cerr  << "Expected \"set_value\" Format:  Proxy.Property[.index]" << std::endl;
  //   }

  // vtkSMProxy* proxy = vtkSMProxyManager::GetProxyManager()->GetProxy( token[0].c_str() );
  // vtkSMProperty* property;
  // if( proxy )
  //   {
  //   property = proxy->GetProperty( token[1].c_str());
  //   }
  // else
  //   {
  //   return;
  //   }
}

//----------------------------------------------------------------------private
std::vector<std::string> vtkVRStyleGrabNRotateWorld::tokenize( std::string input)
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
