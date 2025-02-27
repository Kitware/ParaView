// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
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
#include "pqTabbedMultiViewWidget.h"
#include "pqUndoStack.h"
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
  bool is_enabled = (activeObjects->activeServer() != nullptr);
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
void pqSaveAnimationReaction::saveAnimation()
{
  SCOPED_UNDO_EXCLUDE();
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
  vtkTypeUInt32 location;
  QString filename = pqSaveScreenshotReaction::promptFileName(ahProxy, "*.png", location);
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
  ahProxy->UpdateDefaultsAndVisibilities(filename.toUtf8().data());
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
  dialog.setWindowTitle(tr("Save Animation Options"));
  dialog.setEnableSearchBar(true);
  dialog.setSettingsKey("SaveAnimationDialog");

  if (dialog.exec() == QDialog::Accepted)
  {
    pqAnimationProgressDialog progress(
      tr("Save animation progress"), tr("Abort"), 0, 100, pqCoreUtilities::mainWidget());
    progress.setWindowTitle(tr("Saving Animation ..."));
    progress.setAnimationScene(scene);
    progress.show();

    auto appcore = pqApplicationCore::instance();
    auto pgm = appcore->getProgressManager();
    // this is essential since pqProgressManager blocks all interaction
    // events when progress events are pending. since we have a QProgressDialog
    // as modal, we don't need to that. Plus, we want the cancel button on the
    // dialog to work.
    const auto prev = pgm->unblockEvents(true);
    ahProxy->WriteAnimation(filename.toUtf8().data(), location);
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
}
