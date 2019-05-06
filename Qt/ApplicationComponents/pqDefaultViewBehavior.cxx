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
#include "vtkPVGeneralSettings.h"
#include "vtkPVOpenGLInformation.h"
#include "vtkPVRenderingCapabilitiesInformation.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <QCoreApplication>
#include <QMessageBox>

#include <cassert>

namespace
{
QString openGLVersionInfo(vtkSMSession* session, vtkPVSession::ServerFlags server_flag)
{
  vtkNew<vtkPVOpenGLInformation> glinfo;
  session->GatherInformation(server_flag, glinfo.GetPointer(), 0);
  return QString(QCoreApplication::translate("pqDefaultViewBehavior",
                   ("OpenGL Vendor: %1\nOpenGL Version: %2\nOpenGL Renderer: %3")))
    .arg(glinfo->GetVendor().c_str(), glinfo->GetVersion().c_str(), glinfo->GetRenderer().c_str());
}
}

//-----------------------------------------------------------------------------
pqDefaultViewBehavior::pqDefaultViewBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(onServerCreation(pqServer*)));

  this->WarningsTimer.setSingleShot(true);
  this->connect(&this->WarningsTimer, SIGNAL(timeout()), SLOT(showWarnings()));
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::showWarnings()
{
  using RCInfo = vtkPVRenderingCapabilitiesInformation;
  if (RCInfo::Supports(this->ClientCapabilities, RCInfo::OPENGL) &&
    RCInfo::Supports(this->ServerCapabilities, RCInfo::OPENGL))
  {
    // all's well!
    return;
  }

  if (this->Server == nullptr)
  {
    return;
  }

  if (!RCInfo::Supports(this->ClientCapabilities, RCInfo::OPENGL))
  {
    QString msg = tr("Your OpenGL drivers don't support\n"
                     "required OpenGL features for basic rendering.\n"
                     "Application cannot continue. Please exit and use an older version.\n\n"
                     "CONTINUE AT YOUR OWN RISK!\n\n");
    msg += openGLVersionInfo(this->Server->session(), vtkSMSession::CLIENT);
    QMessageBox::warning(
      pqCoreUtilities::mainWidget(), tr("OpenGL support inadequate!"), msg, QMessageBox::Ok);
    return;
  }

  if (!this->Server->isRemote())
  {
    // for non remote server, that's the only message.
    return;
  }

  if (!RCInfo::Supports(this->ServerCapabilities, RCInfo::RENDERING))
  {
    QMessageBox::warning(pqCoreUtilities::mainWidget(), tr("Server DISPLAY not accessible!"),
      tr("Display is not accessible on the server side.\n"
         "Remote rendering will be disabled."),
      QMessageBox::Ok);
  }
  else if (!RCInfo::Supports(this->ServerCapabilities, RCInfo::OPENGL))
  {
    QString msg = tr("OpenGL drivers on the server side don't support\n"
                     "required OpenGL features for basic rendering.\n"
                     "Remote rendering will be disabled.");
    msg += openGLVersionInfo(this->Server->session(), vtkSMSession::RENDER_SERVER);
    QMessageBox::warning(
      pqCoreUtilities::mainWidget(), tr("Server OpenGL support inadequate!"), msg, QMessageBox::Ok);
  }
}

//-----------------------------------------------------------------------------
void pqDefaultViewBehavior::onServerCreation(pqServer* server)
{
  using RCInfo = vtkPVRenderingCapabilitiesInformation;

  pqApplicationCore* core = pqApplicationCore::instance();

  // Get information about rendering capabilities.
  vtkNew<RCInfo> info;
  server->session()->GatherInformation(vtkSMSession::RENDER_SERVER, info.GetPointer(), 0);

  this->Server = server;
  this->ServerCapabilities = this->ClientCapabilities = info->GetCapabilities();

  // if server is remote, we separately get client capabilities.
  if (server->isRemote())
  {
    this->ClientCapabilities = RCInfo::GetLocalCapabilities();
  }

  // Setup a timer to show warning messages, if needed.
  this->WarningsTimer.start(500);

  // See if some view are already present and if we're in a collaborative
  // session, we are the master.
  if (core->getServerManagerModel()->getNumberOfItems<pqView*>() == 0 && server->isMaster())
  {
    pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
    const QString viewType = vtkPVGeneralSettings::GetInstance()->GetDefaultViewType();
    if (viewType != "None" && !viewType.isEmpty() &&
      RCInfo::Supports(this->ClientCapabilities, RCInfo::OPENGL))
    {
      // When a server is created, we create a new render view for it.
      if (auto pqview = builder->createView(viewType, server))
      {
        // let's put this view under a layout.
        builder->addToLayout(pqview);
      }
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
