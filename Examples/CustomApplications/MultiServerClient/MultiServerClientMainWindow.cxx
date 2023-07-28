// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "MultiServerClientMainWindow.h"
#include "ui_MultiServerClientMainWindow.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqObjectBuilder.h>
#include <pqParaViewBehaviors.h>
#include <pqParaViewMenuBuilders.h>
#include <pqPipelineBrowserWidget.h>
#include <pqServer.h>
#include <pqServerConfiguration.h>
#include <pqServerConnectReaction.h>
#include <pqServerManagerModel.h>
#include <vtkProcessModule.h>
#include <vtkSMSession.h>
#include <vtkSession.h>

#include <QComboBox>

//-----------------------------------------------------------------------------
MultiServerClientMainWindow::MultiServerClientMainWindow(
  QWidget* parentObject, Qt::WindowFlags wflags)
  : Superclass(parentObject, wflags)
{
  // tells the ParaView libraries to enable support for multiple simultaneous
  // server connections.
  vtkProcessModule::GetProcessModule()->MultipleSessionsSupportOn();
  Ui::MultiServerClientMainWindow ui;
  ui.setupUi(this);

  this->tabifyDockWidget(ui.objectInspectorDock, ui.informationDock);
  ui.objectInspectorDock->raise();

  // create the representation when user hits "Apply";
  // ui.proxyTabWidget->setShowOnAccept(true);

  new pqServerConnectReaction(ui.action_Connect);

  pqParaViewMenuBuilders::buildSourcesMenu(*ui.menu_Sources, this);
  new pqParaViewBehaviors(this, this);

  // Keep around GUI components
  this->pipelineBrowser = ui.pipelineBrowser;
  this->comboBox = ui.filteringServer;

  this->pipelineBrowser2 = ui.pipelineBrowser2;
  this->comboBox2 = ui.filteringServer2;

  // Add empty filtering
  this->comboBox->addItem("No filtering", QVariant());
  this->comboBox2->addItem("No filtering", QVariant());

  // Add current server in filtering
  addServerInFiltering(pqActiveObjects::instance().activeServer());

  // Listen when new connection occurs
  QObject::connect(pqApplicationCore::instance()->getServerManagerModel(),
    SIGNAL(serverAdded(pqServer*)), this, SLOT(addServerInFiltering(pqServer*)),
    Qt::QueuedConnection);

  // Listen when we filter with different criteria
  QObject::connect(this->comboBox, SIGNAL(currentIndexChanged(int)), this,
    SLOT(applyPipelineFiltering(int)), Qt::QueuedConnection);
  QObject::connect(this->comboBox2, SIGNAL(currentIndexChanged(int)), this,
    SLOT(applyPipelineFiltering2(int)), Qt::QueuedConnection);
}

//-----------------------------------------------------------------------------
MultiServerClientMainWindow::~MultiServerClientMainWindow() = default;

//-----------------------------------------------------------------------------
void MultiServerClientMainWindow::applyPipelineFiltering(int index)
{
  QVariant sessionIdFiltering = this->comboBox->itemData(index);
  if (sessionIdFiltering.isValid())
  {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    this->pipelineBrowser->enableSessionFilter(
      pm->GetSession(static_cast<vtkIdType>(sessionIdFiltering.toInt())));
    this->pipelineBrowser->expandAll();
  }
  else
  {
    this->pipelineBrowser->disableAnnotationFilter();
    this->pipelineBrowser->disableSessionFilter();
    this->pipelineBrowser->expandAll();
  }
}

//-----------------------------------------------------------------------------
void MultiServerClientMainWindow::applyPipelineFiltering2(int index)
{
  QVariant sessionIdFiltering = this->comboBox2->itemData(index);
  if (sessionIdFiltering.isValid())
  {
    vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
    this->pipelineBrowser2->enableSessionFilter(
      pm->GetSession(static_cast<vtkIdType>(sessionIdFiltering.toInt())));
    this->pipelineBrowser2->expandAll();
  }
  else
  {
    this->pipelineBrowser2->disableAnnotationFilter();
    this->pipelineBrowser2->disableSessionFilter();
    this->pipelineBrowser2->expandAll();
  }
}

//-----------------------------------------------------------------------------
void MultiServerClientMainWindow::addServerInFiltering(pqServer* server)
{
  QVariant sessionID = vtkProcessModule::GetProcessModule()->GetSessionID(server->session());
  QString label = "Server ";
  label.append(sessionID.toString());
  this->comboBox->addItem(label, sessionID);
  this->comboBox2->addItem(label, sessionID);
}
