/*=========================================================================

   Program: ParaView
   Module:  pqCylinderPropertyWidget.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
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

========================================================================*/
#include "pqCylinderPropertyWidget.h"
#include "ui_pqCylinderPropertyWidget.h"

#include "vtkSMPropertyGroup.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyHelper.h"

#include <QDoubleValidator>

//-----------------------------------------------------------------------------
pqCylinderPropertyWidget::pqCylinderPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
    "representations", "ImplicitCylinderWidgetRepresentation",
    smproxy, smgroup, parentObject)
{
  Ui::CylinderPropertyWidget ui;
  ui.setupUi(this);

  new QDoubleValidator(ui.centerX);
  new QDoubleValidator(ui.centerY);
  new QDoubleValidator(ui.centerZ);
  new QDoubleValidator(ui.axisX);
  new QDoubleValidator(ui.axisY);
  new QDoubleValidator(ui.axisZ);
  new QDoubleValidator(ui.radius);

  if (vtkSMProperty* center = smgroup->GetProperty("Center"))
    {
    this->addPropertyLink(
      ui.centerX, "text2", SIGNAL(textChangedAndEditingFinished()), center, 0);
    this->addPropertyLink(
      ui.centerY, "text2", SIGNAL(textChangedAndEditingFinished()), center, 1);
    this->addPropertyLink(
      ui.centerZ, "text2", SIGNAL(textChangedAndEditingFinished()), center, 2);
    ui.centerLabel->setText(center->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for function 'Center'.");
    }

  if (vtkSMProperty* axis = smgroup->GetProperty("Axis"))
    {
    this->addPropertyLink(
      ui.axisX, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 0);
    this->addPropertyLink(
      ui.axisY, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 1);
    this->addPropertyLink(
      ui.axisZ, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 2);
    ui.axisLabel->setText(axis->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for function 'Axis'.");
    }

  if (vtkSMProperty* radius = smgroup->GetProperty("Radius"))
    {
    this->addPropertyLink(
      ui.radius, "text2", SIGNAL(textChangedAndEditingFinished()), radius);
    ui.radiusLabel->setText(radius->GetXMLLabel());
    }
  else
    {
    qCritical("Missing required property for function 'Radius'.");
    }

  if (smgroup->GetProperty("Input"))
    {
    this->connect(ui.resetBounds, SIGNAL(clicked()), SLOT(resetBounds()));
    }
  else
    {
    ui.resetBounds->hide();
    }

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  vtkSMProxy* wdgProxy = this->widgetProxy();
  this->WidgetLinks.addPropertyLink(ui.tubing, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("Tubing"));
  this->WidgetLinks.addPropertyLink(ui.outsideBounds, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("OutsideBounds"));
  this->WidgetLinks.addPropertyLink(ui.scaling, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("ScaleEnabled"));
  this->WidgetLinks.addPropertyLink(ui.outlineTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("OutlineTranslation"));
  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  this->placeWidget();
}

//-----------------------------------------------------------------------------
pqCylinderPropertyWidget::~pqCylinderPropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
    {
    return;
    }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  Q_ASSERT(wdgProxy);

  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::resetBounds()
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  Q_ASSERT(wdgProxy);

  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
    {
    return;
    }

  if (wdgProxy)
    {
    double center[3];
    bbox.GetCenter(center);
    const double bnds[6] = { 0., 1., 0., 1., 0., 1. };
    vtkSMPropertyHelper(wdgProxy, "Center").Set(center, 3);
    vtkSMPropertyHelper(wdgProxy, "Radius").Set(bbox.GetMaxLength()/10.0);
    vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(bnds, 6);
    wdgProxy->UpdateProperty("PropertyWidget", true);

    double input_bounds[6];
    bbox.GetBounds(input_bounds);
    vtkSMPropertyHelper(wdgProxy, "PlaceWidget").Set(input_bounds, 6);
    wdgProxy->UpdateVTKObjects();

    emit this->changeAvailable();
    this->render();
    }
}
