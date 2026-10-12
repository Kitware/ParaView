// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAbortTesting.h"

#include "pqApplicationCore.h"
#include "pqProgressManager.h"
#include "pqSettings.h"

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
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool autoAbort = settings->value("AbortTestingSettings.AutoAbort", true).toBool();
  int autoAbortValue = settings->value("AbortTestingSettings.AutoAbortValue", 50).toInt();
  if (autoAbort && progress > autoAbortValue)
  {
    pqProgressManager* pmManager = pqApplicationCore::instance()->getProgressManager();
    pmManager->triggerAbort();
  }
}
