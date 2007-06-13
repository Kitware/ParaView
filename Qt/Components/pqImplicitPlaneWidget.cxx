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
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqSMSignalAdaptors.h"

#include "ui_pqImplicitPlaneWidget.h"

#include <QDoubleValidator>

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
#include <vtkSMRenderViewProxy.h>
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

pqImplicitPlaneWidget::pqImplicitPlaneWidget(pqProxy* o, vtkSMProxy* pxy, QWidget* p) :
  Superclass(o, pxy, p),
  Implementation(new pqImplementation())
{
  this->ScaleFactor[0] = 1;
  this->ScaleFactor[1] = 1;
  this->ScaleFactor[2] = 1;

  this->ScaleOrigin[0] = 0;
  this->ScaleOrigin[1] = 0;
  this->ScaleOrigin[2] = 0;

  this->Implementation->StartDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetStartDrag));
  this->Implementation->EndDragObserver.TakeReference(
    vtkMakeMemberFunctionCommand(*this, &pqImplicitPlaneWidget::on3DWidgetEndDrag));
    
  this->Implementation->UI->setupUi(this);
  this->Implementation->UI->show3DWidget->setChecked(this->widgetVisible());

  // Set validators for all line edit boxes.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->UI->originX->setValidator(validator);
  this->Implementation->UI->originY->setValidator(validator);
  this->Implementation->UI->originZ->setValidator(validator);
  this->Implementation->UI->normalX->setValidator(validator);
  this->Implementation->UI->normalY->setValidator(validator);
  this->Implementation->UI->normalZ->setValidator(validator);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));
  QObject::connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

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
    this, SLOT(setModified()));

  // Trigger a render when use explicitly edits the positions.
  QObject::connect(this->Implementation->UI->originX, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->originY, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->originZ,
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->normalX, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->normalY, 
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);
  QObject::connect(this->Implementation->UI->normalZ,
    SIGNAL(editingFinished()), 
    this, SLOT(render()), Qt::QueuedConnection);

  this->createWidget(o->getServer());
}

pqImplicitPlaneWidget::~pqImplicitPlaneWidget()
{
  this->cleanupWidget();

  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::createWidget(pqServer* server)
{
  vtkSMNew3DWidgetProxy* widget = 
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ImplicitPlaneWidgetDisplay", server);
  this->setWidgetProxy(widget);
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();
  widget->AddObserver(vtkCommand::StartInteractionEvent,
    this->Implementation->StartDragObserver);
  widget->AddObserver(vtkCommand::EndInteractionEvent,
    this->Implementation->EndDragObserver);

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

}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::cleanupWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    widget->RemoveObserver(
      this->Implementation->EndDragObserver);
    widget->RemoveObserver(
      this->Implementation->StartDragObserver);
    pqApplicationCore::instance()->get3DWidgetFactory()->
      free3DWidget(widget);
    }
  this->setWidgetProxy(0);
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
  this->Superclass::setControlledProperty(function, controlled_property);
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::setOriginProperty(vtkSMProperty* origin_property)
{
  this->Implementation->OriginProperty = 
    vtkSMDoubleVectorProperty::SafeDownCast(origin_property);
  if (origin_property->GetXMLLabel())
    {
    this->Implementation->UI->labelOrigin->setText(
      origin_property->GetXMLLabel());
    }
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::setNormalProperty(vtkSMProperty* normal_property)
{
  this->Implementation->NormalProperty = 
    vtkSMDoubleVectorProperty::SafeDownCast(normal_property);
  if (normal_property->GetXMLLabel())
    {
    this->Implementation->UI->labelNormal->setText(
      normal_property->GetXMLLabel());
    }
}


//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);
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
void pqImplicitPlaneWidget::setScaleFactor(double scale[3])
{
  this->ScaleFactor[0] = scale[0];
  this->ScaleFactor[1] = scale[1];
  this->ScaleFactor[2] = scale[2];
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::setScaleOrigin(double orgin[3])
{
  this->ScaleOrigin[0] = orgin[0];
  this->ScaleOrigin[1] = orgin[1];
  this->ScaleOrigin[2] = orgin[2];
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
  double input_bounds[6];
  if(!widget || !this->getReferenceInputBounds(input_bounds))
    {
    return;
    }
  
  double input_origin[3];
  input_origin[0] = (input_bounds[0] + input_bounds[1]) / 2.0;
  input_origin[1] = (input_bounds[2] + input_bounds[3]) / 2.0;
  input_origin[2] = (input_bounds[4] + input_bounds[5]) / 2.0;

  double input_size[3];
  input_size[0] = fabs(input_bounds[1] - input_bounds[0]) * 1.2;
  input_size[1] = fabs(input_bounds[3] - input_bounds[2]) * 1.2;
  input_size[2] = fabs(input_bounds[5] - input_bounds[4]) * 1.2;

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

    if (this->ScaleFactor[0] != 1 || this->ScaleFactor[1] != 1
      || this->ScaleFactor[2] != 1)
      {
      // We have some scale factor specified.
      // widget bounds need to be scaled.
      widget_bounds[0] = (widget_bounds[0] - this->ScaleOrigin[0])* 
        this->ScaleFactor[0] + this->ScaleOrigin[0];
      widget_bounds[1] = (widget_bounds[1] - this->ScaleOrigin[0])* 
        this->ScaleFactor[0] + this->ScaleOrigin[0];

      widget_bounds[2] = (widget_bounds[2] - this->ScaleOrigin[1])* 
        this->ScaleFactor[1] + this->ScaleOrigin[1];
      widget_bounds[3] = (widget_bounds[3] - this->ScaleOrigin[1])* 
        this->ScaleFactor[1] + this->ScaleOrigin[1];

      widget_bounds[4] = (widget_bounds[4] - this->ScaleOrigin[2])* 
        this->ScaleFactor[2] + this->ScaleOrigin[2];
      widget_bounds[5] = (widget_bounds[5] - this->ScaleOrigin[2])* 
        this->ScaleFactor[2] + this->ScaleOrigin[2];
      }


    place_widget->SetElements(widget_bounds);
    widget->UpdateVTKObjects(); 
    vtkSMDoubleVectorProperty* const origin = 
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Origin"));
    if(origin)
      {
      origin->SetElements(input_origin);
      }
    widget->UpdateProperty("Origin");
    if(this->renderView())
      {
      this->renderView()->render();
      }
    this->setModified();
    }
}

void pqImplicitPlaneWidget::accept()
{
  Superclass::accept();
  this->hidePlane();
}

void pqImplicitPlaneWidget::reset()
{
  Superclass::reset();
  this->hidePlane();
}


void pqImplicitPlaneWidget::onUseCenterBounds()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMProxyProperty* const input_property =
      vtkSMProxyProperty::SafeDownCast(
        this->proxy()->GetProperty("Input")))
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
          if(this->renderView())
            {
            this->renderView()->render();
            }
          this->setModified();
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
      if(this->renderView())
        {
        this->renderView()->render();
        }
      this->setModified();
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
      if(this->renderView())
        {
        this->renderView()->render();
        }
      this->setModified();
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
      if(this->renderView())
        {
        this->renderView()->render();
        }
      this->setModified();
      }
    }
}

void pqImplicitPlaneWidget::onUseCameraNormal()
{
  vtkSMNew3DWidgetProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkCamera* const camera = this->renderView()->getRenderViewProxy()->GetActiveCamera())
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
        if(this->renderView())
          {
          this->renderView()->render();
          }
        this->setModified();
        }
      }
    }
}

void pqImplicitPlaneWidget::on3DWidgetStartDrag()
{
  this->setModified();
  emit widgetStartInteraction();
  this->showPlane();
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
    
    if(this->getRenderModule())
      {
      this->getRenderModule()->render();
      }
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

