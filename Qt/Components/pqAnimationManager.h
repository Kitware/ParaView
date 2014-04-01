/*=========================================================================

   Program: ParaView
   Module:    pqAnimationManager.h

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
#ifndef __pqAnimationManager_h
#define __pqAnimationManager_h

#include "pqComponentsModule.h"
#include <QObject>

class QSize;

class pqAnimationCue;
class pqAnimationScene;
class pqProxy;
class pqServer;
class pqView;
class vtkSMProxy;

//// pqAnimationManager manages the Animation sub-system.
//// It encapsulates the initialization of animation scene per server
//// connection i.e. this class basically keeps track of the active 
//// animation scene.
class PQCOMPONENTS_EXPORT pqAnimationManager : public QObject
{
  Q_OBJECT
public:
  pqAnimationManager(QObject* parent=0);
  virtual ~pqAnimationManager();

  /// Returns the scene for the active server connection, if any.
  pqAnimationScene* getActiveScene() const;

  /// Returns the scene on the server connection, if any.
  pqAnimationScene* getScene(pqServer* server) const;

  /// In the given \c scene, returns the cue that animates the given 
  /// \c index of the given \c property on the \c proxy.
  /// This method simply calls getCue() on the pqAnimationScene instance.
  pqAnimationCue* getCue(pqAnimationScene* scene, 
    vtkSMProxy* proxy, const char* propertyname, int index) const;

  /// Saves the animation from the active scene. The active scene
  /// is determined using the active server.
  /// Returns true if the save was successful.
  bool saveAnimation();

  /// Saves the animation geometry from the active scene
  /// as visible in the given view.
  bool saveGeometry(const QString& filename, pqView* view);
  
  /// Save the settings of "save animation" with QSettings.
  void saveSettings();

  /// Apply the settings from QSettings to "save animation".
  void restoreSettings();

signals:
  /// emitted when the active scene changes (\c scene may be NULL).
  void activeSceneChanged(pqAnimationScene* scene);

  /// emitted when the active server changes and updated active scene.
  void activeServerChanged(pqServer* scene);

  /// emitted with the current save progress.
  void saveProgress(const QString&, int);

  /// emitted when the manager begins changes that should not get 
  /// recorded on the undo stack. 
  void beginNonUndoableChanges();

  /// emitted when the manager is done with changes that
  /// should not get recorded on the undo stack.
  void endNonUndoableChanges();

  /// emitted to request the application to disconnect from the server
  /// connection. This is done when the user requested to save animation after
  /// disconnecting from the server.
  void disconnectServer();

  /// emitted to indicate an animation is being written out to a file.
  void writeAnimation(const QString& filename, int magnification, double frameRate);

public slots:
  // Called when the active server changes.
  void onActiveServerChanged(pqServer*);

protected slots:
  void onProxyAdded(pqProxy*);
  void onProxyRemoved(pqProxy*);

  void updateGUI();

  /// Update the ViewModules property in the active scene.
  void updateViewModules();

  /// Called on every tick while saving animation.
  void onTick(int);

  /// Manages locking the aspect ratio.
  void onWidthEdited();
  void onHeightEdited();
  void onLockAspectRatio(bool lock);
private:
  pqAnimationManager(const pqAnimationManager&); // Not implemented.
  void operator=(const pqAnimationManager&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
  
  // the most recently used file extension
  QString AnimationExtension;
};


#endif

