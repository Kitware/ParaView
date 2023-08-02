// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
