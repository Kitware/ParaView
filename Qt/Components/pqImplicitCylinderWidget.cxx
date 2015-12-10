/*=========================================================================

   Program: ParaView
   Module:    pqImplicitCylinderWidget.cxx

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
#include "pqImplicitCylinderWidget.h"
#include "ui_pqImplicitCylinderWidget.h"

#include "pq3DWidgetFactory.h"
#include "pqApplicationCore.h"
#include "pqPipelineFilter.h"
#include "pqPipelineSource.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"

#include <QDoubleValidator>

#include "vtkMath.h"
#include "vtkBoundingBox.h"
#include "vtkPVDataInformation.h"
#include "vtkRenderer.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxyProperty.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMSourceProxy.h"

/////////////////////////////////////////////////////////////////////////
// pqImplicitCylinderWidget::pqImplementation

class pqImplicitCylinderWidget::pqImplementation
{
public:
  pqImplementation() :
    UI(new Ui::pqImplicitCylinderWidget())
  {
  }

  ~pqImplementation()
  {
    delete this->UI;
  }

  /// Stores the Qt widgets
  Ui::pqImplicitCylinderWidget* const UI;

  pqPropertyLinks Links;
};

#define PVCYLINDERWIDGET_TRIGGER_RENDER(ui)  \
  QObject::connect(this->Implementation->UI->ui,\
    SIGNAL(editingFinished()),\
    this, SLOT(render()), Qt::QueuedConnection);

/////////////////////////////////////////////////////////////////////////
// pqImplicitCylinderWidget

pqImplicitCylinderWidget::pqImplicitCylinderWidget(vtkSMProxy* o, vtkSMProxy* pxy, QWidget* p) :
  Superclass(o, pxy, p),
  Implementation(new pqImplementation())
{
  this->Implementation->UI->setupUi(this);
  this->Implementation->UI->show3DWidget->setChecked(this->widgetVisible());

  // Set validators for all line edit boxes.
  QDoubleValidator* validator = new QDoubleValidator(this);
  this->Implementation->UI->centerX->setValidator(validator);
  this->Implementation->UI->centerY->setValidator(validator);
  this->Implementation->UI->centerZ->setValidator(validator);
  this->Implementation->UI->axisX->setValidator(validator);
  this->Implementation->UI->axisY->setValidator(validator);
  this->Implementation->UI->axisZ->setValidator(validator);

  validator = new QDoubleValidator(this);
  validator->setBottom(0.0);
  this->Implementation->UI->radius->setValidator(validator);

  // Trigger a render when use explicitly edits the positions.
  PVCYLINDERWIDGET_TRIGGER_RENDER(centerX);
  PVCYLINDERWIDGET_TRIGGER_RENDER(centerY);
  PVCYLINDERWIDGET_TRIGGER_RENDER(centerZ);
  PVCYLINDERWIDGET_TRIGGER_RENDER(axisX);
  PVCYLINDERWIDGET_TRIGGER_RENDER(axisY);
  PVCYLINDERWIDGET_TRIGGER_RENDER(axisZ);
  PVCYLINDERWIDGET_TRIGGER_RENDER(radius);

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(onShow3DWidget(bool)));

  connect(this, SIGNAL(widgetVisibilityChanged(bool)),
    this, SLOT(onWidgetVisibilityChanged(bool)));

  connect(this->Implementation->UI->show3DWidget,
    SIGNAL(toggled(bool)), this, SLOT(setWidgetVisible(bool)));

  connect(this->Implementation->UI->tubing,
    SIGNAL(toggled(bool)), this, SLOT(onTubing(bool)));
  connect(this->Implementation->UI->outlineTranslation,
    SIGNAL(toggled(bool)), this, SLOT(onOutlineTranslation(bool)));
  connect(this->Implementation->UI->outsideBounds,
    SIGNAL(toggled(bool)), this, SLOT(onOutsideBounds(bool)));
  connect(this->Implementation->UI->scaling,
    SIGNAL(toggled(bool)), this, SLOT(onScaling(bool)));

  connect(this->Implementation->UI->resetBounds,
    SIGNAL(clicked()), this, SLOT(resetBounds()));

  connect(&this->Implementation->Links, SIGNAL(qtWidgetChanged()),
    this, SLOT(setModified()));

  pqServerManagerModel* smmodel =
    pqApplicationCore::instance()->getServerManagerModel();
  this->createWidget(smmodel->findServer(o->GetSession()));
}

//-----------------------------------------------------------------------------
pqImplicitCylinderWidget::~pqImplicitCylinderWidget()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
#define PVCYLINDERWIDGET_LINK(ui, smproperty, index)\
{\
  this->Implementation->Links.addPropertyLink(\
    this->Implementation->UI->ui, "text2",\
    SIGNAL(textChanged(const QString&)),\
    widget, widget->GetProperty(smproperty), index);\
}

#define PVCYLINDERWIDGET_LINK2(ui, signal, smproperty)\
{\
  this->Implementation->Links.addPropertyLink(\
    this->Implementation->UI->ui, "checked",\
    SIGNAL(toggled(bool)),\
    widget, widget->GetProperty(smproperty));\
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::createWidget(pqServer* server)
{
  vtkSMNewWidgetRepresentationProxy* widget =
    pqApplicationCore::instance()->get3DWidgetFactory()->
    get3DWidget("ImplicitCylinderWidgetRepresentation", server, this->getReferenceProxy());
  this->setWidgetProxy(widget);
  widget->UpdateVTKObjects();
  widget->UpdatePropertyInformation();

  // Now bind the GUI widgets to the 3D widget.
  PVCYLINDERWIDGET_LINK(centerX, "Center", 0);
  PVCYLINDERWIDGET_LINK(centerY, "Center", 1);
  PVCYLINDERWIDGET_LINK(centerZ, "Center", 2);
  PVCYLINDERWIDGET_LINK(axisX, "Axis", 0);
  PVCYLINDERWIDGET_LINK(axisY, "Axis", 1);
  PVCYLINDERWIDGET_LINK(axisZ, "Axis", 2);
  PVCYLINDERWIDGET_LINK(radius, "Radius", 0);

  PVCYLINDERWIDGET_LINK2(tubing, onTubing, "Tubing");
  PVCYLINDERWIDGET_LINK2(outlineTranslation, onOutlineTranslation, "OutlineTranslation");
  PVCYLINDERWIDGET_LINK2(outsideBounds, onOutsideBounds, "OutsideBounds");
  PVCYLINDERWIDGET_LINK2(scaling, onScaling, "ScaleEnabled");
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onWidgetVisibilityChanged(bool visible)
{
  this->Implementation->UI->show3DWidget->blockSignals(true);
  this->Implementation->UI->show3DWidget->setChecked(visible);
  this->Implementation->UI->show3DWidget->blockSignals(false);
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::select()
{
  double input_bounds[6];
  if (this->getReferenceInputBounds(input_bounds))
    {
    this->resetBounds(input_bounds);
    }

  this->Superclass::select();
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::showCylinder()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    vtkSMPropertyHelper(widget, "DrawCylinder").Set(1);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::hideCylinder()
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    vtkSMPropertyHelper(widget, "DrawCylinder").Set(0);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onShow3DWidget(bool show_widget)
{
  if (show_widget)
    {
    this->showCylinder();
    }
  else
    {
    this->hideCylinder();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::resetBounds(double input_bounds[6])
{
  vtkSMNewWidgetRepresentationProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    vtkBoundingBox box(input_bounds);
    double center[3];
    box.GetCenter(center);
    const double bnds[6] = { 0., 1., 0., 1., 0., 1. };
    vtkSMPropertyHelper(widget, "Center").Set(center, 3);
    vtkSMPropertyHelper(widget, "Radius").Set(box.GetMaxLength()/10.0);
    vtkSMPropertyHelper(widget, "PlaceWidget").Set(bnds, 6);
    widget->UpdateProperty("PropertyWidget", true);
    vtkSMPropertyHelper(widget, "PlaceWidget").Set(input_bounds, 6);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::accept()
{
  this->Superclass::accept();
  this->hideCylinder();
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::reset()
{
  this->Superclass::reset();
  this->hideCylinder();
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onTubing(bool b)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqSMAdaptor::setElementProperty(widget->GetProperty("Tubing"), b ? 1 : 0);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onOutlineTranslation(bool b)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqSMAdaptor::setElementProperty(widget->GetProperty("OutlineTranslation"), b ? 1 : 0);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onOutsideBounds(bool b)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqSMAdaptor::setElementProperty(widget->GetProperty("OutsideBounds"), b ? 1 : 0);
    widget->UpdateVTKObjects();
    }
}

//-----------------------------------------------------------------------------
void pqImplicitCylinderWidget::onScaling(bool b)
{
  vtkSMProxy* widget = this->getWidgetProxy();
  if (widget)
    {
    pqSMAdaptor::setElementProperty(widget->GetProperty("ScaleEnabled"), b ? 1 : 0);
    widget->UpdateVTKObjects();
    }
}
