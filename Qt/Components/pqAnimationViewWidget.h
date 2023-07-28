// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationViewWidget_h
#define pqAnimationViewWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

#include "vtkParaViewDeprecation.h"

class pqAnimationKeyFrame;
class pqAnimationScene;
class pqAnimationTrack;
class pqPipelineSource;
class pqView;
class vtkSMProxy;

/**
 * This is the Animation panel widget. It controls the behavior
 * of the Animation panel which includes adding of key frames,
 * changing of keyframes etc etc.
 */

class PARAVIEW_DEPRECATED_IN_5_12_0(
  "Use `pqTimeManagerWidget` instead") PQCOMPONENTS_EXPORT pqAnimationViewWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqAnimationViewWidget(QWidget* parent = nullptr);
  ~pqAnimationViewWidget() override;

public Q_SLOTS: // NOLINT(readability-redundant-access-specifiers)

  /**
   * set the scene to view
   */
  void setScene(pqAnimationScene* scene);

protected Q_SLOTS:

  /**
   * The cues in the scene have changed, so we make sure that we are not
   * displaying a removed or added cue, if so we update the GUI.
   */
  void onSceneCuesChanged();

  /**
   * called when keyframes change
   */
  void keyFramesChanged(QObject*);

  /**
   * called when scene time range changes
   */
  void updateSceneTimeRange();
  /**
   * called when scene time changes
   */
  void updateSceneTime();
  /**
   * called when time steps changes
   */
  void updateTicks();

  // called when track is double clicked
  void trackSelected(pqAnimationTrack* track);

  // called when play mode changes
  void updatePlayMode();

  // called when number of keyframe/timesteps changed
  void updateStrideRange();

  /**
   * Called to toggle a track's enabled state.
   */
  void toggleTrackEnabled(pqAnimationTrack* track);

  // called when deleting a track
  void deleteTrack(pqAnimationTrack* track);
  // called when creating a track
  void createTrack();

  /**
   * called to create a new python animation track.
   * we allow creating as many python tracks as needed.
   */
  void createPythonTrack();

  // set active view changed
  void setActiveView(pqView*);
  // set the current proxy selection
  void setCurrentSelection(pqPipelineSource*);
  void setCurrentProxy(vtkSMProxy* pxy);

  // sets the current time
  void setCurrentTime(double);
  void setKeyFrameTime(pqAnimationTrack*, pqAnimationKeyFrame*, int, double);

  // update the time labels
  void onTimeLabelChanged();

  // called when the user selects a proxy in the combo box
  void selectedDataProxyChanged(vtkSMProxy*);

  // called when the user accepts the change data proxy dialog
  // when configuring the follow-data animation
  void changeDataProxyDialogAccepted();

  void generalSettingsChanged();

  void onStrideChanged();

private:
  Q_DISABLE_COPY(pqAnimationViewWidget)

  class pqInternal;
  pqInternal* Internal = nullptr;
};

#endif
