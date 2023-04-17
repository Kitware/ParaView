/*=========================================================================

   Program: ParaView
   Module:  pqTimelineWidget.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

========================================================================*/
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

  ///@{
  /**
   * Update the AnimationScene from the timeline track states.
   */
  // Update the scene and underlying timekeeper from the sources track states.
  void updateSceneFromEnabledSources();
  // Update the cues from the animation tracks.
  void updateSceneFromEnabledAnimations();
  ///@}

  ///@{
  /**
   * Update track data when proxy is updated
   */
  // Update DisplayRole and REGISTRATIONNAME data
  void renameTrackFromProxy(pqServerManagerModelItem* item);
  // Update SOURCETIME data
  void updateSourceTimeFromProxy(pqServerManagerModelItem* item);
  // Update cue cache. Useful to cleanup on proxy removal.
  void updateCueCache(const QString& group, const QString& name, vtkSMProxy* proxy);
  ///@}

  ///@{
  /**
   * Update animation tracks when cues changes.
   */
  // Add a track for new cue.
  void addCueTrack(pqAnimationCue* cue);
  // Remove the track from cue.
  void removeCueTrack(pqAnimationCue* cue);
  // Update track data from cue keyframes.
  void updateCueTracksData();
  // Update track checkstate from cue enabled state.
  void updateCueTracksState();
  ///@}

private:
  Q_DISABLE_COPY(pqTimelineWidget)
  struct pqInternals;
  std::unique_ptr<pqInternals> Internals;
};

#endif
