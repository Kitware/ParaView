/*=========================================================================

   Program: ParaView
   Module:    pqServerDisconnectReaction.cxx

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
#include "pqServerDisconnectReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QMessageBox>

//-----------------------------------------------------------------------------
pqServerDisconnectReaction::pqServerDisconnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->UpdateTimer.setInterval(500);
  this->UpdateTimer.setSingleShot(true);

  QObject::connect(&pqActiveObjects::instance(), SIGNAL(serverChanged(pqServer*)),
    &this->UpdateTimer, SLOT(start()));
  QObject::connect(&this->UpdateTimer, SIGNAL(timeout()), this, SLOT(updateState()));
  this->updateState();

  // needed to disable server disconnection while an animation is playing
  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(beginPlay()), this,
    SLOT(updateState()));
  QObject::connect(pqPVApplicationCore::instance()->animationManager(), SIGNAL(endPlay()), this,
    SLOT(updateState()));
}

//-----------------------------------------------------------------------------
bool pqServerDisconnectReaction::disconnectFromServerWithWarning()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  pqServer* server = pqActiveObjects::instance().activeServer();

  if (server && smmodel->findItems<pqPipelineSource*>(server).size() > 0)
  {
    int ret = QMessageBox::warning(pqCoreUtilities::mainWidget(),
      tr("Disconnect from current server?"), tr("The current connection will be closed and \n"
                                                "the state will be discarded.\n\n"
                                                "Are you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
    {
      return false;
    }
  }

  pqServerDisconnectReaction::disconnectFromServer();
  return true;
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::disconnectFromServer()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer* server = pqActiveObjects::instance().activeServer();
  if (server)
  {
    core->getObjectBuilder()->removeServer(server);
  }
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::updateState()
{
  if (pqPVApplicationCore::instance()->animationManager()->animationPlaying())
  {
    this->parentAction()->setEnabled(false);
  }
  else
  {
    this->parentAction()->setEnabled(pqActiveObjects::instance().activeServer() != nullptr);
  }
}

//-----------------------------------------------------------------------------
void pqServerDisconnectReaction::onTriggered()
{
  this->parentAction()->setEnabled(false);
  if (!pqServerDisconnectReaction::disconnectFromServerWithWarning())
  {
    // re-enable the action since user cancelled the disconnect, otherwise the
    // action will be re-enabled when a new session, if any is created.
    this->parentAction()->setEnabled(true);
  }
}
