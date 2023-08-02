// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveSourceBehavior_h
#define pqLiveSourceBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QScopedPointer>

class pqPipelineSource;
class pqView;

/**
 * @class pqLiveSourceBehavior
 * @ingroup Behaviors
 *
 * pqLiveSourceBehavior adds support for "live" algorithms. These are
 * vtkAlgorithm subclasses that have a method `GetNeedsUpdate` which returns
 * true (bool) when source may have new data and should be refreshed.
 *
 * To indicate a source is a "live source", one needs to simply add
 * `<LiveSource>` XML hint for the source proxy.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqLiveSourceBehavior : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqLiveSourceBehavior(QObject* parent = nullptr);
  ~pqLiveSourceBehavior() override;

  /**
   * Pause live updates.
   */
  static void pause();

  /**
   * Resume live updates.
   */
  static void resume();

  /**
   * Returns true if live updates are paused.
   */
  static bool isPaused() { return pqLiveSourceBehavior::PauseLiveUpdates; }

protected Q_SLOTS:
  void viewAdded(pqView*);
  void sourceAdded(pqPipelineSource*);
  void timeout();

private:
  Q_DISABLE_COPY(pqLiveSourceBehavior);
  void startInteractionEvent();
  void endInteractionEvent();

  class pqInternals;
  QScopedPointer<pqInternals> Internals;

  static bool PauseLiveUpdates;
};

#endif
