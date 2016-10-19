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

#include "pqActiveObjects.h"
#include "pqView.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMProxyLocator.h"
#include "vtkSMProxyManager.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkTransform.h"
#include "vtkVRQueue.h"
#include <algorithm>
#include <iostream>
#include <sstream>

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
bool vtkVRStyleGrabNRotateSliceNormal::configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (Superclass::configure(child, locator))
  {
    vtkPVXMLElement* property = child->GetNestedElement(3);
    if (property && property->GetName() && strcmp(property->GetName(), "NormalProperty") == 0)
    {
      std::string propertyStr = property->GetAttributeOrEmpty("name");
      std::vector<std::string> token = this->tokenize(propertyStr);
      if (token.size() != 2)
      {
        std::cerr << "Expected Property \"name\" Format:  Proxy.Property" << std::endl;
        return false;
      }
      else
      {
        this->NormalProxyName = token[0];
        this->NormalPropertyName = token[1];
        this->IsFoundNormalProxyProperty = GetNormalProxyNProperty();
        if (!this->IsFoundProxyProperty)
        {
          std::cout << "NormalProxyPropert Not Found" << std::endl;
          return false;
        }
      }
    }
    else
    {
      std::cerr << "vtkVRStyleGrabNUpdateMatrix::configure(): "
                << "Please Specify Property" << std::endl
                << "<Property name=\"ProxyName.PropertyName\"/>" << std::endl;
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
  property->AddAttribute("name", propertyStr.str().c_str());
  child->AddNestedElement(property);
  property->FastDelete();

  return child;
}

bool vtkVRStyleGrabNRotateSliceNormal::GetNormalProxyNProperty()
{
  if (this->GetProxy(this->NormalProxyName, &this->NormalProxy))
  {
    std::cout << "Got normalproxy" << this->NormalProxy << std::endl;

    if (!this->GetProperty(this->NormalProxy, this->NormalPropertyName, &this->NormalProperty))
    {
      std::cerr << this->metaObject()->className() << "::GetNormalProxyNProperty" << std::endl
                << "Property ( " << this->NormalPropertyName << ") :Not Found" << std::endl;
      return false;
    }
    std::cout << "Got normal property " << this->NormalProperty << std::endl;
  }
  else
  {
    std::cerr << this->metaObject()->className() << "::GetNormalProxyNProperty" << std::endl
              << "Proxy ( " << this->NormalProxyName << ") :Not Found" << std::endl;
    return false;
  }
  return true;
}

void vtkVRStyleGrabNRotateSliceNormal::GetPropertyData()
{
  vtkSMPropertyHelper(this->NormalProxy, "Normal").Get(this->Normal, 3);
  this->Normal[3] = 0;
}

void vtkVRStyleGrabNRotateSliceNormal::SetProperty()
{
  double normal[4];
  vtkMatrix4x4::MultiplyPoint(&this->OutPose->GetMatrix()->Element[0][0], this->Normal, normal);
  vtkSMPropertyHelper(this->OutProxy, this->OutPropertyName.c_str()).Set(normal, 3);
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

void vtkVRStyleGrabNRotateSliceNormal::HandleButton(const vtkVREventData& data)
{
  if (this->Button == data.name)
  {
    this->Enabled = data.data.button.state;
  }
}

void vtkVRStyleGrabNRotateSliceNormal::HandleTracker(const vtkVREventData& data)
{
  if (this->Tracker == data.name)
  {
    if (this->Enabled)
    {
      if (!this->IsInitialRecorded)
      {
        this->InitialInvertedPose->SetMatrix(data.data.tracker.matrix);
        this->InitialInvertedPose->Inverse();
        double modelTransformationMatrix[16];
        vtkSMPropertyHelper(this->Proxy, this->PropertyName.c_str())
          .Get(modelTransformationMatrix, 16);
        this->InitialInvertedPose->Concatenate(modelTransformationMatrix);
        this->GetPropertyData();
        this->IsInitialRecorded = true;
      }
      else
      {
        this->OutPose->SetMatrix(data.data.tracker.matrix);
        this->OutPose->Concatenate(this->InitialInvertedPose);
        this->SetProperty();
      }
    }
    else
    {
      this->IsInitialRecorded = false;
    }
  }
}
