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
#include "vtkVRStyleScaleWorld.h"

#include "pqActiveObjects.h"
#include "pqView.h"
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
vtkVRStyleScaleWorld::vtkVRStyleScaleWorld(QObject* parentObject)
  : Superclass(parentObject)
{
  this->ScaleFactor = 1.0;
}

//------------------------------------------------------------------------destr
vtkVRStyleScaleWorld::~vtkVRStyleScaleWorld()
{
}

//-----------------------------------------------------------------------public
bool vtkVRStyleScaleWorld::configure(vtkPVXMLElement* child, vtkSMProxyLocator* locator)
{
  if (child->GetName() && strcmp(child->GetName(), "Style") == 0 &&
    strcmp(this->metaObject()->className(), child->GetAttributeOrEmpty("class")) == 0)
  {
    child->GetScalarAttribute("scale_by", (double*)&this->ScaleFactor);
    if (child->GetNumberOfNestedElements() != 2)
    {
      std::cerr << "vtkVRStyleScaleWorld::configure(): "
                << "There has to be only 2 elements present " << std::endl
                << "<Button+ name=\"buttonEventName\"/>" << std::endl
                << "<Button- name=\"buttonEventName\"/>" << std::endl;
    }
    vtkPVXMLElement* button = child->GetNestedElement(0);
    if (button && button->GetName() && strcmp(button->GetName(), "Button") == 0)
    {
      this->ButtonPlus = button->GetAttributeOrEmpty("name");
    }
    else
    {
      std::cerr << "vtkVRStyleScaleWorld::configure(): "
                << "Button event has to be specified" << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl;
      return false;
    }
    vtkPVXMLElement* tracker = child->GetNestedElement(1);
    if (tracker && tracker->GetName() && strcmp(tracker->GetName(), "Button") == 0)
    {
      this->ButtonMinus = tracker->GetAttributeOrEmpty("name");
    }
    else
    {
      std::cerr << "vtkVRStyleScaleWorld::configure(): "
                << "Button event has to be specified" << std::endl
                << "<Button name=\"buttonEventName\"/>" << std::endl;
      return false;
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------public
vtkPVXMLElement* vtkVRStyleScaleWorld::saveConfiguration() const
{
  vtkPVXMLElement* child = vtkPVXMLElement::New();
  child->SetName("Style");
  child->AddAttribute("class", this->metaObject()->className());
  std::stringstream value;
  value << double(this->ScaleFactor);
  child->AddAttribute("scale_by", value.str().c_str());

  vtkPVXMLElement* buttonPlus = vtkPVXMLElement::New();
  buttonPlus->SetName("Button");
  buttonPlus->AddAttribute("name", this->ButtonPlus.c_str());
  child->AddNestedElement(buttonPlus);
  buttonPlus->FastDelete();

  vtkPVXMLElement* buttonMinus = vtkPVXMLElement::New();
  buttonMinus->SetName("Button");
  buttonMinus->AddAttribute("name", this->ButtonMinus.c_str());
  child->AddNestedElement(buttonMinus);
  buttonMinus->FastDelete();

  return child;
}

//-----------------------------------------------------------------------public
bool vtkVRStyleScaleWorld::handleEvent(const vtkVREventData& data)
{
  switch (data.eventType)
  {
    case BUTTON_EVENT:
      if (this->ButtonPlus == data.name)
      {
        this->HandleButtonPlus(data);
      }
      if (this->ButtonMinus == data.name)
      {
        this->HandleButtonMinus(data);
      }
      break;
      // case ANALOG_EVENT:
      //   this->HandleAnalog( data );
      //   break;
      // case TRACKER_EVENT:
      //   if ( this->Tracker == data.name )
      //  {
      //  this->HandleTracker( data );
      //  }
      //   break;
  }
  return false;
}

//-----------------------------------------------------------------------public
bool vtkVRStyleScaleWorld::update()
{
  pqView* view = 0;
  vtkSMRenderViewProxy* proxy = 0;
  view = pqActiveObjects::instance().activeView();
  if (view)
  {
    proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
    if (proxy)
    {
      proxy->UpdateVTKObjects();
      proxy->StillRender();
    }
  }
}

//----------------------------------------------------------------------private
void vtkVRStyleScaleWorld::HandleButtonPlus(const vtkVREventData& data)
{
  std::cout << data.name << std::endl;
  if (data.data.button.state)
  {
    std::cout << "its time to scaleup " << std::endl;
    this->ScaleFactor *= this->ScaleFactor;
    vtkSMRenderViewProxy* proxy = 0;
    vtkSMDoubleVectorProperty* prop = 0;

    pqView* view = 0;
    view = pqActiveObjects::instance().activeView();
    if (view)
    {
      proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
      if (proxy)
      {
        prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("ModelTransformMatrix"));
        if (prop)
        {
          vtkTransform* scaleMatrix = vtkTransform::New();
          scaleMatrix->Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);
          double old[16], neo[16];
          vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(&old[0], 16);
          vtkMatrix4x4::Multiply4x4(&(scaleMatrix->GetMatrix()->Element[0][0]), old, neo);
          prop->SetElement(0, neo[0]);
          prop->SetElement(1, neo[1]);
          prop->SetElement(2, neo[2]);
          prop->SetElement(3, neo[3]);

          prop->SetElement(4, neo[4]);
          prop->SetElement(5, neo[5]);
          prop->SetElement(6, neo[6]);
          prop->SetElement(7, neo[7]);

          prop->SetElement(8, neo[8]);
          prop->SetElement(9, neo[9]);
          prop->SetElement(10, neo[10]);
          prop->SetElement(11, neo[11]);

          prop->SetElement(12, 0.0);
          prop->SetElement(13, 0.0);
          prop->SetElement(14, 0.0);
          prop->SetElement(15, 1.0);

          scaleMatrix->Delete();
        }
      }
    }
  }
}

void vtkVRStyleScaleWorld::HandleButtonMinus(const vtkVREventData& data)
{
  std::cout << data.name << std::endl;
  if (data.data.button.state)
  {
    std::cout << "its time to scale-down " << std::endl;
    this->ScaleFactor /= this->ScaleFactor;
    vtkSMRenderViewProxy* proxy = 0;
    vtkSMDoubleVectorProperty* prop = 0;

    pqView* view = 0;
    view = pqActiveObjects::instance().activeView();
    if (view)
    {
      proxy = vtkSMRenderViewProxy::SafeDownCast(view->getViewProxy());
      if (proxy)
      {
        prop = vtkSMDoubleVectorProperty::SafeDownCast(proxy->GetProperty("ModelTransformMatrix"));
        if (prop)
        {
          vtkTransform* scaleMatrix = vtkTransform::New();
          scaleMatrix->Scale(this->ScaleFactor, this->ScaleFactor, this->ScaleFactor);
          double old[16], neo[16];
          vtkSMPropertyHelper(proxy, "ModelTransformMatrix").Get(&old[0], 16);
          vtkMatrix4x4::Multiply4x4(&(scaleMatrix->GetMatrix()->Element[0][0]), old, neo);
          prop->SetElement(0, neo[0]);
          prop->SetElement(1, neo[1]);
          prop->SetElement(2, neo[2]);
          prop->SetElement(3, neo[3]);

          prop->SetElement(4, neo[4]);
          prop->SetElement(5, neo[5]);
          prop->SetElement(6, neo[6]);
          prop->SetElement(7, neo[7]);

          prop->SetElement(8, neo[8]);
          prop->SetElement(9, neo[9]);
          prop->SetElement(10, neo[10]);
          prop->SetElement(11, neo[11]);

          prop->SetElement(12, 0.0);
          prop->SetElement(13, 0.0);
          prop->SetElement(14, 0.0);
          prop->SetElement(15, 1.0);

          scaleMatrix->Delete();
        }
      }
    }
  }
}

//----------------------------------------------------------------------private
std::vector<std::string> vtkVRStyleScaleWorld::tokenize(std::string input)
{
  std::replace(input.begin(), input.end(), '.', ' ');
  std::istringstream stm(input);
  std::vector<std::string> token;
  for (;;)
  {
    std::string word;
    if (!(stm >> word))
      break;
    token.push_back(word);
  }
  return token;
}
