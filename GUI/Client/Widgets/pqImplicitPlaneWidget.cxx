/*=========================================================================

   Program: ParaView
   Module:    pqImplicitPlaneWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqImplicitPlaneWidget.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"
#include "pqPipelineSource.h"
#include "pqPipelineFilter.h"
#include "pqPipelineDisplay.h"
#include "pqPropertyLinks.h"
#include "pqSMSignalAdaptors.h"

#include "ui_pqImplicitPlaneWidget.h"

#include <vtkCamera.h>
#include <vtkMemberFunctionCommand.h>
#include <vtkImplicitPlaneRepresentation.h>
#include <vtkProcessModule.h>
#include <vtkPVDataInformation.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

/////////////////////////////////////////////////////////////////////////
// pqImplicitPlaneWidget::pqImplementation

class pqImplicitPlaneWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqImplicitPlaneWidget()),
    OriginProperty(0),
    NormalProperty(0)
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
  /// Stores the Qt widgets
  Ui::pqImplicitPlaneWidget* const UI;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> StartDragObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> EndDragObserver;
  
  vtkSMDoubleVectorProperty* OriginProperty;
  vtkSMDoubleVectorProperty* NormalProperty;
  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqImplicitPlaneWidget

pqImplicitPlaneWidget::pqImplicitPlaneWidget(QWidget* p) :
  pq3DWidget(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetStartDrag));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetEndDrag));
    
  this->Implementation->UI->setupUi(this);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this->Implementation->UI->useXNormal,
    SIGNAL(clicked()), this, SLOT(onUseXNormal()));
  connect(this->Implementation->UI->useYNormal,
    SIGNAL(clicked()), this, SLOT(onUseYNormal()));
  connect(this->Implementation->UI->useZNormal,
    SIGNAL(clicked()), this, SLOT(onUseZNormal()));
  connect(this->Implementation->UI->useCameraNormal,
    SIGNAL(clicked()), this, SLOT(onUseCameraNormal()));
  connect(this->Implementation->UI->resetBounds,
    SIGNAL(clicked()), this, SLOT(onResetBounds()));
  connect(this->Implementation->UI->useCenterBounds,
    SIGNAL(clicked()), this, SLOT(onUseCenterBounds()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SIGNAL(widgetChanged()));

  QObject::connect(&this->Implementation->Links, SIGNAL(smPropertyChanged()),
    this, SIGNAL(widgetChanged()));
}

pqImplicitPlaneWidget::~pqImplicitPlaneWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();

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
void pqImplicitPlaneWidget::setControlledProxy(vtkSMProxy* proxy)
{
  if(!this->getWidgetProxy())
    {
    // We won't have to do this once setProxy() takes pqProxy as an argument.
    pqServer* const server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(
        proxy->GetConnectionID());

    vtkSMNew3DWidgetProxy* widget = 
      pqApplicationCore::instance()->get3DWidgetFactory()->
      get3DWidget("ImplicitPlaneWidgetDisplay", server);
    this->setWidgetProxy(widget);
    widget->UpdateVTKObjects();
  
    // Now bind the GUI widgets to the 3D widget.

    // The adaptor is used to format the text value.
    pqSignalAdaptorDouble* adaptor = 
      new pqSignalAdaptorDouble(this->Implementation->UI->originX,
        "text", SIGNAL(textChanged(const QString&)));

    this->Implementation->Links.addPropertyLink(
      adaptor, "value", 
      SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Origin"), 0);

    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->originY,
      "text", SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Origin"), 1);
    
    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->originZ,
      "text", SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Origin"), 2);

    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->normalX,
      "text", SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", 
      SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Normal"), 0);

    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->normalY,
      "text", SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Normal"), 1);
    
    adaptor = new pqSignalAdaptorDouble(
      this->Implementation->UI->normalZ,
      "text", SIGNAL(textChanged(const QString&)));
    this->Implementation->Links.addPropertyLink(
      adaptor, "value", SIGNAL(valueChanged(const QString&)),
      widget, widget->GetProperty("Normal"), 2);

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
void pqImplicitPlaneWidget::setControlledProperty(const char* function,
  vtkSMProperty* controlled_property)
{
  if (strcmp(function, "Origin") ==0)
    {
    this->setOriginProperty(controlled_property);
    }
  else if (strcmp(function, "Normal") == 0)
    {
    this->setNormalProperty(controlled_property);
    }
  this->pq3DWidget::setControlledProperty(function, controlled_property);
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::setOriginProperty(vtkSMProperty* origin_property)
{
  this->Implementation->OriginProperty = 
    vtkSMDoubleVectorProperty::SafeDownCast(origin_property);
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::setNormalProperty(vtkSMProperty* normal_property)
{
  this->Implementation->NormalProperty = 
    vtkSMDoubleVectorProperty::SafeDownCast(normal_property);
}


//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::set3DWidgetVisibility(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);

  this->pq3DWidget::set3DWidgetVisibility(visible);
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::showPlane()
{
  if(this->getWidgetProxy())
    {
    if(vtkSMIntVectorProperty* const show_plane =
      vtkSMIntVectorProperty::SafeDownCast(
        this->getWidgetProxy()->GetProperty("DrawPlane")))
      {
      show_plane->SetElement(0, true);
      this->getWidgetProxy()->UpdateVTKObjects();
      }
    }
}

void pqImplicitPlaneWidget::hidePlane()
{
  if(this->getWidgetProxy())
    {
    if(vtkSMIntVectorProperty* const show_plane =
      vtkSMIntVectorProperty::SafeDownCast(
        this->getWidgetProxy()->GetProperty("DrawPlane")))
      {
      show_plane->SetElement(0, false);
      this->getWidgetProxy()->UpdateVTKObjects();
      }
    }
}

void pqImplicitPlaneWidget::onShow3DWidget(bool show_widget)
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
void pqImplicitPlaneWidget::onResetBounds()
{
  this->resetBounds();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::resetBounds()
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

        double input_size[3];
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 1.2;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 1.2;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 1.2;
        
        if(vtkSMDoubleVectorProperty* const origin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          }
        
        if(vtkSMDoubleVectorProperty* const place_widget =
          vtkSMDoubleVectorProperty::SafeDownCast(
            widget->GetProperty("PlaceWidget")))
          {
          double widget_bounds[6];
          widget_bounds[0] = input_origin[0] - input_size[0];
          widget_bounds[1] = input_origin[0] + input_size[0];
          widget_bounds[2] = input_origin[1] - input_size[1];
          widget_bounds[3] = input_origin[1] + input_size[1];
          widget_bounds[4] = input_origin[2] - input_size[2];
          widget_bounds[5] = input_origin[2] + input_size[2];
          
          place_widget->SetElements(widget_bounds);
          
          widget->UpdateVTKObjects();
          qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
          }
        }
      }
    }
}

void pqImplicitPlaneWidget::onUseCenterBounds()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
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

        if(vtkSMDoubleVectorProperty* const origin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          widget->UpdateVTKObjects();
          qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
          }
        }
      }
    }
}

void pqImplicitPlaneWidget::onUseXNormal()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(1, 0, 0);
      widget->UpdateVTKObjects();
      qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
      }
    }
}

void pqImplicitPlaneWidget::onUseYNormal()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 1, 0);
      widget->UpdateVTKObjects();
      qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
      }
    }
}

void pqImplicitPlaneWidget::onUseZNormal()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 0, 1);
      widget->UpdateVTKObjects();
      qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
      }
    }
}

void pqImplicitPlaneWidget::onUseCameraNormal()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
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
    if(vtkCamera* const camera = renModule->getRenderModuleProxy()->GetRenderer()->GetActiveCamera())
      {
      if(vtkSMDoubleVectorProperty* const normal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          widget->GetProperty("Normal")))
        {
        double camera_normal[3];
        camera->GetViewPlaneNormal(camera_normal);
        normal->SetElements3(
          -camera_normal[0], -camera_normal[1], -camera_normal[2]);
        
        widget->UpdateVTKObjects();
        qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
        }
      }
    }
}

void pqImplicitPlaneWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

void pqImplicitPlaneWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

void pqImplicitPlaneWidget::get3DWidgetState(double* origin, double* normal)
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Origin")))
      {
      origin[0] = widget_origin->GetElement(0);
      origin[1] = widget_origin->GetElement(1);
      origin[2] = widget_origin->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal[0] = widget_normal->GetElement(0);
      normal[1] = widget_normal->GetElement(1);
      normal[2] = widget_normal->GetElement(2);
      }
    }
}

#if 0
void pqImplicitPlaneWidget::set3DWidgetState(const double* origin, const double* normal)
{
  this->Ignore3DWidget = true;
   
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Origin")))
      {
      widget_origin->SetElements(origin);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      widget_normal->SetElements(normal);
      }
    
    widget->UpdateVTKObjects();
    
    qobject_cast<pqPipelineSource*>(this->getReferenceProxy())->renderAllViews();
    }
    
  this->Ignore3DWidget = false;
}

void pqImplicitPlaneWidget::setQtWidgetState(const double* origin, const double* normal)
{
  this->Implementation->IgnoreQtWidgets = true;
  
  this->Implementation->UI->originX->setText(
    QString::number(origin[0], 'g', 3));
  this->Implementation->UI->originY->setText(
    QString::number(origin[1], 'g', 3));  
  this->Implementation->UI->originZ->setText(
    QString::number(origin[2], 'g', 3));  
  this->Implementation->UI->normalX->setText(
    QString::number(normal[0], 'g', 3));  
  this->Implementation->UI->normalY->setText(
    QString::number(normal[1], 'g', 3));  
  this->Implementation->UI->normalZ->setText(
    QString::number(normal[2], 'g', 3));
  
  this->Implementation->IgnoreQtWidgets = false;
}
#endif

