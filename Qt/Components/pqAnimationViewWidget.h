/*=========================================================================

   Program: ParaView
   Module:    pqAnimationViewWidget.h

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/
#ifndef pqAnimationViewWidget_h
#define pqAnimationViewWidget_h

#include "pqComponentsModule.h"
#include <QWidget>

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
class PQCOMPONENTS_EXPORT pqAnimationViewWidget : public QWidget
{
  Q_OBJECT
  typedef QWidget Superclass;

public:
  pqAnimationViewWidget(QWidget* parent = nullptr);
  ~pqAnimationViewWidget() override;

public Q_SLOTS:

  /**
  * set the scene to view
  */
  void setScene(pqAnimationScene* scene);

protected Q_SLOTS:

  /**
  * The cues in the scene have changed, so we make sure
  * that we are not displaying a removed or added cue, if so
  * we update the GUI.
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

private:
  Q_DISABLE_COPY(pqAnimationViewWidget)

  class pqInternal;
  pqInternal* Internal = nullptr;
};

#endif
