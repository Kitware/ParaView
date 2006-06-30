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
    Widget(0),
    OriginProperty(0),
    NormalProperty(0),
    IgnoreVisibilityWidget(false),
    IgnoreQtWidgets(false),
    Ignore3DWidget(false),
    IgnorePropertyChange(false)
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
  vtkSmartPointer<vtkCommand> ChangeObserver;
  /// Callback object used to connect 3D widget events to member methods
  vtkSmartPointer<vtkCommand> EndDragObserver;
  /// Callback object used to connect property events to member methods
  vtkSmartPointer<vtkCommand> PropertyObserver;
  /// References the 3D implicit plane widget
  vtkSMNew3DWidgetProxy* Widget;
  /// Source proxy that will supply the bounding box for the 3D widget
  pqSMProxy ReferenceProxy;
  pqSMProxy ControlledProxy;
  vtkSMDoubleVectorProperty* OriginProperty;
  vtkSMDoubleVectorProperty* NormalProperty;
  /// Used to avoid recursion when updating the visiblity checkbox
  bool IgnoreVisibilityWidget;
  /// Used to avoid recursion when updating the Qt widgets  
  bool IgnoreQtWidgets;
  /// Used to avoid recursion when updating the 3D widget
  bool Ignore3DWidget;
  /// Used to avoid recursion when updating the controlled properties
  bool IgnorePropertyChange;
  
  static QMap<pqSMProxy, bool> Visibility;
};

QMap<pqSMProxy, bool> pqImplicitPlaneWidget::pqImplementation::Visibility;

/////////////////////////////////////////////////////////////////////////
// pqImplicitPlaneWidget

pqImplicitPlaneWidget::pqImplicitPlaneWidget(QWidget* p) :
  QWidget(p),
  Implementation(new pqImplementation())
{
  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetStartDrag));
  this->Implementation->ChangeObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetChanged));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetEndDrag));
  this->Implementation->PropertyObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::onControlledPropertyChanged));
    
  this->Implementation->UI->setupUi(this);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this->Implementation->UI->originX,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->originY,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->originZ,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->normalX,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->normalY,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  connect(this->Implementation->UI->normalZ,
    SIGNAL(editingFinished()), this, SLOT(onQtWidgetChanged()));
  
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
}

pqImplicitPlaneWidget::~pqImplicitPlaneWidget()
{
  if(this->Implementation->OriginProperty)
    {
    this->Implementation->OriginProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }
  if(this->Implementation->NormalProperty)
    {
    this->Implementation->NormalProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }

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

void pqImplicitPlaneWidget::setDataSources(
  pqSMProxy reference_proxy,
  pqSMProxy controlled_proxy,
  vtkSMDoubleVectorProperty* origin_property,
  vtkSMDoubleVectorProperty* normal_property)
{
  if(this->Implementation->OriginProperty)
    {
    this->Implementation->OriginProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }
  if(this->Implementation->NormalProperty)
    {
    this->Implementation->NormalProperty->RemoveObserver(
      this->Implementation->PropertyObserver);
    }

  if(this->Implementation->Widget)
    {
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->EndDragObserver);
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->ChangeObserver);
    this->Implementation->Widget->RemoveObserver(
      this->Implementation->StartDragObserver);
    }
  
  this->Implementation->ReferenceProxy = reference_proxy;
  this->Implementation->ControlledProxy = controlled_proxy;
  this->Implementation->OriginProperty = origin_property;
  this->Implementation->NormalProperty = normal_property;
  
  if(!this->Implementation->Visibility.contains(this->Implementation->ReferenceProxy))
    {
    this->Implementation->Visibility.insert(
      this->Implementation->ReferenceProxy, this->Implementation->ReferenceProxy.GetPointer() ? true : false);
    }

  if(!this->Implementation->Widget)
    {
    // We won't have to do this once setProxy() takes pqProxy as an argument.
    pqServer* const server = pqApplicationCore::instance()->
      getServerManagerModel()->getServer(
        this->Implementation->ReferenceProxy->GetConnectionID());
          
    this->Implementation->Widget =
      pqApplicationCore::instance()->get3DWidgetFactory()->
        get3DWidget("ImplicitPlaneWidgetDisplay", server);

      
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

  if(this->Implementation->OriginProperty)
    {
    this->Implementation->OriginProperty->AddObserver(
      vtkCommand::ModifiedEvent,
      this->Implementation->PropertyObserver);
    }
  if(this->Implementation->NormalProperty)
    {
    this->Implementation->NormalProperty->AddObserver(
      vtkCommand::ModifiedEvent,
      this->Implementation->PropertyObserver);
    }

  this->Implementation->IgnoreVisibilityWidget = true;
  this->Implementation->UI->show3DWidget->setChecked(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
  this->Implementation->IgnoreVisibilityWidget = false;

  this->show3DWidget(this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
  
  this->reset();
}

void pqImplicitPlaneWidget::showWidget()
{
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqImplicitPlaneWidget::showPlane()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMIntVectorProperty* const show_plane =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("DrawPlane")))
      {
      show_plane->SetElement(0, true);
      this->Implementation->Widget->UpdateVTKObjects();
      }
    }
}

void pqImplicitPlaneWidget::accept()
{
  // Get the current values from the 3D implicit plane widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  this->get3DWidgetState(origin, normal);

  // Push the new values into the controlled properties ...
  this->Implementation->IgnorePropertyChange = true;
  
  if(this->Implementation->OriginProperty)
    {
    this->Implementation->OriginProperty->SetElements(origin);
    }

  if(this->Implementation->NormalProperty)
    {
    this->Implementation->NormalProperty->SetElements(normal);
    }

  if(this->Implementation->ControlledProxy)
    {
    this->Implementation->ControlledProxy->UpdateVTKObjects();
    }

  this->Implementation->IgnorePropertyChange = false;
}

void pqImplicitPlaneWidget::reset()
{
  // Restore the state of the implicit plane widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };

  if(this->Implementation->OriginProperty)
    {
    origin[0] = this->Implementation->OriginProperty->GetElement(0);
    origin[1] = this->Implementation->OriginProperty->GetElement(1);
    origin[2] = this->Implementation->OriginProperty->GetElement(2);
    }

  if(this->Implementation->NormalProperty)
    {
    normal[0] = this->Implementation->NormalProperty->GetElement(0);
    normal[1] = this->Implementation->NormalProperty->GetElement(1);
    normal[2] = this->Implementation->NormalProperty->GetElement(2);
    }

  this->set3DWidgetState(origin, normal);
  this->setQtWidgetState(origin, normal);
}

void pqImplicitPlaneWidget::hidePlane()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMIntVectorProperty* const show_plane =
      vtkSMIntVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("DrawPlane")))
      {
      show_plane->SetElement(0, false);
      this->Implementation->Widget->UpdateVTKObjects();
      }
    }
}

void pqImplicitPlaneWidget::hideWidget()
{
  this->show3DWidget(false);
}

void pqImplicitPlaneWidget::onShow3DWidget(bool show_widget)
{
  this->Implementation->Visibility.insert(
    this->Implementation->ReferenceProxy, show_widget);
    
  this->show3DWidget(
    this->Implementation->Visibility[this->Implementation->ReferenceProxy]);
}

void pqImplicitPlaneWidget::onResetBounds()
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
        input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 1.2;
        input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 1.2;
        input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 1.2;
        
        if(vtkSMDoubleVectorProperty* const origin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            this->Implementation->Widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          }
        
        if(vtkSMDoubleVectorProperty* const place_widget =
          vtkSMDoubleVectorProperty::SafeDownCast(
            this->Implementation->Widget->GetProperty("PlaceWidget")))
          {
          double widget_bounds[6];
          widget_bounds[0] = input_origin[0] - input_size[0];
          widget_bounds[1] = input_origin[0] + input_size[0];
          widget_bounds[2] = input_origin[1] - input_size[1];
          widget_bounds[3] = input_origin[1] + input_size[1];
          widget_bounds[4] = input_origin[2] - input_size[2];
          widget_bounds[5] = input_origin[2] + input_size[2];
          
          place_widget->SetElements(widget_bounds);
          
          this->Implementation->Widget->UpdateVTKObjects();
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqImplicitPlaneWidget::onUseCenterBounds()
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

        if(vtkSMDoubleVectorProperty* const origin =
          vtkSMDoubleVectorProperty::SafeDownCast(
            this->Implementation->Widget->GetProperty("Origin")))
          {
          origin->SetElements(input_origin);
          this->Implementation->Widget->UpdateVTKObjects();
          pqApplicationCore::instance()->render();
          }
        }
      }
    }
}

void pqImplicitPlaneWidget::onUseXNormal()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(1, 0, 0);
      this->Implementation->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqImplicitPlaneWidget::onUseYNormal()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 1, 0);
      this->Implementation->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqImplicitPlaneWidget::onUseZNormal()
{
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 0, 1);
      this->Implementation->Widget->UpdateVTKObjects();
      pqApplicationCore::instance()->render();
      }
    }
}

void pqImplicitPlaneWidget::onUseCameraNormal()
{
  if(this->Implementation->Widget)
    {
    if(vtkCamera* const camera =
      pqApplicationCore::instance()->getActiveRenderModule()->
        getRenderModuleProxy()->GetRenderer()->GetActiveCamera())
      {
      if(vtkSMDoubleVectorProperty* const normal =
        vtkSMDoubleVectorProperty::SafeDownCast(
          this->Implementation->Widget->GetProperty("Normal")))
        {
        double camera_normal[3];
        camera->GetViewPlaneNormal(camera_normal);
        normal->SetElements3(
          -camera_normal[0], -camera_normal[1], -camera_normal[2]);
        
        this->Implementation->Widget->UpdateVTKObjects();
        pqApplicationCore::instance()->render();
        }
      }
    }
}

void pqImplicitPlaneWidget::onQtWidgetChanged()
{
  if(this->Implementation->IgnoreQtWidgets)
    return;

  // Get the new values from the Qt widgets ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };

  origin[0] = this->Implementation->UI->originX->text().toDouble();
  origin[1] = this->Implementation->UI->originY->text().toDouble();
  origin[2] = this->Implementation->UI->originZ->text().toDouble();
  normal[0] = this->Implementation->UI->normalX->text().toDouble();
  normal[1] = this->Implementation->UI->normalY->text().toDouble();
  normal[2] = this->Implementation->UI->normalZ->text().toDouble();
  
  // Push the new values into the 3D widget ...
  this->set3DWidgetState(origin, normal);

  emit widgetChanged();
}

void pqImplicitPlaneWidget::on3DWidgetStartDrag()
{
  emit widgetStartInteraction();
}

void pqImplicitPlaneWidget::on3DWidgetChanged()
{
  if(this->Implementation->Ignore3DWidget)
    return;
    
  // Get the new values from the 3D widget ...
  double origin[3] = { 0, 0, 0 };
  double normal[3] = { 0, 0, 1 };
  this->get3DWidgetState(origin, normal);
  
  // Push the new values into the Qt widgets (ideally, this should happen automatically when the implicit plane is updated)
  this->setQtWidgetState(origin, normal);

  emit widgetChanged();
}

void pqImplicitPlaneWidget::on3DWidgetEndDrag()
{
  emit widgetEndInteraction();
}

void pqImplicitPlaneWidget::onControlledPropertyChanged()
{
  if(this->Implementation->IgnorePropertyChange)
    {
    return;
    }
    
  // Synchronize the 3D and Qt widgets with the controlled properties
  this->reset();
}

void pqImplicitPlaneWidget::show3DWidget(bool show_widget)
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

void pqImplicitPlaneWidget::get3DWidgetState(double* origin, double* normal)
{
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Origin")))
      {
      origin[0] = widget_origin->GetElement(0);
      origin[1] = widget_origin->GetElement(1);
      origin[2] = widget_origin->GetElement(2);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Normal")))
      {
      normal[0] = widget_normal->GetElement(0);
      normal[1] = widget_normal->GetElement(1);
      normal[2] = widget_normal->GetElement(2);
      }
    }
}

void pqImplicitPlaneWidget::set3DWidgetState(const double* origin, const double* normal)
{
  this->Implementation->Ignore3DWidget = true;
   
  if(this->Implementation->Widget)
    {
    if(vtkSMDoubleVectorProperty* const widget_origin =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Origin")))
      {
      widget_origin->SetElements(origin);
      }

    if(vtkSMDoubleVectorProperty* const widget_normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        this->Implementation->Widget->GetProperty("Normal")))
      {
      widget_normal->SetElements(normal);
      }
    
    this->Implementation->Widget->UpdateVTKObjects();
    
    pqApplicationCore::instance()->render();
    }
    
  this->Implementation->Ignore3DWidget = false;
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
