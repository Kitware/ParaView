/*=========================================================================

   Program: ParaView
   Module:    pqAnimationScene.h

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

========================================================================*/
#ifndef pqAnimationScene_h
#define pqAnimationScene_h

#include "pqProxy.h"
#include <QPair>
#include <QSet>

class pqAnimationCue;
class QSize;
class vtkObject;

/**
* pqAnimationScene is a representation for a vtkSMAnimationScene
* proxy. It provides API to access AnimationCues in the scene.
*/
class PQCORE_EXPORT pqAnimationScene : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  pqAnimationScene(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = NULL);
  ~pqAnimationScene() override;

  /**
  * Returns the cue that animates the given
  * \c index of the given \c property on the given \c proxy, in this scene,
  * if any.
  */
  pqAnimationCue* getCue(vtkSMProxy* proxy, const char* propertyname, int index) const;

  /**
  * Creates and initializes a new cue that can animate
  * the \c index of the \c property on the given \c proxy
  * in this scene. This method does not check is such a cue already
  * exists, use getCue() before calling this to avoid duplicates.
  */
  pqAnimationCue* createCue(vtkSMProxy* proxy, const char* propertyname, int index);
  pqAnimationCue* createCue(
    vtkSMProxy* proxy, const char* propertyname, int index, const QString& cuetype);
  pqAnimationCue* createCue(const QString& cuetype);

  /**
  * Removes all cues which animate the indicated proxy, if any.
  */
  void removeCues(vtkSMProxy* proxy);

  /**
  * Removes cue
  */
  void removeCue(pqAnimationCue* cue);

  /**
  * returns true is the cue is present in this scene.
  */
  bool contains(pqAnimationCue*) const;

  /**
  * Get the clock time range set on the animation scene proxy.
  */
  QPair<double, double> getClockTimeRange() const;

  /**
  * Get all the cues in this scene
  */
  QSet<pqAnimationCue*> getCues() const;

  /**
  * Returns the current animation time.
  */
  double getAnimationTime() const;

  /**
  * Returns the timesteps from the timekeeper for the animation scene.
  */
  QList<double> getTimeSteps() const;

Q_SIGNALS:
  /**
  * Fired before a new cue is added to the scene.
  */
  void preAddedCue(pqAnimationCue*);

  /**
  * Fired after a new cue has been added to the scene.
  */
  void addedCue(pqAnimationCue*);

  /**
  * Fired before a cue is removed from the scene.
  */
  void preRemovedCue(pqAnimationCue*);

  /**
  * Fired after a cue has been removed from the scene.
  */
  void removedCue(pqAnimationCue*);

  /**
  * Fired after cues have been added/removed.
  */
  void cuesChanged();

  /**
  * Emitted when the play mode changes.
  */
  void playModeChanged();

  /**
  * Emitted when the looping state changes on the
  * underlying proxy.
  */
  void loopChanged();

  /**
  * Emitted when the clock time ranges change.
  */
  void clockTimeRangesChanged();

  /**
  * Emitted when animation starts playing.
  */
  void beginPlay();

  /**
  * Emitted when animation ends playing.
  */
  void endPlay();

  /**
  * Emitted when playing animation.
  * Argument is the percent of play completed.
  */
  void tick(int percentCompleted);

  /**
  * Emitted when playing (or when animation time for the scene is set).
  * \c time is the animation clock time.
  */
  void animationTime(double time);

  /**
  * Emitted when the number of frames changes
  */
  void frameCountChanged();

  /**
  * Emitted when the number of timesteps changes
  */
  void timeStepsChanged();

  /**
  * Emitted when a source is changing the label that should be use for time
  */
  void timeLabelChanged();

public Q_SLOTS:
  /**
  * Play animation.
  */
  void play();

  /**
  * Pause animation.
  */
  void pause();

  /**
  * Set the animation time.
  */
  void setAnimationTime(double time);

private Q_SLOTS:
  /**
  * Called when the "Cues" property on the AnimationScene proxy
  * is changed. Updates the internal datastructure to reflect the current
  * state of the scene.
  */
  void onCuesChanged();

  /**
  * Called on animation tick.
  */
  void onTick(vtkObject* caller, unsigned long, void*, void* info);

  /**
  * called when "AnimationTime" property changes. We fire the animationTime()
  * signal.
  */
  void onAnimationTimePropertyChanged();

protected:
  /**
  * Initializes the newly create animation cue with default keyframes.
  */
  void initializeCue(vtkSMProxy* proxy, const char* propertyname, int index, pqAnimationCue* cue);

private:
  Q_DISABLE_COPY(pqAnimationScene)

  class pqInternals;
  pqInternals* Internals;

  pqAnimationCue* createCueInternal(
    const QString& cuetype, vtkSMProxy* proxy, const char* propertyname, int index);
};

#endif
