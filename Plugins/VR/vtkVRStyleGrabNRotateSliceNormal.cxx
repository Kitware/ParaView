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
#include "vtkVRStyleGrabNRotateSliceNormal.h"

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
#include "vtkSMPropertyHelper.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

//------------------------------------------------------------------------cnstr
vtkVRStyleGrabNRotateSliceNormal::vtkVRStyleGrabNRotateSliceNormal(QObject* parentObject)
  : Superclass(parentObject)
{
  this->Enabled = false;
  this->InitialOrientationRecored = false;
  this->InitialInvertedPose = vtkMatrix4x4::New();
}

//------------------------------------------------------------------------destr
vtkVRStyleGrabNRotateSliceNormal::~vtkVRStyleGrabNRotateSliceNormal()
{
  this->InitialInvertedPose->Delete();
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateSliceNormal::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if (child->GetName() && strcmp(child->GetName(),"Style") == 0 &&
    strcmp(this->metaObject()->className(),
      child->GetAttributeOrEmpty("class")) == 0)
    {
    this->NormalPropStr = child->GetAttributeOrEmpty( "normal" );
    if ( !this->NormalPropStr.size() )
      {
      std::cerr << "vtkVRStyleGrabNRotateSliceNormal::configure(): "
                << "Origin property not specified " << std::endl
                << "<Style class=\"vtkVRStyleGrabNRotateSliceNormal\" normal=\"buttonEventName\"/>" << std::endl
                << std::endl;
      return false;
      }

    std::vector<std::string> token = this->tokenize( this->NormalPropStr );
    if ( token.size()!=2 )
      {
      std::cerr << "Expected \"origin\" Format:  Proxy.Property" << std::endl;
      }
    this->Proxy = vtkSMProxyManager::GetProxyManager()->GetProxy( token[0].c_str() );
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
      std::cerr << "vtkVRStyleGrabNRotateSliceNormal::configure(): "
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
      std::cerr << "vtkVRStyleGrabNRotateSliceNormal::configure(): "
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
      std::cerr << "vtkVRStyleGrabNRotateSliceNormal::configure(): "
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
vtkPVXMLElement* vtkVRStyleGrabNRotateSliceNormal::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName( "Style" );
  child->AddAttribute("class", this->metaObject()->className());
  child->AddAttribute( "normal", this->NormalPropStr.c_str() );

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

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateSliceNormal::handleEvent(const vtkVREventData& data)
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

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateSliceNormal::update()
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
      proxy->StillRender();
      }
    }
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateSliceNormal::HandleButton( const vtkVREventData& data )
{
  std::cout << data.name << std::endl;
  this->Enabled = data.data.button.state;
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateSliceNormal::HandleAnalog( const vtkVREventData& data )
{
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateSliceNormal::HandleTracker( const vtkVREventData& data )
{
  if ( this->Enabled )
    {
    std::cout << "its time to rotate " << std::endl;
    vtkSMRenderViewProxy *proxy =0;
    vtkSMDoubleVectorProperty *prop =0;
    // property->SetElement( atoi(token[2].c_str() ), value );
    // proxy->UpdateVTKObjects();

    pqView *view = 0;
    view = pqActiveObjects::instance().activeView();
    if ( view )
      {
      proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
      if ( proxy )
        {
        prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "WandPose" ) );
        if ( prop )
          {
          if( !this->InitialOrientationRecored)
            {
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
            this->InitialOrientationRecored = true;
            }
          else
            {
            double wandPose[16];
            vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
                                      &this->InitialInvertedPose->Element[0][0],
                                      wandPose);
            double normal[4];
            vtkSMPropertyHelper( this->Proxy, "Normal" ).Get(normal, 3 );
            normal[3] =0;
            vtkMatrix4x4::MultiplyPoint( wandPose, normal, normal );
            vtkSMPropertyHelper( this->Proxy, "Normal" ).Set(normal, 3 );
            //vtkSMPropertyHelper(proxy,"WandPose").Set(wandPose,16);
            }
          }
        }
      }
    }
  else
    {
    this->InitialOrientationRecored = false;
    }
}

//----------------------------------------------------------------------private
std::vector<std::string> vtkVRStyleGrabNRotateSliceNormal::tokenize( std::string input)
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

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateSliceNormal::RecordOrientation(vtkSMRenderViewProxy* proxy,  const vtkVREventData& data)
{
  double mat[3][3];

  // Collect the initial rotation matrix
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      mat[i][j] = data.data.tracker.matrix[i*4+j];
      }
    }

  vtkMath::Matrix3x3ToQuaternion( mat, this->InitialTrackerQuat );

  // Collect the current rotation matrix
  double old[16];
  double oldMat[3][3];
  vtkSMPropertyHelper( proxy, "WandPose" ).Get( &old[0], 16 );
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      oldMat[i][j] = old[i*4+j];
      }
    }
  // Convert rotation matrix to quaternion
  vtkMath::Matrix3x3ToQuaternion( oldMat, this->InitialQuat );
}

//----------------------------------------------------------------------private
void vtkVRStyleGrabNRotateSliceNormal::UpdateOrientation(const vtkVREventData& data)
{
  double mat[3][3];
  double quat[4];
  double deltaQuat[4];
  // Collect the initial rotation matrix
  for (int i = 0; i < 3; ++i)
    {
    for (int j = 0; j < 3; ++j)
      {
      mat[i][j] = data.data.tracker.matrix[i*4+j];
      }
    }

  // Make quaternion
  vtkMath::Matrix3x3ToQuaternion( mat, quat );

  // Get the delta rotation
  deltaQuat[0] = quat[0] - this->InitialTrackerQuat[0];
  deltaQuat[1] = quat[1] - this->InitialTrackerQuat[1];
  deltaQuat[2] = quat[2] - this->InitialTrackerQuat[2];
  deltaQuat[3] = quat[3] - this->InitialTrackerQuat[3];

  // if ( fabs( deltaQuat[0] ) > 0 &&
  //      ( fabs( deltaQuat[1] ) > 0 ||
  //      fabs( deltaQuat[2] ) > 0 ||
  //      fabs( deltaQuat[3] ) > 0 ) )
  //   {
  // Multiply new quaternion into inital quaternion

    // double mag = fabs( sqrt ( deltaQuat[0]*deltaQuat[0] +
    //                        deltaQuat[1]*deltaQuat[1] +
    //                        deltaQuat[2]*deltaQuat[2] +
    //                        deltaQuat[3]*deltaQuat[3] ) );
    // if ( mag > 0 )
    // deltaQuat[0] = deltaQuat[0]/mag;
    // deltaQuat[1] = deltaQuat[1]/mag;
    // deltaQuat[2] = deltaQuat[2]/mag;
    // deltaQuat[3] = deltaQuat[3]/mag;
    vtkMath::MultiplyQuaternion( deltaQuat,  this->InitialQuat,  this->UpdatedQuat );
    //vtkMath::MultiplyQuaternion( quat,  this->InitialQuat,  this->UpdatedQuat );
    // std::cout << "deltaQuat : ["
    //           << deltaQuat[0] << " "
    //           << deltaQuat[1] << " "
    //           << deltaQuat[2] << " "
    //           << deltaQuat[3] << "]" << std::endl;
    // this->InitialTrackerQuat[0] = quat[0];
    // this->InitialTrackerQuat[1] = quat[1];
    // this->InitialTrackerQuat[2] = quat[2];
    // this->InitialTrackerQuat[3] = quat[3];
  // }
}
