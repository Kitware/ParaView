/*=========================================================================

   Program: ParaView
   Module:    pqAnimationManager.h

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

========================================================================*/
#ifndef __pqAnimationManager_h
#define __pqAnimationManager_h

#include "pqComponentsExport.h"
#include <QObject>

class pqAnimationCue;
class pqAnimationScene;
class pqProxy;
class pqRenderViewModule;
class pqServer;
class vtkSMProxy;

// pqAnimationManager manages the Animation sub-system.
// It encapsulates the initialization of animation scene per server
// connection i.e. this class basically keeps track of the active 
// animation scene.
class PQCOMPONENTS_EXPORT pqAnimationManager : public QObject
{
  Q_OBJECT
public:
  pqAnimationManager(QObject* parent=0);
  virtual ~pqAnimationManager();

  // Returns the scene for the active server connection, if any.
  pqAnimationScene* getActiveScene() const;

  // Returns the scene on the server connection, if any.
  pqAnimationScene* getScene(pqServer* server) const;

  // Creates a new scene for the active server connection,
  // if possible, and returns it.
  pqAnimationScene* createActiveScene();

  // In the given \c scene, returns the cue that animates the given 
  // \c index of the given \c property on the \c proxy.
  // This method simply calls getCue() on the pqAnimationScene instance.
  pqAnimationCue* getCue(pqAnimationScene* scene, 
    vtkSMProxy* proxy, const char* propertyname, int index) const;

  // Creates a cue and adds it to the scene. Intializes the cue
  // so that it animates the \c index'th element of 
  // given \c property on the \c proxy.
  pqAnimationCue* createCue(pqAnimationScene* scene, 
    vtkSMProxy* proxy, const char* propertyname, int index);

  // Saves the animation from the active scene.
  // Currently, only 1 view is saved in the generated movie.
  // Returns true if the save was successful.
  bool saveAnimation(const QString& filename, pqRenderViewModule* view);

signals:
  // emitted when the active scene changes (\c scene may be NULL).
  void activeSceneChanged(pqAnimationScene* scene);
 
public slots:
  // Called when the active server changes.
  void onActiveServerChanged(pqServer*);

protected slots:
  void onProxyAdded(pqProxy*);
  void onProxyRemoved(pqProxy*);

  void durationChanged();
  void frameRateChanged();
  void numberOfFramesChanged();

private:
  pqAnimationManager(const pqAnimationManager&); // Not implemented.
  void operator=(const pqAnimationManager&); // Not implemented.

  class pqInternals;
  pqInternals* Internals;
};


#endif

