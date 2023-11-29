// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqMyApplicationStarter.h"

#include <pqAnimationManager.h>
#include <pqAnimationScene.h>
#include <pqPVApplicationCore.h>
#include <vtkLogger.h>

#include <QTimer>

//-----------------------------------------------------------------------------
pqMyApplicationStarter::pqMyApplicationStarter(QObject* p /*=0*/)
  : QObject(p)
{
}

//-----------------------------------------------------------------------------
pqMyApplicationStarter::~pqMyApplicationStarter() = default;

//-----------------------------------------------------------------------------
void pqMyApplicationStarter::onStartup()
{
  vtkLog(INFO, "Message from pqMyApplicationStarter: Application Started");

  // Create a single shot timer that will trigger as soon as paraview starts processing events.
  // Needed because playing the animation at startup is not supported.
  QTimer::singleShot(1, this, &pqMyApplicationStarter::playAnimation);
}

//-----------------------------------------------------------------------------
void pqMyApplicationStarter::playAnimation()
{
  vtkLog(INFO, "Message from pqMyApplicationStarter: Playing animation");
  pqPVApplicationCore::instance()->animationManager()->getActiveScene()->play();
}
//-----------------------------------------------------------------------------
void pqMyApplicationStarter::onShutdown()
{
  vtkLog(INFO, "Message from pqMyApplicationStarter: Application Shutting down");
}
