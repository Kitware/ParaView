// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqAlwaysConnectedBehavior.h"

#include "pqObjectBuilder.h"
#include "pqPVApplicationCore.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "vtkNetworkAccessManager.h"
#include "vtkProcessModule.h"

#include <cassert>

//-----------------------------------------------------------------------------
pqAlwaysConnectedBehavior::pqAlwaysConnectedBehavior(QObject* parentObject)
  : Superclass(parentObject)
  , DefaultServer("builtin:")
{
  auto core = pqPVApplicationCore::instance();
  assert(core != nullptr);

  // check for valid server when application becomes ready.
  this->connect(core, SIGNAL(clientEnvironmentDone()), SLOT(serverCheck()));

  // check for valid server after disconnect.
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  this->Timer.setSingleShot(true);
  this->Timer.setInterval(0);
  this->connect(&this->Timer, SIGNAL(timeout()), SLOT(serverCheck()));
  this->Timer.connect(smmodel, SIGNAL(finishedRemovingServer()), SLOT(start()));

  this->serverCheck();
}

//-----------------------------------------------------------------------------
pqAlwaysConnectedBehavior::~pqAlwaysConnectedBehavior() = default;

//-----------------------------------------------------------------------------
void pqAlwaysConnectedBehavior::serverCheck()
{
  pqPVApplicationCore* core = pqPVApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfItems<pqServer*>() != 0)
  {
    return;
  }
  if (core->getObjectBuilder()->waitingForConnection())
  {
    // Try again later, we are waiting for server to connect.
    this->Timer.start();
    return;
  }

  core->getObjectBuilder()->createServer(this->DefaultServer);
}
