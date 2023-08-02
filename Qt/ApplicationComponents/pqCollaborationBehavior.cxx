// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqCollaborationBehavior.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqCollaborationManager.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include "vtkSMSession.h"

//-----------------------------------------------------------------------------
pqCollaborationBehavior::pqCollaborationBehavior(QObject* parentObject)
  : Superclass(parentObject)
{
  this->CollaborationManager = new pqCollaborationManager(this);
  pqApplicationCore* core = pqApplicationCore::instance();
  core->registerManager("COLLABORATION_MANAGER", this->CollaborationManager);

  QObject::connect(core->getServerManagerModel(), SIGNAL(preServerAdded(pqServer*)),
    this->CollaborationManager, SLOT(onServerAdded(pqServer*)));

  QObject::connect(core->getServerManagerModel(), SIGNAL(aboutToRemoveServer(pqServer*)),
    this->CollaborationManager, SLOT(onServerRemoved(pqServer*)));

  QObject::connect(this->CollaborationManager, SIGNAL(triggeredMasterChanged(bool)), core,
    SIGNAL(updateMasterEnableState(bool)));

  // We attach mouse listener on active change time otherwise when the view
  // is just added, the widget is not yet correctly initialized...
  QObject::connect(&pqActiveObjects::instance(), SIGNAL(viewChanged(pqView*)),
    this->CollaborationManager, SLOT(attachMouseListenerTo3DViews()), Qt::UniqueConnection);
}
