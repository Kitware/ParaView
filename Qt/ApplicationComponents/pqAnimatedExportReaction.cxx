// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAnimatedExportReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPVApplicationCore.h"
#include "pqProgressManager.h"
#include "pqRenderView.h"
#include "vtkSMAnimationSceneWebWriter.h"

#include <QDebug>
#include <QProgressDialog>

//-----------------------------------------------------------------------------
pqAnimatedExportReaction::pqAnimatedExportReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // Update state depends on whether we are connected to an active
  // server or not and whether the view is not null and has changed
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqAnimatedExportReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  bool is_enabled =
    (activeObjects->activeServer() != nullptr && activeObjects->activeView() != nullptr);
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
void pqAnimatedExportReaction::Export()
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  if (!mgr || !mgr->getActiveScene())
  {
    qDebug() << "Cannot save animation since there is no active scene.";
    return;
  }

  auto rview = dynamic_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!rview)
  {
    qDebug() << "Cannot save animation since there is no active render view.";
    return;
  }

  QString filters = "vtk.js Web Archives (*.vtkjs)";
  pqFileDialog fileDialog(nullptr, pqCoreUtilities::mainWidget(), tr("Export Animated Scene ..."),
    QString(), filters, false);
  fileDialog.setObjectName("ExportAnimatedSceneFileDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pqAnimatedExportReaction::Export(fileDialog.getSelectedFiles()[0]);
  }
}

//-----------------------------------------------------------------------------
void pqAnimatedExportReaction::Export(const QString& filename)
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  pqAnimationScene* scene;
  if (mgr == nullptr || (scene = mgr->getActiveScene()) == nullptr)
  {
    qDebug() << "Cannot save web animation since no active scene is present.";
    return;
  }

  auto rview = dynamic_cast<pqRenderView*>(pqActiveObjects::instance().activeView());
  if (!rview)
  {
    qDebug() << "Cannot save web animation since there is no active render view.";
    return;
  }

  QProgressDialog progress(
    tr("Export animated scene progress"), tr("Abort"), 0, 100, pqCoreUtilities::mainWidget());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setWindowTitle(tr("Saving animated scene ..."));
  progress.show();
  QObject::connect(&progress, &QProgressDialog::canceled, [scene, &progress]() {
    progress.hide();
    scene->pause();
  });

  auto sceneConnection =
    QObject::connect(scene, &pqAnimationScene::tick, [&progress](int progressInPercent) {
      if (progress.isVisible())
      {
        progress.setValue(progressInPercent);
      }
    });

  auto pgm = pqPVApplicationCore::instance()->getProgressManager();
  // this is essential since pqProgressManager blocks all interaction
  // events when progress events are pending. since we have a QProgressDialog
  // as modal, we don't need to that. Plus, we want the cancel button on the
  // dialog to work.
  const auto prev = pgm->unblockEvents(true);

  { // Save file
    vtkNew<vtkSMAnimationSceneWebWriter> writer;
    writer->SetFileName(filename.toUtf8().data());
    writer->SetAnimationScene(scene->getProxy());
    writer->SetRenderView(rview->getRenderViewProxy());
    if (!writer->Save())
    {
      qDebug() << "Save animated scene failed!";
    }
  }
  pgm->unblockEvents(prev);
  scene->disconnect(sceneConnection);
}
