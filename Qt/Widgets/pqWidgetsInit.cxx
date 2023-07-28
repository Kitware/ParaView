// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqWidgetsInit.h"
#include <QObject>
#include <QtPlugin>

void pqWidgetsInit()
{
  Q_INIT_RESOURCE(QtWidgets);
}
