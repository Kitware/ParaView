// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationScene_h
#define pqAnimationScene_h

#include "pqProxy.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_11_0
#include <QPair>
#include <QSet>

struct pqAnimatedPropertyInfo;
class pqAnimationCue;
class QSize;
class vtkObject;

/**
 * pqAnimationScene is a representation for a vtkSMAnimationScene proxy.
 *
 * It provides API to access AnimationCues in the scene and to the scene times.
 */
class PQCORE_EXPORT pqAnimationScene : public pqProxy
{
  Q_OBJECT
  typedef pqProxy Superclass;

public:
  pqAnimationScene(const QString& group, const QString& name, vtkSMProxy* proxy, pqServer* server,
    QObject* parent = nullptr);
  ~pqAnimationScene() override;

  /**
   * Returns the cue that animates the given
   * \c index of the given \c property on the given \c proxy, in this scene,
   * if any.
   */
  pqAnimationCue* getCue(vtkSMProxy* proxy, const char* propertyname, int index) const;

  ///@{
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
  pqAnimationCue* createCue(const pqAnimatedPropertyInfo& propInfo);
  ///@}

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
   * @deprecated in ParaView 5.11.0
   */
  PARAVIEW_DEPRECATED_IN_5_11_0("Use the overload with VTK callback signature.")
  void beginPlay();

  /**
   * @deprecated in ParaView 5.11.0
   */
  PARAVIEW_DEPRECATED_IN_5_11_0("Use the overload with VTK callback signature.")
  void endPlay();

  /**
   * Emitted when animation starts playing.
   * `reversed` is a pointer to boolean that
   * indicates if the playback direction is backward
   */
  void beginPlay(vtkObject* caller, unsigned long, void*, void* reversed);

  /**
   * Emitted when animation ends playing.
   * `reversed` is a pointer to boolean that
   * indicates if the playback direction is backward
   */
  void endPlay(vtkObject* caller, unsigned long, void*, void* reversed);

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
