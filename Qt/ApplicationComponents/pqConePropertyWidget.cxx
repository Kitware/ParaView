// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqConePropertyWidget.h"

#include "ui_pqConePropertyWidget.h"

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

#include <cassert>

namespace
{

//-----------------------------------------------------------------------------
// Scale the bounds by the given factor
static void pqAdjustBounds(vtkBoundingBox& bbox, double scaleFactor)
{
  vtkVector3d minPoint, maxPoint;
  bbox.GetMinPoint(minPoint.GetData());
  bbox.GetMaxPoint(maxPoint.GetData());

  // note: this loop could be simplified if we had an element-wise subtraction operator for vector
  // types.
  for (int i = 0; i < 3; ++i)
  {
    double mid = (minPoint[i] + maxPoint[i]) / 2.0;
    minPoint[i] = mid + scaleFactor * (minPoint[i] - mid);
    maxPoint[i] = mid + scaleFactor * (maxPoint[i] - mid);
  }
  bbox.SetMinPoint(minPoint.GetData());
  bbox.SetMaxPoint(maxPoint.GetData());
}
}

//-----------------------------------------------------------------------------
pqConePropertyWidget::pqConePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
      "representations", "ImplicitConeWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::ConePropertyWidget ui;
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

  if (vtkSMProperty* angle = smgroup->GetProperty("Angle"))
  {
    this->addPropertyLink(ui.angle, "text2", SIGNAL(textChangedAndEditingFinished()), angle);
    ui.angleLabel->setText(QCoreApplication::translate("ServerManagerXML", angle->GetXMLLabel()));
    QString tooltip = this->getTooltip(angle);
    ui.angle->setToolTip(tooltip);
    ui.angleLabel->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Angle'.");
  }

  if (smgroup->GetProperty("Input"))
  {
    QObject::connect(
      ui.resetBounds, &QPushButton::clicked, this, &pqConePropertyWidget::resetBounds);
  }
  else
  {
    ui.resetBounds->hide();
  }

  // link a few buttons
  QObject::connect(ui.useXAxis, &QPushButton::clicked, this, &pqConePropertyWidget::useXAxis);
  QObject::connect(ui.useYAxis, &QPushButton::clicked, this, &pqConePropertyWidget::useYAxis);
  QObject::connect(ui.useZAxis, &QPushButton::clicked, this, &pqConePropertyWidget::useZAxis);
  QObject::connect(
    ui.useCameraAxis, &QPushButton::clicked, this, &pqConePropertyWidget::useCameraAxis);
  QObject::connect(
    ui.resetCameraToAxis, &QPushButton::clicked, this, &pqConePropertyWidget::resetCameraToAxis);

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  vtkSMProxy* wdgProxy = this->widgetProxy();
  this->WidgetLinks.addPropertyLink(
    ui.scaling, "checked", SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("ScaleEnabled"));
  this->WidgetLinks.addPropertyLink(ui.outlineTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("OutlineTranslation"));
  QObject::connect(
    &this->WidgetLinks, &pqPropertyLinks::qtWidgetChanged, this, &pqConePropertyWidget::render);

  // link show3DWidget checkbox
  QObject::connect(
    ui.show3DWidget, &QCheckBox::toggled, this, &pqConePropertyWidget::setWidgetVisible);
  QObject::connect(
    this, &pqConePropertyWidget::widgetVisibilityToggled, ui.show3DWidget, &QCheckBox::setChecked);
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  // save pointers to advanced widgets to toggle their visibility in updateWidget
  this->AdvancedPropertyWidgets[0] = ui.scaling;
  this->AdvancedPropertyWidgets[1] = ui.outlineTranslation;

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqConePropertyWidget::placeWidget);
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  assert(wdgProxy);

  double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
  pqAdjustBounds(bbox, scaleFactor);
  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::resetBounds()
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
    pqAdjustBounds(bbox, scaleFactor);

    vtkVector3d origin;
    bbox.GetCenter(origin.GetData());
    vtkVector<double, 6> bnds;
    bbox.GetBounds(bnds.GetData());

    vtkSMPropertyHelper(wdgProxy, "Origin").Set(origin.GetData(), 3);
    vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bnds.GetData(), 6);
    wdgProxy->UpdateProperty("WidgetBounds", true);

    Q_EMIT this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::resetCameraToAxis()
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
void pqConePropertyWidget::useCameraAxis()
{
  vtkSMRenderViewProxy* viewProxy =
    this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : nullptr;
  if (viewProxy)
  {
    vtkCamera* camera = viewProxy->GetActiveCamera();

    vtkVector3d cameraNormal;
    camera->GetViewPlaneNormal(cameraNormal.GetData());
    cameraNormal = -cameraNormal;
    this->setAxis(cameraNormal);
  }
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::useXAxis()
{
  this->setAxis(vtkVector3d(1, 0, 0));
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::useYAxis()
{
  this->setAxis(vtkVector3d(0, 1, 0));
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::useZAxis()
{
  this->setAxis(vtkVector3d(0, 0, 1));
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::setAxis(const vtkVector3d& axis)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Axis").Set(axis.GetData(), 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqConePropertyWidget::updateWidget(bool showingAdvancedProperties)
{
  for (auto widget : this->AdvancedPropertyWidgets)
  {
    widget->setVisible(showingAdvancedProperties);
  }
}
