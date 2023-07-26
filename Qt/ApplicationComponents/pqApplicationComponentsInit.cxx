// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE
#include "pqApplicationComponentsInit.h"

#include "pqComponentsInit.h"
#include <QObject> // for Q_INIT_RESOURCE
#include <QtPlugin>

void pqApplicationComponentsInit()
{
  // init dependents
  pqComponentsInit();

  // init resources
  Q_INIT_RESOURCE(pqApplicationComponents);
}
