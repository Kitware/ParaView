// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqMainControlsToolbar_h
#define pqMainControlsToolbar_h

#include "pqApplicationComponentsModule.h"
#include <QToolBar>

/**
 * pqMainControlsToolbar is the toolbar with actions (and reactions) for the
 * "Main Controls" toolbar in ParaView. It includes buttons like "Open Data",
 * "Save Data", "Connect", "Disconnect", "Undo", "Redo".
 * Simply instantiate this and put it in your application UI file or
 * QMainWindow to use it.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqMainControlsToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;

public:
  pqMainControlsToolbar(const QString& title, QWidget* parentObject = nullptr)
    : Superclass(title, parentObject)
  {
    this->constructor();
  }
  pqMainControlsToolbar(QWidget* parentObject = nullptr)
    : Superclass(parentObject)
  {
    this->constructor();
  }

private:
  Q_DISABLE_COPY(pqMainControlsToolbar)

  void constructor();
};

#endif
