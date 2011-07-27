/*=========================================================================

   Program: ParaView
   Module:    pqImplicitPlaneWidget.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#include "pqImplicitPlaneWidget.h"
#include "ui_pqImplicitPlaneWidget.h"

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqServerManagerModel.h"

#include <QDoubleValidator>

#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkPVDataInformation.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

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
  
  vtkSMDoubleVectorProperty* OriginProperty;
  vtkSMDoubleVectorProperty* NormalProperty;
  pqPropertyLinks Links;
};

namespace
{
  // implicit plane widget does not like it when any of the dimensions is 0. So
  // we ensure that each dimension has some thickness.
  static void pqFixBounds(vtkBoundingBox& bbox)
    {
    double max_length = bbox.GetMaxLength();
    max_length = max_length > 0? max_length * 0.05 : 1;
    double min_point[3], max_point[3];
    bbox.GetMinPoint(min_point[0], min_point[1], min_point[2]);
    bbox.GetMaxPoint(max_point[0], max_point[1], max_point[2]);
    for (int cc=0; cc < 3; cc++)
      {
      if (bbox.GetLength(cc) == 0)
        {
        min_point[cc] -= max_length;
        max_point[cc] += max_length;
        }
      }
    bbox.SetMinPoint(min_point);
    bbox.SetMaxPoint(max_point);
    }
}

/////////////////////////////////////////////////////////////////////////
// pqImplicitPlaneWidget

pqImplicitPlaneWidget::pqImplicitPlaneWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p) :
  Superclass(o, pxy, p),
  Implementation(new pqImplementation())
{
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
    SIGNAL(clicked()), this, SLOT(resetBounds()));
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

  // We need to mark the plane when inteaction starts.
  QObject::connect(this, SIGNAL(widgetStartInteraction()),
    this, SLOT(onStartInteraction()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(o->GetSession()));
}

pqImplicitPlaneWidget::~pqImplicitPlaneWidget()
{
  this->cleanupWidget();

  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget = 
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ImplicitPlaneWidgetRepresentation", server);
  this->setWidgetProxy(widget);
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  // Now bind the GUI widgets to the 3D widget.

  // The adaptor is used to format the text value.

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->originX, 
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Origin"), 0);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->originY,
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Origin"), 1);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->originZ,
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Origin"), 2);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->normalX,
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Normal"), 0);

  this->Implementation->Links.addPropertyLink(
    this->Implementation->UI->normalY,
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Normal"), 1);

  this->Implementation->Links.addPropertyLink(
  this->Implementation->UI->normalZ,
    "text2", SIGNAL(textChanged(const QString&)),
    widget, widget->GetProperty("Normal"), 2);
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::cleanupWidget()
{
  this->Implementation->Links.removeAllPropertyLinks();
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
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

//-----------------------------------------------------------------------------
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
/// Need to update the widget bounds.
void pqImplicitPlaneWidget::select()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_bounds[6];
  if (!widget || !this->getReferenceInputBounds(input_bounds))
    {
    return;
    }

  double center[3];
  vtkSMPropertyHelper(widget, "Origin").Get(center, 3);
  vtkBoundingBox box(input_bounds);
  box.AddPoint(center);
  pqFixBounds(box);
  box.GetBounds(input_bounds);  

  vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
  widget->UpdateVTKObjects();
  vtkSMPropertyHelper(widget, "Origin").Set(center, 3);
  widget->UpdateVTKObjects();

  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::resetBounds(double input_bounds[6])
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  vtkBoundingBox box(input_bounds);
  pqFixBounds(box);

  double input_origin[3];
  box.GetCenter(input_origin);

  double bounds[6];
  box.GetBounds(bounds);

  widget->InvokeCommand("Reset");
  vtkSMPropertyHelper(widget, "PlaceWidget").Set(bounds, 6);
  widget->UpdateVTKObjects();
  vtkSMPropertyHelper(widget, "Origin").Set(input_origin, 3);
  widget->UpdateVTKObjects();

  this->setModified();
  this->render();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::accept()
{
  this->Superclass::accept();
  this->hidePlane();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::reset()
{
  this->Superclass::reset();
  this->hidePlane();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onUseCenterBounds()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  double input_bounds[6];
  if(!widget || !this->getReferenceInputBounds(input_bounds))
    {
    return;
    }

  vtkBoundingBox box(input_bounds);
  pqFixBounds(box);
  double input_origin[3];
  box.GetCenter(input_origin);
  vtkSMPropertyHelper(widget, "Origin").Set(input_origin, 3);
  widget->UpdateVTKObjects();
  this->render();
  this->setModified();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onUseXNormal()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(1, 0, 0);
      widget->UpdateVTKObjects();
      this->render();
      this->setModified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onUseYNormal()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 1, 0);
      widget->UpdateVTKObjects();
      this->render();
      this->setModified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onUseZNormal()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if(widget)
    {
    if(vtkSMDoubleVectorProperty* const normal =
      vtkSMDoubleVectorProperty::SafeDownCast(
        widget->GetProperty("Normal")))
      {
      normal->SetElements3(0, 0, 1);
      widget->UpdateVTKObjects();
      this->render();
      this->setModified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onUseCameraNormal()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqRenderView* renView = qobject_cast<pqRenderView*>(this->renderView());
    if (vtkCamera* const camera = renView? 
      renView->getRenderViewProxy()->GetActiveCamera() : 0)
      {
      double camera_normal[3];
      camera->GetViewPlaneNormal(camera_normal);
      camera_normal[0] = -camera_normal[0];
      camera_normal[1] = -camera_normal[1];
      camera_normal[2] = -camera_normal[2];
      vtkSMPropertyHelper(widget, "Normal").Set(camera_normal, 3);
      widget->UpdateVTKObjects();
      this->render();
      this->setModified();
      }
    }
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::onStartInteraction()
{
  this->showPlane();
}

//-----------------------------------------------------------------------------
void pqImplicitPlaneWidget::get3DWidgetState(double* origin, double* normal)
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    vtkSMPropertyHelper originHelper(widget, "Origin");
    origin[0] = originHelper.GetAsDouble(0);
    origin[1] = originHelper.GetAsDouble(1);
    origin[2] = originHelper.GetAsDouble(2);

    vtkSMPropertyHelper normalHelper(widget, "Normal");
    normal[0] = normalHelper.GetAsDouble(0);
    normal[1] = normalHelper.GetAsDouble(1);
    normal[2] = normalHelper.GetAsDouble(2);
    }
}

