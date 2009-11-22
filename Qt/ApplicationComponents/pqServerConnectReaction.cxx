/*=========================================================================

   Program: ParaView
   Module:    pqServerConnectReaction.cxx

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
#include "pqServerConnectReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqPipelineSource.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerStartupBrowser.h"
#include "pqServerStartups.h"
#include "pqSimpleServerStartup.h"

#include <QMessageBox>

//-----------------------------------------------------------------------------
pqServerConnectReaction::pqServerConnectReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::connectToServerWithWarning()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  pqServer* server = pqActiveObjects::instance().activeServer();

  if (smmodel->findItems<pqPipelineSource*>(server).size() > 0)
    {
    int ret = QMessageBox::warning(
      pqCoreUtilities::mainWidget(),
      tr("Disconnect from current server?"),
      tr("Before connecting to a new server, \n"
        "the current connection will be closed and \n"
        "the state will be discarded.\n\n"
        "Are you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
      {
      return;
      }
    }

  pqServerConnectReaction::connectToServer();
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::connectToServer()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerStartupBrowser server_browser (core->serverStartups(),
    pqCoreUtilities::mainWidget());
  QStringList ignoreList;
  ignoreList << "builtin";
  server_browser.setIgnoreList(ignoreList);
  server_browser.exec();
}

//-----------------------------------------------------------------------------
void pqServerConnectReaction::connectToServer(
  const char* serverresource_name)
{
  if (serverresource_name)
    {
    pqServerStartup* startUp = 
      pqApplicationCore::instance()->serverStartups().getStartup(serverresource_name);
    if (startUp)
      {
      pqSimpleServerStartup starter;
      starter.startServerBlocking(*startUp);
      }
    }
}

