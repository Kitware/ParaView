// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-CLAUSE

#include "pqWaitCursor.h"

#include <QApplication>
#include <QCursor>

pqWaitCursor::pqWaitCursor()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

pqWaitCursor::~pqWaitCursor()
{
  QApplication::restoreOverrideCursor();
}
