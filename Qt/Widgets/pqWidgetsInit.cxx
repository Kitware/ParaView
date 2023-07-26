// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqWidgetsInit.h"
#include <QObject>
#include <QtPlugin>

void pqWidgetsInit()
{
  Q_INIT_RESOURCE(QtWidgets);
}
