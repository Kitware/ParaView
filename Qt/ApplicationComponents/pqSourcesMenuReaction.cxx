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
#include "pqCollaborationManager.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqProxyGroupMenuManager.h"
#include "pqServer.h"
#include "pqUndoStack.h"
#include "vtkPVServerInformation.h"
#include "vtkPVXMLElement.h"
#include "vtkSMCollaborationManager.h"
#include "vtkSMProxy.h"
#include "vtkSMSessionProxyManager.h"

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
    activeObjects->activeServer() != nullptr && activeObjects->activeServer()->isMaster());
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
bool pqSourcesMenuReaction::warnOnCreate(
  const QString& xmlgroup, const QString& xmlname, pqServer* server)
{
  server = server ? server : pqActiveObjects::instance().activeServer();
  if (server)
  {
    vtkSMSessionProxyManager* pxm = server->proxyManager();
    vtkSMProxy* prototype =
      pxm->GetPrototypeProxy(xmlgroup.toLocal8Bit().data(), xmlname.toLocal8Bit().data());
    if (!prototype)
    {
      // skip the prompt.
      return true;
    }

    if (vtkPVXMLElement* hints = prototype->GetHints()
        ? prototype->GetHints()->FindNestedElementByName("WarnOnCreate")
        : nullptr)
    {
      const char* title = hints->GetAttributeOrDefault("title", "Are you sure?");
      QString txt = hints->GetCharacterData();
      if (txt.isEmpty())
      {
        txt = QString("Creating '%1'. Do you want to continue?").arg(prototype->GetXMLLabel());
      }
      return pqCoreUtilities::promptUser(QString("WarnOnCreate/%1/%2").arg(xmlgroup).arg(xmlname),
        QMessageBox::Information, QString::fromStdString(pqProxy::rstToHtml(title)),
        pqProxy::rstToHtml(txt), QMessageBox::Yes | QMessageBox::No | QMessageBox::Save);
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqSourcesMenuReaction::createSource(const QString& group, const QString& name)
{
  if (pqSourcesMenuReaction::warnOnCreate(group, name))
  {
    pqApplicationCore* core = pqApplicationCore::instance();
    pqObjectBuilder* builder = core->getObjectBuilder();
    BEGIN_UNDO_SET(QString("Create '%1'").arg(name));
    pqPipelineSource* source =
      builder->createSource(group, name, pqActiveObjects::instance().activeServer());
    END_UNDO_SET();
    return source;
  }
  return nullptr;
}
