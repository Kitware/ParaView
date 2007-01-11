/*=========================================================================

   Program: ParaView
   Module:    pqAnimationPanel.h

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#ifndef __pqAnimationPanel_h
#define __pqAnimationPanel_h

#include "pqComponentsExport.h"
#include <QWidget>

class pqAnimationCue;
class pqAnimationManager;
class pqAnimationScene;
class pqPipelineSource;
class pqServerManagerModelItem;

// This is the Animation panel widget. It controls the behaviour
// of the Animation panel which includes adding of key frames,
// changing of keyframes etc etc.
class PQCOMPONENTS_EXPORT pqAnimationPanel : public QWidget
{
  Q_OBJECT
public:
  pqAnimationPanel(QWidget* parent);
  virtual ~pqAnimationPanel();

  typedef QWidget Superclass;
  class pqInternals;

  // Set the animation manager to use.
  void setManager(pqAnimationManager* mgr);

  // Insert a key frame at the given index. The time for the keyframe
  // is computed using the mind point of the neighbouring two 
  // key frames (if any).
  void insertKeyFrame(int index);

  // Delete the keyframe at the given index.
  void deleteKeyFrame(int index);

public slots:
  // Show the keyframe GUI for the keyframe at the given index.
  void showKeyFrame(int index);

  void showKeyFrameCallback(int index);

signals:
  // fired before the panel performs an undoable operation.
  void beginUndoSet(const QString&);

  // fired after the panel has performed an undoable operation.
  void endUndoSet();

protected slots:
  // Called when the application selection changes.
  // We upadte the Panel to show the tracks for the selected source.
  void onCurrentChanged(pqServerManagerModelItem*);

  // Called when the user changes the combo box selection.
  void onCurrentSourceChanged(int index);

  // Called when the user changes the property combox box.
  void onCurrentPropertyChanged(int index);

  // updates the enable state of part of the GUI that depends on the 
  // selected source.
  void updateEnableState();

  // Adds keyframe to active cue. Called when user hits the "Add Key Frame"
  // button.
  void addKeyFrameCallback();

  // Deletes the active keyframe. Called when user hits the "Delete Key Frame"
  // button.
  void deleteKeyFrameCallback();

  // Called when the cue tells us that the keyframes have somehow changed.
  void onKeyFramesModified();

  // Called before a source is removed. We clean up the
  // animation cue/keyframes for this source.
  void onSourceRemoved(pqPipelineSource* src);

  // Called when a source's name is changed.
  void onNameChanged(pqServerManagerModelItem*);

  // Called when the active scene changes.
  void onActiveSceneChanged(pqAnimationScene* scene);

  // Called when animation scene end times change.
  void onSceneTimeChanged();

  // The cues in the scene have changed, so we make sure
  // that we are not displaying a removed or added cue, if so
  // we update the GUI.
  void onSceneCuesChanged();
protected:
  // updates the "Property To Animate" list.
  void buildPropertyList();

private:
  pqAnimationPanel(const pqAnimationPanel&); // Not implemented.
  void operator=(const pqAnimationPanel&); // Not implemented.

  void setActiveCue(pqAnimationCue*);
  pqInternals *Internal;
};

#endif

