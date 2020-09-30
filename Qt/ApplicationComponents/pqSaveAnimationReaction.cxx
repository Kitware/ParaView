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
#include "pqAnimationProgressDialog.h"
#include "pqAnimationScene.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqObjectBuilder.h"
#include "pqProgressManager.h"
#include "pqProxyWidgetDialog.h"
#include "pqSaveScreenshotReaction.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSettings.h"
#include "pqTabbedMultiViewWidget.h"
#include "pqView.h"
#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMProperty.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMPropertyLink.h"
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
void pqSaveAnimationReaction::saveAnimation()
{
  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save image. No active view.";
    return;
  }

  pqServer* server = view->getServer();
  vtkSMSession* session = server->session();
  vtkNew<vtkSMParaViewPipelineController> controller;
  vtkWeakPointer<vtkSMProxy> scene = controller->FindAnimationScene(session);
  vtkSMSessionProxyManager* pxm = server->proxyManager();
  vtkWeakPointer<vtkSMViewProxy> viewProxy = view->getViewProxy();
  vtkWeakPointer<vtkSMViewLayoutProxy> layout = vtkSMViewLayoutProxy::FindLayout(viewProxy);

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pxm->NewProxy("misc", "SaveAnimation"));
  vtkSMSaveAnimationProxy* ahProxy = vtkSMSaveAnimationProxy::SafeDownCast(proxy);
  if (!ahProxy)
  {
    qCritical() << "Incorrect type for `SaveAnimation` proxy.";
    return;
  }

  // Get the filename first, this will determine some of the options shown.
  QString filename = pqSaveScreenshotReaction::promptFileName(ahProxy, "*.png");
  if (filename.isEmpty())
  {
    return;
  }

  bool restorePreviewMode = false;

  // Cache the separator width and color
  int width = vtkSMPropertyHelper(ahProxy, "SeparatorWidth").GetAsInt();
  double color[3];
  vtkSMPropertyHelper(ahProxy, "SeparatorColor").Get(color, 3);
  // Link the vtkSMViewLayoutProxy to vtkSMSaveScreenshotProxy to update
  // the SeparatorWidth and SeparatorColor
  vtkNew<vtkSMPropertyLink> widthLink, colorLink;
  if (layout)
  {
    widthLink->AddLinkedProperty(ahProxy, "SeparatorWidth", vtkSMLink::INPUT);
    widthLink->AddLinkedProperty(layout, "SeparatorWidth", vtkSMLink::OUTPUT);
    colorLink->AddLinkedProperty(ahProxy, "SeparatorColor", vtkSMLink::INPUT);
    colorLink->AddLinkedProperty(layout, "SeparatorColor", vtkSMLink::OUTPUT);
  }

  controller->PreInitializeProxy(ahProxy);
  vtkSMPropertyHelper(ahProxy, "View").Set(viewProxy);
  vtkSMPropertyHelper(ahProxy, "Layout").Set(layout);
  vtkSMPropertyHelper(ahProxy, "AnimationScene").Set(scene);
  ahProxy->UpdateDefaultsAndVisibilities(filename.toLocal8Bit().data());
  controller->PostInitializeProxy(ahProxy);

  if (layout)
  {
    int previewMode[2] = { -1, -1 };
    vtkSMPropertyHelper previewHelper(layout, "PreviewMode");
    previewHelper.Get(previewMode, 2);
    if (previewMode[0] == 0 && previewMode[1] == 0)
    {
      // If we are not in the preview mode, enter it with maximum size possible
      vtkVector2i layoutSize = layout->GetSize();
      previewHelper.Set(layoutSize.GetData(), 2);
      restorePreviewMode = true;
    }
    else
    {
      // if in preview mode, check "save all views".
      vtkSMPropertyHelper(ahProxy, "SaveAllViews").Set(1);
    }
  }

  pqProxyWidgetDialog dialog(ahProxy, pqCoreUtilities::mainWidget());
  dialog.setObjectName("SaveAnimationDialog");
  dialog.setApplyChangesImmediately(true);
  dialog.setWindowTitle("Save Animation Options");
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveAnimationDialog");

  if (dialog.exec() == QDialog::Accepted)
  {
    pqAnimationProgressDialog progress(
      "Save animation progress", "Abort", 0, 100, pqCoreUtilities::mainWidget());
    progress.setWindowTitle("Saving Animation ...");
    progress.setAnimationScene(scene);
    progress.show();

    auto appcore = pqApplicationCore::instance();
    auto pgm = appcore->getProgressManager();
    // this is essential since pqProgressManager blocks all interaction
    // events when progress events are pending. since we have a QProgressDialog
    // as modal, we don't need to that. Plus, we want the cancel button on the
    // dialog to work.
    const auto prev = pgm->unblockEvents(true);
    ahProxy->WriteAnimation(filename.toUtf8().data());
    pgm->unblockEvents(prev);
  }

  if (layout)
  {
    // Reset the separator width and color
    vtkSMPropertyHelper(layout, "SeparatorWidth").Set(width);
    vtkSMPropertyHelper(layout, "SeparatorColor").Set(color, 3);
    // Reset to the previous preview resolution or exit preview mode
    if (restorePreviewMode)
    {
      int psize[2] = { 0, 0 };
      vtkSMPropertyHelper(layout, "PreviewMode").Set(psize, 2);
    }
    layout->UpdateVTKObjects();
    widthLink->RemoveAllLinks();
    colorLink->RemoveAllLinks();
  }

  // This should not be needed as image capturing code only affects back buffer,
  // however it is currently needed due to paraview/paraview#17256. Once that's
  // fixed, we should remove this.
  pqApplicationCore::instance()->render();
}
