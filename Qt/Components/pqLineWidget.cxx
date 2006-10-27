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
#include "pqPropertyLinks.h"
#include "pqRenderModule.h"
#include "pqSMSignalAdaptors.h"
#include "pqServerManagerModel.h"

#include "ui_pqLineWidget.h"

#include <vtkMemberFunctionCommand.h>
#include <vtkPVDataInformation.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMNew3DWidgetProxy.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>

/////////////////////////////////////////////////////////////////////////
// pqLineWidget::pqImplementation

class pqLineWidget::pqImplementation
{
public:
  pqImplementation() :
    WidgetPoint1(0),
    WidgetPoint2(0)
  {
    this->Links.setUseUncheckedProperties(false);
    this->Links.setAutoUpdateVTKObjects(true);
  }
  
  ~pqImplementation()
  {
  }
  
  /// Stores the Qt widgets
  Ui::pqLineWidget UI;
  
  /// Stores the 3D widget properties
  vtkSMDoubleVectorProperty* WidgetPoint1;
  vtkSMDoubleVectorProperty* WidgetPoint2;
  
  /// Maps Qt widgets to the 3D widget
  pqPropertyLinks Links;
};

/////////////////////////////////////////////////////////////////////////
// pqLineWidget

pqLineWidget::pqLineWidget(QWidget* p) :
  Superclass(p),
  Implementation(new pqImplementation())
{
  this->Implementation->UI.setupUi(this);
  this->Implementation->UI.visible->setChecked(this->widgetVisible());

  QObject::connect(this->Implementation->UI.visible,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  QObject::connect(this->Implementation->UI.xAxis,
    SIGNAL(clicked()), this, SLOT(onXAxis()));
  QObject::connect(this->Implementation->UI.yAxis,
    SIGNAL(clicked()), this, SLOT(onYAxis()));
  QObject::connect(this->Implementation->UI.zAxis,
    SIGNAL(clicked()), this, SLOT(onZAxis()));

  QObject::connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SIGNAL(widgetChanged()));

  QObject::connect(&this->Implementation->Links, SIGNAL(smPropertyChanged()),
    this, SIGNAL(widgetChanged()));
}

pqLineWidget::~pqLineWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();
  
  if(vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy())
    {
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
      
    this->setWidgetProxy(0);
    }

  delete this->Implementation;
}

void pqLineWidget::setControlledProxy(vtkSMProxy* proxy)
{
  if (!this->getWidgetProxy())
    {
    pqServerManagerModel* smModel = 
      pqApplicationCore::instance()->getServerManagerModel();
    this->createWidget(smModel->getServer(proxy->GetConnectionID()));
    }

  this->Superclass::setControlledProxy(proxy);
}

void pqLineWidget::setControlledProperties(vtkSMProperty* point1, vtkSMProperty* point2)
{
  this->Implementation->WidgetPoint1->Copy(point1);
  this->Implementation->WidgetPoint2->Copy(point2);

  // Map widget properties to controlled properties ...
  this->setControlledProperty(this->Implementation->WidgetPoint1, point1);
  this->setControlledProperty(this->Implementation->WidgetPoint2, point2);
}

void pqLineWidget::onXAxis()
{
  double object_center[3];
  double object_size[3];
  this->getReferenceBoundingBox(object_center, object_size);
       
  if(this->Implementation->WidgetPoint1 && this->Implementation->WidgetPoint2)
    {
    this->Implementation->WidgetPoint1->SetElement(0, object_center[0] - object_size[0] * 0.6);
    this->Implementation->WidgetPoint1->SetElement(1, object_center[1]);
    this->Implementation->WidgetPoint1->SetElement(2, object_center[2]);

    this->Implementation->WidgetPoint2->SetElement(0, object_center[0] + object_size[0] * 0.6);
    this->Implementation->WidgetPoint2->SetElement(1, object_center[1]);
    this->Implementation->WidgetPoint2->SetElement(2, object_center[2]);
  
    this->getWidgetProxy()->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    }
}

void pqLineWidget::onYAxis()
{
  double object_center[3];
  double object_size[3];
  this->getReferenceBoundingBox(object_center, object_size);
       
  if(this->Implementation->WidgetPoint1 && this->Implementation->WidgetPoint2)
    {
    this->Implementation->WidgetPoint1->SetElement(0, object_center[0]);
    this->Implementation->WidgetPoint1->SetElement(1, object_center[1] - object_size[1] * 0.6);
    this->Implementation->WidgetPoint1->SetElement(2, object_center[2]);

    this->Implementation->WidgetPoint2->SetElement(0, object_center[0]);
    this->Implementation->WidgetPoint2->SetElement(1, object_center[1] + object_size[1] * 0.6);
    this->Implementation->WidgetPoint2->SetElement(2, object_center[2]);
  
    this->getWidgetProxy()->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    }
}

void pqLineWidget::onZAxis()
{
  double object_center[3];
  double object_size[3];
  this->getReferenceBoundingBox(object_center, object_size);
       
  if(this->Implementation->WidgetPoint1 && this->Implementation->WidgetPoint2)
    {
    this->Implementation->WidgetPoint1->SetElement(0, object_center[0]);
    this->Implementation->WidgetPoint1->SetElement(1, object_center[1]);
    this->Implementation->WidgetPoint1->SetElement(2, object_center[2] - object_size[2] * 0.6);

    this->Implementation->WidgetPoint2->SetElement(0, object_center[0]);
    this->Implementation->WidgetPoint2->SetElement(1, object_center[1]);
    this->Implementation->WidgetPoint2->SetElement(2, object_center[2] + object_size[2] * 0.6);
  
    this->getWidgetProxy()->UpdateVTKObjects();
    pqApplicationCore::instance()->render();
    }
}

void pqLineWidget::createWidget(pqServer* server)
{
  vtkSMNew3DWidgetProxy* const widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
      get3DWidget("LineSourceWidgetDisplay", server);
  this->setWidgetProxy(widget);
  
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  
  this->Implementation->WidgetPoint1 = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("Point1WorldPosition"));
  this->Implementation->WidgetPoint2 = vtkSMDoubleVectorProperty::SafeDownCast(
    widget->GetProperty("Point2WorldPosition"));

  pqSignalAdaptorDouble* adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point1X, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint1, 0);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point1Y, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint1, 1);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point1Z, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint1, 2);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point2X, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint2, 0);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point2Y, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint2, 1);

  adaptor = new pqSignalAdaptorDouble(
    this->Implementation->UI.point2Z, "text",
    SIGNAL(textChanged(const QString&)));
  this->Implementation->Links.addPropertyLink(
    adaptor, "value", SIGNAL(valueChanged(const QString&)),
    widget, this->Implementation->WidgetPoint2, 2);

 }

void pqLineWidget::resetBounds()
{
}

void pqLineWidget::getReferenceBoundingBox(double center[3], double sz[3])
{
  if(this->getReferenceProxy())
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

        center[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
        center[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
        center[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

        sz[0] = fabs(input_bounds[1] - input_bounds[0]);
        sz[1] = fabs(input_bounds[3] - input_bounds[2]);
        sz[2] = fabs(input_bounds[5] - input_bounds[4]);
        }
      }
    }
}

void pqLineWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->UI.visible->blockSignals(true);
  this->Implementation->UI.visible->setChecked(visible);
  this->Implementation->UI.visible->blockSignals(false);
}
