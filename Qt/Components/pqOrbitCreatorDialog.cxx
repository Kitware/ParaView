// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqOrbitCreatorDialog.h"
#include "ui_pqOrbitCreatorDialog.h"

#include "pqActiveObjects.h"
#include "pqServer.h"
#include "vtkBoundingBox.h"
#include "vtkPoints.h"
#include "vtkSMProxySelectionModel.h"
#include "vtkSMUtilities.h"

#include <QDoubleValidator>
#include <QVector3D>

class pqOrbitCreatorDialog::pqInternals : public Ui::OrbitCreatorDialog
{
};

//-----------------------------------------------------------------------------
pqOrbitCreatorDialog::pqOrbitCreatorDialog(QWidget* parentObject)
  : Superclass(parentObject)
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->Internals->center0->setValidator(new QDoubleValidator(this));
  this->Internals->center1->setValidator(new QDoubleValidator(this));
  this->Internals->center2->setValidator(new QDoubleValidator(this));

  this->Internals->normal0->setValidator(new QDoubleValidator(this));
  this->Internals->normal1->setValidator(new QDoubleValidator(this));
  this->Internals->normal2->setValidator(new QDoubleValidator(this));

  this->Internals->origin0->setValidator(new QDoubleValidator(this));
  this->Internals->origin1->setValidator(new QDoubleValidator(this));
  this->Internals->origin2->setValidator(new QDoubleValidator(this));

  QObject::connect(this->Internals->resetCenter, SIGNAL(clicked()), this, SLOT(resetCenter()));

  this->Internals->normal0->setText("0");
  this->Internals->normal1->setText("1");
  this->Internals->normal2->setText("0");
  this->Internals->origin0->setText("0");
  this->Internals->origin1->setText("0");
  this->Internals->origin2->setText("10");
  this->Internals->center0->setText("0");
  this->Internals->center1->setText("0");
  this->Internals->center2->setText("0");

  this->resetCenter();
}

//-----------------------------------------------------------------------------
pqOrbitCreatorDialog::~pqOrbitCreatorDialog()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqOrbitCreatorDialog::setNormal(double xyz[3])
{
  this->Internals->normal0->setText(QString::number(xyz[0]));
  this->Internals->normal1->setText(QString::number(xyz[1]));
  this->Internals->normal2->setText(QString::number(xyz[2]));
}
//-----------------------------------------------------------------------------
void pqOrbitCreatorDialog::setOrigin(double xyz[3])
{
  this->Internals->origin0->setText(QString::number(xyz[0]));
  this->Internals->origin1->setText(QString::number(xyz[1]));
  this->Internals->origin2->setText(QString::number(xyz[2]));
}
//-----------------------------------------------------------------------------
void pqOrbitCreatorDialog::setCenter(double xyz[3])
{
  this->Internals->center0->setText(QString::number(xyz[0]));
  this->Internals->center1->setText(QString::number(xyz[1]));
  this->Internals->center2->setText(QString::number(xyz[2]));
}

//-----------------------------------------------------------------------------
void pqOrbitCreatorDialog::resetCenter()
{
  double input_bounds[6];
  vtkSMProxySelectionModel* selModel =
    pqActiveObjects::instance().activeServer()->activeSourcesSelectionModel();
  if (selModel->GetSelectionDataBounds(input_bounds))
  {
    vtkBoundingBox box;
    box.SetBounds(input_bounds);
    double box_center[3];
    box.GetCenter(box_center);
    this->Internals->center0->setText(QString::number(box_center[0]));
    this->Internals->center1->setText(QString::number(box_center[1]));
    this->Internals->center2->setText(QString::number(box_center[2]));
  }
}

//-----------------------------------------------------------------------------
QVector3D pqOrbitCreatorDialog::normal() const
{
  QVector3D value;
  value.setX(this->Internals->normal0->text().toDouble());
  value.setY(this->Internals->normal1->text().toDouble());
  value.setZ(this->Internals->normal2->text().toDouble());
  return value;
}

//-----------------------------------------------------------------------------
QList<QVariant> pqOrbitCreatorDialog::orbitPoints(int resolution) const
{
  double box_center[3];
  double normal[3];
  double origin[3];

  box_center[0] = this->Internals->center0->text().toDouble();
  box_center[1] = this->Internals->center1->text().toDouble();
  box_center[2] = this->Internals->center2->text().toDouble();

  normal[0] = this->Internals->normal0->text().toDouble();
  normal[1] = this->Internals->normal1->text().toDouble();
  normal[2] = this->Internals->normal2->text().toDouble();

  origin[0] = this->Internals->origin0->text().toDouble();
  origin[1] = this->Internals->origin1->text().toDouble();
  origin[2] = this->Internals->origin2->text().toDouble();

  QList<QVariant> points;
  vtkPoints* pts = vtkSMUtilities::CreateOrbit(box_center, normal, resolution, origin);
  for (vtkIdType cc = 0; cc < pts->GetNumberOfPoints(); cc++)
  {
    double coords[3];
    pts->GetPoint(cc, coords);
    points << coords[0] << coords[1] << coords[2];
  }
  pts->Delete();
  return points;
}
