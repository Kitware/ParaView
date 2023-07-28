// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqTimeKeeper.h"

#include "vtkCommand.h"
#include "vtkEventQtSlotConnect.h"
#include "vtkNew.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMProxy.h"
#include "vtkSMTimeKeeperProxy.h"

#include <QList>
#include <QtDebug>

#include <vector>
//-----------------------------------------------------------------------------
class pqTimeKeeper::pqInternals
{
public:
  vtkNew<vtkEventQtSlotConnect> VTKConnect;
};

//-----------------------------------------------------------------------------
pqTimeKeeper::pqTimeKeeper(const QString& group, const QString& name, vtkSMProxy* timekeeper,
  pqServer* server, QObject* _parent /*=0*/)
  : pqProxy(group, name, timekeeper, server, _parent)
{
  this->Internals = new pqInternals();
  this->Internals->VTKConnect->Connect(
    timekeeper->GetProperty("Time"), vtkCommand::ModifiedEvent, this, SIGNAL(timeChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimestepValues"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeStepsChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimestepValues"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeRangeChanged()));
  this->Internals->VTKConnect->Connect(timekeeper->GetProperty("TimeRange"),
    vtkCommand::ModifiedEvent, this, SIGNAL(timeRangeChanged()));
}

//-----------------------------------------------------------------------------
pqTimeKeeper::~pqTimeKeeper()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
QList<double> pqTimeKeeper::getTimeSteps() const
{
  vtkSMPropertyHelper helper(this->getProxy(), "TimestepValues");
  QList<double> list;
  for (unsigned int cc = 0; cc < helper.GetNumberOfElements(); cc++)
  {
    list.push_back(helper.GetAsDouble(cc));
  }
  return list;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getNumberOfTimeStepValues() const
{
  return vtkSMPropertyHelper(this->getProxy(), "TimestepValues").GetNumberOfElements();
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTimeStepValue(int index) const
{
  if (index < this->getNumberOfTimeStepValues())
  {
    return vtkSMPropertyHelper(this->getProxy(), "TimestepValues").GetAsDouble(index);
  }

  return 0.0;
}

//-----------------------------------------------------------------------------
int pqTimeKeeper::getTimeStepValueIndex(double time) const
{
  return vtkSMTimeKeeperProxy::GetLowerBoundTimeStepIndex(this->getProxy(), time);
}

//-----------------------------------------------------------------------------
QPair<double, double> pqTimeKeeper::getTimeRange() const
{
  vtkSMPropertyHelper helper(this->getProxy(), "TimeRange");
  return QPair<double, double>(helper.GetAsDouble(0), helper.GetAsDouble(1));
}

//-----------------------------------------------------------------------------
double pqTimeKeeper::getTime() const
{
  return vtkSMPropertyHelper(this->getProxy(), "Time").GetAsDouble(0);
}

//-----------------------------------------------------------------------------
void pqTimeKeeper::setTime(double time)
{
  vtkSMPropertyHelper(this->getProxy(), "Time").Set(time);
  this->getProxy()->UpdateVTKObjects();
}
