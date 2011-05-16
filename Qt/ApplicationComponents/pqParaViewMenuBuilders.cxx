/*=========================================================================

   Program: ParaView
   Module:    pqParaViewMenuBuilders.cxx

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
#include "pqParaViewMenuBuilders.h"
#include "vtkPVConfig.h"

#include "ui_pqFileMenuBuilder.h"
#include "ui_pqEditMenuBuilder.h"
#include "ui_pqPipelineBrowserContextMenu.h"

#include "pqAboutDialogReaction.h"
#include "pqAnimationTimeToolbar.h"
#include "pqApplicationCore.h"
#include "pqApplicationSettingsReaction.h"
#include "pqAxesToolbar.h"
#include "pqCameraLinkReaction.h"
#include "pqCameraToolbar.h"
#include "pqCameraUndoRedoReaction.h"
#include "pqCategoryToolbarsBehavior.h"
#include "pqChangePipelineInputReaction.h"
#include "pqColorToolbar.h"
#include "pqCopyReaction.h"
#include "pqCreateCustomFilterReaction.h"
#include "pqDataQueryReaction.h"
#include "pqDeleteReaction.h"
#include "pqExportReaction.h"
#include "pqFiltersMenuReaction.h"
#include "pqHelpReaction.h"
#include "pqIgnoreSourceTimeReaction.h"
#include "pqLoadDataReaction.h"
#include "pqLoadStateReaction.h"
#include "pqMainControlsToolbar.h"
#include "pqManageCustomFiltersReaction.h"
#include "pqManageLinksReaction.h"
#include "pqManagePluginsReaction.h"
#include "pqProxyGroupMenuManager.h"
#include "pqPVApplicationCore.h"
#include "pqPythonShellReaction.h"
#include "pqRecentFilesMenu.h"
#include "pqRepresentationToolbar.h"
#include "pqSaveAnimationGeometryReaction.h"
#include "pqSaveAnimationReaction.h"
#include "pqSaveDataReaction.h"
#include "pqSaveScreenshotReaction.h"
#include "pqSaveStateReaction.h"

#include "pqSelectionToolbar.h"
#include "pqServerConnectReaction.h"
#include "pqServerDisconnectReaction.h"
#include "pqSetName.h"
#include "pqSourcesMenuReaction.h"
#include "pqTestingReaction.h"
#include "pqTimerLogReaction.h"
#include "pqUndoRedoReaction.h"
#include "pqVCRToolbar.h"
#include "pqViewMenuManager.h"
#include "pqViewSettingsReaction.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqMacroReaction.h"
#include "pqPythonManager.h"
#include "pqTraceReaction.h"
#endif

#include <QDockWidget>
#include <QKeySequence>
#include <QLayout>
#include <QMainWindow>
#include <QMenu>

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildFileMenu(QMenu& menu)
{
  QString objectName = menu.objectName();
  Ui::pqFileMenuBuilder ui;
  ui.setupUi(&menu);
  // since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  QObject::connect(ui.actionFileExit, SIGNAL(triggered()),
    pqApplicationCore::instance(), SLOT(quit()));

  // now setup reactions.
  new pqLoadDataReaction(ui.actionFileOpen);
  new pqRecentFilesMenu(*ui.menuRecentFiles, ui.menuRecentFiles);

  new pqLoadStateReaction(ui.actionFileLoadServerState);
  new pqSaveStateReaction(ui.actionFileSaveServerState);

  new pqServerConnectReaction(ui.actionServerConnect);
  new pqServerDisconnectReaction(ui.actionServerDisconnect);

  new pqSaveScreenshotReaction(ui.actionFileSaveScreenshot);
  new pqSaveAnimationReaction(ui.actionFileSaveAnimation);
  new pqSaveAnimationGeometryReaction(ui.actionFileSaveGeometry);

  new pqExportReaction(ui.actionExport);
  new pqSaveDataReaction(ui.actionFileSaveData);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildEditMenu(QMenu& menu)
{
  QString objectName = menu.objectName();
  Ui::pqEditMenuBuilder ui;
  ui.setupUi(&menu);
  // since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  new pqUndoRedoReaction(ui.actionEditUndo, true);
  new pqUndoRedoReaction(ui.actionEditRedo, false);
  new pqCameraUndoRedoReaction(ui.actionEditCameraUndo, true);
  new pqCameraUndoRedoReaction(ui.actionEditCameraRedo, false);
  new pqChangePipelineInputReaction(ui.actionChangeInput);
  new pqIgnoreSourceTimeReaction(ui.actionIgnoreTime);
  new pqDeleteReaction(ui.actionDelete);
  new pqDeleteReaction(ui.actionDelete_All, true);
  new pqCopyReaction(ui.actionCopy);
  new pqCopyReaction(ui.actionPaste, true);
  new pqApplicationSettingsReaction(ui.actionEditSettings);
  new pqViewSettingsReaction(ui.actionEditViewSettings);
  new pqDataQueryReaction(ui.actionQuery);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildSourcesMenu(QMenu&  menu,
  QMainWindow* mainWindow)
{
  pqProxyGroupMenuManager* mgr = new pqProxyGroupMenuManager(&menu, "ParaViewSources");
  mgr->addProxyDefinitionUpdateListener("sources");
  new pqSourcesMenuReaction(mgr);
  pqPVApplicationCore::instance()->registerForQuicklaunch(&menu);
  if (mainWindow)
    {
    // create toolbars for categories as needed.
    new pqCategoryToolbarsBehavior(mgr, mainWindow);
    }
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildFiltersMenu(QMenu& menu,
  QMainWindow* mainWindow)
{
  pqProxyGroupMenuManager* mgr = new pqProxyGroupMenuManager(&menu, "ParaViewFilters");
  mgr->addProxyDefinitionUpdateListener("filters");
  mgr->setRecentlyUsedMenuSize(10);
  new pqFiltersMenuReaction(mgr);
  pqPVApplicationCore::instance()->registerForQuicklaunch(&menu);

  if (mainWindow)
    {
    // create toolbars for categories as needed.
    new pqCategoryToolbarsBehavior(mgr, mainWindow);
    }
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildToolsMenu(QMenu& menu)
{
  new pqCreateCustomFilterReaction(menu.addAction("Create Custom Filter") <<
    pqSetName("actionToolsCreateCustomFilter"));
  new pqCameraLinkReaction(menu.addAction("Add Camera Link") <<
    pqSetName("actionToolsAddCameraLink"));
  menu.addSeparator();
  new pqManageCustomFiltersReaction(menu.addAction("Manage Custom Filters")
    << pqSetName("actionToolsManageCustomFilters"));
  new pqManageLinksReaction(menu.addAction("Manage Links") <<
    pqSetName("actionToolsManageLinks"));
  //<addaction name="actionToolsAddCameraLink" />
#ifdef BUILD_SHARED_LIBS
  // Add support for importing plugins only if ParaView was built shared.
  new pqManagePluginsReaction(menu.addAction("Manage Plugins") <<
    pqSetName("actionManage_Plugins"));
#else
  QAction* action2 = menu.addAction("Manage Plugins");
  action2->setEnabled(false);
  action2->setToolTip(
    "Use BUILD_SHARED:ON while compiling to enable plugin support.");
  action2->setStatusTip(action2->toolTip());
#endif


  menu.addSeparator(); // --------------------------------------------------

  //<addaction name="actionToolsDumpWidgetNames" />
  new pqTestingReaction(menu.addAction("Record Test")
    << pqSetName("actionToolsRecordTest"),
    pqTestingReaction::RECORD);
  new pqTestingReaction(menu.addAction("Play Test")
    << pqSetName("actionToolsPlayTest"),
    pqTestingReaction::PLAYBACK,Qt::QueuedConnection);
  new pqTestingReaction(menu.addAction("Lock View Size")
    << pqSetName("actionTesting_Window_Size"),
    pqTestingReaction::LOCK_VIEW_SIZE);
  new pqTestingReaction(menu.addAction("Lock View Size Custom...")
    << pqSetName("actionTesting_Window_Size_Custom"),
    pqTestingReaction::LOCK_VIEW_SIZE_CUSTOM);
  menu.addSeparator();
  new pqTimerLogReaction(menu.addAction("Timer Log")
    << pqSetName("actionToolsTimerLog"));
  QAction* action = menu.addAction("&Output Window")
    << pqSetName("actionToolsOutputWindow");
  QObject::connect(action, SIGNAL(triggered()),
    pqApplicationCore::instance(),
    SLOT(showOutputWindow()));


  menu.addSeparator(); // --------------------------------------------------

  new pqPythonShellReaction(menu.addAction("Python Shell")
    << pqSetName("actionToolsPythonShell"));

#ifdef PARAVIEW_ENABLE_PYTHON
  menu.addSeparator(); // --------------------------------------------------

  new pqTraceReaction( menu.addAction("Start Trace")
                       << pqSetName("actionToolsStartTrace"), true);
  new pqTraceReaction(menu.addAction("Stop Trace")
                      << pqSetName("actionToolsStartTrace"), false);
#endif
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildViewMenu(QMenu& menu, QMainWindow& mainWindow)
{
  new pqViewMenuManager(&mainWindow, &menu);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(QWidget& widget)
{
  QString objectName = widget.objectName();
  Ui::pqPipelineBrowserContextMenu ui;
  ui.setupUi(&widget);
  // since the UI file tends to change the name of the menu.
  widget.setObjectName(objectName);
  widget.setContextMenuPolicy(Qt::ActionsContextMenu);

  QByteArray signalName=QMetaObject::normalizedSignature("deleteKey()");
  if (widget.metaObject()->indexOfSignal(signalName) != -1)
    {
    // Trigger a delete when the user requests a delete.
    QObject::connect(&widget, SIGNAL(deleteKey()),
      ui.actionPBDelete, SLOT(trigger()), Qt::QueuedConnection);
    }

  // And here the reactions come in handy! Just reuse the reaction used for
  // File | Open.
  new pqLoadDataReaction(ui.actionPBOpen);
  new pqChangePipelineInputReaction(ui.actionPBChangeInput);
  new pqCreateCustomFilterReaction(ui.actionPBCreateCustomFilter);
  new pqIgnoreSourceTimeReaction(ui.actionPBIgnoreTime);
  new pqDeleteReaction(ui.actionPBDelete);
  new pqCopyReaction(ui.actionPBCopy);
  new pqCopyReaction(ui.actionPBPaste, true);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildMacrosMenu
#ifdef PARAVIEW_ENABLE_PYTHON
(QMenu& menu)
#else
(QMenu& )
#endif
{
#ifdef PARAVIEW_ENABLE_PYTHON
  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager = pqPVApplicationCore::instance()->pythonManager();
  if (manager)
    {
    new pqMacroReaction(menu.addAction("Add new macro")
                        << pqSetName("actionMacroCreate"));
    QMenu *editMenu = menu.addMenu("Edit...");
    QMenu *deleteMenu = menu.addMenu("Delete...");
    menu.addSeparator();
    manager->addWidgetForRunMacros(&menu);
    manager->addWidgetForEditMacros(editMenu);
    manager->addWidgetForDeleteMacros(deleteMenu);
    }
#endif
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildHelpMenu(QMenu& menu)
{
  QAction * help = menu.addAction("Help") <<
    pqSetName("actionHelp");
  help->setShortcut(QKeySequence::HelpContents);
  new pqHelpReaction(help);

  new pqAboutDialogReaction(
    menu.addAction("About")
    << pqSetName("actionAbout"));
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildToolbars(QMainWindow& mainWindow)
{
  QToolBar* mainToolBar = new pqMainControlsToolbar(&mainWindow)
    << pqSetName("MainControlsToolbar");
  mainToolBar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, mainToolBar);

  QToolBar* selectionToolbar = new pqSelectionToolbar(&mainWindow)
    << pqSetName("selectionToolbar");
  selectionToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, selectionToolbar);

  QToolBar* vcrToolbar = new pqVCRToolbar(&mainWindow)
    << pqSetName("VCRToolbar");
  vcrToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, vcrToolbar);

  QToolBar* timeToolbar = new pqAnimationTimeToolbar(&mainWindow)
    << pqSetName("currentTimeToolbar");
  timeToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, timeToolbar);

  QToolBar* colorToolbar = new pqColorToolbar(&mainWindow)
    << pqSetName("variableToolbar");
  colorToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, colorToolbar);
  mainWindow.insertToolBarBreak(colorToolbar);

  QToolBar* reprToolbar = new pqRepresentationToolbar(&mainWindow)
    << pqSetName("representationToolbar");
  reprToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, reprToolbar);

  QToolBar* cameraToolbar = new pqCameraToolbar(&mainWindow)
    << pqSetName("cameraToolbar");
  cameraToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, cameraToolbar);

  QToolBar* axesToolbar = new pqAxesToolbar(&mainWindow)
    << pqSetName("axesToolbar");
  axesToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, axesToolbar);

#ifdef PARAVIEW_ENABLE_PYTHON
  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager = qobject_cast<pqPythonManager*>(
    pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
    {
    QToolBar* macrosToolbar = new QToolBar("Macros Toolbars", &mainWindow)
      << pqSetName("MacrosToolbar");
    manager->addWidgetForRunMacros(macrosToolbar);
    mainWindow.addToolBar(Qt::TopToolBarArea, macrosToolbar);
    }
#endif
}
