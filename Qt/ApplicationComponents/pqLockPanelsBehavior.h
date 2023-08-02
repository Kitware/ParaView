// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLockPanelsBehavior_h
#define pqLockPanelsBehavior_h

#include <QObject>
#include <QScopedPointer>

#include "pqApplicationComponentsModule.h"

class QAction;

/**
 * @ingroup Behaviors
 * Central location for controlling whether dock widgets are locked down
 * or movable.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLockPanelsBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  /**
   * Note: parent is assumed to be a subclass of QMainWindow, e.g.,
   * ParaViewMainWindow.
   */
  pqLockPanelsBehavior(QObject* parent = nullptr);
  ~pqLockPanelsBehavior() override;

  /**
   * Invoked when the vtkPVGeneralSettings singleton is modified.
   */
  void generalSettingsChanged();

  /**
   * Toggles the dock panel locking state.
   */
  static void toggleLockPanels();

private:
  Q_DISABLE_COPY(pqLockPanelsBehavior);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  /**
   * Set the dock panel locking state to locked or unlocked.
   */
  void lockPanels(bool lock);
};

#endif // pqLockPanelsBehavior_h
