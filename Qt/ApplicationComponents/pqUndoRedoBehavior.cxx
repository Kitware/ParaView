// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqUndoRedoBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqServerManagerModel.h"
#include "pqUndoStack.h"
#include "pqUndoStackBuilder.h"

#include "vtkSMProxyManager.h"
#include "vtkSMSession.h"
#include "vtkSMSessionProxyManager.h"

#include <QDebug>

//-----------------------------------------------------------------------------
pqUndoRedoBehavior::pqUndoRedoBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getUndoStack())
  {
    qCritical() << "Application wide undo-stack has already been initialized.";
    return;
  }

  // setup Undo Stack.
  pqUndoStackBuilder* builder = pqUndoStackBuilder::New();
  pqUndoStack* stack = new pqUndoStack(builder, this);
  vtkSMProxyManager::GetProxyManager()->SetUndoStackBuilder(builder);
  builder->Delete();
  core->setUndoStack(stack);

  // clear undo stack when state is loaded.
  QObject::connect(
    core, SIGNAL(stateLoaded(vtkPVXMLElement*, vtkSMProxyLocator*)), stack, SLOT(clear()));

  // clear stack when server connects/disconnects.
  QObject::connect(
    core->getServerManagerModel(), SIGNAL(serverAdded(pqServer*)), stack, SLOT(clear()));
  QObject::connect(
    core->getServerManagerModel(), SIGNAL(finishedRemovingServer()), stack, SLOT(clear()));
}
