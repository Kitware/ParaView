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

#include "pqRenderView.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

#include <cassert>

namespace
{

// Scale the bounds by the given factor
static void pqAdjustBounds(vtkBoundingBox& bbox, double scaleFactor)
{
  double min_point[3], max_point[3];
  bbox.GetMinPoint(min_point[0], min_point[1], min_point[2]);
  bbox.GetMaxPoint(max_point[0], max_point[1], max_point[2]);

  for (int i = 0; i < 3; ++i)
  {
    double mid = (min_point[i] + max_point[i]) / 2.0;
    min_point[i] = mid + scaleFactor * (min_point[i] - mid);
    max_point[i] = mid + scaleFactor * (max_point[i] - mid);
  }
  bbox.SetMinPoint(min_point);
  bbox.SetMaxPoint(max_point);
}
}

//-----------------------------------------------------------------------------
pqCylinderPropertyWidget::pqCylinderPropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass(
      "representations", "ImplicitCylinderWidgetRepresentation", smproxy, smgroup, parentObject)
{
  Ui::CylinderPropertyWidget ui;
  ui.setupUi(this);

  if (vtkSMProperty* center = smgroup->GetProperty("Center"))
  {
    this->addPropertyLink(ui.centerX, "text2", SIGNAL(textChangedAndEditingFinished()), center, 0);
    this->addPropertyLink(ui.centerY, "text2", SIGNAL(textChangedAndEditingFinished()), center, 1);
    this->addPropertyLink(ui.centerZ, "text2", SIGNAL(textChangedAndEditingFinished()), center, 2);
    ui.centerLabel->setText(center->GetXMLLabel());
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
    ui.axisLabel->setText(axis->GetXMLLabel());
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

  if (smgroup->GetProperty("Input"))
  {
    this->connect(ui.resetBounds, SIGNAL(clicked()), SLOT(resetBounds()));
  }
  else
  {
    ui.resetBounds->hide();
  }

  // link a few buttons
  this->connect(ui.useXAxis, SIGNAL(clicked()), SLOT(useXAxis()));
  this->connect(ui.useYAxis, SIGNAL(clicked()), SLOT(useYAxis()));
  this->connect(ui.useZAxis, SIGNAL(clicked()), SLOT(useZAxis()));
  this->connect(ui.useCameraAxis, SIGNAL(clicked()), SLOT(useCameraAxis()));
  this->connect(ui.resetCameraToAxis, SIGNAL(clicked()), SLOT(resetCameraToAxis()));

  // Let's link some of the UI elements that only affect the interactive widget
  // properties without affecting properties on the main proxy.
  vtkSMProxy* wdgProxy = this->widgetProxy();
  this->WidgetLinks.addPropertyLink(
    ui.scaling, "checked", SIGNAL(toggled(bool)), wdgProxy, wdgProxy->GetProperty("ScaleEnabled"));
  this->WidgetLinks.addPropertyLink(ui.outlineTranslation, "checked", SIGNAL(toggled(bool)),
    wdgProxy, wdgProxy->GetProperty("OutlineTranslation"));
  this->connect(&this->WidgetLinks, SIGNAL(qtWidgetChanged()), SLOT(render()));

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  // save pointers to advanced widgets to toggle their visibility in updateWidget
  this->AdvancedPropertyWidgets[0] = ui.scaling;
  this->AdvancedPropertyWidgets[1] = ui.outlineTranslation;

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
  assert(wdgProxy);

  double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
  pqAdjustBounds(bbox, scaleFactor);
  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::resetBounds()
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

    double center[3];
    bbox.GetCenter(center);
    double bnds[6];
    bbox.GetBounds(bnds);

    vtkSMPropertyHelper(wdgProxy, "Center").Set(center, 3);
    vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bnds, 6);
    wdgProxy->UpdateProperty("WidgetBounds", true);

    emit this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::resetCameraToAxis()
{
  if (pqRenderView* renView = qobject_cast<pqRenderView*>(this->view()))
  {
    vtkCamera* camera = renView->getRenderViewProxy()->GetActiveCamera();
    vtkSMProxy* wdgProxy = this->widgetProxy();
    double up[3], forward[3];
    camera->GetViewUp(up);
    vtkSMPropertyHelper(wdgProxy, "Axis").Get(forward, 3);
    vtkMath::Cross(up, forward, up);
    vtkMath::Cross(forward, up, up);
    renView->resetViewDirection(forward[0], forward[1], forward[2], up[0], up[1], up[2]);
    renView->render();
  }
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::useCameraAxis()
{
  vtkSMRenderViewProxy* viewProxy =
    this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : NULL;
  if (viewProxy)
  {
    vtkCamera* camera = viewProxy->GetActiveCamera();

    double camera_normal[3];
    camera->GetViewPlaneNormal(camera_normal);
    camera_normal[0] = -camera_normal[0];
    camera_normal[1] = -camera_normal[1];
    camera_normal[2] = -camera_normal[2];
    this->setAxis(camera_normal[0], camera_normal[1], camera_normal[2]);
  }
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::setAxis(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double axis[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Axis").Set(axis, 3);
  wdgProxy->UpdateVTKObjects();
  emit this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqCylinderPropertyWidget::updateWidget(bool showing_advanced_properties)
{
  for (int i = 0; i < 2; ++i)
  {
    this->AdvancedPropertyWidgets[i]->setVisible(showing_advanced_properties);
  }
}
