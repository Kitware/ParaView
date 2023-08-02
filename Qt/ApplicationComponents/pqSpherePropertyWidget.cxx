// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
    ui.centerLabel->setText(QCoreApplication::translate("ServerManagerXML", center->GetXMLLabel()));
    ui.pickLabel->setText(ui.pickLabel->text().arg(
      QCoreApplication::translate("ServerManagerXML", center->GetXMLLabel())));
    QString tooltip = this->getTooltip(center);
    ui.centerX->setToolTip(tooltip);
    ui.centerY->setToolTip(tooltip);
    ui.centerZ->setToolTip(tooltip);
    ui.centerLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Center'.");
    ui.pickLabel->setText(ui.pickLabel->text().arg(tr("Center")));
  }

  if (vtkSMProperty* normal = smgroup->GetProperty("Normal"))
  {
    this->addPropertyLink(ui.normalX, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 0);
    this->addPropertyLink(ui.normalY, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 1);
    this->addPropertyLink(ui.normalZ, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 2);
    ui.normalLabel->setText(QCoreApplication::translate("ServerManagerXML", normal->GetXMLLabel()));
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
    ui.radiusLabel->setText(QCoreApplication::translate("ServerManagerXML", radius->GetXMLLabel()));
    QString tooltip = this->getTooltip(radius);
    ui.radius->setToolTip(tooltip);
    ui.radiusLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Radius'.");
  }

  if (smgroup->GetProperty("Input") == nullptr)
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
  this->connect(
    pickHelper, SIGNAL(pick(double, double, double)), SLOT(setCenter(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper2, SIGNAL(pick(double, double, double)), SLOT(setCenter(double, double, double)));
}

//-----------------------------------------------------------------------------
pqSpherePropertyWidget::~pqSpherePropertyWidget() = default;

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
  Q_EMIT this->changeAvailable();
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
    Q_EMIT this->changeAvailable();
    this->render();
  }
}
