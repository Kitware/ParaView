// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveSourceManager_h
#define pqLiveSourceManager_h

#include <QObject>

#include "pqComponentsModule.h" // for exports

class pqPipelineSource;
class pqLiveSourceItem;
class vtkSMProxy;

/**
 * pqLiveSourceManager is the manager that handle all live sources in ParaView
 * It is usually instantiated by the pqLiveSourceBehavior.
 * It provide feature to control live sources.
 *
 * @sa pqLiveSourceBehavior pqLiveSourceItem
 */
class PQCOMPONENTS_EXPORT pqLiveSourceManager : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqLiveSourceManager(QObject* parent = nullptr);
  ~pqLiveSourceManager() override;

  ///@{
  /**
   * Pause / Resume live updates for all live sources.
   * Also pause / resume emulated time timer.
   */
  void pause();
  void resume();
  ///@}

  /**
   * Returns true if all live source updates is paused.
   */
  bool isPaused();

  /**
   * Returns the pqLiveSourceItem corresponding to the proxy
   */
  pqLiveSourceItem* getLiveSourceItem(vtkSMProxy*);

  ///@{
  /**
   * Pause / Resume internal emulated time shared timer.
   */
  void pauseEmulatedTime();
  void resumeEmulatedTime();
  ///@}

  /**
   * Returns true if the emulated time timer is paused.
   */
  bool isEmulatedTimePaused();

  ///@{
  /**
   * Set/get playing speed for emulated time algorithms (in seconds).
   * Default to 1.
   */
  void setEmulatedSpeedMultiplier(double speed);
  double getEmulatedSpeedMultiplier();
  ///@}

  ///@{
  /**
   * Set/get current global time for emulated time algorithms.
   */
  void setEmulatedCurrentTime(double time);
  double getEmulatedCurrentTime();
  ///@}

Q_SIGNALS:
  /**
   * Triggered when emulated time shared timer is paused / resumed.
   */
  void emulatedTimeStateChanged(bool isPaused);

private Q_SLOTS:
  ///@{
  /**
   * Theses slots handle view and sources addition/removal.
   */
  void onSourceAdded(pqPipelineSource*);
  void onSourceRemove(pqPipelineSource*);
  ///@}

  /**
   * Update internal state of time ranges for all emulated
   * time algorithms live sources.
   */
  void onUpdateTimeRanges();

private: // NOLINT(readability-redundant-access-specifiers)
  Q_DISABLE_COPY(pqLiveSourceManager);

  class pqInternals;
  QScopedPointer<pqInternals> Internals;
};

#endif
