// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqWaitCursor.h"

#include <QApplication>
#include <QCursor>

pqWaitCursor::pqWaitCursor()
  : pqScopedOverrideCursor(Qt::WaitCursor)
{
}
