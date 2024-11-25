// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqVCRController_h
#define pqVCRController_h

#include "pqComponentsModule.h"
#include <QObject>
#include <QPointer>

class pqPipelineSource;
class pqAnimationScene;
class vtkObject;

/**
 * pqVCRController is the QObject that encapsulates the VCR control functionality.
 * It provides a slot to set the scene that this object is using for animation.
 * Typically, one would connect this slot to a pqAnimationManager like object
 * which keeps track of the active animation scene.
 */
class PQCOMPONENTS_EXPORT pqVCRController : public QObject
{
  Q_OBJECT
public:
  pqVCRController(QObject* parent = nullptr);
  ~pqVCRController() override;

  /**
   * Get the animation scene currently used in the VCR control.
   */
  pqAnimationScene* getAnimationScene() const;

Q_SIGNALS:
  /**
   * Emitted for each tick.
   */
  void timestepChanged();

  /**
   * Emitted as `playing(true, reversed)` when playback begins
   * and `playing(false, reversed)` when playback ends.
   * The value of `reversed` argument is true if playback was
   * requested in backward direction
   */
  void playing(bool, bool);

  /**
   * Fired when the enable state of the VCR control changes and
   * each time setAnimationScene() is called.
   * If called when a valid scene, enabled(true) is fired,
   * else enabled(false).
   */
  void enabled(bool);

  /**
   * Emitted to update the check state of the loop.
   */
  void loop(bool);

  /**
   * Emitted when the time ranges is updated.
   */
  void timeRanges(double, double);

public Q_SLOTS:
  /**
   * Set the animation scene. If null, the VCR control is disabled
   * (emits enabled(false)).
   */
  virtual void setAnimationScene(pqAnimationScene*);

  /**
   * Called when timeranges change. (emits timeRanges(first, last))
   */
  virtual void onTimeRangesChanged();

  ///@{
  /**
   * Connect these signals to appropriate VCR buttons.
   */
  virtual void onFirstFrame();
  virtual void onPreviousFrame();
  virtual void onNextFrame();
  virtual void onLastFrame();
  virtual void onPlay();
  virtual void onReverse();
  virtual void onPause();
  virtual void onLoop(bool checked);
  ///@}

protected Q_SLOTS:
  void onTick();
  void onLoopPropertyChanged();
  void onBeginPlay(vtkObject* caller, unsigned long, void*, void* reversed);
  void onEndPlay(vtkObject* caller, unsigned long, void*, void* reversed);

private:
  Q_DISABLE_COPY(pqVCRController)

  void onPlayInternal();
  void onReverseInternal();

  QMetaObject::Connection LamdaPlayConnection;
  QMetaObject::Connection LamdaReverseConnection;
  QPointer<pqAnimationScene> Scene;
};

#endif
