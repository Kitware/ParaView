/*=========================================================================

   Program: ParaView
   Module:  pqSpherePropertyWidget.cxx

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
#include "pqSpherePropertyWidget.h"
#include "ui_pqSpherePropertyWidget.h"

#include "pqPointPickingHelper.h"
#include "vtkBoundingBox.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

//-----------------------------------------------------------------------------
pqSpherePropertyWidget::pqSpherePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "SphereWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::SpherePropertyWidget ui;
  ui.setupUi(this);

  if (vtkSMProperty* center = smgroup->GetProperty("Center"))
  {
    this->addPropertyLink(ui.centerX, "text2", SIGNAL(textChangedAndEditingFinished()), center, 0);
    this->addPropertyLink(ui.centerY, "text2", SIGNAL(textChangedAndEditingFinished()), center, 1);
    this->addPropertyLink(ui.centerZ, "text2", SIGNAL(textChangedAndEditingFinished()), center, 2);
    ui.centerLabel->setText(center->GetXMLLabel());
    ui.pickLabel->setText(
      ui.pickLabel->text().replace("'Center'", QString("'%1'").arg(center->GetXMLLabel())));
    QString tooltip = this->getTooltip(center);
    ui.centerX->setToolTip(tooltip);
    ui.centerY->setToolTip(tooltip);
    ui.centerZ->setToolTip(tooltip);
    ui.centerLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Center'.");
  }

  if (vtkSMProperty* normal = smgroup->GetProperty("Normal"))
  {
    this->addPropertyLink(ui.normalX, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 0);
    this->addPropertyLink(ui.normalY, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 1);
    this->addPropertyLink(ui.normalZ, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 2);
    ui.normalLabel->setText(normal->GetXMLLabel());
    QString tooltip = this->getTooltip(normal);
    ui.normalX->setToolTip(tooltip);
    ui.normalY->setToolTip(tooltip);
    ui.normalZ->setToolTip(tooltip);
    ui.normalLabel->setToolTip(tooltip);
  }
  else
  {
    ui.normalLabel->hide();
    ui.normalX->hide();
    ui.normalY->hide();
    ui.normalZ->hide();
  }

  if (vtkSMProperty* radius = smgroup->GetProperty("Radius"))
  {
    this->addPropertyLink(ui.radius, "text2", SIGNAL(textChangedAndEditingFinished()), radius);
    ui.radiusLabel->setText(radius->GetXMLLabel());
    QString tooltip = this->getTooltip(radius);
    ui.radius->setToolTip(tooltip);
    ui.radiusLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Radius'.");
  }

  if (smgroup->GetProperty("Input") == NULL)
  {
    this->connect(ui.centerOnBounds, SIGNAL(clicked()), SLOT(centerOnBounds()));
  }
  else
  {
    ui.centerOnBounds->hide();
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(
    pickHelper, SIGNAL(pick(double, double, double)), SLOT(setCenter(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  pickHelper2->connect(this, SIGNAL(widgetVisibilityUpdated(bool)), SLOT(setShortcutEnabled(bool)));
  this->connect(
    pickHelper2, SIGNAL(pick(double, double, double)), SLOT(setCenter(double, double, double)));
}

//-----------------------------------------------------------------------------
pqSpherePropertyWidget::~pqSpherePropertyWidget()
{
}

//-----------------------------------------------------------------------------
void pqSpherePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqSpherePropertyWidget::setCenter(double x, double y, double z)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double origin[3] = { x, y, z };
  vtkSMPropertyHelper(wdgProxy, "Center").Set(origin, 3);
  wdgProxy->UpdateVTKObjects();
  emit this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqSpherePropertyWidget::centerOnBounds()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (bbox.IsValid())
  {
    vtkSMProxy* wdgProxy = this->widgetProxy();
    double origin[3];
    bbox.GetCenter(origin);
    vtkSMPropertyHelper(wdgProxy, "Center").Set(origin, 3);
    vtkSMPropertyHelper(wdgProxy, "Radius").Set(bbox.GetMaxLength() / 2.0);
    wdgProxy->UpdateVTKObjects();
    emit this->changeAvailable();
    this->render();
  }
}
