/*=========================================================================

   Program:   ParaQ
   Module:    pqLineWidget.cxx

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
#include "pqLineWidget.h"
#include "pqRenderModule.h"
#include "pqServerManagerModel.h"

#include "ui_pqLineWidget.h"

#include <vtkCamera.h>
#include <vtkMemberFunctionCommand.h>
#include <vtkLineRepresentation.h>
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
// pqLineWidget::pqImplementation

class pqLineWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqLineWidget()),
    Widget(0),
    IgnoreVisibilityWidget(false),
    IgnoreQtWidgets(false),
    Ignore3DWidget(false)
  {
  }
  
  ~pqImplementation()
  {
    delete this->UI;
  }
  
  /// Stores the Qt widgets
  Ui::pqLineWidget* const UI;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> StartDragObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> ChangeObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> EndDragObserver;
  /// References the 3D implicit plane widget
  vtkSMNew3DWidgetProxy* Widget;
  /// Source proxy that will supply the bounding box for the 3D widget
  pqSMProxy ReferenceProxy;
  /// Used to avoid recursion when updating the visiblity checkbox
  bool IgnoreVisibilityWidget;
  /// Used to avoid recursion when updating the Qt widgets  
  bool IgnoreQtWidgets;
  /// Used to avoid recursion when updating the 3D widget
  bool Ignore3DWidget;
  
  static QMap<pqSMProxy, bool> Visibility;
};

QMap<pqSMProxy, bool> pqLineWidget::pqImplementation::Visibility;

/////////////////////////////////////////////////////////////////////////
// pqLineWidget

pqLineWidget::pqLineWidget(QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqLineWidget::on3DWidgetStartDrag));
  this->Implementation->ChangeObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqLineWidget::on3DWidgetChanged));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqLineWidget::on3DWidgetEndDrag));
    
  this->Implementation->UI->setupUi(this);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this->Implementation->UI->point1X,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->point1Y,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->point1Z,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));

  connect(this->Implementation->UI->point2X,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->point2Y,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->point2Z,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
    
  connect(this->Implementation->UI->xAxis,
    SIGNAL(clicked()), this, SLOT(onUseXAxis()));
  connect(this->Implementation->UI->yAxis,
    SIGNAL(clicked()), this, SLOT(onUseYAxis()));
  connect(this->Implementation->UI->zAxis,
    SIGNAL(clicked()), this, SLOT(onUseZAxis()));
}

pqLineWidget::~pqLineWidget()
{
  if(this->Implementation->Widget)
    {
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->EndDragObserver);
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->ChangeObserver);
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->StartDragObserver);
    }

  if(this->Implementation->Widget)
    {
    if(pqRenderModule* const renModule = 
      pqApplicationCore::instance()->getActiveRenderModule())
      {
      if(vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy())
        {
        if(vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
          rm->GetProperty("Displays")))
          {
          pp->RemoveProxy(this->Implementation->Widget);
          rm->UpdateVTKObjects();
          renModule->render();
          }
        }
      }

    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(this->Implementation->Widget);
      
    this->Implementation->Widget = 0;
    }

  delete this->Implementation;
}

void pqLineWidget::showWidget()
{
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqLineWidget::hideWidget()
{
  this->show3DWidget(false);
}

//-----------------------------------------------------------------------------
void pqLineWidget::setReferenceProxy(pqSMProxy proxy)
{
  this->Implementation->ReferenceProxy = proxy;
  
  if(!this->Implementation->Visibility.contains(proxy))
    {
    this->Implementation->Visibility.insert(
      proxy, proxy.GetPointer() ? true : false);
    }
  
  if(!this->Implementation->Widget)
    {
    pqServer* const server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(
        this->Implementation->ReferenceProxy->GetConnectionID());
          
    this->Implementation->Widget =
      pqApplicationCore::instance()->get3DWidgetFactory()->
        get3DWidget("LineWidgetDisplay", server);
    this->Implementation->Widget->UpdateVTKObjects();

    // Synchronize the 3D widget bounds with the source data ...
    this->onUseXAxis();

    pqRenderModule* const renModule = 
      pqApplicationCore::instance()->getActiveRenderModule();
      
    if(this->Implementation->Widget && renModule)
      {
      vtkSMRenderModuleProxy* rm = renModule->getRenderModuleProxy() ;
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        rm->GetProperty("Displays"));
      pp->AddProxy(this->Implementation->Widget);
      rm->UpdateVTKObjects();
      renModule->render();
      }
      
    if(this->Implementation->Widget)
      {
      this->Implementation->Widget->AddObserver(
        vtkCommand::StartInteractionEvent,
        this->Implementation->StartDragObserver);
      this->Implementation->Widget->AddObserver(
        vtkCommand::PropertyModifiedEvent,
        this->Implementation->ChangeObserver);
      this->Implementation->Widget->AddObserver(
        vtkCommand::EndInteractionEvent,
        this->Implementation->EndDragObserver);
      }
    }

  this->Implementation->IgnoreVisibilityWidget = true;
  this->Implementation->UI->show3DWidget->setChecked(
    this->Implementation->Visibility[proxy]);
  this->Implementation->IgnoreVisibilityWidget = false;

  this->show3DWidget(this->Implementation->Visibility[proxy]);
}

void pqLineWidget::getWidgetState(double point1[3], double point2[3])
{
  point1[0] = point1[1] = point1[2] = 0;
  point2[0] = point2[1] = point2[2] = 0;

  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point1WorldPosition")))
      {
      point1[0] = widget_position->GetElement(0);
      point1[1] = widget_position->GetElement(1);
      point1[2] = widget_position->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point2WorldPosition")))
      {
      point2[0] = widget_position->GetElement(0);
      point2[1] = widget_position->GetElement(1);
      point2[2] = widget_position->GetElement(2);
      }
    }
}

void pqLineWidget::setWidgetState(const double point1[3], const double point2[3])
{
  // Push the current values into the Qt widgets ...
  this->updateQtWidgets(point1, point2);
  
  // Push the current values into the 3D widget ...
  this->update3DWidget(point1, point2);
}

void pqLineWidget::onShow3DWidget(bool show_widget)
{
  this->Implementation->Visibility.insert(
    this->Implementation->ReferenceProxy, show_widget);
    
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqLineWidget::onUseXAxis()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->Implementation->ReferenceProxy->GetProperty("Input")))
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
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 0.6;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 0.6;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 0.6;

        if(this->Implementation->Widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point1WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0] - input_size[0]);
            widget_position->SetElement(1, input_origin[1]);
            widget_position->SetElement(2, input_origin[2]);
            }
          
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point2WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0] + input_size[0]);
            widget_position->SetElement(1, input_origin[1]);
            widget_position->SetElement(2, input_origin[2]);
            }
          
          this->Implementation->Widget->UpdateVTKObjects();
          
          this->on3DWidgetChanged();
          
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqLineWidget::onUseYAxis()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->Implementation->ReferenceProxy->GetProperty("Input")))
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
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 0.6;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 0.6;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 0.6;
        
        if(this->Implementation->Widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point1WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0]);
            widget_position->SetElement(1, input_origin[1] - input_size[1]);
            widget_position->SetElement(2, input_origin[2]);
            }
          
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point2WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0]);
            widget_position->SetElement(1, input_origin[1] + input_size[1]);
            widget_position->SetElement(2, input_origin[2]);
            }
          
          this->Implementation->Widget->UpdateVTKObjects();
          
          /** \todo This shouldn't be necessary, not sure why we don't get an event */
          this->on3DWidgetChanged();
          
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqLineWidget::onUseZAxis()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->Implementation->ReferenceProxy->GetProperty("Input")))
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
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 0.6;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 0.6;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 0.6;
        
        if(this->Implementation->Widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point1WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0]);
            widget_position->SetElement(1, input_origin[1]);
            widget_position->SetElement(2, input_origin[2] - input_size[2]);
            }
          
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("Point2WorldPosition")))
            {
            widget_position->SetElement(0, input_origin[0]);
            widget_position->SetElement(1, input_origin[1]);
            widget_position->SetElement(2, input_origin[2] + input_size[2]);
            }
          
          this->Implementation->Widget->UpdateVTKObjects();
          
          /** \todo This shouldn't be necessary, not sure why we don't get an event */
          this->on3DWidgetChanged();
          
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqLineWidget::show3DWidget(bool show_widget)
{
  if(this->Implementation->Widget)
    {
    this->Implementation->Ignore3DWidget = true;
    
    if(vtkSMIntVectorProperty* const visibility =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Visibility")))
      {
      visibility->SetElement(0, show_widget);
      }

    if(vtkSMIntVectorProperty* const enabled =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Enabled")))
      {
      enabled->SetElement(0, show_widget);
      }

    this->Implementation->Widget->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    this->Implementation->Ignore3DWidget = false;
    }
}

void pqLineWidget::onQtWidgetChanged()
{
  if(this->Implementation->IgnoreQtWidgets)
    return;

  // Get the new values from the Qt widgets ...
  double point1[3] = { 0, 0, 0 };
  double point2[3] = { 0, 0, 0 };

  point1[0] = this->Implementation->UI->point1X->text().toDouble();
  point1[1] = this->Implementation->UI->point1Y->text().toDouble();
  point1[2] = this->Implementation->UI->point1Z->text().toDouble();
  
  point2[0] = this->Implementation->UI->point2X->text().toDouble();
  point2[1] = this->Implementation->UI->point2Y->text().toDouble();
  point2[2] = this->Implementation->UI->point2Z->text().toDouble();
  
  // Push the new values into the 3D widget ...
  this->update3DWidget(point1, point2);

  emit widgetChanged();
}

void pqLineWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

void pqLineWidget::on3DWidgetChanged()
{
  if(this->Implementation->Ignore3DWidget)
    return;
    
  // Get the new values from the 3D widget ...
  double point1[3] = { 0, 0, 0 };
  double point2[3] = { 0, 0, 0 };
  
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point1WorldPosition")))
      {
      point1[0] = widget_position->GetElement(0);
      point1[1] = widget_position->GetElement(1);
      point1[2] = widget_position->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point2WorldPosition")))
      {
      point2[0] = widget_position->GetElement(0);
      point2[1] = widget_position->GetElement(1);
      point2[2] = widget_position->GetElement(2);
      }
    }
  
  // Push the new values into the Qt widgets (ideally, this should happen automatically when the implicit plane is updated)
  this->updateQtWidgets(point1, point2);

  emit widgetChanged();
}

void pqLineWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

void pqLineWidget::updateQtWidgets(const double point1[3], const double point2[3])
{
  this->Implementation->IgnoreQtWidgets = true;
  
  this->Implementation->UI->point1X->setText(
    QString::number(point1[0], 'g', 3));
  this->Implementation->UI->point1Y->setText(
    QString::number(point1[1], 'g', 3));  
  this->Implementation->UI->point1Z->setText(
    QString::number(point1[2], 'g', 3));  
  
  this->Implementation->UI->point2X->setText(
    QString::number(point2[0], 'g', 3));
  this->Implementation->UI->point2Y->setText(
    QString::number(point2[1], 'g', 3));  
  this->Implementation->UI->point2Z->setText(
    QString::number(point2[2], 'g', 3));  
  
  this->Implementation->IgnoreQtWidgets = false;
}

void pqLineWidget::update3DWidget(const double point1[3], const double point2[3])
{
  this->Implementation->Ignore3DWidget = true;
   
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point1WorldPosition")))
      {
      widget_position->SetElements(point1);
      }
    
    if(vtkSMDoubleVectorProperty* const widget_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Point2WorldPosition")))
      {
      widget_position->SetElements(point2);
      }
    
    this->Implementation->Widget->UpdateVTKObjects();
    
    pqApplicationCore::instance()->render();
    }
    
  this->Implementation->Ignore3DWidget = false;
}

