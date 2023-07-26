// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqComponentsInit.h"
#include "pqCoreInit.h"
#include "pqWidgetsInit.h"
#include <QObject> // for Q_INIT_RESOURCE

void pqComponentsInit()
{
  // init dependents
  pqCoreInit();
  pqWidgetsInit();

  // init resources
  Q_INIT_RESOURCE(pqComponents);
}
