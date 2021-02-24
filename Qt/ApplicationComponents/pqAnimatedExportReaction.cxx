/*=========================================================================

   Program: ParaView
   Module:    pqAnimatedExportReaction.cxx

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
  pqFileDialog fileDialog(pqActiveObjects::instance().activeServer(), pqCoreUtilities::mainWidget(),
    tr("Export Animated Scene ..."), QString(), filters);
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
    "Export animated scene progress", "Abort", 0, 100, pqCoreUtilities::mainWidget());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setWindowTitle("Saving animated scene ...");
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
    writer->SetFileName(filename.toLocal8Bit().data());
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
