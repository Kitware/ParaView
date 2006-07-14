/*=========================================================================

   Program:   ParaQ
   Module:    pqHandleWidget.cxx

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

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqHandleWidget.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqPropertyLinks.h"
#include "pqSMSignalAdaptors.h"

#include "ui_pqHandleWidget.h"

#include <vtkCamera.h>
#include <vtkMemberFunctionCommand.h>
#include <vtkHandleRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget::pqImplementation

class pqHandleWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqHandleWidget())
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
  /// Stores the Qt widgets
  Ui::pqHandleWidget* const UI;
  // Controlled property for position.
  vtkSmartPointer<vtkSMDoubleVectorProperty> PositionProperty;

  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> StartDragObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> EndDragObserver;
  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget

pqHandleWidget::pqHandleWidget(QWidget* p) :
  pq3DWidget(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetStartDrag));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetEndDrag));

  this->Implementation->UI->setupUi(this);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this->Implementation->UI->useCenterBounds,
    SIGNAL(clicked()), this, SLOT(onResetBounds()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SIGNAL(widgetChanged()));

  QObject::connect(&this->Implementation->Links, SIGNAL(smPropertyChanged()),
    this, SIGNAL(widgetChanged()));
}

pqHandleWidget::~pqHandleWidget()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    widget->RemoveObserver(
      this->Implementation->EndDragObserver);
    widget->RemoveObserver(
      this->Implementation->StartDragObserver);
    }

  if(widget)
    {
    pqPipelineFilter* source;
    pqPipelineSource* source1 = NULL;
    pqPipelineDisplay* display = NULL;
    pqRenderModule* renModule = NULL;
    
    source = qobject_cast<pqPipelineFilter*>(this->getReferenceProxy());
    if(source)
      {
      source1 = source->getInput(0);
      }
    if(source1)
      {
      display = source1->getDisplay(0);
      }
    if(display)
      {
      renModule = display->getRenderModule(0);
      }

    if(renModule)
      {
      if(vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy())
        {
        if(vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
          rm->GetProperty("Displays")))
          {
          pp->RemoveProxy(widget);
          rm->UpdateVTKObjects();
          renModule->render();
          }
        }
      }

    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
      
    this->setWidgetProxy(0);
    }

  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqHandleWidget::setControlledProxy(vtkSMProxy* proxy)
{
  if(!this->getWidgetProxy())
    {
    pqServer* const server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(proxy->GetConnectionID());
          
    vtkSMNew3DWidgetProxy* widget =
      pqApplicationCore::instance()->get3DWidgetFactory()->
        get3DWidget("PointSourceWidgetDisplay", server);
    this->setWidgetProxy(widget);

    pqSignalAdaptorDouble* adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->worldPositionX, "text",
      SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("WorldPosition"), 0);

    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->worldPositionY, "text",
      SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("WorldPosition"), 1);

    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->worldPositionZ, "text",
      SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("WorldPosition"), 2);

    widget->UpdateVTKObjects();


    pqPipelineFilter* source;
    pqPipelineSource* source1 = NULL;
    pqPipelineDisplay* display = NULL;
    pqRenderModule* renModule = NULL;
    source = qobject_cast<pqPipelineFilter*>(this->getReferenceProxy());
    if(source)
      {
      source1 = source->getInput(0);
      }
    if(source1)
      {
      display = source1->getDisplay(0);
      }
    if(display)
      {
      renModule = display->getRenderModule(0);
      }
      
    if(widget && renModule)
      {
      vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy() ;
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        rm->GetProperty("Displays"));
      pp->AddProxy(widget);
      rm->UpdateVTKObjects();
      renModule->render();
      }
      
    if(widget)
      {
      widget->AddObserver(vtkCommand::StartInteractionEvent,
        this->Implementation->StartDragObserver);
      widget->AddObserver(vtkCommand::EndInteractionEvent,
        this->Implementation->EndDragObserver);
      }
    }
  this->pq3DWidget::setControlledProxy(proxy);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::setControlledProperty(const char* function,
    vtkSMProperty * controlled_property)
{
  if (strcmp(function, "WorldPosition") == 0)
    {
    this->Implementation->PositionProperty = 
      vtkSMDoubleVectorProperty::SafeDownCast(controlled_property);
    }
  this->pq3DWidget::setControlledProperty(function, controlled_property);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::set3DWidgetVisibility(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);

  this->pq3DWidget::set3DWidgetVisibility(visible);
}

//-----------------------------------------------------------------------------
void pqHandleWidget::onShow3DWidget(bool show_widget)
{
  if (show_widget)
    {
    this->showWidget();
    }
  else
    {
    this->hideWidget();
    }
}

//-----------------------------------------------------------------------------
void pqHandleWidget::resetBounds()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget && this->getReferenceProxy())
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->getReferenceProxy()->getProxy()->GetProperty("Input")))
      {
      if(vtkSMSourceProxy* const input_proxy = vtkSMSourceProxy::SafeDownCast(
          input_property->GetProxy(0)))
        {
        double input_bounds[6];
        input_proxy->GetDataInformation()->GetBounds(input_bounds);

        double input_origin[3];
        input_origin[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
        input_origin[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
        input_origin[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

        if(widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              widget->GetProperty("WorldPosition")))
            {
            widget_position->SetElements(input_origin);
            }

          widget->UpdateVTKObjects();

          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqHandleWidget::onResetBounds()
{
  this->resetBounds();
}

//-----------------------------------------------------------------------------
void pqHandleWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

//-----------------------------------------------------------------------------
void pqHandleWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

