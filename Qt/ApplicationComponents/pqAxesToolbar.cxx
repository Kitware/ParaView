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
#include "pqDataRepresentation.h"
#include "pqPropertyLinks.h"
#include "pqRenderView.h"
#include "pqRenderViewSelectionReaction.h"
#include "pqView.h"
#include "vtkSMRenderViewProxy.h"
#include "vtkSMTrace.h"

class pqAxesToolbar::pqInternals : public Ui::pqAxesToolbar
{
public:
  pqView* View = nullptr;
  pqPropertyLinks Links;
};

//-----------------------------------------------------------------------------
void pqAxesToolbar::constructor()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(setView(pqView*)), Qt::QueuedConnection);
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnabledState()), Qt::QueuedConnection);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(sourceChanged(pqPipelineSource*)), this,
    SLOT(updateEnabledState()));

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(representationChanged(pqRepresentation*)),
    this, SLOT(updateEnabledState()));

  QObject::connect(this->Internals->actionShowOrientationAxes, SIGNAL(toggled(bool)), this,
    SLOT(showOrientationAxes(bool)));

  QObject::connect(
    this->Internals->actionShowCenterAxes, SIGNAL(toggled(bool)), this, SLOT(showCenterAxes(bool)));

  QObject::connect(this->Internals->actionResetCenter, SIGNAL(triggered()), this,
    SLOT(resetCenterOfRotationToCenterOfCurrentData()));

  pqRenderViewSelectionReaction* selectionReaction =
    new pqRenderViewSelectionReaction(this->Internals->actionPickCenter,
      NULL /* track active view*/, pqRenderViewSelectionReaction::SELECT_CUSTOM_BOX);
  QObject::connect(selectionReaction, SIGNAL(selectedCustomBox(int, int, int, int)), this,
    SLOT(pickCenterOfRotation(int, int)));

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqAxesToolbar::~pqAxesToolbar()
{
  delete this->Internals;
  this->Internals = 0;
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::setView(pqView* view)
{
  if (this->Internals->View == view)
  {
    return;
  }

  this->Internals->View = view;
  this->Internals->Links.removeAllPropertyLinks();

  if (!(view && view->getProxy()->GetProperty("OrientationAxesVisibility")))
  {
    return;
  }

  this->Internals->Links.addPropertyLink(this->Internals->actionShowOrientationAxes, "checked",
    SIGNAL(toggled(bool)), view->getProxy(),
    view->getProxy()->GetProperty("OrientationAxesVisibility"));
  this->Internals->Links.addPropertyLink(this->Internals->actionShowCenterAxes, "checked",
    SIGNAL(toggled(bool)), view->getProxy(), view->getProxy()->GetProperty("CenterAxesVisibility"));
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::updateEnabledState()
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  this->Internals->actionShowOrientationAxes->setEnabled(renderView != NULL);
  this->Internals->actionShowCenterAxes->setEnabled(renderView != NULL);
  this->Internals->actionResetCenter->setEnabled(
    pqActiveObjects::instance().activeRepresentation() != NULL);
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::showOrientationAxes(bool show_axes)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }

  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", renderView->getProxy())
    .arg(
      "comment", QString(" %1 orientation axes").arg(show_axes ? "Show" : "Hide").toUtf8().data());
  renderView->setOrientationAxesVisibility(show_axes);
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::showCenterAxes(bool show_axes)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!renderView)
  {
    return;
  }

  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", renderView->getProxy())
    .arg("comment", QString(" %1 center axes").arg(show_axes ? "Show" : "Hide").toUtf8().data());
  renderView->setCenterAxesVisibility(show_axes);
  renderView->render();
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::resetCenterOfRotationToCenterOfCurrentData()
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  pqDataRepresentation* repr = pqActiveObjects::instance().activeRepresentation();
  if (!repr || !renderView)
  {
    // qDebug() << "Active source not shown in active view. Cannot set center.";
    return;
  }

  SM_SCOPED_TRACE(PropertiesModified)
    .arg("proxy", renderView->getProxy())
    .arg("comment", " update center of rotation");

  double bounds[6];
  if (repr->getDataBounds(bounds))
  {
    double center[3];
    center[0] = (bounds[1] + bounds[0]) / 2.0;
    center[1] = (bounds[3] + bounds[2]) / 2.0;
    center[2] = (bounds[5] + bounds[4]) / 2.0;
    renderView->setCenterOfRotation(center);
    renderView->render();
  }
}

//-----------------------------------------------------------------------------
void pqAxesToolbar::pickCenterOfRotation(int posx, int posy)
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (rm)
  {
    int posxy[2] = { posx, posy };
    double center[3];

    vtkSMRenderViewProxy* proxy = rm->getRenderViewProxy();
    SM_SCOPED_TRACE(PropertiesModified)
      .arg("proxy", proxy)
      .arg("comment", " update center of rotation");
    if (proxy->ConvertDisplayToPointOnSurface(posxy, center))
    {
      rm->setCenterOfRotation(center);
      rm->render();
    }
  }
}
