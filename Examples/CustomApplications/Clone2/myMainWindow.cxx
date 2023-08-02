// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "myMainWindow.h"
#include "ui_myMainWindow.h"

#include <pqAlwaysConnectedBehavior.h>
#include <pqApplicationCore.h>
#include <pqApplyBehavior.h>
#include <pqAutoLoadPluginXMLBehavior.h>
#include <pqAxesToolbar.h>
#include <pqDefaultViewBehavior.h>
#include <pqHelpReaction.h>
#include <pqInterfaceTracker.h>
#include <pqLoadDataReaction.h>
#include <pqMainControlsToolbar.h>
#include <pqParaViewBehaviors.h>
#include <pqParaViewMenuBuilders.h>
#include <pqRepresentationToolbar.h>
#include <pqSetName.h>
#include <pqStandardViewFrameActionsImplementation.h>

#include <QAction>
#include <QList>
#include <QToolBar>

class myMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow()
{
  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Enable help from the properties panel.
  QObject::connect(this->Internals->proxyTabWidget,
    SIGNAL(helpRequested(const QString&, const QString&)), this,
    SLOT(showHelpForProxy(const QString&, const QString&)));

// Populate application menus with actions.
#if 0
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
#else
  QList<QAction*> qa = this->Internals->menu_File->actions();
  QAction* mqa = qa.at(0);
  new pqLoadDataReaction(mqa);
#endif

  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

// Populate toolbars
#if 0
  pqParaViewMenuBuilders::buildToolbars(*this);
#else
  QToolBar* mainToolBar = new pqMainControlsToolbar(this) << pqSetName("MainControlsToolbar");
  mainToolBar->layout()->setSpacing(0);
  this->addToolBar(Qt::TopToolBarArea, mainToolBar);

  QToolBar* reprToolbar = new pqRepresentationToolbar(this) << pqSetName("representationToolbar");
  reprToolbar->layout()->setSpacing(0);
  this->addToolBar(Qt::TopToolBarArea, reprToolbar);

  QToolBar* axesToolbar = new pqAxesToolbar(this) << pqSetName("axesToolbar");
  axesToolbar->layout()->setSpacing(0);
  this->addToolBar(Qt::TopToolBarArea, axesToolbar);
#endif

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

#if 1
  // Final step, define application behaviors. Since we want some paraview behaviors
  // we can use static method to configure the pqParaViewBehaviors and select
  // only the components we want
  pqParaViewBehaviors::setEnableStandardPropertyWidgets(true);
  pqParaViewBehaviors::setEnableStandardRecentlyUsedResourceLoader(false);
  pqParaViewBehaviors::setEnableDataTimeStepBehavior(false);
  pqParaViewBehaviors::setEnableSpreadSheetVisibilityBehavior(false);
  pqParaViewBehaviors::setEnablePipelineContextMenuBehavior(false);
  pqParaViewBehaviors::setEnableObjectPickingBehavior(false);
  pqParaViewBehaviors::setEnableUndoRedoBehavior(false);
  pqParaViewBehaviors::setEnableCrashRecoveryBehavior(false);
  pqParaViewBehaviors::setEnablePluginDockWidgetsBehavior(false);
  pqParaViewBehaviors::setEnableVerifyRequiredPluginBehavior(false);
  pqParaViewBehaviors::setEnablePluginActionGroupBehavior(false);
  pqParaViewBehaviors::setEnableCommandLineOptionsBehavior(false);
  pqParaViewBehaviors::setEnablePersistentMainWindowStateBehavior(false);
  pqParaViewBehaviors::setEnableCollaborationBehavior(false);
  pqParaViewBehaviors::setEnableViewStreamingBehavior(false);
  pqParaViewBehaviors::setEnablePluginSettingsBehavior(false);
  pqParaViewBehaviors::setEnableQuickLaunchShortcuts(false);
  pqParaViewBehaviors::setEnableLockPanelsBehavior(false);

  // This is actually useless, as they are activated by default
  pqParaViewBehaviors::setEnableStandardViewFrameActions(true);
  pqParaViewBehaviors::setEnableDefaultViewBehavior(true);
  pqParaViewBehaviors::setEnableAlwaysConnectedBehavior(true);
  pqParaViewBehaviors::setEnableAutoLoadPluginXMLBehavior(true);
  pqParaViewBehaviors::setEnableApplyBehavior(true);
  new pqParaViewBehaviors(this, this);

#else
  // Or do everything manually, not recommended

  // Register standard types of view-frame actions.
  // Needed for Default View Behavior
  pqInterfaceTracker* pgm = pqApplicationCore::instance()->interfaceTracker();
  pgm->addInterface(new pqStandardViewFrameActionsImplementation(pgm));

  // Create behaviors
  new pqDefaultViewBehavior(this);
  new pqAlwaysConnectedBehavior(this);
  new pqAutoLoadPluginXMLBehavior(this);
  pqApplyBehavior* applyBehavior = new pqApplyBehavior(this);

  // Register panels
  Q_FOREACH (pqPropertiesPanel* ppanel, this->findChildren<pqPropertiesPanel*>())
  {
    applyBehavior->registerPanel(ppanel);
  }
#endif
}

//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void myMainWindow::showHelpForProxy(const QString& groupname, const QString& proxyname)
{
  pqHelpReaction::showProxyHelp(groupname, proxyname);
}
