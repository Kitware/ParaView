// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "pqScopedOverrideCursor.h"

#include <QApplication>
#include <QCursor>

//-----------------------------------------------------------------------------
pqScopedOverrideCursor::pqScopedOverrideCursor(Qt::CursorShape cursorShape)
{
  QApplication::setOverrideCursor(QCursor(cursorShape));
}

//-----------------------------------------------------------------------------
pqScopedOverrideCursor::~pqScopedOverrideCursor()
{
  QApplication::restoreOverrideCursor();
}
