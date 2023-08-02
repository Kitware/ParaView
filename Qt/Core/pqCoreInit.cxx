// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqCoreInit.h"
#include <QObject>
#include <QtPlugin>

void pqCoreInit()
{
  // #if !BUILD_SHARED_LIBS
  Q_INIT_RESOURCE(pqCore);
  // Q_INIT_RESOURCE(QtWidgets);
  // #endif
}
