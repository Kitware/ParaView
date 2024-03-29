// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqLinePropertyWidget.h"
#include "ui_pqLinePropertyWidget.h"

#include "pqCoreUtilities.h"
#include "pqPointPickingHelper.h"
#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"

class pqLinePropertyWidget::pqInternals
{
public:
  Ui::LinePropertyWidget Ui;
  bool PickPoint1;
  pqInternals()
    : PickPoint1(true)
  {
  }
};

//-----------------------------------------------------------------------------
pqLinePropertyWidget::pqLinePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "LineSourceWidgetRepresentation", smproxy, smgroup, parentObject)
  , Internals(new pqLinePropertyWidget::pqInternals())
{
  Ui::LinePropertyWidget& ui = this->Internals->Ui;
  ui.setupUi(this);

#ifdef Q_OS_MAC
  ui.pickLabel->setText(ui.pickLabel->text().replace("Ctrl", "Cmd"));
#endif

  if (vtkSMProperty* p1 = smgroup->GetProperty("Point1WorldPosition"))
  {
    ui.labelPoint1->setText(QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()));
    this->addPropertyLink(ui.point1X, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 0);
    this->addPropertyLink(ui.point1Y, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 1);
    this->addPropertyLink(ui.point1Z, "text2", SIGNAL(textChangedAndEditingFinished()), p1, 2);
    ui.labelPoint1->setText(QCoreApplication::translate("ServerManagerXML", p1->GetXMLLabel()));
  }
  else
  {
    qCritical("Missing required property for function 'Point1WorldPosition'.");
  }

  if (vtkSMProperty* p2 = smgroup->GetProperty("Point2WorldPosition"))
  {
    ui.labelPoint2->setText(QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()));
    this->addPropertyLink(ui.point2X, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 0);
    this->addPropertyLink(ui.point2Y, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 1);
    this->addPropertyLink(ui.point2Z, "text2", SIGNAL(textChangedAndEditingFinished()), p2, 2);
    ui.labelPoint2->setText(QCoreApplication::translate("ServerManagerXML", p2->GetXMLLabel()));
  }
  else
  {
    qCritical("Missing required property for function 'Point2WorldPosition'.");
  }

  if (smgroup->GetProperty("Input"))
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

  this->connect(ui.xAxis, SIGNAL(clicked()), SLOT(useXAxis()));
  this->connect(ui.yAxis, SIGNAL(clicked()), SLOT(useYAxis()));
  this->connect(ui.zAxis, SIGNAL(clicked()), SLOT(useZAxis()));

  this->connect(ui.flipP2, SIGNAL(clicked()), SLOT(flipP2()));

  pqPointPickingHelper* pickHelper = new pqPointPickingHelper(QKeySequence(tr("P")), false, this);
  pickHelper->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper, SIGNAL(pick(double, double, double)), SLOT(pick(double, double, double)));

  pqPointPickingHelper* pickHelper2 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+P")), true, this);
  pickHelper2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper2, SIGNAL(pick(double, double, double)), SLOT(pick(double, double, double)));

  pqPointPickingHelper* pickHelper3 = new pqPointPickingHelper(QKeySequence(tr("1")), false, this);
  pickHelper3->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper3, SIGNAL(pick(double, double, double)), SLOT(pickPoint1(double, double, double)));

  pqPointPickingHelper* pickHelper4 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+1")), true, this);
  pickHelper4->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper4, SIGNAL(pick(double, double, double)), SLOT(pickPoint1(double, double, double)));

  pqPointPickingHelper* pickHelper5 = new pqPointPickingHelper(QKeySequence(tr("2")), false, this);
  pickHelper5->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper5, SIGNAL(pick(double, double, double)), SLOT(pickPoint2(double, double, double)));

  pqPointPickingHelper* pickHelper6 =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+2")), true, this);
  pickHelper6->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(
    pickHelper6, SIGNAL(pick(double, double, double)), SLOT(pickPoint2(double, double, double)));

  pqPointPickingHelper* pickHelperTest = new pqPointPickingHelper(
    QKeySequence(tr("N")), false, this, pqPointPickingHelper::CoordinatesAndNormal);
  pickHelperTest->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelperTest, SIGNAL(pickNormal(double, double, double, double, double, double)),
    SLOT(pickNormal(double, double, double, double, double, double)));

  pqCoreUtilities::connect(
    this->widgetProxy(), vtkCommand::PropertyModifiedEvent, this, SLOT(updateLengthLabel()));
  this->updateLengthLabel();
}

//-----------------------------------------------------------------------------
pqLinePropertyWidget::~pqLinePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::updateLengthLabel()
{
  double p1[3], p2[3];
  vtkSMProxy* wproxy = this->widgetProxy();
  vtkSMPropertyHelper(wproxy, "Point1WorldPosition").Get(p1, 3);
  vtkSMPropertyHelper(wproxy, "Point2WorldPosition").Get(p2, 3);

  double distance = sqrt(vtkMath::Distance2BetweenPoints(p1, p2));
  Ui::LinePropertyWidget& ui = this->Internals->Ui;
  ui.labelLength->setText(QString("<b>%1</b> <i>%2</i> ").arg(tr("Length: ")).arg(distance));
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
vtkBoundingBox pqLinePropertyWidget::referenceBounds() const
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
    double pt[3];
    vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Get(pt, 3);
    bbox.AddPoint(pt);

    vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Get(pt, 3);
    bbox.AddPoint(pt);
  }
  return bbox;
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::useAxis(int axis)
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkBoundingBox bbox = this->referenceBounds();
  if (bbox.IsValid())
  {
    const auto delta =
      0.5 * (bbox.GetLength(axis) > 0 ? bbox.GetLength(axis) : bbox.GetDiagonalLength());

    double center[3];
    bbox.GetCenter(center);

    center[axis] -= delta;
    vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Set(center, 3);

    bbox.GetCenter(center);
    center[axis] += delta;

    vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Set(center, 3);
    wdgProxy->UpdateVTKObjects();
    Q_EMIT this->changeAvailable();
    this->render();
  }
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::flipP2()
{
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  double origin[3];
  vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Get(origin, 3);
  double p2[3];
  vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Get(p2, 3);
  for (int i = 0; i < 3; ++i)
  {
    p2[i] = 2.0 * origin[i] - p2[i];
  }
  vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Set(p2, 3);

  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::centerOnBounds()
{
  vtkBoundingBox bbox = this->dataBounds();
  if (!bbox.IsValid())
  {
    return;
  }

  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Set(bbox.GetMinPoint(), 3);
  vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Set(bbox.GetMaxPoint(), 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::setLineColor(const QColor& color)
{
  vtkSMProxy* widget = this->widgetProxy();
  vtkSMPropertyHelper(widget, "LineColor").Set(0, color.redF());
  vtkSMPropertyHelper(widget, "LineColor").Set(1, color.greenF());
  vtkSMPropertyHelper(widget, "LineColor").Set(2, color.blueF());
  widget->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::pick(double wx, double wy, double wz)
{
  if (this->Internals->PickPoint1)
  {
    this->pickPoint1(wx, wy, wz);
  }
  else
  {
    this->pickPoint2(wx, wy, wz);
  }
  this->Internals->PickPoint1 = !this->Internals->PickPoint1;
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::pickPoint1(double wx, double wy, double wz)
{
  double position[3] = { wx, wy, wz };
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Set(position, 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::pickPoint2(double wx, double wy, double wz)
{
  double position[3] = { wx, wy, wz };
  vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();
  vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Set(position, 3);
  wdgProxy->UpdateVTKObjects();
  Q_EMIT this->changeAvailable();
  this->render();
}

//-----------------------------------------------------------------------------
void pqLinePropertyWidget::pickNormal(
  double wx, double wy, double wz, double nx, double ny, double nz)
{
  if (nx != 0.0 || ny != 0.0 || nz != 0.0)
  {
    vtkSMNewWidgetRepresentationProxy* wdgProxy = this->widgetProxy();

    double p1[3] = { wx, wy, wz };
    vtkSMPropertyHelper(wdgProxy, "Point1WorldPosition").Set(p1, 3);
    double p2[3] = { wx + nx, wy + ny, wz + nz };
    vtkSMPropertyHelper(wdgProxy, "Point2WorldPosition").Set(p2, 3);

    wdgProxy->UpdateVTKObjects();
    Q_EMIT this->changeAvailable();
    this->render();
  }
}
