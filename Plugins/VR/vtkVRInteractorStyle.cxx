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
#include "vtkSMProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkVRQueue.h"
#include <sstream>
#include <algorithm>

// ----------------------------------------------------------------------------
vtkVRInteractorStyle::vtkVRInteractorStyle(QObject* parentObject)
  : Superclass(parentObject)
{
  this->OutProxy =0;
  this->OutProperty=0;
  this->IsFoundOutProxyProperty=false;
}

// ----------------------------------------------------------------------------
vtkVRInteractorStyle::~vtkVRInteractorStyle()
{
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::configure(vtkPVXMLElement* child, vtkSMProxyLocator*)
{
  if (child->GetName() && strcmp(child->GetName(),"Style") == 0 &&
      strcmp(this->metaObject()->className(),
      child->GetAttributeOrEmpty("class")) == 0)
    {
    std::string outStr = child->GetAttributeOrEmpty( "set_property" );
    if ( !outStr.size() )
      {
      std::cerr << "vtkVRStyleGrabNTranslateSliceOrigin::configure(): "
                << "set_property not specified "
                << std::endl
                << "<Style class=\""
                << this->metaObject()->className()
                << "\" "
                << "set_property=\"ProxyName.PropertyName\"/>"
                << std::endl;
      return false;
      }
    std::vector<std::string> token = this->tokenize( outStr );
    if ( token.size()!=2 )
      {
      std::cerr << "Expected \"set_property\" Format:  Proxy.Property" << std::endl;
      return false;
      }
    else
      {
      this->OutProxyName = token[0];
      this->OutPropertyName = token[1];
      this->IsFoundOutProxyProperty = GetOutProxyNProperty();
      return this->IsFoundOutProxyProperty;
      }
    return true;
    }
  return false;
}

// ----------------------------------------------------------------------------
vtkPVXMLElement* vtkVRInteractorStyle::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class",this->metaObject()->className());
  std::stringstream propertyStr;
  propertyStr << this->OutProxyName << "." << this->OutPropertyName;
  child->AddAttribute( "set_property",propertyStr.str().c_str() );

  return child;
}

// ----------------------------------------------------------------------------
std::vector<std::string> vtkVRInteractorStyle::tokenize( std::string input)
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
bool vtkVRInteractorStyle::GetOutProxyNProperty()
{
  if(this->GetProxy( this->OutProxyName, &this->OutProxy ) )
    {
    if ( !this->GetProperty( this->OutProxy, this->OutPropertyName, &this->OutProperty ) )
      {
      std::cerr << this->metaObject()->className() << "::GetOutProxyNProperty"
                << std::endl
                << "Property ( " << this->OutPropertyName << ") :Not Found"
                <<std::endl;
      return false;
      }
    }
  else
    {
    std::cerr << this->metaObject()->className() << "::GetOutProxyNProperty"
              << std::endl
              << "Proxy ( " << this->OutProxyName << ") :Not Found" << std::endl;
    return false;
    }
  return true;
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::handleEvent(const vtkVREventData& data)
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
bool vtkVRInteractorStyle::update()
{
  if(this->OutProxy)
    {
    this->OutProxy->UpdateVTKObjects();
    }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::GetProxy( std::string name, vtkSMProxy ** proxy )
{
  vtkSMProxy *p =0;
  p = vtkSMProxyManager::GetProxyManager()->GetActiveSessionProxyManager()->GetProxy( name.c_str() );
  if( p )
    {
    *proxy = p;
    return true;
    }
  return false;
}

// ----------------------------------------------------------------------------
bool vtkVRInteractorStyle::GetProperty( vtkSMProxy* proxy,
                                        std::string name,
                                        vtkSMDoubleVectorProperty** prop )
{
  vtkSMDoubleVectorProperty *p =0;
  p =  vtkSMDoubleVectorProperty::
      SafeDownCast( proxy->GetProperty( name.c_str()) );
  if ( p )
    {
    *prop = p;
    return true;
    }
  return false;
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
