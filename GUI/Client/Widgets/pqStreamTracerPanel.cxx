/*=========================================================================

   Program:   ParaQ
   Module:    pqStreamTracerPanel.cxx

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
#include "pqHandleWidget.h"
#include "pqLineWidget.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPropertyManager.h"
#include "pqServerManagerModel.h"
#include "pqStreamTracerPanel.h"

#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMProxyProperty.h>

#include <QFrame>
#include <QVBoxLayout>

//////////////////////////////////////////////////////////////////////////////
// pqStreamTracerPanel::pqImplementation

class pqStreamTracerPanel::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    HandleWidget(0 /*new pqHandleWidget(parent)*/),
    LineWidget(new pqLineWidget(parent))
  {
  }

  ~pqImplementation()
  {
    delete this->LineWidget;
    delete this->HandleWidget;
  }
  
  /// Manages a 3D handle widget, plus Qt controls
  pqHandleWidget* const HandleWidget;
  /// Manages a 3D line widget, plus Qt controls  
  pqLineWidget* const LineWidget;
};

pqStreamTracerPanel::pqStreamTracerPanel(QWidget* p) :
  base(p),
  Implementation(new pqImplementation(this))
{
//  QFrame* const separator = new QFrame();
//  separator->setFrameShape(QFrame::HLine);

  QVBoxLayout* const panel_layout = new QVBoxLayout();
  
  if(this->Implementation->HandleWidget)
    {
    panel_layout->addWidget(this->Implementation->HandleWidget);

    connect(
      this->Implementation->HandleWidget,
      SIGNAL(widgetChanged()),
      this,
      SLOT(on3DWidgetChanged()));
    }

  if(this->Implementation->LineWidget)
    {
    panel_layout->addWidget(this->Implementation->LineWidget);

    connect(
      this->Implementation->LineWidget,
      SIGNAL(widgetChanged()),
      this,
      SLOT(on3DWidgetChanged()));
    }
    
//  panel_layout->addWidget(separator);

  this->setLayout(panel_layout);
  
  connect(
    this->getPropertyManager(), SIGNAL(accepted()), this, SLOT(onAccepted()));
    
  connect(
    this->getPropertyManager(), SIGNAL(rejected()), this, SLOT(onRejected()));
}

pqStreamTracerPanel::~pqStreamTracerPanel()
{
  this->setProxy(0);
  delete this->Implementation;
}

void pqStreamTracerPanel::on3DWidgetChanged()
{
  // Signal the UI that there are changes to accept/reject ...
  this->getPropertyManager()->propertyChanged();
}

void pqStreamTracerPanel::onAccepted()
{
/*
  // Get the current values from the 3D implicit plane widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  this->Implementation->LineWidget.getWidgetState(origin, normal);

  // Get the current values from the sample scalar widget ...
  const QList<double> samples = this->Implementation->SampleScalarWidget.getSamples();
  
  // Push the new values into the cut filter ...  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("StreamTracerFunction")))
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
      
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      contours->SetNumberOfElements(samples.size());
      for(int i = 0; i != samples.size(); ++i)
        {
        contours->SetElement(i, samples[i]);
        }
        
      this->Proxy->UpdateVTKObjects();
      }
    }
*/
  
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

void pqStreamTracerPanel::onRejected()
{
/*
  // Restore the state of the implicit plane widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("StreamTracerFunction")))
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
  this->Implementation->LineWidget.setWidgetState(origin, normal);

  // Reset the state of the sample scalar widget ...
  QList<double> values;
  if(this->Proxy)
    {
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      const int value_count = contours->GetNumberOfElements();
      for(int i = 0; i != value_count; ++i)
        {
        values.push_back(contours->GetElement(i));
        }
      }
    }
  this->Implementation->SampleScalarWidget.setSamples(values);
*/
}

void pqStreamTracerPanel::setProxyInternal(pqSMProxy p)
{
  base::setProxyInternal(p);
 
  if(this->Implementation->HandleWidget)
    {
    this->Implementation->HandleWidget->setReferenceProxy(p);
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->setReferenceProxy(p);
    }
  
  if(!this->Proxy)
    return;

/*
  // Setup the implicit plane widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  
  if(this->Proxy)
    {
    if(vtkSMProxyProperty* const clip_function_property = vtkSMProxyProperty::SafeDownCast(
      this->Proxy->GetProperty("StreamTracerFunction")))
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

  this->Implementation->LineWidget.setWidgetState(origin, normal);

  // Setup the sample scalar widget ...
  QList<double> values;
  
  if(this->Proxy)
    {
    if(vtkSMDoubleVectorProperty* const contours =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Proxy->GetProperty("ContourValues")))
      {
      const int value_count = contours->GetNumberOfElements();
      for(int i = 0; i != value_count; ++i)
        {
        values.push_back(contours->GetElement(i));
        }
      }
    }
    
  this->Implementation->SampleScalarWidget.setSamples(values);
*/
}

void pqStreamTracerPanel::select()
{
  if(this->Implementation->HandleWidget)
    {
    this->Implementation->HandleWidget->showWidget();
    }
    
  if(this->Implementation->LineWidget)
    {
    this->Implementation->LineWidget->showWidget();
    }
}

void pqStreamTracerPanel::deselect()
{
  if(this->Implementation->HandleWidget)
    {
    this->Implementation->HandleWidget->hideWidget();
    }

  if(this->Implementation->LineWidget)
    {    
    this->Implementation->LineWidget->hideWidget();
    }
}
