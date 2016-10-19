/*=========================================================================

   Program: ParaView
   Module:    pqSourcesMenuReaction.cxx

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
#include "pqSourcesMenuReaction.h"

#include "pqActiveObjects.h"
#include "pqObjectBuilder.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqUndoStack.h"

#include "pqCollaborationManager.h"
#include "vtkPVServerInformation.h"
#include "vtkSMCollaborationManager.h"

//-----------------------------------------------------------------------------
pqSourcesMenuReaction::pqSourcesMenuReaction(pqProxyGroupMenuManager* menuManager)
  : Superclass(menuManager)
{
  QObject::connect(menuManager, SIGNAL(triggered(const QString&, const QString&)), this,
    SLOT(onTriggered(const QString&, const QString&)));

  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(menuManager, SIGNAL(menuPopulated()), this, SLOT(updateEnableState()));
  QObject::connect(pqApplicationCore::instance(), SIGNAL(updateMasterEnableState(bool)), this,
    SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSourcesMenuReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  this->updateEnableState(
    activeObjects->activeServer() != NULL && activeObjects->activeServer()->isMaster());
}
//-----------------------------------------------------------------------------
void pqSourcesMenuReaction::updateEnableState(bool enabled)
{
  pqProxyGroupMenuManager* mgr = static_cast<pqProxyGroupMenuManager*>(this->parent());
  mgr->setEnabled(enabled);
  foreach (QAction* action, mgr->actions())
  {
    action->setEnabled(enabled);
  }
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSourcesMenuReaction::createSource(const QString& group, const QString& name)
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();

  BEGIN_UNDO_SET(QString("Create '%1'").arg(name));
  pqPipelineSource* source = builder->createSource(group, name, activeObjects->activeServer());
  END_UNDO_SET();
  return source;
}
