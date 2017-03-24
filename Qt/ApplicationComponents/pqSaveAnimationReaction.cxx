/*=========================================================================

   Program: ParaView
   Module:    pqSaveAnimationReaction.cxx

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
#include "pqSaveAnimationReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqProxyWidgetDialog.h"
#include "pqServer.h"
#include "pqSettings.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSaveAnimationProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSMViewLayoutProxy.h"
#include "vtkSMViewProxy.h"
#include "vtkSmartPointer.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqSaveAnimationReaction::pqSaveAnimationReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveAnimationReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  bool is_enabled = (activeObjects->activeServer() != NULL);
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
QString pqSaveAnimationReaction::promptFileName(pqServer* server, bool remote)
{
  QString lastUsedExt;

  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("extensions/AnimationExtension"))
  {
    lastUsedExt = settings->value("extensions/AnimationExtension").toString();
  }

  vtkSMSession* session = server->session();

  QStringList filters;
  if (vtkSMSaveAnimationProxy::SupportsAVI(session, remote))
  {
    filters << "AVI files (*.avi)";
  }
  if (vtkSMSaveAnimationProxy::SupportsOGV(session, remote))
  {
    filters << "Ogg/Theora files (*.ogv)";
  }
  filters << "PNG images (*.png)"
          << "JPG images (*.jpg)"
          << "TIFF images (*.tif)"
          << "BMP images (*.bmp)"
          << "PPM images (*.ppm)";

  pqFileDialog file_dialog(remote ? server : NULL, pqCoreUtilities::mainWidget(),
    tr("Save Animation"), QString(), filters.join(";;"));
  file_dialog.setRecentlyUsedExtension(lastUsedExt);
  file_dialog.setObjectName("FileSaveAnimationDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
  {
    return QString();
  }

  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo(file);
  lastUsedExt = QString("*.") + fileInfo.suffix();
  settings->setValue("extensions/AnimationExtension", lastUsedExt);
  return file;
}

//-----------------------------------------------------------------------------
void pqSaveAnimationReaction::saveAnimation()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannnot save image. No active view.";
    return;
  }

  pqServer* server = view->getServer();
  vtkSMSession* session = server->session();
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkWeakPointer<vtkSMProxy> scene = controller->FindAnimationScene(session);
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkWeakPointer<vtkSMViewProxy> viewProxy = view->getViewProxy();
  vtkWeakPointer<vtkSMViewLayoutProxy> layout = vtkSMViewLayoutProxy::FindLayout(viewProxy);
  int showWindowDecorations = -1;

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "SaveAnimation"));
  vtkSMSaveAnimationProxy* ahProxy = vtkSMSaveAnimationProxy::SafeDownCast(proxy);
  if (!ahProxy)
  {
    qCritical() << "Incorrect type for `SaveAnimation` proxy.";
    return;
  }

  controller->PreInitializeProxy(ahProxy);
  vtkSMPropertyHelper(ahProxy, "View").Set(viewProxy);
  vtkSMPropertyHelper(ahProxy, "Layout").Set(layout);
  vtkSMPropertyHelper(ahProxy, "AnimationScene").Set(scene);
  controller->PostInitializeProxy(ahProxy);

  if (ahProxy->UpdateSaveAllViewsPanelVisibility())
  {
    Q_ASSERT(layout != NULL);
    // let's hide window decorations.
    vtkSMPropertyHelper helper(layout, "ShowWindowDecorations");
    showWindowDecorations = helper.GetAsInt();
    helper.Set(0);
  }

  if (!vtkSMSaveAnimationProxy::SupportsDisconnectAndSave(session))
  {
    vtkSMPropertyHelper(ahProxy, "DisconnectAndSave").Set(0);
    ahProxy->GetProperty("DisconnectAndSave")->SetPanelVisibility("never");
  }

  // scope to ensure that pqProxyWidgetDialog is destroyed and releases the
  // ahProxy reference when it's done.
  {
    pqProxyWidgetDialog dialog(ahProxy, pqCoreUtilities::mainWidget());
    dialog.setObjectName("SaveAnimationDialog");
    dialog.setApplyChangesImmediately(true);
    dialog.setWindowTitle("Save Animation Options");
    dialog.setEnableSearchBar(true);
    dialog.setSettingsKey("SaveAnimationDialog");
    if (dialog.exec() != QDialog::Accepted)
    {
      if (layout)
      {
        vtkSMPropertyHelper(layout, "ShowWindowDecorations").Set(showWindowDecorations);
        layout->UpdateVTKObjects();
      }
      return;
    }
  }

  bool disconnectAndSave = vtkSMPropertyHelper(ahProxy, "DisconnectAndSave").GetAsInt() != 0;
  QString filename = pqSaveAnimationReaction::promptFileName(server, disconnectAndSave);
  if (!filename.isEmpty())
  {
    if (ahProxy->WriteAnimation(filename.toLatin1().data()) && disconnectAndSave)
    {
      Q_ASSERT(ahProxy->GetReferenceCount() == 1);
      ahProxy = NULL;
      proxy = NULL; // release reference.

      pqObjectBuilder* ob = pqApplicationCore::instance()->getObjectBuilder();
      ob->removeServer(server);
    }
  }

  if (layout && showWindowDecorations != -1)
  {
    vtkSMPropertyHelper(layout, "ShowWindowDecorations").Set(showWindowDecorations);
    layout->UpdateVTKObjects();
  }

  if (!disconnectAndSave)
  {
    // This should not be needed as image capturing code only affects back buffer,
    // however it is currently needed due to paraview/paraview#17256. Once that's
    // fixed, we should remove this.
    pqApplicationCore::instance()->render();
  }
}
