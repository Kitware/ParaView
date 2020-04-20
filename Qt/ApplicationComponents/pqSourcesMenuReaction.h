/*=========================================================================

   Program: ParaView
   Module:    pqSourcesMenuReaction.h

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
#ifndef pqSourcesMenuReaction_h
#define pqSourcesMenuReaction_h

#include "pqApplicationComponentsModule.h"
#include <QObject>

class pqPipelineSource;
class pqProxyGroupMenuManager;
class pqServer;
/**
* @ingroup Reactions
* Reaction to handle creation of sources from the sources menu.
*/
class PQAPPLICATIONCOMPONENTS_EXPORT pqSourcesMenuReaction : public QObject
{
  Q_OBJECT
  typedef QObject Superclass;

public:
  pqSourcesMenuReaction(pqProxyGroupMenuManager* menuManager);

  static pqPipelineSource* createSource(const QString& group, const QString& name);

  /**
   * Method used to prompt user before creating a proxy. If the proxy hints has
   * "WarnOnCreate" hint, then the user will be prompted according to the
   * contents on the hint. Returns `false` if the user rejects the confirmation,
   * `true` otherwise.
   *
   * If `server` is null, then the active server is used.
   */
  static bool warnOnCreate(const QString& group, const QString& name, pqServer* server = nullptr);

public Q_SLOTS:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  virtual void updateEnableState();
  void updateEnableState(bool);

protected Q_SLOTS:
  /**
  * Called when the action is triggered.
  */
  virtual void onTriggered(const QString& group, const QString& name)
  {
    pqSourcesMenuReaction::createSource(group, name);
  }

private:
  Q_DISABLE_COPY(pqSourcesMenuReaction)
};

#endif
