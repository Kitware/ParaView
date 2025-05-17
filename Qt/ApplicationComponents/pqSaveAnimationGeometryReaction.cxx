// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqSaveAnimationGeometryReaction.h"

#include "pqActiveObjects.h"
#include "pqAnimationManager.h"
#include "pqAnimationScene.h"
#include "pqCoreUtilities.h"
#include "pqFileDialog.h"
#include "pqPVApplicationCore.h"
#include "pqProgressManager.h"

#include <QDebug>
#include <QProgressDialog>

//-----------------------------------------------------------------------------
pqSaveAnimationGeometryReaction::pqSaveAnimationGeometryReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  // load state enable state depends on whether we are connected to an active
  // server or not and whether
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  QObject::connect(
    activeObjects, SIGNAL(serverChanged(pqServer*)), this, SLOT(updateEnableState()));
  QObject::connect(activeObjects, SIGNAL(viewChanged(pqView*)), this, SLOT(updateEnableState()));
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqSaveAnimationGeometryReaction::updateEnableState()
{
  pqActiveObjects* activeObjects = &pqActiveObjects::instance();
  bool is_enabled =
    (activeObjects->activeServer() != nullptr && activeObjects->activeView() != nullptr);
  this->parentAction()->setEnabled(is_enabled);
}

//-----------------------------------------------------------------------------
void pqSaveAnimationGeometryReaction::saveAnimationGeometry()
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  if (!mgr || !mgr->getActiveScene())
  {
    qDebug() << "Cannot save animation since no active scene is present.";
    return;
  }

  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save animation geometry since no active view.";
    return;
  }

  QString filters = "ParaView Data files (*.pvd);;All files (*)";
  pqFileDialog fileDialog(pqActiveObjects::instance().activeServer(), pqCoreUtilities::mainWidget(),
    tr("Save Animation Geometry"), QString(), filters, false);
  fileDialog.setObjectName("FileSaveAnimationDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    pqSaveAnimationGeometryReaction::saveAnimationGeometry(fileDialog.getSelectedFiles()[0]);
  }
}

//-----------------------------------------------------------------------------
void pqSaveAnimationGeometryReaction::saveAnimationGeometry(const QString& filename)
{
  pqAnimationManager* mgr = pqPVApplicationCore::instance()->animationManager();
  if (!mgr || !mgr->getActiveScene())
  {
    qDebug() << "Cannot save animation since no active scene is present.";
    return;
  }

  pqView* view = pqActiveObjects::instance().activeView();
  if (!view)
  {
    qDebug() << "Cannot save animation geometry since no active view.";
    return;
  }

  auto pqscene = mgr->getActiveScene();

  QProgressDialog progress(
    tr("Save geometry progress"), tr("Abort"), 0, 100, pqCoreUtilities::mainWidget());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setWindowTitle(tr("Saving Geometry ..."));
  progress.show();
  QObject::connect(&progress, &QProgressDialog::canceled,
    [pqscene, &progress]()
    {
      progress.hide();
      pqscene->pause();
    });

  auto sceneConnection = QObject::connect(pqscene, &pqAnimationScene::tick,
    [&progress](int progressInPercent)
    {
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
  if (!mgr->saveGeometry(filename, view))
  {
    qDebug() << "Animation save geometry failed!";
  }
  pgm->unblockEvents(prev);
  pqscene->disconnect(sceneConnection);
}
