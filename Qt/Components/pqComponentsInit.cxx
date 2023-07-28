// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
