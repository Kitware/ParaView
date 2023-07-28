// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqDataQueryReaction.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"

#include <QDockWidget>

//-----------------------------------------------------------------------------
pqDataQueryReaction::pqDataQueryReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqDataQueryReaction::~pqDataQueryReaction() = default;

//-----------------------------------------------------------------------------
void pqDataQueryReaction::showQueryPanel()
{
  // Raise the color editor is present in the application.
  QDockWidget* widget =
    qobject_cast<QDockWidget*>(pqApplicationCore::instance()->manager("FIND_DATA_PANEL"));
  if (widget)
  {
    widget->setVisible(true);
    // widget->setFloating(true);
    widget->raise();
  }
  else
  {
    qDebug("Failed to find 'FIND_DATA_PANEL'.");
  }
}
