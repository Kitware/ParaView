// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqLiveSourceBehavior_h
#define pqLiveSourceBehavior_h

#include "pqApplicationComponentsModule.h"
#include <QObject>
#include <QScopedPointer>

#include "vtkParaViewDeprecation.h" // for deprecation macro

class pqPipelineSource;
class pqView;
class pqLiveSourceManager;

/**
 * @class pqLiveSourceBehavior
 * @ingroup Behaviors
 *
 * pqLiveSourceBehavior adds support for generated "live" sources algorithms.
 * These are vtkAlgorithm subclasses that have a method `GetNeedsUpdate`
 * which returns true (bool) when source may have new data and should be refreshed.
 * This must be not confused with ParaView Catalyst.
 *
 * To indicate a source is a "live source", one needs to simply add
 * `<LiveSource>` XML hint for the source proxy.
 *
 * This tag can have multiple attributes:
 *   - `interval`: Call `GetNeedsUpdate` at interval value in milliseconds
 *     (default to 100).
 *   - `emulated_time`: Boolean to mark the live source as an algorithm inheriting
 *     from `vtkEmulatedTimeAlgorithm`, which will play all timesteps available in
 *     an emulated real time.
 *
 * @sa pqLiveSourceManager pqLiveSourceItem vtkEmulatedTimeAlgorithm
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
  PARAVIEW_DEPRECATED_IN_5_13_0("Please use pqLiveSourceManager::pause() instead")
  static void pause();

  /**
   * Resume live updates.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Please use pqLiveSourceManager::resume() instead")
  static void resume();

  /**
   * Returns true if live updates are paused.
   */
  PARAVIEW_DEPRECATED_IN_5_13_0("Please use pqLiveSourceManager::isPaused() instead")
  static bool isPaused();

private:
  Q_DISABLE_COPY(pqLiveSourceBehavior);
};

#endif
