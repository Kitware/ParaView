/*=========================================================================

   Program:   ParaQ
   Module:    pqCutPanel.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

=========================================================================*/

#include "pqApplicationCore.h"
#include "pqCutPanel.h"
#include "pqImplicitPlaneWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyProperty.h>

#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqCutPanel::pqImplementation

class pqCutPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    ImplicitPlaneWidget(parent)
  {
  }
    
  /// Manages a 3D implicit plane widget, plus Qt controls  
  pqImplicitPlaneWidget ImplicitPlaneWidget;
  /// Manages the overall Qt layout
  QVBoxLayout Layout;
};

pqCutPanel::pqCutPanel(QWidget* p) :
  base(p),
  Implementation(new pqImplementation(this))
{
  this->Implementation->Layout.addWidget(&this->Implementation->ImplicitPlaneWidget);
  this->setLayout(&this->Implementation->Layout);

  connect(&this->Implementation->ImplicitPlaneWidget, SIGNAL(widgetChanged()), this, SLOT(onWidgetChanged()));
  connect(this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
  connect(this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqCutPanel::~pqCutPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqCutPanel::onWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqCutPanel::onAccepted()
{
  // Get the current values from the 3D widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  this->Implementation->ImplicitPlaneWidget.getWidgetState(origin, normal);
  
  // Push the new values into the cut filter ...  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("CutFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          plane_origin->SetElements(origin);
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          plane_normal->SetElements(normal);
          }

        clip_function->UpdateVTKObjects();
        }
      }
    }
  
  // If this is the first time we've been accepted since our creation, hide the source
  if(pqPipelineFilter* const pipeline_filter =
    dynamic_cast<pqPipelineFilter*>(pqServerManagerModel::instance()->getPQSource(this->Proxy)))
    {
    if(0 == pipeline_filter->getDisplayCount())
      {
      for(int i = 0; i != pipeline_filter->getInputCount(); ++i)
        {
        if(pqPipelineSource* const pipeline_source = pipeline_filter->getInput(i))
          {
          for(int j = 0; j != pipeline_source->getDisplayCount(); ++j)
            {
            pqPipelineDisplay* const pipeline_display =
              pipeline_source->getDisplay(j);
              
            vtkSMDataObjectDisplayProxy* const display_proxy =
              pipeline_display->getDisplayProxy();

            display_proxy->SetVisibilityCM(false);
            }
          }
        }
      }
    }
}

void pqCutPanel::onRejected()
{
  // Get the current values from the implicit plane ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("CutFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          origin[0] = plane_origin->GetElement(0);
          origin[1] = plane_origin->GetElement(1);
          origin[2] = plane_origin->GetElement(2);
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          normal[0] = plane_normal->GetElement(0);
          normal[1] = plane_normal->GetElement(1);
          normal[2] = plane_normal->GetElement(2);
          }
        }
      }
    }

  this->Implementation->ImplicitPlaneWidget.setWidgetState(origin, normal);
}

void pqCutPanel::setProxyInternal(pqSMProxy p)
{
  base::setProxyInternal(p);
 
  this->Implementation->ImplicitPlaneWidget.setBoundingBoxProxy(p);
  
  if(!this->Proxy)
    return;
  
  // Get the current values from the implicit plane ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("CutFunction")))
      {
      if(vtkSMProxy* const clip_function = clip_function_property->GetProxy(0))
        {
        if(vtkSMDoubleVectorProperty* const plane_origin = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Origin")))
          {
          origin[0] = plane_origin->GetElement(0);
          origin[1] = plane_origin->GetElement(1);
          origin[2] = plane_origin->GetElement(2);
          }

        if(vtkSMDoubleVectorProperty* const plane_normal = vtkSMDoubleVectorProperty::SafeDownCast(
          clip_function->GetProperty("Normal")))
          {
          normal[0] = plane_normal->GetElement(0);
          normal[1] = plane_normal->GetElement(1);
          normal[2] = plane_normal->GetElement(2);
          }
        }
      }
    }

  this->Implementation->ImplicitPlaneWidget.setWidgetState(origin, normal);
}

void pqCutPanel::select()
{
  this->Implementation->ImplicitPlaneWidget.enableWidget();
}

void pqCutPanel::deselect()
{
  this->Implementation->ImplicitPlaneWidget.disableWidget();
}
