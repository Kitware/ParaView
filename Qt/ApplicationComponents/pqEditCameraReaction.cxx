// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqEditCameraReaction.h"

#include "pqActiveObjects.h"
#include "pqCameraDialog.h"
#include "pqCoreUtilities.h"
#include "pqRenderView.h"

//-----------------------------------------------------------------------------
pqEditCameraReaction::pqEditCameraReaction(QAction* parentObject, pqView* view)
  : Superclass(parentObject)
  , View(view)
{
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)), this,
    SLOT(updateEnableState()), Qt::QueuedConnection);

  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqEditCameraReaction::updateEnableState()
{
  this->View = pqActiveObjects::instance().activeView();
  if (qobject_cast<pqRenderView*>(this->View))
  {
    this->parentAction()->setEnabled(true);
  }
  else
  {
    this->parentAction()->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------
void pqEditCameraReaction::editCamera(pqView* view)
{
  static QPointer<pqCameraDialog> dialog;

  pqRenderView* renModule = qobject_cast<pqRenderView*>(view);
  if (!renModule)
  {
    if (dialog)
    {
      dialog->SetCameraGroupsEnabled(false);
    }
    return;
  }

  if (!dialog)
  {
    dialog = new pqCameraDialog(pqCoreUtilities::mainWidget());
    dialog->setWindowTitle(tr("Adjust Camera"));
    QObject::connect(dialog, &QWidget::close, dialog, &QObject::deleteLater);
    dialog->setRenderModule(renModule);
  }
  else
  {
    dialog->SetCameraGroupsEnabled(true);
    dialog->setRenderModule(renModule);
    dialog->raise();
    dialog->activateWindow();
  }
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqEditCameraReaction::onTriggered()
{
  pqEditCameraReaction::editCamera(this->View);
}
