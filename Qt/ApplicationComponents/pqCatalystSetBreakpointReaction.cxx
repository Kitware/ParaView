// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCatalystSetBreakpointReaction.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqLiveInsituManager.h"
#include "pqLiveInsituVisualizationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqSetBreakpointDialog.h"
#include "vtkProcessModule.h"
#include "vtkSMLiveInsituLinkProxy.h"
#include "vtkSMPropertyHelper.h"
#include "vtkSMSession.h"

#include <QInputDialog>
#include <QMessageBox>

class pqCatalystSetBreakpointReaction::sbrInternal
{
public:
  sbrInternal(QWidget* parent)
    : Dialog(parent)
  {
  }
  pqSetBreakpointDialog Dialog;
};

//-----------------------------------------------------------------------------
pqCatalystSetBreakpointReaction::pqCatalystSetBreakpointReaction(QAction* parentObject)
  : Superclass(parentObject)
  , Internal(new sbrInternal(pqCoreUtilities::mainWidget()))
{
  QObject::connect(parentObject->parent(), SIGNAL(aboutToShow()), this, SLOT(updateEnableState()));
}

//-----------------------------------------------------------------------------
void pqCatalystSetBreakpointReaction::onTriggered()
{
  // we set the breakpoint time inside the dialog
  this->Internal->Dialog.exec();
}

//-----------------------------------------------------------------------------
void pqCatalystSetBreakpointReaction::updateEnableState()
{
  this->parentAction()->setEnabled(pqLiveInsituManager::instance()->linkProxy() ? true : false);
}
