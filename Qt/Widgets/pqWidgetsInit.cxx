// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqWidgetsInit.h"
#include <QObject>
#include <QtPlugin>

void pqWidgetsInit()
{
  Q_INIT_RESOURCE(QtWidgets);
}
