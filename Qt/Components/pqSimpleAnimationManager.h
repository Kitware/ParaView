/*=========================================================================

   Program: ParaView
   Module:    pqSimpleAnimationManager.h

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
#ifndef __pqSimpleAnimationManager_h
#define __pqSimpleAnimationManager_h

#include <QObject>
#include "pqComponentsExport.h"

class pqPipelineSource;
class pqRenderModule;
class pqServer;
class pqSimpleAnimationManagerProxies;

// This class is a temporary animation manager. It simply saves
// animations in which the timestep of a reader is animated.
class PQCOMPONENTS_EXPORT pqSimpleAnimationManager : public QObject
{
  Q_OBJECT;
public:
  pqSimpleAnimationManager(QObject* parent=0);
  virtual ~pqSimpleAnimationManager();

  // Returns if the manager can create an animation for the 
  // given source.
  static bool canAnimate(pqPipelineSource* source);

  // Creates the animation. This will pop-up a dialog asking for
  // relevant information about the animation.
  bool createTimestepAnimation(pqPipelineSource* source, const QString& filename);

public slots:
  // Call to interrupt saving of the animation.
  void abortSavingAnimation();

  // Set the server.
  void setServer(pqServer* server)
    {this->Server = server; }

  // Set the render module. The view in this render module will be
  // saved in the animation.
  void setRenderModule(pqRenderModule* ren)
    { this->RenderModule = ren; }
private slots:
  void onAnimationTick();

private:
  pqPipelineSource* Source;
  pqServer* Server;
  pqRenderModule* RenderModule;
  pqSimpleAnimationManagerProxies* Proxies;
};
#endif

