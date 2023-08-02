// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqPersistentMainWindowStateBehavior_h
#define pqPersistentMainWindowStateBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class QMainWindow;

/**
 * @ingroup Behaviors
 * pqPersistentMainWindowStateBehavior saves and restores the MainWindow state
 * on shutdown and restart. Simply instantiate this behavior if you want your
 * main window layout to be persistent.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqPersistentMainWindowStateBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Parent cannot be nullptr.
   */
  pqPersistentMainWindowStateBehavior(QMainWindow* parent);
  ~pqPersistentMainWindowStateBehavior() override;

  static void restoreState(QMainWindow*);
  static void saveState(QMainWindow*);

protected Q_SLOTS:
  void saveState();
  void restoreState();

private:
  Q_DISABLE_COPY(pqPersistentMainWindowStateBehavior)
};

#endif
