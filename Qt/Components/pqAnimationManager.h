// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#ifndef pqAnimationManager_h
#define pqAnimationManager_h

#include <QObject>

#include "pqComponentsModule.h"     // for exports
#include "vtkParaViewDeprecation.h" // for PARAVIEW_DEPRECATED_IN_5_11_0

class QSize;

class pqAnimationCue;
class pqAnimationScene;
class pqProxy;
class pqServer;
class pqView;
class vtkSMProxy;
class vtkObject;

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
  pqAnimationManager(QObject* parent = nullptr);
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
   * Emitted when the active animation scene begins playing.
   */
  void beginPlay(vtkObject* caller, unsigned long, void*, void* reversed);

  /**
   * Emitted when the active animation scene finishes playing.
   */
  void endPlay(vtkObject* caller, unsigned long, void*, void* reversed);

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
   * @deprecated in ParaView 5.11.0
   */
  PARAVIEW_DEPRECATED_IN_5_11_0("Use the overload with VTK callback signature.")
  void onBeginPlay();

  /**
   * @deprecated in ParaView 5.11.0
   */
  PARAVIEW_DEPRECATED_IN_5_11_0("Use the overload with VTK callback signature.")
  void onEndPlay();

  /**
   * Called to demarcate the start and end of an animation
   */
  void onBeginPlay(vtkObject* caller, unsigned long, void*, void* reversed);
  void onEndPlay(vtkObject* caller, unsigned long, void*, void* reversed);

private:
  Q_DISABLE_COPY(pqAnimationManager)

  class pqInternals;
  pqInternals* Internals;
};

#endif
