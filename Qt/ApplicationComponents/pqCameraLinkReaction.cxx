// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCameraLinkReaction.h"

#include "pqActiveObjects.h"
#include "pqRenderView.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqCameraLinkReaction::pqCameraLinkReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnableState()), Qt::QueuedConnection);
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqCameraLinkReaction::updateEnableState()
{
  this->parentAction()->setEnabled(
    qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView()) != nullptr &&
    this->IsMaster);
}

//-----------------------------------------------------------------------------
void pqCameraLinkReaction::addCameraLink()
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (rm)
  {
    rm->linkToOtherView();
  }
  else
  {
    qCritical() << "No render module is active";
  }
}
