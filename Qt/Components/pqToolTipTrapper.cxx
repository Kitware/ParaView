// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqToolTipTrapper.h"

#include <QCoreApplication>

///////////////////////////////////////////////////////////////////////////////
// pqToolTipTrapper

pqToolTipTrapper::pqToolTipTrapper()
{
  QCoreApplication::instance()->installEventFilter(this);
}

pqToolTipTrapper::~pqToolTipTrapper()
{
  QCoreApplication::instance()->removeEventFilter(this);
}

bool pqToolTipTrapper::eventFilter(QObject* /*watched*/, QEvent* input_event)
{
  if (input_event->type() == QEvent::ToolTip)
    return true;

  return false;
}
