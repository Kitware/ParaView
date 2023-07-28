// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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

#include <cmath>

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

  QObject::connect(
    &pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this, SLOT(setView(pqView*)));
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
      nullptr /* track active view*/, pqRenderViewSelectionReaction::SELECT_CUSTOM_BOX);
  QObject::connect(selectionReaction, SIGNAL(selectedCustomBox(int, int, int, int)), this,
    SLOT(pickCenterOfRotation(int, int)));

  this->updateEnabledState();
}

//-----------------------------------------------------------------------------
pqAxesToolbar::~pqAxesToolbar()
{
  delete this->Internals;
  this->Internals = nullptr;
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
  this->Internals->actionShowOrientationAxes->setEnabled(renderView != nullptr);
  this->Internals->actionShowCenterAxes->setEnabled(renderView != nullptr);
  this->Internals->actionResetCenter->setEnabled(
    pqActiveObjects::instance().activeRepresentation() != nullptr);
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
    .arg("comment",
      qPrintable(show_axes ? tr(" Show orientation axes") : tr(" Hide orientation axes")));
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
    .arg("comment", qPrintable(show_axes ? tr(" Show center axes") : tr(" Hide center axes")));
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
    .arg("comment", qPrintable(tr(" update center of rotation")));

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
    double center[3], normal[3];

    vtkSMRenderViewProxy* proxy = rm->getRenderViewProxy();
    SM_SCOPED_TRACE(PropertiesModified)
      .arg("proxy", proxy)
      .arg("comment", qPrintable(tr(" update center of rotation")));
    proxy->ConvertDisplayToPointOnSurface(posxy, center, normal);
    if (!std::isnan(center[0]) || !std::isnan(center[1]) || !std::isnan(center[2]))
    {
      rm->setCenterOfRotation(center);
      rm->render();
    }
  }
}
