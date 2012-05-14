/*=========================================================================

   Program: ParaView
   Module:    pqAxesToolbar.cxx

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
#include "pqAxesToolbar.h"
#include "ui_pqAxesToolbar.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"
#include "pqDataRepresentation.h"
#include "pqRubberBandHelper.h"


class pqAxesToolbar::pqInternals : public Ui::pqAxesToolbar
{
};

//-----------------------------------------------------------------------------
void pqAxesToolbar::constructor()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  this->PickHelper = new pqRubberBandHelper(this);
  QObject::connect(this->PickHelper, SIGNAL(enablePick(bool)),
    this->Internals->actionPickCenter, SLOT(setEnabled(bool)));
  QObject::connect(this->PickHelper, SIGNAL(selecting(bool)),
    this->Internals->actionPickCenter, SLOT(setChecked(bool)));
  QObject::connect(this->PickHelper,
    SIGNAL(intersectionFinished(double, double, double)),
    this, SLOT(pickCenterOfRotationFinished(double, double, double)));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(viewChanged(pqView*)),
    this, SLOT(updateEnabledState()),
    Qt::QueuedConnection);

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(sourceChanged(pqPipelineSource*)),
    this, SLOT(updateEnabledState()));

  QObject::connect(&pqActiveObjects::instance(),
    SIGNAL(representationChanged(pqRepresentation*)),
    this, SLOT(updateEnabledState()));

  QObject::connect(this->Internals->actionShowOrientationAxes,
    SIGNAL(toggled(bool)), this, SLOT(showOrientationAxes(bool)));

  QObject::connect(this->Internals->actionShowCenterAxes, SIGNAL(toggled(bool)),
    this, SLOT(showCenterAxes(bool)));

  QObject::connect(this->Internals->actionResetCenter, SIGNAL(triggered()),
    this, SLOT(resetCenterOfRotationToCenterOfCurrentData()));

  QObject::connect(this->Internals->actionPickCenter, SIGNAL(toggled(bool)),
    this, SLOT(pickCenterOfRotation(bool)));

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqAxesToolbar::~pqAxesToolbar()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::updateEnabledState()
{
  pqRenderView* renderView =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());

  this->Internals->actionShowOrientationAxes->setEnabled(renderView != NULL);
  this->Internals->actionShowOrientationAxes->blockSignals(true);
  this->Internals->actionShowOrientationAxes->setChecked(
    renderView? renderView->getOrientationAxesVisibility() : false);
  this->Internals->actionShowOrientationAxes->blockSignals(false);

  this->Internals->actionShowCenterAxes->setEnabled(renderView != NULL);
  this->Internals->actionShowCenterAxes->blockSignals(true);
  this->Internals->actionShowCenterAxes->setChecked(
    renderView? renderView->getCenterAxesVisibility() : false);
  this->Internals->actionShowCenterAxes->blockSignals(false);
  this->Internals->actionResetCenter->setEnabled(
    pqActiveObjects::instance().activeRepresentation() != NULL);
  this->PickHelper->setView(renderView);
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::showOrientationAxes(bool show_axes)
{
  pqRenderView* renderView =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
    {
    return;
    }

  renderView->setOrientationAxesVisibility(show_axes);
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::showCenterAxes(bool show_axes)
{
  pqRenderView* renderView =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
    {
    return;
    }

  renderView->setCenterAxesVisibility(show_axes);
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::resetCenterOfRotationToCenterOfCurrentData()
{
  pqRenderView* renderView =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr || !renderView)
    {
    //qDebug() << "Active source not shown in active view. Cannot set center.";
    return;
    }

  double bounds[6];
  if (repr->getDataBounds(bounds))
    {
    double center[3];
    center[0] = (bounds[1]+bounds[0])/2.0;
    center[1] = (bounds[3]+bounds[2])/2.0;
    center[2] = (bounds[5]+bounds[4])/2.0;
    renderView->setCenterOfRotation(center);
    renderView->render();
    }

}

//-----------------------------------------------------------------------------
void pqAxesToolbar::pickCenterOfRotation(bool begin)
{
  if (begin)
    {
    this->PickHelper->beginFastIntersect();
    }
  else
    {
    this->PickHelper->endSelection();
    }
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::pickCenterOfRotationFinished(double _x, double _y, double _z)
{
  this->pickCenterOfRotation(false);
  pqRenderView* rm =
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!rm)
    {
    qDebug("No active render module. Cannot reset center of rotation.");
    return;
    }

  double center[3];
  center[0] = _x;
  center[1] = _y;
  center[2] = _z;
  rm->setCenterOfRotation(center);
  rm->render();
}
