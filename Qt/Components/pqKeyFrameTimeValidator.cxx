// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqKeyFrameTimeValidator.h"

#include "vtkEventQtSlotConnect.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSmartPointer.h"

#include <QPointer>

#include "pqAnimationScene.h"

//-----------------------------------------------------------------------------
class pqKeyFrameTimeValidator::pqInternals
{
public:
  vtkSmartPointer<vtkSMDoubleRangeDomain> Domain;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnect;
  QPointer<pqAnimationScene> AnimationScene;

  pqInternals() { this->VTKConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New(); }
};

//-----------------------------------------------------------------------------
pqKeyFrameTimeValidator::pqKeyFrameTimeValidator(QObject* p)
  : QDoubleValidator(p)
{
  this->Internals = new pqInternals;
}

//-----------------------------------------------------------------------------
pqKeyFrameTimeValidator::~pqKeyFrameTimeValidator()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::setDomain(vtkSMDomain* domain)
{
  vtkSMDoubleRangeDomain* drd = vtkSMDoubleRangeDomain::SafeDownCast(domain);
  this->Internals->VTKConnect->Disconnect();

  this->Internals->Domain = drd;
  if (drd)
  {
    this->Internals->VTKConnect->Connect(
      drd, vtkCommand::DomainModifiedEvent, this, SLOT(onDomainModified()));
  }
  this->onDomainModified();
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::setAnimationScene(pqAnimationScene* scene)
{
  if (this->Internals->AnimationScene)
  {
    QObject::disconnect(this->Internals->AnimationScene, nullptr, this, nullptr);
  }
  this->Internals->AnimationScene = scene;
  if (scene)
  {
    QObject::connect(scene, SIGNAL(clockTimeRangesChanged()), this, SLOT(onDomainModified()),
      Qt::QueuedConnection);
  }
  this->onDomainModified();
}

//-----------------------------------------------------------------------------
void pqKeyFrameTimeValidator::onDomainModified()
{
  if (!this->Internals->Domain)
  {
    return;
  }

  double min = this->Internals->Domain->GetMinimum(0);
  double max = this->Internals->Domain->GetMaximum(0);

  if (this->Internals->AnimationScene)
  {
    QPair<double, double> range = this->Internals->AnimationScene->getClockTimeRange();
    min = range.first + (range.second - range.first) * min;
    max = range.first + (range.second - range.first) * max;
  }
  this->setRange(min, max);
}
