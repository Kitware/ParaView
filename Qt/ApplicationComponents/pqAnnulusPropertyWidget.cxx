// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqAnnulusPropertyWidget.h"

#include "ui_pqAnnulusPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVector.h"

#include <QObject>

#include <cassert>

//-----------------------------------------------------------------------------
pqAnnulusPropertyWidget::pqAnnulusPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
      "representations", "ImplicitAnnulusWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::AnnulusPropertyWidget ui;
  ui.setupUi(this);

  if (vtkSMProperty* center = smgroup->GetProperty("Center"))
  {
    this->addPropertyLink(ui.centerX, "text2", SIGNAL(textChangedAndEditingFinished()), center, 0);
    this->addPropertyLink(ui.centerY, "text2", SIGNAL(textChangedAndEditingFinished()), center, 1);
    this->addPropertyLink(ui.centerZ, "text2", SIGNAL(textChangedAndEditingFinished()), center, 2);
    ui.centerLabel->setText(QCoreApplication::translate("ServerManagerXML", center->GetXMLLabel()));
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

  if (vtkSMProperty* axis = smgroup->GetProperty("Axis"))
  {
    this->addPropertyLink(ui.axisX, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 0);
    this->addPropertyLink(ui.axisY, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 1);
    this->addPropertyLink(ui.axisZ, "text2", SIGNAL(textChangedAndEditingFinished()), axis, 2);
    ui.axisLabel->setText(QCoreApplication::translate("ServerManagerXML", axis->GetXMLLabel()));
    QString tooltip = this->getTooltip(axis);
    ui.axisX->setToolTip(tooltip);
    ui.axisY->setToolTip(tooltip);
    ui.axisZ->setToolTip(tooltip);
    ui.axisLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Axis'.");
  }

  if (vtkSMProperty* innerRadius = smgroup->GetProperty("InnerRadius"))
  {
    this->addPropertyLink(
      ui.innerRadius, "text2", SIGNAL(textChangedAndEditingFinished()), innerRadius);
    ui.innerRadiusLabel->setText(
      QCoreApplication::translate("ServerManagerXML", innerRadius->GetXMLLabel()));
    QString tooltip = this->getTooltip(innerRadius);
    ui.innerRadius->setToolTip(tooltip);
    ui.innerRadiusLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'InnerRadius'.");
  }

  if (vtkSMProperty* outerRadius = smgroup->GetProperty("OuterRadius"))
  {
    this->addPropertyLink(
      ui.outerRadius, "text2", SIGNAL(textChangedAndEditingFinished()), outerRadius);
    ui.outerRadiusLabel->setText(
      QCoreApplication::translate("ServerManagerXML", outerRadius->GetXMLLabel()));
    QString tooltip = this->getTooltip(outerRadius);
    ui.outerRadius->setToolTip(tooltip);
    ui.outerRadiusLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'InnerRadius'.");
  }

  if (smgroup->GetProperty("Input"))
  {
    QObject::connect(
      ui.resetBounds, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::resetBounds);
  }
  else
  {
    ui.resetBounds->hide();
  }

  // link a few buttons
  QObject::connect(ui.useXAxis, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::useXAxis);
  QObject::connect(ui.useYAxis, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::useYAxis);
  QObject::connect(ui.useZAxis, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::useZAxis);
  QObject::connect(
    ui.useCameraAxis, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::useCameraAxis);
  QObject::connect(
    ui.resetCameraToAxis, &QPushButton::clicked, this, &pqAnnulusPropertyWidget::resetCameraToAxis);

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  vtkSMProxy* wdgProxy = this->widgetProxy();
  this->WidgetLinks.addPropertyLink(
    ui.scaling, "checked", SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("ScaleEnabled"));
  this->WidgetLinks.addPropertyLink(ui.outlineTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("OutlineTranslation"));
  QObject::connect(
    &this->WidgetLinks, &pqPropertyLinks::qtWidgetChanged, this, &pqAnnulusPropertyWidget::render);

  // link show3DWidget checkbox
  QObject::connect(
    ui.show3DWidget, &QCheckBox::toggled, this, &pqAnnulusPropertyWidget::setWidgetVisible);
  QObject::connect(this, &pqAnnulusPropertyWidget::widgetVisibilityToggled, ui.show3DWidget,
    &QCheckBox::setChecked);
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  // save pointers to advanced widgets to toggle their visibility in updateWidget
  this->AdvancedPropertyWidgets[0] = ui.scaling;
  this->AdvancedPropertyWidgets[1] = ui.outlineTranslation;

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqAnnulusPropertyWidget::placeWidget);
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
  bbox.ScaleAboutCenter(scaleFactor);

  vtkVector<double, 6> bounds;
  bbox.GetBounds(bounds.GetData());
  vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bounds.GetData(), 6);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::resetBounds()
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  if (wdgProxy)
  {
    double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
    bbox.ScaleAboutCenter(scaleFactor);

    vtkVector3d center;
    bbox.GetCenter(center.GetData());
    vtkVector<double, 6> bnds;
    bbox.GetBounds(bnds.GetData());

    vtkSMPropertyHelper(wdgProxy, "Center").Set(center.GetData(), 3);
    vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bnds.GetData(), 6);
    wdgProxy->UpdateProperty("WidgetBounds", true);

    Q_EMIT this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::resetCameraToAxis()
{
  if (pqRenderView* renView = qobject_cast<pqRenderView*>(this->view()))
  {
    vtkCamera* camera = renView->getRenderViewProxy()->GetActiveCamera();
    vtkSMProxy* wdgProxy = this->widgetProxy();

    vtkVector3d up, forward;
    camera->GetViewUp(up.GetData());
    vtkSMPropertyHelper(wdgProxy, "Axis").Get(forward.GetData(), 3);

    vtkMath::Cross(up.GetData(), forward.GetData(), up.GetData());
    vtkMath::Cross(forward.GetData(), up.GetData(), up.GetData());

    renView->resetViewDirection(forward[0], forward[1], forward[2], up[0], up[1], up[2]);
    renView->render();
  }
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::useCameraAxis()
{
  vtkSMRenderViewProxy* viewProxy =
    this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : nullptr;
  if (viewProxy)
  {
    vtkCamera* camera = viewProxy->GetActiveCamera();

    vtkVector3d cameraNormal;
    camera->GetViewPlaneNormal(cameraNormal.GetData());
    this->setAxis(-cameraNormal);
  }
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::setAxis(const vtkVector3d& axis)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Axis").Set(axis.GetData(), 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::useXAxis()
{
  this->setAxis({ 1, 0, 0 });
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::useYAxis()
{
  this->setAxis({ 0, 1, 0 });
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::useZAxis()
{
  this->setAxis({ 0, 0, 1 });
}

//-----------------------------------------------------------------------------
void pqAnnulusPropertyWidget::updateWidget(bool showingAdvancedProperties)
{
  for (auto widget : this->AdvancedPropertyWidgets)
  {
    widget->setVisible(showingAdvancedProperties);
  }
}
