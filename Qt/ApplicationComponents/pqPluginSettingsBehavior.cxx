// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "pqPluginSettingsBehavior.h"

#include "pqApplicationCore.h"
#include "pqPluginManager.h"
#include "pqServer.h"

#include "vtkNew.h"
#include "vtkSMParaViewPipelineController.h"

//----------------------------------------------------------------------------
pqPluginSettingsBehavior::pqPluginSettingsBehavior(QObject* parent)
  : Superclass(parent)
{
  pqPluginManager* pluginManager = pqApplicationCore::instance()->getPluginManager();
  this->connect(pluginManager, SIGNAL(pluginsUpdated()), this, SLOT(updateSettings()));
}

//----------------------------------------------------------------------------
void pqPluginSettingsBehavior::updateSettings()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  vtkSMSession* session = server->session();
  vtkNew<vtkSMParaViewPipelineController> controller;
  controller->UpdateSettingsProxies(session);
}
