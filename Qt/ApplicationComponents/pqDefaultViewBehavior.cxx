/*=========================================================================

   Program: ParaView
   Module:    pqDefaultViewBehavior.cxx

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
#include "pqDefaultViewBehavior.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqObjectBuilder.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "vtkPVDisplayInformation.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <QMessageBox>

//-----------------------------------------------------------------------------
pqDefaultViewBehavior::pqDefaultViewBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(), 
    SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onServerCreation(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::onServerCreation(pqServer* server)
{
  pqApplicationCore* core = pqApplicationCore::instance();

  // Check if it is possible to access display on the server. If not, we show a
  // message.
  vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
  server->session()->GatherInformation(
    vtkSMSession::RENDER_SERVER, di, 0);
  if (!di->GetCanOpenDisplay())
    {
    QMessageBox::warning(pqCoreUtilities::mainWidget(),
      tr("Server DISPLAY not accessible"),
      tr("Display is not accessible on the server side.\n"
        "Remote rendering will be disabled."),
      QMessageBox::Ok);
    }
  di->Delete();

  // See if some view are already present. This allow us to create one by
  // default if needed and use the existing one if a client connect to a
  // collaborative visualization server.
  if(core->getServerManagerModel()->getNumberOfItems<pqView*>() == 0)
    {
    pqObjectBuilder* builder =
      pqApplicationCore::instance()->getObjectBuilder();

    // before creating a view, ensure that a layout (vtkSMViewLayoutProxy) is
    // present.
    if (server->proxyManager()->GetNumberOfProxies("layouts") == 0)
      {
      vtkSMProxy* vlayout = builder->createProxy(
        "misc", "ViewLayout", server, "layouts");
      Q_ASSERT(vlayout != NULL);
      (void)vlayout;
      }

    pqSettings* settings = core->settings();
    QString curView = settings->value("/defaultViewType",
                                      pqRenderView::renderViewType()).toString();
    if (curView != "None" && !curView.isEmpty())
      {
      // When a server is created, we create a new render view for it.
      builder->createView(curView, server);
      }
    }

  // Show warning dialogs before server times out.
  QObject::connect(server, SIGNAL(fiveMinuteTimeoutWarning()), 
    this, SLOT(fiveMinuteTimeoutWarning()));
  QObject::connect(server, SIGNAL(finalTimeoutWarning()), 
    this, SLOT(finalTimeoutWarning()));
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::fiveMinuteTimeoutWarning()
{
  QMessageBox::warning(pqCoreUtilities::mainWidget(),
    tr("Server Timeout Warning"),
    tr("The server connection will timeout under 5 minutes.\n"
    "Please save your work."),
    QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::finalTimeoutWarning()
{
  QMessageBox::critical(pqCoreUtilities::mainWidget(),
    tr("Server Timeout Warning"),
    tr("The server connection will timeout shortly.\n"
    "Please save your work."),
    QMessageBox::Ok);
}
