// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqLogViewerReaction.h"

#include "pqCoreUtilities.h"
#include "pqLogViewerDialog.h"

#include <QApplication>
#include <QPointer>

//-----------------------------------------------------------------------------
void pqLogViewerReaction::showLogViewer()
{
  static QPointer<pqLogViewerDialog> viewer;
  if (!viewer)
  {
    viewer = new pqLogViewerDialog(pqCoreUtilities::mainWidget());
  }
  viewer->setAttribute(Qt::WA_DeleteOnClose, false);
  viewer->show();
  viewer->raise();
  viewer->activateWindow();
  viewer->refresh();
}
