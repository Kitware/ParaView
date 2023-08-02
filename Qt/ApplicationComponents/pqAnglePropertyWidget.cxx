// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnglePropertyWidget.h"
#include "ui_pqAnglePropertyWidget.h"

#include "pqCoreUtilities.h"
#include "pqPointPickingHelper.h"

#include "vtkMath.h"
#include "vtkSMNewWidgetRepresentationProxy.h"
#include "vtkSMPropertyGroup.h"
#include "vtkSMPropertyHelper.h"
#include "vtkVectorOperators.h"

#include <QHeaderView>
#include <QPointer>

#include <sstream>
#include <utility>
#include <vector>

//-----------------------------------------------------------------------------
struct pqAnglePropertyWidget::pqInternals
{
  Ui::AnglePropertyWidget Ui;
  pqPropertyLinks InternalLinks;
  std::array<double, 9> InlinedCoordinates{ { 1, 0, 0, 0, 0, 0, 0, 1, 0 } };
};

//-----------------------------------------------------------------------------
pqAnglePropertyWidget::pqAnglePropertyWidget(
  vtkSMProxy* smproxy, vtkSMPropertyGroup* smgroup, QWidget* parentObject)
  : Superclass("representations", "PolyLineWidgetRepresentation", smproxy, smgroup, parentObject)
  , Internals(new pqAnglePropertyWidget::pqInternals())
{
  pqInternals& internals = (*this->Internals);
  Ui::AnglePropertyWidget& ui = internals.Ui;
  ui.setupUi(this);

  if (vtkSMProperty* handlePositions = smgroup->GetProperty("HandlePositions"))
  {
    ui.labelPoint1->setText("Point 1");
    auto connectPoint = [this](pqDoubleLineEdit* widget, int index) {
      QObject::connect(
        widget, &pqDoubleLineEdit::textChangedAndEditingFinished, [this, index, widget]() {
          this->Internals->InlinedCoordinates[index] =
            widget->property("text").toString().toDouble();
          Q_EMIT this->pointsChanged();
        });
    };

    connectPoint(ui.point1X, 0);
    connectPoint(ui.point1Y, 1);
    connectPoint(ui.point1Z, 2);
    connectPoint(ui.centerX, 3);
    connectPoint(ui.centerY, 4);
    connectPoint(ui.centerZ, 5);
    connectPoint(ui.point2X, 6);
    connectPoint(ui.point2Y, 7);
    connectPoint(ui.point2Z, 8);

    this->addPropertyLink(this, "points", SIGNAL(pointsChanged()), handlePositions);
  }

  // link show3DWidget checkbox
  this->connect(ui.show3DWidget, SIGNAL(toggled(bool)), SLOT(setWidgetVisible(bool)));
  ui.show3DWidget->connect(this, SIGNAL(widgetVisibilityToggled(bool)), SLOT(setChecked(bool)));
  this->setWidgetVisible(ui.show3DWidget->isChecked());

  using PickOption = pqPointPickingHelper::PickOption;

  // link shortcuts for P1
  auto updateP1 = [this](double x, double y, double z) {
    this->Internals->InlinedCoordinates[0] = x;
    this->Internals->InlinedCoordinates[1] = y;
    this->Internals->InlinedCoordinates[2] = z;
    Q_EMIT this->pointsChanged();
  };
  pqPointPickingHelper* pickHelperP1 = new pqPointPickingHelper(QKeySequence(tr("1")), false, this);
  pickHelperP1->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  QObject::connect(pickHelperP1, &pqPointPickingHelper::pick, updateP1);
  pqPointPickingHelper* pickHelperP1Coord =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+1")), true, this, PickOption::Coordinates);
  pickHelperP1Coord->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelperP1Coord, &pqPointPickingHelper::pick, updateP1);

  // link shortcuts for center
  auto updateCn = [this](double x, double y, double z) {
    this->Internals->InlinedCoordinates[3] = x;
    this->Internals->InlinedCoordinates[4] = y;
    this->Internals->InlinedCoordinates[5] = z;
    Q_EMIT this->pointsChanged();
  };
  pqPointPickingHelper* pickHelperCenter =
    new pqPointPickingHelper(QKeySequence(tr("C")), false, this);
  pickHelperCenter->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  QObject::connect(pickHelperCenter, &pqPointPickingHelper::pick, updateCn);
  pqPointPickingHelper* pickHelperCenterCoord =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+C")), true, this, PickOption::Coordinates);
  pickHelperCenterCoord->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelperCenterCoord, &pqPointPickingHelper::pick, updateCn);

  // link shortcuts for P2
  auto updateP2 = [this](double x, double y, double z) {
    this->Internals->InlinedCoordinates[6] = x;
    this->Internals->InlinedCoordinates[7] = y;
    this->Internals->InlinedCoordinates[8] = z;
    Q_EMIT this->pointsChanged();
  };
  pqPointPickingHelper* pickHelperP2 = new pqPointPickingHelper(QKeySequence(tr("2")), false, this);
  pickHelperP2->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  QObject::connect(pickHelperP2, &pqPointPickingHelper::pick, updateP2);
  pqPointPickingHelper* pickHelperP2Coord =
    new pqPointPickingHelper(QKeySequence(tr("Ctrl+2")), true, this, PickOption::Coordinates);
  pickHelperP2Coord->connect(this, SIGNAL(viewChanged(pqView*)), SLOT(setView(pqView*)));
  this->connect(pickHelperP2Coord, &pqPointPickingHelper::pick, updateP2);

  pqCoreUtilities::connect(
    this->widgetProxy(), vtkCommand::PropertyModifiedEvent, this, SLOT(updateLabels()));

  Q_EMIT this->pointsChanged();
}

//-----------------------------------------------------------------------------
pqAnglePropertyWidget::~pqAnglePropertyWidget() = default;

//-----------------------------------------------------------------------------
void pqAnglePropertyWidget::placeWidget()
{
  // nothing to do.
}

//-----------------------------------------------------------------------------
void pqAnglePropertyWidget::updateLabels()
{
  const auto points = this->Internals->InlinedCoordinates;

  Ui::AnglePropertyWidget& ui = this->Internals->Ui;
  ui.point1X->setText(QString::number(points[0]));
  ui.point1Y->setText(QString::number(points[1]));
  ui.point1Z->setText(QString::number(points[2]));
  ui.centerX->setText(QString::number(points[3]));
  ui.centerY->setText(QString::number(points[4]));
  ui.centerZ->setText(QString::number(points[5]));
  ui.point2X->setText(QString::number(points[6]));
  ui.point2Y->setText(QString::number(points[7]));
  ui.point2Z->setText(QString::number(points[8]));

  const vtkVector3d pnt1 = { points[0], points[1], points[2] };
  const vtkVector3d center = { points[3], points[4], points[5] };
  const vtkVector3d pnt2 = { points[6], points[7], points[8] };
  const auto vec1 = pnt1 - center;
  const auto vec2 = pnt2 - center;
  const double angle =
    vtkMath::DegreesFromRadians(std::acos(vec1.Dot(vec2) / (vec1.Norm() * vec2.Norm())));
  ui.labelAngle->setText(QString("<b>%1</b> <i>%2</i> ").arg(tr("Angle: ")).arg(angle));
}

//-----------------------------------------------------------------------------
QList<QVariant> pqAnglePropertyWidget::points() const
{
  QList<QVariant> result;
  for (double x : this->Internals->InlinedCoordinates)
  {
    result.push_back(x);
  }
  return result;
}

//-----------------------------------------------------------------------------
void pqAnglePropertyWidget::setPoints(const QList<QVariant>& pts)
{
  if (pts.size() < 9)
  {
    return;
  }

  for (int i = 0; i < 9; ++i)
  {
    this->Internals->InlinedCoordinates[i] = pts[i].toDouble();
  }
  Q_EMIT this->pointsChanged();
}
