// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveExtractsReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationProgressDialog.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqProgressManager.h"
#include "pqProxyWidgetDialog.h"

#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSaveAnimationExtractsProxy.h"
#include "vtkSMSessionProxyManager.h"
#include "vtkSmartPointer.h"

//-----------------------------------------------------------------------------
pqSaveExtractsReaction::pqSaveExtractsReaction(QAction* parentObject)
  : Superclass(parentObject)
{
}

//-----------------------------------------------------------------------------
pqSaveExtractsReaction::~pqSaveExtractsReaction() = default;

//-----------------------------------------------------------------------------
bool pqSaveExtractsReaction::generateExtracts()
{
  vtkNew<vtkSMParaViewPipelineController> controller;
  auto pxm = pqActiveObjects::instance().proxyManager();
  auto proxy = vtkSmartPointer<vtkSMProxy>::Take(pxm->NewProxy("misc", "SaveAnimationExtracts"));
  auto exporter = vtkSMSaveAnimationExtractsProxy::SafeDownCast(proxy);
  if (exporter)
  {
    controller->PreInitializeProxy(exporter);
    auto scene = controller->FindAnimationScene(pxm->GetSession());
    vtkSMPropertyHelper(exporter, "AnimationScene").Set(scene);
    controller->PostInitializeProxy(exporter);

    pqProxyWidgetDialog dialog(exporter, pqCoreUtilities::mainWidget());
    dialog.setWindowTitle(tr("Save Extracts Options"));
    dialog.setSettingsKey("SaveAnimationExtracts");
    if (dialog.exec() == QDialog::Accepted)
    {
      pqAnimationProgressDialog progress(
        "Save Extracts Progress", "Abort", 0, 100, pqCoreUtilities::mainWidget());
      progress.setWindowTitle(tr("Saving Extracts ..."));
      progress.setAnimationScene(scene);
      progress.show();

      auto appcore = pqApplicationCore::instance();
      auto pgm = appcore->getProgressManager();
      // this is essential since pqProgressManager blocks all interaction
      // events when progress events are pending. since we have a QProgressDialog
      // as modal, we don't need to that. Plus, we want the cancel button on the
      // dialog to work.
      const auto prev = pgm->unblockEvents(true);
      const auto status = exporter->SaveExtracts();
      pgm->unblockEvents(prev);
      return status;
    }
  }
  return false;
}
