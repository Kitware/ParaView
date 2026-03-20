// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAbortTesting.h"

#include "pqApplicationCore.h"
#include "pqProgressManager.h"

//-----------------------------------------------------------------------------
pqAbortTesting::pqAbortTesting(QObject* parent)
  : QObject(parent)
{
}

//-----------------------------------------------------------------------------
pqAbortTesting::~pqAbortTesting() = default;

//-----------------------------------------------------------------------------
void pqAbortTesting::onStartup()
{
  pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
  QObject::connect(pmManager, &pqProgressManager::progress, this, &pqAbortTesting::onProgress);
}

//-----------------------------------------------------------------------------
void pqAbortTesting::onProgress(const QString&, int progress)
{
  if (progress > 25)
  {
    pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
    pmManager->triggerAbort();
  }
}
