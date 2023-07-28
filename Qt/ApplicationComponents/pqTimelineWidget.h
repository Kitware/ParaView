// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqTimelineWidget_h
#define pqTimelineWidget_h

#include "pqApplicationComponentsModule.h"

#include <QWidget>

#include <memory> // for std::unique_ptr

class pqAnimationCue;
class pqAnimationScene;
class pqServerManagerModelItem;
class vtkSMProxy;

/**
 * pqTimelineWidget holds the timeline views and models for the time sources
 * and the animation tracks.
 */
class PQAPPLICATIONCOMPONENTS_EXPORT pqTimelineWidget : public QWidget
{
  Q_OBJECT

  typedef QWidget Superclass;

public:
  pqTimelineWidget(QWidget* parent = nullptr);
  ~pqTimelineWidget() override;

  /**
   * Set advanced mode. This is used to control advanced widgets visibility.
   */
  void setAdvancedMode(bool advanced);

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)
  /**
   * Updates internal connections with the session timekeeper.
   */
  void onNewSession();

  /**
   * Recreate time sources from time keeper.
   */
  void recreateTimeSources();

  /**
   * Updates Checkstate and TIMES data on TIME track from animation scene.
   */
  void updateTimeTrackFromScene();

  /** @name Scene update
   * Update the AnimationScene from the timeline track states.
   */
  ///@{
  /// Update the scene and underlying timekeeper from the sources track states.
  void updateSceneFromEnabledSources();
  /// Update the cues from the animation tracks.
  void updateSceneFromEnabledAnimations();
  ///@}

  /** @name Track update
   * Update track data when proxy is updated
   */
  ///@{
  /// Update DisplayRole and REGISTRATIONNAME data
  void renameTrackFromProxy(pqServerManagerModelItem* item);
  /// Update SOURCETIME data
  void updateSourceTimeFromProxy(pqServerManagerModelItem* item);
  /// Update cue cache. Useful to cleanup on proxy removal.
  void updateCueCache(const QString& group, const QString& name, vtkSMProxy* proxy);
  ///@}

  /** @name Cue update
   * Update animation tracks when cues changes.
   */
  ///@{
  /// Add a track for new cue.
  void addCueTrack(pqAnimationCue* cue);
  /// Remove the track from cue.
  void removeCueTrack(pqAnimationCue* cue);
  /// Update track data from cue keyframes.
  void updateCueTracksData();
  /// Update track checkstate from cue enabled state.
  void updateCueTracksState();
  ///@}

private:
  Q_DISABLE_COPY(pqTimelineWidget)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
