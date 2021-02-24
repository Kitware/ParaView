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
#ifndef pqAnimationManager_h
#define pqAnimationManager_h

#include <QObject>

#include "pqComponentsModule.h" // for exports

class QSize;

class pqAnimationCue;
class pqAnimationScene;
class pqProxy;
class pqServer;
class pqView;
class vtkSMProxy;

/**
* pqAnimationManager manages the Animation sub-system.
* It encapsulates the initialization of animation scene per server
* connection i.e. this class basically keeps track of the active
* animation scene.
*/
class PQCOMPONENTS_EXPORT pqAnimationManager : public QObject
{
  Q_OBJECT
public:
  pqAnimationManager(QObject* parent = 0);
  ~pqAnimationManager() override;

  /**
  * Returns the scene for the active server connection, if any.
  */
  pqAnimationScene* getActiveScene() const;

  /**
  * Returns the scene on the server connection, if any.
  */
  pqAnimationScene* getScene(pqServer* server) const;

  /**
  * In the given \c scene, returns the cue that animates the given
  * \c index of the given \c property on the \c proxy.
  * This method simply calls getCue() on the pqAnimationScene instance.
  */
  pqAnimationCue* getCue(
    pqAnimationScene* scene, vtkSMProxy* proxy, const char* propertyname, int index) const;

  /**
  * Saves the animation geometry from the active scene
  * as visible in the given view.
  */
  bool saveGeometry(const QString& filename, pqView* view);

  /**
  * Query whether or not an animation is currently playing
  */
  bool animationPlaying() const;

Q_SIGNALS:
  /**
  * emitted when the active scene changes (\c scene may be nullptr).
  */
  void activeSceneChanged(pqAnimationScene* scene);

  /**
  * emitted when the active server changes and updated active scene.
  */
  void activeServerChanged(pqServer* scene);

  /**
  * emitted with the current save progress.
  */
  void saveProgress(const QString&, int);

  /**
  * emitted when the manager begins changes that should not get
  * recorded on the undo stack.
  */
  void beginNonUndoableChanges();

  /**
  * emitted when the manager is done with changes that
  * should not get recorded on the undo stack.
  */
  void endNonUndoableChanges();

  /**
  * emitted to indicate an animation is being written out to a file.
  */
  void writeAnimation(const QString& filename, int magnification, double frameRate);

  /**
  * Emitted when the active animation scene begins playing.
  */
  void beginPlay();

  /**
  * Emitted when the active animation scene finishes playing.
  */
  void endPlay();

public Q_SLOTS:
  // Called when the active server changes.
  void onActiveServerChanged(pqServer*);

protected Q_SLOTS:
  void onProxyAdded(pqProxy*);
  void onProxyRemoved(pqProxy*);

  /**
  * Called on every tick while saving animation.
  */
  void onTick(int);

  /**
  * Called to demarcate the start and end of an animation
  */
  void onBeginPlay();
  void onEndPlay();

private:
  Q_DISABLE_COPY(pqAnimationManager)

  class pqInternals;
  pqInternals* Internals;
};

#endif
