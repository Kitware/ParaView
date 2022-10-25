/*=========================================================================

   Program: ParaView
   Module:    pqVCRController.h

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

#ifndef pqVCRController_h
#define pqVCRController_h

#include "pqComponentsModule.h"
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_11_0
#include <QObject>
#include <QPointer>

class pqPipelineSource;
class pqAnimationScene;
class vtkObject;

// pqVCRController is the QObject that encapsulates the
// VCR control functionality.
// It provides a slot to set the scene that this object
// is using for animation. Typically, one would connect this
// slot to a pqAnimationManager like object which keeps track
// of the active animation scene.
class PQCOMPONENTS_EXPORT pqVCRController : public QObject
{
  Q_OBJECT
public:
  pqVCRController(QObject* parent = nullptr);
  ~pqVCRController() override;

Q_SIGNALS:
  void timestepChanged();

  // deprecated in 5.11.0
  PARAVIEW_DEPRECATED_IN_5_11_0("Use overload with additional reversed argument.")
  void playing(bool);

  // emitted as `playing(true, reversed)` when playback
  // begins and `playing(false, reversed)` when playback ends.
  // The value of `reversed` argument is true if
  // playback was requested in backward direction.
  void playing(bool, bool);

  // Fired when the enable state of the VCR control changes.
  // fired each time setAnimationScene() is called. If
  // called when a valid scene enabled(true) is fired,
  // else enabled(false).
  void enabled(bool);

  // Emitted to update the check state
  // of the loop.
  void loop(bool);

  void timeRanges(double, double);

public Q_SLOTS:
  // Set the animation scene. If null, the VCR control is disabled
  // (emits enabled(false)).
  void setAnimationScene(pqAnimationScene*);

  // Called when timeranges change.
  void onTimeRangesChanged();

  // Connect these signals to appropriate VCR buttons.
  void onFirstFrame();
  void onPreviousFrame();
  void onNextFrame();
  void onLastFrame();
  void onPlay();
  void onReverse();
  void onPause();
  void onLoop(bool checked);

protected Q_SLOTS:
  void onTick();
  void onLoopPropertyChanged();
  void onBeginPlay(vtkObject* caller, unsigned long, void*, void* reversed);
  void onEndPlay(vtkObject* caller, unsigned long, void*, void* reversed);

private:
  Q_DISABLE_COPY(pqVCRController)

  QPointer<pqAnimationScene> Scene;
};

#endif
