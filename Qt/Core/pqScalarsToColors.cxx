/*=========================================================================

   Program: ParaView
   Module:    pqScalarsToColors.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "pqScalarsToColors.h"

#include "vtkSMProxy.h"

#include <QPointer>
#include <QList>
#include <QtDebug>

#include "pqApplicationCore.h"
#include "pqPipelineDisplay.h"
#include "pqScalarBarDisplay.h"
#include "pqServerManagerModel.h"
#include "pqSMAdaptor.h"
#include "pqRenderViewModule.h"

//-----------------------------------------------------------------------------
class pqScalarsToColorsInternal
{
public:
  QList<QPointer<pqScalarBarDisplay> > ScalarBars;
  bool ScalarRangeInitialized;
};

//-----------------------------------------------------------------------------
pqScalarsToColors::pqScalarsToColors(const QString& group, const QString& name,
    vtkSMProxy* proxy, pqServer* server, QObject* _parent/*=NULL*/)
: pqProxy(group, name, proxy, server, _parent)
{
  this->Internal = new pqScalarsToColorsInternal;
  this->Internal->ScalarRangeInitialized = false;
}

//-----------------------------------------------------------------------------
pqScalarsToColors::~pqScalarsToColors()
{
  delete this->Internal;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::addScalarBar(pqScalarBarDisplay* sb)
{
  if (this->Internal->ScalarBars.indexOf(sb) == -1)
    {
    this->Internal->ScalarBars.push_back(sb);
    }
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::removeScalarBar(pqScalarBarDisplay* sb)
{
  this->Internal->ScalarBars.removeAll(sb);
}

//-----------------------------------------------------------------------------
pqScalarBarDisplay* pqScalarsToColors::getScalarBar(pqRenderViewModule* ren) const
{
  foreach(pqScalarBarDisplay* sb, this->Internal->ScalarBars)
    {
    if (sb && sb->shownIn(ren))
      {
      return sb;
      }
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setScalarRangeLock(bool lock)
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("LockScalarRange");
  if (prop)
    {
    pqSMAdaptor::setElementProperty(prop, (lock? 1: 0));
    }
  this->getProxy()->UpdateVTKObjects();
}

//-----------------------------------------------------------------------------
bool pqScalarsToColors::getScalarRangeLock() const
{
  vtkSMProperty* prop = this->getProxy()->GetProperty("LockScalarRange");
  if (prop && pqSMAdaptor::getElementProperty(prop).toInt() != 0)
    {
    return true;
    }
  // we may keep some GUI only state for vtkLookupTable proxies.
  return false;
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::hideUnusedScalarBars()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  QList<pqPipelineDisplay*> displays = smmodel->getPipelineDisplays(
    this->getServer());

  bool used_at_all = false;
  foreach(pqPipelineDisplay* display, displays)
    {
    if (display->getLookupTableProxy() == this->getProxy())
      {
      used_at_all = true;
      break;
      }
    }
  if (!used_at_all)
    {
    foreach(pqScalarBarDisplay* sb, this->Internal->ScalarBars)
      {
      sb->setVisible(false);
      sb->renderAllViews();
      }
    }
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setScalarRange(double min, double max)
{
  if (min > max)
    {
    double t = min;
    min = max;
    max = t;
    }
  QList<QVariant> curRange;
  curRange << min << max;

  pqSMAdaptor::setMultipleElementProperty(
    this->getProxy()->GetProperty("ScalarRange"), curRange);
  this->getProxy()->UpdateVTKObjects();

  this->Internal->ScalarRangeInitialized = true;
}

//-----------------------------------------------------------------------------
QPair<double, double> pqScalarsToColors::getScalarRange() const
{
  QList<QVariant> range = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ScalarRange"));
  return QPair<double, double>(range[0].toDouble(), range[1].toDouble());
}

//-----------------------------------------------------------------------------
void pqScalarsToColors::setWholeScalarRange(double min, double max)
{
  if (this->getScalarRangeLock())
    {
    return;
    }

  QList<QVariant> curRange = pqSMAdaptor::getMultipleElementProperty(
    this->getProxy()->GetProperty("ScalarRange"));
  min = (!this->Internal->ScalarRangeInitialized || min < curRange[0].toDouble())? 
    min :  curRange[0].toDouble();
  max = (!this->Internal->ScalarRangeInitialized || max > curRange[1].toDouble())? 
    max :  curRange[1].toDouble();

  this->setScalarRange(min, max);
}
