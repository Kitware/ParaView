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
#include "vtkVRPropertyStyle.h"

#include "vtkPVXMLElement.h"
#include "vtkSMProperty.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkVRQueue.h"
#include <iostream>
#include <sstream>
#include <algorithm>

//-----------------------------------------------------------------------------
vtkVRPropertyStyle::vtkVRPropertyStyle(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
vtkVRPropertyStyle::~vtkVRPropertyStyle()
{
}

//-----------------------------------------------------------------------------
bool vtkVRPropertyStyle::configure(vtkPVXMLElement* child,
  vtkSMProxyLocator* locator)
{
  if (!this->Superclass::configure(child, locator))
    {
    return false;
    }

  const char* proxy = child->GetAttributeOrEmpty("proxy");
  const char* property_name = child->GetAttributeOrEmpty("property");
  if (proxy && property_name)
    {
    this->setSMProperty(
      locator->LocateProxy(atoi(proxy)),
      property_name);
    return true;
    }
  return false;
}

//-----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRPropertyStyle::saveConfiguration() const
{
  vtkPVXMLElement* element = this->Superclass::saveConfiguration();
  if (element)
    {
    element->AddAttribute(
      "proxy", this->Proxy? this->Proxy->GetGlobalIDAsString() : "0");
    element->AddAttribute(
      "property", this->PropertyName.toAscii().data());
    }

  return element;
}

//-----------------------------------------------------------------------------
void vtkVRPropertyStyle::setSMProperty(
  vtkSMProxy* proxy, const QString& property_name)
{
  this->Proxy = proxy;
  this->PropertyName = property_name;
}

//-----------------------------------------------------------------------------
vtkSMProperty* vtkVRPropertyStyle::getSMProperty() const
{
  return this->Proxy? this->Proxy->GetProperty(
    this->PropertyName.toAscii().data()) : NULL;
}

//-----------------------------------------------------------------------------
vtkSMProxy* vtkVRPropertyStyle::getSMProxy() const
{
  return this->Proxy;
}

bool vtkVRPropertyStyle::handleEvent(const vtkVREventData& data)
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

bool vtkVRPropertyStyle::update()
{
}

void vtkVRPropertyStyle::HandleButton( const vtkVREventData& data )
{
  std::stringstream event;
  event << "BUTTON."<<data.name;
  if ( this->Map.find(event.str() )!= this->Map.end() )
    {
    SetButtonValue( this->Map[event.str()], data.data.button.state );
    }
}

void vtkVRPropertyStyle::HandleAnalog( const vtkVREventData& data )
{
  for (int i = 0; i < data.data.analog.num_channel; ++i)
    {
    std::stringstream event;
    event << "ANALOG."<<data.name<<"."<<i;
    if ( this->Map.find(event.str() )!= this->Map.end() )
      {
      SetAnalogValue( this->Map[event.str()], data.data.analog.channel[i] );
      }
    }

}

void vtkVRPropertyStyle::HandleTracker( const vtkVREventData& data )
{
  for (int i = 0; i < 16; ++i)
    {
    std::stringstream event;
    event << "TRACKER."<<data.name<<"."<<i;
    if ( this->Map.find(event.str() )!= this->Map.end() )
      {
      SetTrackerValue( this->Map[event.str()], data.data.tracker.matrix[i] );
      }
    }
}

void vtkVRPropertyStyle::SetButtonValue( std::string dest, int value )
{
  std::vector<std::string>token = this->tokenize( dest );
  if ( token.size()==2 || token.size()==3 )
    {
    std::cerr << "Expected \"value\" Format:  Proxy.Property[.index]" <<std::endl;
    }
}

void vtkVRPropertyStyle::SetAnalogValue( std::string dest, double value )
{
  std::vector<std::string>token = this->tokenize( dest );
  if ( token.size()==2 || token.size()==3 )
    {
    std::cerr << "Expected \"value\" Format:  Proxy.Property[.index]" << std::endl;
    }
}

void vtkVRPropertyStyle::SetTrackerValue( std::string dest, double value )
{
  std::vector<std::string>token = this->tokenize( dest );
  if ( token.size()==2 || token.size()==3 )
    {
    std::cerr  << "Expected \"value\" Format:  Proxy.Property[.index]" << std::endl;
    }
}

std::vector<std::string> vtkVRPropertyStyle::tokenize( std::string input)
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
