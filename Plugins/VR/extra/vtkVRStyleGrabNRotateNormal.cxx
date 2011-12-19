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
#include "vtkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"

//------------------------------------------------------------------------cnstr
vtkVRStyleGrabNRotateSliceNormal::vtkVRStyleGrabNRotateSliceNormal(QObject* parentObject)
  : Superclass(parentObject)
{
}

//------------------------------------------------------------------------destr
vtkVRStyleGrabNRotateSliceNormal::~vtkVRStyleGrabNRotateSliceNormal()
{
}

//-----------------------------------------------------------------------public
bool vtkVRStyleGrabNRotateSliceNormal::configure(vtkPVXMLElement* child,
                                   vtkSMProxyLocator* locator)
{
  if ( Superclass::configure( child,locator ) )
    {
    vtkPVXMLElement* property = child->GetNestedElement(3);
    if (property && property->GetName() && strcmp(property->GetName(), "NormalProperty")==0)
      {
      std::string propertyStr = property->GetAttributeOrEmpty("name");
      std::vector<std::string> token = this->tokenize( propertyStr );
      if ( token.size()!=2 )
        {
        std::cerr << "Expected Property \"name\" Format:  Proxy.Property" << std::endl;
        return false;
        }
      else
        {
        this->NormalProxyName = token[0];
        this->NormalPropertyName = token[1];
        this->IsFoundNormalProxyProperty = GetNormalProxyNProperty();
        if ( !this->IsFoundProxyProperty )
          {
          std::cout<< "NormalProxyPropert Not Found" << std::endl;
          return false;
          }
        }
      }
    else
      {
      std::cerr << "vtkVRStyleGrabNUpdateMatrix::configure(): "
                << "Please Specify Property" <<std::endl
                << "<Property name=\"ProxyName.PropertyName\"/>"
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
  vtkPVXMLElement* child = Superclass::saveConfiguration();

  vtkPVXMLElement* property = vtkPVXMLElement::New();
  property->SetName("NormalProperty");
  std::stringstream propertyStr;
  propertyStr << this->NormalProxyName << "." << this->NormalPropertyName;
  property->AddAttribute("name", propertyStr.str().c_str() );
  child->AddNestedElement(property);
  property->FastDelete();

  return child;
}

// //----------------------------------------------------------------------private
// void vtkVRStyleGrabNRotateSliceNormal::HandleTracker( const vtkVREventData& data )
// {
//   if ( this->Enabled )
//     {
//     vtkSMRenderViewProxy *proxy =0;
//     vtkSMDoubleVectorProperty *prop =0;

//     pqView *view = 0;
//     view = pqActiveObjects::instance().activeView();
//     if ( view )
//       {
//       proxy = vtkSMRenderViewProxy::SafeDownCast( view->getViewProxy() );
//       if ( proxy )
//         {
//         prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty( "WandPose" ) );
//         if ( prop )
//           {
//           if( !this->InitialOrientationRecored)
//             {
//             // Copy the data into matrix
//             for (int i = 0; i < 4; ++i)
//               {
//               for (int j = 0; j < 4; ++j)
//                 {
//                 this->InitialInvertedPose->SetElement( i,j,data.data.tracker.matrix[i*4+j] );
//                 }
//               }
//             // invert the matrix
//             vtkMatrix4x4::Invert( this->InitialInvertedPose, this->InitialInvertedPose );
//             double wandPose[16];
//             vtkSMPropertyHelper( proxy, "WandPose" ).Get( wandPose, 16 );
//             vtkMatrix4x4::Multiply4x4(&this->InitialInvertedPose->Element[0][0],
//                                       wandPose,
//                                       &this->InitialInvertedPose->Element[0][0]);

//             this->InitialOrientationRecored = true;
//             }
//           else
//             {
//             double wandPose[16];

//             vtkMatrix4x4::Multiply4x4(data.data.tracker.matrix,
//                                       &this->InitialInvertedPose->Element[0][0],
//                                       wandPose);
//             vtkMatrix4x4::MultiplyPoint( wandPose, this->Normal, normal );
//             vtkSMPropertyHelper( this->Proxy, "Normal" ).Set(normal, 3 );
//             }
//           }
//         }
//       }
//     }
//   else
//     {
//     this->InitialOrientationRecored = false;
//     }
// }

bool vtkVRStyleGrabNRotateSliceNormal::GetNormalProxyNProperty()
{
  if ( this->GetProxy( this->NormalProxyName,  &this->NormalProxy ) )
    {
    std::cout << "Got normalproxy" << this->NormalProxy<< std::endl;

    if ( !this->GetProperty( this->NormalProxy,
                             this->NormalPropertyName,
                             &this->NormalProperty ) )
      {
      std::cerr << this->metaObject()->className() << "::GetNormalProxyNProperty"
                << std::endl
                << "Property ( " << this->NormalPropertyName << ") :Not Found"
                <<std::endl;
      return false;
      }
    std::cout << "Got normal property "<< this->NormalProperty << std::endl;

    }
  else
    {
    std::cerr << this->metaObject()->className() << "::GetNormalProxyNProperty"
              << std::endl
              << "Proxy ( " << this->NormalProxyName << ") :Not Found" << std::endl;
    return false;
    }
  return true;
}

void vtkVRStyleGrabNRotateSliceNormal::GetPropertyData()
{
  vtkSMPropertyHelper( this->NormalProxy, "Normal" ).Get(this->Normal, 3 );
  this->Normal[3] =0;
}

void vtkVRStyleGrabNRotateSliceNormal::SetProperty()
{
  double normal[4];
  vtkMatrix4x4::MultiplyPoint( &this->OutPose->GetMatrix()->Element[0][0], this->Normal, normal );
  vtkSMPropertyHelper( this->OutProxy, this->OutPropertyName.c_str() ).Set(normal, 3 );
}

// void vtkVRStyleGrabNRotateSliceNormal::HandleTracker( const vtkVREventData& data )
// {
//   Superclass::HandleTracker( data );
// }

bool vtkVRStyleGrabNRotateSliceNormal::update()
{
  this->OutProxy->UpdateVTKObjects();
  return false;
}

void vtkVRStyleGrabNRotateSliceNormal::HandleButton( const vtkVREventData& data )
{
  if ( this->Button == data.name )
    {
    this->Enabled = data.data.button.state;
    }
}

void vtkVRStyleGrabNRotateSliceNormal::HandleTracker( const vtkVREventData& data )
{
  if ( this->Tracker == data.name )
    {
    if ( this->Enabled )
      {
      if ( !this->IsInitialRecorded )
        {
        this->InitialInvertedPose->SetMatrix( data.data.tracker.matrix );
        this->InitialInvertedPose->Inverse();
        double wandPose[16];
        vtkSMPropertyHelper( this->Proxy,
                             this->PropertyName.c_str()).Get( wandPose, 16 );
        this->InitialInvertedPose->Concatenate( wandPose );
        this->GetPropertyData();
        this->IsInitialRecorded = true;
        }
      else
        {
        this->OutPose->SetMatrix( data.data.tracker.matrix );
        this->OutPose->Concatenate( this->InitialInvertedPose );
        this->SetProperty();
        }
      }
    else
      {
      this->IsInitialRecorded = false;
      }
    }
}
