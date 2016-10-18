/*=========================================================================

   Program: ParaView
   Module:    pqTimeKeeper.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
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
