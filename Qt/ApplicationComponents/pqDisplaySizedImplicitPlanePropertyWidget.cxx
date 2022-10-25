/*=========================================================================

   Program: ParaView
   Module:  pqDisplaySizedImplicitPlanePropertyWidget.cxx

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
#include "pqDisplaySizedImplicitPlanePropertyWidget.h"
#include "ui_pqDisplaySizedImplicitPlanePropertyWidget.h"

#include "pqActiveObjects.h"
#include "pqPointPickingHelper.h"
#include "pqRenderView.h"

#include "vtkCamera.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMRenderViewProxy.h"

namespace
{
// Display sized implicit plane widget does not like it when any of the dimensions is 0. So
// we ensure that each dimension has some thickness. Then we scale the bounds
// by the given factor
static void pqAdjustBounds(vtkBoundingBox& bbox, double scaleFactor)
{
  double max_length = bbox.GetMaxLength();
  max_length = max_length > 0 ? max_length * 0.05 : 1;
  double min_point[3], max_point[3];
  bbox.GetMinPoint(min_point[0], min_point[1], min_point[2]);
  bbox.GetMaxPoint(max_point[0], max_point[1], max_point[2]);
  for (int cc = 0; cc < 3; cc++)
  {
    if (bbox.GetLength(cc) == 0)
    {
      min_point[cc] -= max_length;
      max_point[cc] += max_length;
    }

    double mid = (min_point[cc] + max_point[cc]) / 2.0;
    min_point[cc] = mid + scaleFactor * (min_point[cc] - mid);
    max_point[cc] = mid + scaleFactor * (max_point[cc] - mid);
  }
  bbox.SetMinPoint(min_point);
  bbox.SetMaxPoint(max_point);
}
}

//-----------------------------------------------------------------------------
pqDisplaySizedImplicitPlanePropertyWidget::pqDisplaySizedImplicitPlanePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "DisplaySizedImplicitPlaneWidgetRepresentation", smproxy, smgroup,
      parentObject)
{
  Ui::DisplaySizedImplicitPlanePropertyWidget ui;
  ui.setupUi(this);
  if (vtkSMProperty* origin = smgroup->GetProperty("Origin"))
  {
    this->addPropertyLink(ui.originX, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 0);
    this->addPropertyLink(ui.originY, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 1);
    this->addPropertyLink(ui.originZ, "text2", SIGNAL(textChangedAndEditingFinished()), origin, 2);
    ui.labelOrigin->setText(origin->GetXMLLabel());
    ui.pickLabel->setText(
      ui.pickLabel->text().replace("'Origin'", QString("'%1'").arg(origin->GetXMLLabel())));
    QString tooltip = this->getTooltip(origin);
    ui.originX->setToolTip(tooltip);
    ui.originY->setToolTip(tooltip);
    ui.originZ->setToolTip(tooltip);
    ui.labelOrigin->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Origin'.");
  }

  if (vtkSMProperty* normal = smgroup->GetProperty("Normal"))
  {
    this->addPropertyLink(ui.normalX, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 0);
    this->addPropertyLink(ui.normalY, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 1);
    this->addPropertyLink(ui.normalZ, "text2", SIGNAL(textChangedAndEditingFinished()), normal, 2);
    ui.labelNormal->setText(normal->GetXMLLabel());
    QString tooltip = this->getTooltip(normal);
    ui.normalX->setToolTip(tooltip);
    ui.normalY->setToolTip(tooltip);
    ui.normalZ->setToolTip(tooltip);
    ui.labelNormal->setToolTip(tooltip);
  }
  else
  {
    qCritical("Missing required property for function 'Normal'.");
  }

  if (vtkSMIntVectorProperty* alwaysSnapToNearestAxis =
        vtkSMIntVectorProperty::SafeDownCast(smgroup->GetProperty("AlwaysSnapToNearestAxis")))
  {
    if (alwaysSnapToNearestAxis->GetNumberOfElements())
    {
      vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
      vtkSMPropertyHelper(wdgProxy, "AlwaysSnapToNearestAxis")
        .Set(alwaysSnapToNearestAxis->GetElements()[0]);
    }
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  const bool drawOutlineAndIntersectionEdges =
    vtkSMPropertyHelper(wdgProxy, "DrawOutline").GetAsInt() != 0 &&
    vtkSMPropertyHelper(wdgProxy, "DrawIntersectionEdges").GetAsInt() != 0;
  ui.showOutlineAndIntersectionEdges->setChecked(drawOutlineAndIntersectionEdges);

  // link a few buttons
  this->connect(ui.useXNormal, SIGNAL(clicked()), SLOT(useXNormal()));
  this->connect(ui.useYNormal, SIGNAL(clicked()), SLOT(useYNormal()));
  this->connect(ui.useZNormal, SIGNAL(clicked()), SLOT(useZNormal()));
  this->connect(ui.useCameraNormal, SIGNAL(clicked()), SLOT(useCameraNormal()));
  this->connect(ui.resetCameraToNormal, SIGNAL(clicked()), SLOT(resetCameraToNormal()));
  this->connect(ui.resetToDataBounds, SIGNAL(clicked()), SLOT(resetToDataBounds()));
  this->connect(ui.resetRadiusSize, SIGNAL(clicked()), SLOT(resetRadiusSize()));

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());
  this->connect(ui.show3DWidget, &QCheckBox::toggled, ui.showOutlineAndIntersectionEdges,
    &QCheckBox::setEnabled);
  this->connect(ui.showOutlineAndIntersectionEdges, &QCheckBox::toggled, this,
    &pqDisplaySizedImplicitPlanePropertyWidget::setDrawOutlineAndIntersectionEdges);

  // We want to show the translucent plane when interaction starts.
  this->connect(this, SIGNAL(startInteraction()), SLOT(showPlane()));

  using PickOption = pqPointPickingHelper::PickOption;

  // picking origin point actions
  pqPointPickingHelper* pickPointHelper =
    new pqPointPickingHelper(QKeySequence(tr("P")), false, this, PickOption::Coordinates);
  pickPointHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickPointHelper, SIGNAL(pick(double, double, double)), SLOT(setOrigin(double, double, double)));

  pqPointPickingHelper* pickPointHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this, PickOption::Coordinates);
  pickPointHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickPointHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setOrigin(double, double, double)));

  // picking normal actions
  pqPointPickingHelper* pickNormalHelper =
    new pqPointPickingHelper(QKeySequence(tr("N")), false, this, PickOption::Normal);
  pickNormalHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickNormalHelper, SIGNAL(pick(double, double, double)),
    SLOT(setNormal(double, double, double)));

  pqPointPickingHelper* pickNormalHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+N")), true, this, PickOption::Normal);
  pickNormalHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickNormalHelper2, SIGNAL(pick(double, double, double)),
    SLOT(setNormal(double, double, double)));

  QObject::connect(&pqActiveObjects::instance(), &pqActiveObjects::dataUpdated, this,
    &pqDisplaySizedImplicitPlanePropertyWidget::placeWidget);
}

//-----------------------------------------------------------------------------
pqDisplaySizedImplicitPlanePropertyWidget::~pqDisplaySizedImplicitPlanePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::placeWidget()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
  pqAdjustBounds(bbox, scaleFactor);
  double bds[6];
  bbox.GetBounds(bds);
  vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bds, 6);
  wdgProxy->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::setDrawPlane(bool val)
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "DrawPlane").Set(val ? 1 : 0);
  wdgProxy->UpdateVTKObjects();
  this->render();
}

void pqDisplaySizedImplicitPlanePropertyWidget::setDrawOutlineAndIntersectionEdges(bool val)
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "DrawOutline").Set(val ? 1 : 0);
  vtkSMPropertyHelper(wdgProxy, "DrawIntersectionEdges").Set(val ? 1 : 0);
  wdgProxy->UpdateVTKObjects();
  this->render();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::apply()
{
  this->setDrawPlane(false);
  this->Superclass::apply();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::reset()
{
  this->setDrawPlane(false);
  this->Superclass::reset();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::resetRadiusSize()
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "RadiusMultiplier").Set(1.0);
  wdgProxy->UpdateProperty("RadiusMultiplier", true);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::resetToDataBounds()
{
  vtkBoundingBox bbox = this->dataBounds();

  if (bbox.IsValid())
  {
    vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
    double scaleFactor = vtkSMPropertyHelper(wdgProxy, "PlaceFactor").GetAsDouble();
    pqAdjustBounds(bbox, scaleFactor);
    double origin[3], bounds[6];
    bbox.GetCenter(origin);
    bbox.GetBounds(bounds);
    vtkSMPropertyHelper(wdgProxy, "Origin").Set(origin, 3);
    vtkSMPropertyHelper(wdgProxy, "WidgetBounds").Set(bounds, 6);
    wdgProxy->UpdateProperty("WidgetBounds", true);
    wdgProxy->UpdateVTKObjects();
    Q_EMIT this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::resetCameraToNormal()
{
  if (pqRenderView* renView = qobject_cast<pqRenderView*>(this->view()))
  {
    vtkCamera* camera = renView->getRenderViewProxy()->GetActiveCamera();
    vtkSMProxy* wdgProxy = this->widgetProxy();
    double up[3], forward[3];
    camera->GetViewUp(up);
    vtkSMPropertyHelper(wdgProxy, "Normal").Get(forward, 3);
    vtkMath::Cross(up, forward, up);
    vtkMath::Cross(forward, up, up);
    renView->resetViewDirection(forward[0], forward[1], forward[2], up[0], up[1], up[2]);
    renView->render();
  }
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::useCameraNormal()
{
  vtkSMRenderViewProxy* viewProxy =
    this->view() ? vtkSMRenderViewProxy::SafeDownCast(this->view()->getProxy()) : nullptr;
  if (viewProxy)
  {
    vtkCamera* camera = viewProxy->GetActiveCamera();

    double camera_normal[3];
    camera->GetViewPlaneNormal(camera_normal);
    camera_normal[0] = -camera_normal[0];
    camera_normal[1] = -camera_normal[1];
    camera_normal[2] = -camera_normal[2];
    this->setNormal(camera_normal[0], camera_normal[1], camera_normal[2]);
  }
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::setNormal(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double n[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Normal").Set(n, 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqDisplaySizedImplicitPlanePropertyWidget::setOrigin(double wx, double wy, double wz)
{
  vtkSMProxy* wdgProxy = this->widgetProxy();
  double o[3] = { wx, wy, wz };
  vtkSMPropertyHelper(wdgProxy, "Origin").Set(o, 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}
