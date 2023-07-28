// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqManageCustomFiltersReaction.h"

#include "pqApplicationCore.h"
#include "pqCoreUtilities.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"

//-----------------------------------------------------------------------------
pqManageCustomFiltersReaction::pqManageCustomFiltersReaction(QAction* parentObject)
  : Superclass(parentObject)
{
  this->Model = new pqCustomFilterManagerModel(this);

  // Listen for compound proxy register events.
  pqServerManagerObserver* observer = pqApplicationCore::instance()->getServerManagerObserver();
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)), this->Model,
    SLOT(addCustomFilter(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)), this->Model,
    SLOT(removeCustomFilter(QString)));

  pqServerManagerModel* smmodel = pqApplicationCore::instance()->getServerManagerModel();
  QObject::connect(
    smmodel, SIGNAL(serverAdded(pqServer*)), this->Model, SLOT(importCustomFiltersFromSettings()));
  QObject::connect(smmodel, SIGNAL(aboutToRemoveServer(pqServer*)), this->Model,
    SLOT(exportCustomFiltersToSettings()));
}

//-----------------------------------------------------------------------------
void pqManageCustomFiltersReaction::manageCustomFilters()
{
  pqCustomFilterManager dialog(this->Model, pqCoreUtilities::mainWidget());
  dialog.exec();
}
