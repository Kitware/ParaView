// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFrustumPropertyWidget.h"

#include "ui_pqFrustumPropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"

#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkVector.h"

#include <QObject>

#include <cassert>

//-----------------------------------------------------------------------------
pqFrustumPropertyWidget::pqFrustumPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
      "representations", "ImplicitFrustumWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::FrustumPropertyWidget ui;
  ui.setupUi(this);

  if (vtkSMProperty* origin = smgroup->GetProperty("Origin"))
  {
    this->addPropertyLink(ui.originX, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 0);
    this->addPropertyLink(ui.originY, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 1);
    this->addPropertyLink(ui.originZ, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 2);
    ui.originLabel->setText(QCoreApplication::translate("ServerManagerXML", origin->GetXMLLabel()));
    QString tooltip = this->getTooltip(origin);
    ui.originX->setToolTip(tooltip);
    ui.originY->setToolTip(tooltip);
    ui.originZ->setToolTip(tooltip);
    ui.originLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Origin'.");
  }

  if (vtkSMProperty* orientation = smgroup->GetProperty("Orientation"))
  {
    this->addPropertyLink(
      ui.orientationX, "text2", SIGNAL(textChangedAndEditingFinished()), orientation, 0);
    this->addPropertyLink(
      ui.orientationY, "text2", SIGNAL(textChangedAndEditingFinished()), orientation, 1);
    this->addPropertyLink(
      ui.orientationZ, "text2", SIGNAL(textChangedAndEditingFinished()), orientation, 2);
    ui.orientationLabel->setText(
      QCoreApplication::translate("ServerManagerXML", orientation->GetXMLLabel()));
    QString tooltip = this->getTooltip(orientation);
    ui.orientationX->setToolTip(tooltip);
    ui.orientationY->setToolTip(tooltip);
    ui.orientationZ->setToolTip(tooltip);
    ui.orientationLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Orientation'.");
  }

  if (vtkSMProperty* horizontalAngle = smgroup->GetProperty("HorizontalAngle"))
  {
    this->addPropertyLink(
      ui.horizontalAngle, "text2", SIGNAL(textChangedAndEditingFinished()), horizontalAngle);
    ui.horizontalAngleLabel->setText(
      QCoreApplication::translate("ServerManagerXML", horizontalAngle->GetXMLLabel()));
    QString tooltip = this->getTooltip(horizontalAngle);
    ui.horizontalAngle->setToolTip(tooltip);
    ui.horizontalAngleLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'HorizontalAngle'.");
  }

  if (vtkSMProperty* verticalAngle = smgroup->GetProperty("VerticalAngle"))
  {
    this->addPropertyLink(
      ui.verticalAngle, "text2", SIGNAL(textChangedAndEditingFinished()), verticalAngle);
    ui.verticalAngleLabel->setText(
      QCoreApplication::translate("ServerManagerXML", verticalAngle->GetXMLLabel()));
    QString tooltip = this->getTooltip(verticalAngle);
    ui.verticalAngle->setToolTip(tooltip);
    ui.verticalAngleLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'VerticalAngle'.");
  }

  if (vtkSMProperty* nearPlaneDistance = smgroup->GetProperty("NearPlaneDistance"))
  {
    this->addPropertyLink(
      ui.nearPlaneDistance, "text2", SIGNAL(textChangedAndEditingFinished()), nearPlaneDistance);
    ui.nearPlaneDistanceLabel->setText(
      QCoreApplication::translate("ServerManagerXML", nearPlaneDistance->GetXMLLabel()));
    QString tooltip = this->getTooltip(nearPlaneDistance);
    ui.nearPlaneDistance->setToolTip(tooltip);
    ui.nearPlaneDistanceLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'NearPlaneDistance'.");
  }

  if (smgroup->GetProperty("Input"))
  {
    QObject::connect(
      ui.resetBounds, &QPushButton::clicked, this, &pqFrustumPropertyWidget::resetBounds);
    this->resetBounds();
  }
  else
  {
    ui.resetBounds->hide();
  }

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  QObject::connect(
    &this->WidgetLinks, &pqPropertyLinks::qtWidgetChanged, this, &pqFrustumPropertyWidget::render);

  // link show3DWidget checkbox
  QObject::connect(
    ui.show3DWidget, &QCheckBox::toggled, this, &pqFrustumPropertyWidget::setWidgetVisible);
  QObject::connect(this, &pqFrustumPropertyWidget::widgetVisibilityToggled, ui.show3DWidget,
    &QCheckBox::setChecked);
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqFrustumPropertyWidget::placeWidget);
}

//-----------------------------------------------------------------------------
void pqFrustumPropertyWidget::placeWidget()
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
void pqFrustumPropertyWidget::resetBounds()
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
    double nearPlaneDistance = bbox.GetLength(1) * 0.1;

    vtkSMPropertyHelper(wdgProxy, "Origin").Set(center.GetData(), 3);
    vtkSMPropertyHelper(wdgProxy, "NearPlaneDistance").Set(nearPlaneDistance);
    vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bnds.GetData(), 6);
    wdgProxy->UpdateProperty("WidgetBounds", true);

    Q_EMIT this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqFrustumPropertyWidget::setOrientation(const vtkVector3d& orientation)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Orientation").Set(orientation.GetData(), 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}
