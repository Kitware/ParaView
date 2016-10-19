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
#include "vtkNew.h"
#include "vtkPVDisplayInformation.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVOpenGLInformation.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"
#include <QMessageBox>

namespace
{
QString openGLVersionInfo(vtkSMSession* session, vtkPVSession::ServerFlags server_flag)
{
  vtkNew<vtkPVOpenGLInformation> glinfo;
  session->GatherInformation(server_flag, glinfo.GetPointer(), 0);
  return QString("\n\nOpenGL Vendor: %1\nOpenGL Version: %2\nOpenGL Renderer: %3")
    .arg(glinfo->GetVendor().c_str(), glinfo->GetVersion().c_str(), glinfo->GetRenderer().c_str());
}
}

//-----------------------------------------------------------------------------
pqDefaultViewBehavior::pqDefaultViewBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , WarningMode(pqDefaultViewBehavior::NONE)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerCreation(pqServer*)));
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::showWarnings()
{
  switch (this->WarningMode)
  {
    case SERVER_DISPLAY_INACCESSIBLE:
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Server DISPLAY not accessible!"),
        tr("Display is not accessible on the server side.\n"
           "Remote rendering will be disabled."),
        QMessageBox::Ok);
      break;

    case SERVER_OPENGL_INADEQUATE:
    {
      QString msg = tr("OpenGL drivers on the server side don't support\n"
                       "required OpenGL features for basic rendering.\n"
                       "Remote rendering will be disabled.");
      msg += this->ExtraWarningMessage;
      QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Server OpenGL support inadequate!"),
        msg, QMessageBox::Ok);
    }
    break;

    case CLIENT_OPENGL_INADEQUATE:
    {
      QString msg = tr("Your OpenGL drivers don't support\n"
                       "required OpenGL features for basic rendering.\n"
                       "Application cannot continue. Please exit and use an older version.\n\n"
                       "CONTINUE AT YOUR OWN RISK!");
      msg += this->ExtraWarningMessage;
      QMessageBox::warning(
        pqCoreUtilities::mainWidget(), tr("OpenGL support inadequate!"), msg, QMessageBox::Ok);
    }
    break;
    default:
      // nothing to do.
      break;
  }
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::onServerCreation(pqServer* server)
{
  this->WarningMode = NONE;
  this->ExtraWarningMessage.clear();

  pqApplicationCore* core = pqApplicationCore::instance();

  // Check if it is possible to access display on the server. If not, we show a
  // message.
  vtkNew<vtkPVDisplayInformation> di;
  server->session()->GatherInformation(vtkSMSession::RENDER_SERVER, di.GetPointer(), 0);
  if (!di->GetCanOpenDisplay())
  {
    this->WarningMode = SERVER_DISPLAY_INACCESSIBLE;
  }
  else if (!di->GetSupportsOpenGL())
  {
    this->ExtraWarningMessage = openGLVersionInfo(server->session(), vtkSMSession::RENDER_SERVER);
    if (server->isRemote())
    {
      this->WarningMode = SERVER_OPENGL_INADEQUATE;
    }
    else
    {
      this->WarningMode = CLIENT_OPENGL_INADEQUATE;
    }
  }
  if (server->isRemote())
  {
    // Let's also check that OpenGL version is adequate locally. This will
    // override server OpenGL version check, but that's okay. Client version not
    // supported is a greater issue.
    vtkNew<vtkPVDisplayInformation> localDI;
    server->session()->GatherInformation(vtkSMSession::CLIENT, localDI.GetPointer(), 0);
    if (!localDI->GetSupportsOpenGL())
    {
      this->ExtraWarningMessage = openGLVersionInfo(server->session(), vtkSMSession::CLIENT);
      this->WarningMode = CLIENT_OPENGL_INADEQUATE;
    }
  }

  if (this->WarningMode != NONE)
  {
    pqTimer::singleShot(500, this, SLOT(showWarnings()));
  }

  // See if some view are already present and if we're in a collaborative
  // session, we are the master.
  if (core->getServerManagerModel()->getNumberOfItems<pqView*>() == 0 && server->isMaster())
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();

    // before creating a view, ensure that a layout (vtkSMViewLayoutProxy) is
    // present.
    if (server->proxyManager()->GetNumberOfProxies("layouts") == 0)
    {
      vtkSMProxy* vlayout = builder->createProxy("misc", "ViewLayout", server, "layouts");
      Q_ASSERT(vlayout != NULL);
      (void)vlayout;
    }

    QString curView = vtkPVGeneralSettings::GetInstance()->GetDefaultViewType();
    if (curView != "None" && !curView.isEmpty() && this->WarningMode != CLIENT_OPENGL_INADEQUATE)
    {
      // When a server is created, we create a new render view for it.
      builder->createView(curView, server);
    }
  }

  // Show warning dialogs before server times out.
  QObject::connect(
    server, SIGNAL(fiveMinuteTimeoutWarning()), this, SLOT(fiveMinuteTimeoutWarning()));
  QObject::connect(server, SIGNAL(finalTimeoutWarning()), this, SLOT(finalTimeoutWarning()));
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::fiveMinuteTimeoutWarning()
{
  QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Server Timeout Warning"),
    tr("The server connection will timeout under 5 minutes.\n"
       "Please save your work."),
    QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::finalTimeoutWarning()
{
  QMessageBox::critical(pqCoreUtilities::mainWidget(), tr("Server Timeout Warning"),
    tr("The server connection will timeout shortly.\n"
       "Please save your work."),
    QMessageBox::Ok);
}
