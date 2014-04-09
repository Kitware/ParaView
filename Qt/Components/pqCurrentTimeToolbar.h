/*=========================================================================

   Program: ParaView
   Module:    pqCurrentTimeToolbar.h

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
#ifndef __pqCurrentTimeToolbar_h 
#define __pqCurrentTimeToolbar_h

#include <QToolBar>
#include <QPointer>
#include "pqComponentsModule.h"

class QLineEdit;
class QSpinBox;
class QLabel;
class pqAnimationScene;

/// pqCurrentTimeToolbar is a toolbar that shows the current animation time.
/// It stays linked with the current animation scene's animation time. To use
/// this, simply create this toolbar and place it in the GUI as needed. Either
/// set the current animation scene directly or connect it to the
/// pqAnimationManager's scene-changed signal.
class PQCOMPONENTS_EXPORT pqCurrentTimeToolbar : public QToolBar
{
  Q_OBJECT
  typedef QToolBar Superclass;
  Q_PROPERTY(bool showFrameCount READ showFrameCount WRITE setShowFrameCount);

public:
  pqCurrentTimeToolbar(const QString &title, QWidget *parent = 0);
  pqCurrentTimeToolbar(QWidget *parent = 0);
  virtual ~pqCurrentTimeToolbar();

  /// Returns the current animation scene to which this toolbar is linked.
  pqAnimationScene* animationScene() const;

  /// Returns whether frame count is shown in the toolbar, if applicable.
  bool showFrameCount() const
    { return this->ShowFrameCount; }

public slots:
  /// Set the animation scene.
  void setAnimationScene(pqAnimationScene*);

  /// Set whether frame count is shown in the toolbar, if applicable.
  void setShowFrameCount(bool val);

protected slots:
  /// Called when animation scene reports that it's time has changed.
  void sceneTimeChanged(double);

  /// When user edits the line-edit.
  void currentTimeEdited();

  /// When user edits the spin-box
  void currentTimeIndexChanged();
  void onPlayModeChanged();

  /// Update range for the spin box.
  void onTimeStepsChanged();

  /// Update the label text
  void onTimeLabelChanged();
  
signals:
  /// emitted to request the scene to change it's animation time.
  void changeSceneTime(double);

protected:
  QLineEdit* TimeLineEdit;
  QSpinBox* TimeSpinBox;
  QLabel* TimeLabel;
  QLabel* CountLabel;
  QAction* CountLabelAction;
  bool ShowFrameCount;

private:
  pqCurrentTimeToolbar(const pqCurrentTimeToolbar&); // Not implemented.
  void operator=(const pqCurrentTimeToolbar&); // Not implemented.

  void constructor();
  QPointer<pqAnimationScene> Scene;
};

#endif


