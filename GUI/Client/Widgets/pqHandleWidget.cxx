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
    UI(new Ui::pqHandleWidget()),
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
  Ui::pqHandleWidget* const UI;
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

QMap<pqSMProxy, bool> pqHandleWidget::pqImplementation::Visibility;

/////////////////////////////////////////////////////////////////////////
// pqHandleWidget

pqHandleWidget::pqHandleWidget(QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetStartDrag));
  this->Implementation->ChangeObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetChanged));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqHandleWidget::on3DWidgetEndDrag));
    
  this->Implementation->UI->setupUi(this);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this->Implementation->UI->worldPositionX,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->worldPositionY,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->worldPositionZ,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
    
  connect(this->Implementation->UI->useCenterBounds,
    SIGNAL(clicked()), this, SLOT(onResetBounds()));
}

pqHandleWidget::~pqHandleWidget()
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

void pqHandleWidget::setDataSources(pqSMProxy reference_proxy)
{
  this->Implementation->ReferenceProxy = reference_proxy;
  
  if(!this->Implementation->Visibility.contains(this->Implementation->ReferenceProxy))
    {
    this->Implementation->Visibility.insert(
      this->Implementation->ReferenceProxy, this->Implementation->ReferenceProxy.GetPointer() ? true : false);
    }
  
  if(!this->Implementation->Widget)
    {
    pqServer* const server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(
        this->Implementation->ReferenceProxy->GetConnectionID());
          
    this->Implementation->Widget =
      pqApplicationCore::instance()->get3DWidgetFactory()->
        get3DWidget("HandleWidgetDisplay", server);
    this->Implementation->Widget->UpdateVTKObjects();

    // Synchronize the 3D widget bounds with the source data ...
    this->onResetBounds();

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
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
  this->Implementation->IgnoreVisibilityWidget = false;

  this->show3DWidget(this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqHandleWidget::getWidgetState(double world_position[3])
{
  world_position[0] = world_position[1] = world_position[2] = 0;

  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_world_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("WorldPosition")))
      {
      world_position[0] = widget_world_position->GetElement(0);
      world_position[1] = widget_world_position->GetElement(1);
      world_position[2] = widget_world_position->GetElement(2);
      }
    }
}

void pqHandleWidget::setWidgetState(const double world_position[3])
{
  // Push the current values into the Qt widgets ...
  this->updateQtWidgets(world_position);
  
  // Push the current values into the 3D widget ...
  this->update3DWidget(world_position);
}

void pqHandleWidget::showWidget()
{
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqHandleWidget::hideWidget()
{
  this->show3DWidget(false);
}

void pqHandleWidget::onShow3DWidget(bool show_widget)
{
  this->Implementation->Visibility.insert(
    this->Implementation->ReferenceProxy, show_widget);
    
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqHandleWidget::onResetBounds()
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

        if(this->Implementation->Widget)
          {
          if(vtkSMDoubleVectorProperty* const widget_position =
            vtkSMDoubleVectorProperty::SafeDownCast(
              this->Implementation->Widget->GetProperty("WorldPosition")))
            {
            widget_position->SetElements(input_origin);
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

void pqHandleWidget::show3DWidget(bool show_widget)
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

void pqHandleWidget::onQtWidgetChanged()
{
  if(this->Implementation->IgnoreQtWidgets)
    return;

  // Get the new values from the Qt widgets ...
  double world_position[3] = { 0, 0, 0 };

  world_position[0] = this->Implementation->UI->worldPositionX->text().toDouble();
  world_position[1] = this->Implementation->UI->worldPositionY->text().toDouble();
  world_position[2] = this->Implementation->UI->worldPositionZ->text().toDouble();
  
  // Push the new values into the 3D widget ...
  this->update3DWidget(world_position);

  emit widgetChanged();
}

void pqHandleWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

void pqHandleWidget::on3DWidgetChanged()
{
  if(this->Implementation->Ignore3DWidget)
    return;
    
  // Get the new values from the 3D widget ...
  double world_position[3] = { 0, 0, 0 };
  
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_world_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("WorldPosition")))
      {
      world_position[0] = widget_world_position->GetElement(0);
      world_position[1] = widget_world_position->GetElement(1);
      world_position[2] = widget_world_position->GetElement(2);
      }
    }
  
  // Push the new values into the Qt widgets (ideally, this should happen automatically when the implicit plane is updated)
  this->updateQtWidgets(world_position);

  emit widgetChanged();
}

void pqHandleWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

void pqHandleWidget::updateQtWidgets(const double world_position[3])
{
  this->Implementation->IgnoreQtWidgets = true;
  
  this->Implementation->UI->worldPositionX->setText(
    QString::number(world_position[0], 'g', 3));
  this->Implementation->UI->worldPositionY->setText(
    QString::number(world_position[1], 'g', 3));  
  this->Implementation->UI->worldPositionZ->setText(
    QString::number(world_position[2], 'g', 3));  
  
  this->Implementation->IgnoreQtWidgets = false;
}

void pqHandleWidget::update3DWidget(const double world_position[3])
{
  this->Implementation->Ignore3DWidget = true;
   
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_world_position =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("WorldPosition")))
      {
      widget_world_position->SetElements(world_position);
      }
    
    this->Implementation->Widget->UpdateVTKObjects();
    
    pqApplicationCore::instance()->render();
    }
    
  this->Implementation->Ignore3DWidget = false;
}
