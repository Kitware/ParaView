// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation, Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

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
