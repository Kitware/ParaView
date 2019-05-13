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

#include "ui_pqEditMenuBuilder.h"
#include "ui_pqFileMenuBuilder.h"

#include "pqAboutDialogReaction.h"
#include "pqAnimationTimeToolbar.h"
#include "pqApplicationCore.h"
#include "pqApplicationSettingsReaction.h"
#include "pqApplyPropertiesReaction.h"
#include "pqAxesToolbar.h"
#include "pqCameraLinkReaction.h"
#include "pqCameraToolbar.h"
#include "pqCameraUndoRedoReaction.h"
#include "pqCatalystConnectReaction.h"
#include "pqCatalystContinueReaction.h"
#include "pqCatalystExportReaction.h"
#include "pqCatalystPauseSimulationReaction.h"
#include "pqCatalystRemoveBreakpointReaction.h"
#include "pqCatalystScriptGeneratorReaction.h"
#include "pqCatalystSetBreakpointReaction.h"
#include "pqCategoryToolbarsBehavior.h"
#include "pqChangePipelineInputReaction.h"
#include "pqColorToolbar.h"
#include "pqCopyReaction.h"
#include "pqCreateCustomFilterReaction.h"
#include "pqCustomViewpointsToolbar.h"
#include "pqCustomizeShortcutsReaction.h"
#include "pqDataQueryReaction.h"
#include "pqDeleteReaction.h"
#include "pqDesktopServicesReaction.h"
#include "pqExampleVisualizationsDialogReaction.h"
#include "pqExportReaction.h"
#include "pqFiltersMenuReaction.h"
#ifdef PARAVIEW_USE_QTHELP
#include "pqHelpReaction.h"
#endif
#include "pqIgnoreSourceTimeReaction.h"
#include "pqImmediateExportReaction.h"
#include "pqImportCinemaReaction.h"
#include "pqLinkSelectionReaction.h"
#include "pqLoadDataReaction.h"
#include "pqLoadMaterialsReaction.h"
#include "pqLoadRestoreWindowLayoutReaction.h"
#include "pqLoadStateReaction.h"
#include "pqMainControlsToolbar.h"
#include "pqManageCustomFiltersReaction.h"
#include "pqManageFavoritesReaction.h"
#include "pqManageLinksReaction.h"
#include "pqManagePluginsReaction.h"
#include "pqPVApplicationCore.h"
#include "pqPropertiesPanel.h"
#include "pqProxyGroupMenuManager.h"
#include "pqRecentFilesMenu.h"
#include "pqReloadFilesReaction.h"
#include "pqRepresentationToolbar.h"
#include "pqResetDefaultSettingsReaction.h"
#include "pqSaveAnimationGeometryReaction.h"
#include "pqSaveAnimationReaction.h"
#include "pqSaveDataReaction.h"
#include "pqSaveScreenshotReaction.h"
#include "pqSaveStateReaction.h"
#include "pqServerConnectReaction.h"
#include "pqServerDisconnectReaction.h"
#include "pqSetMainWindowTitleReaction.h"
#include "pqSetName.h"
#include "pqShowHideAllReaction.h"
#include "pqSourcesMenuReaction.h"
#include "pqTemporalExportReaction.h"
#include "pqTestingReaction.h"
#include "pqTimerLogReaction.h"
#include "pqUndoRedoReaction.h"
#include "pqVCRToolbar.h"
#include "pqViewMenuManager.h"

#if VTK_MODULE_ENABLE_ParaView_pqPython
#include "pqMacroReaction.h"
#include "pqPythonManager.h"
#include "pqSGWritersMenuManager.h"
#include "pqTraceReaction.h"
#endif

#include <QApplication>
#include <QDockWidget>
#include <QFileInfo>
#include <QKeySequence>
#include <QLayout>
#include <QMainWindow>
#include <QMenu>

#include "vtkPVFileInformation.h"
#include "vtkSMProxyManager.h"

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildFileMenu(QMenu& menu)
{
  QString objectName = menu.objectName();
  Ui::pqFileMenuBuilder ui;
  ui.setupUi(&menu);
  // since the UI file tends to change the name of the menu.
  menu.setObjectName(objectName);

  QObject::connect(
    ui.actionFileExit, SIGNAL(triggered()), QApplication::instance(), SLOT(closeAllWindows()));

  // now setup reactions.
  new pqLoadDataReaction(ui.actionFileOpen);
  new pqImportCinemaReaction(ui.actionFileImportCinemaDatabase);
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  new pqLoadMaterialsReaction(ui.actionFileLoadMaterials);
#else
  delete ui.actionFileLoadMaterials;
#endif
  new pqRecentFilesMenu(*ui.menuRecentFiles, ui.menuRecentFiles);
  new pqReloadFilesReaction(ui.actionReloadFiles);

  new pqLoadStateReaction(ui.actionFileLoadServerState);
  new pqSaveStateReaction(ui.actionFileSaveServerState);

  new pqServerConnectReaction(ui.actionServerConnect);
  new pqServerDisconnectReaction(ui.actionServerDisconnect);

  new pqSaveScreenshotReaction(ui.actionFileSaveScreenshot);
  new pqSaveAnimationReaction(ui.actionFileSaveAnimation);
  new pqSaveAnimationGeometryReaction(ui.actionFileSaveGeometry);

  new pqExportReaction(ui.actionExport);
  new pqImmediateExportReaction(ui.actionExportImmediate);
  new pqSaveDataReaction(ui.actionFileSaveData);

  new pqLoadRestoreWindowLayoutReaction(true, ui.actionFileLoadWindowArrangement);
  new pqLoadRestoreWindowLayoutReaction(false, ui.actionFileSaveWindowArrangement);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildEditMenu(QMenu& menu, pqPropertiesPanel* propertiesPanel)
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
  ui.actionDelete->setShortcut(QKeySequence(Qt::ALT + Qt::Key_D));
  new pqDeleteReaction(ui.actionDelete_All, true);
  new pqShowHideAllReaction(ui.actionShow_All, pqShowHideAllReaction::ActionType::Show);
  new pqShowHideAllReaction(ui.actionHide_All, pqShowHideAllReaction::ActionType::Hide);
  new pqSaveScreenshotReaction(ui.actionCopyScreenshotToClipboard, true);
  new pqCopyReaction(ui.actionCopy);
  new pqCopyReaction(ui.actionPaste, true);
  new pqApplicationSettingsReaction(ui.actionEditSettings);
  new pqDataQueryReaction(ui.actionQuery);
  new pqResetDefaultSettingsReaction(ui.actionResetDefaultSettings);
  new pqSetMainWindowTitleReaction(ui.actionSetMainWindowTitle);

  if (propertiesPanel)
  {
    QAction* applyAction = new QAction(QIcon(":/pqWidgets/Icons/pqUpdate16.png"), "Apply", &menu);
    applyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_A));
    QAction* resetAction = new QAction(QIcon(":/pqWidgets/Icons/pqCancel16.png"), "Reset", &menu);
    resetAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_R));
    menu.insertAction(ui.actionDelete, applyAction);
    menu.insertAction(ui.actionDelete, resetAction);
    new pqApplyPropertiesReaction(propertiesPanel, applyAction, true);
    new pqApplyPropertiesReaction(propertiesPanel, resetAction, false);
  }
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildSourcesMenu(QMenu& menu, QMainWindow* mainWindow)
{
  pqProxyGroupMenuManager* mgr = new pqProxyGroupMenuManager(&menu, "ParaViewSources");
  mgr->addProxyDefinitionUpdateListener("sources");
  mgr->setRecentlyUsedMenuSize(10);
  new pqSourcesMenuReaction(mgr);
  if (mainWindow)
  {
    // create toolbars for categories as needed.
    new pqCategoryToolbarsBehavior(mgr, mainWindow);
  }
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildFiltersMenu(
  QMenu& menu, QMainWindow* mainWindow, bool hideDisabled, bool quickLaunchable)
{
  pqProxyGroupMenuManager* mgr =
    new pqProxyGroupMenuManager(&menu, "ParaViewFilters", quickLaunchable);
  mgr->addProxyDefinitionUpdateListener("filters");
  mgr->setRecentlyUsedMenuSize(10);
  mgr->setEnableFavorites(true);
  pqFiltersMenuReaction* menuReaction = new pqFiltersMenuReaction(mgr, hideDisabled);

  // Connect the filters menu about to show and the quick-launch dialog about to show
  // to update the enabled/disabled state of the menu items via the
  // pqFiltersMenuReaction
  menuReaction->connect(&menu, SIGNAL(aboutToShow()), SLOT(updateEnableState()));

  auto* pvappcore = pqPVApplicationCore::instance();
  if (quickLaunchable && pvappcore)
  {
    menuReaction->connect(pvappcore, SIGNAL(aboutToShowQuickLaunch()), SLOT(updateEnableState()));
  }

  if (mainWindow)
  {
    // create toolbars for categories as needed.
    new pqCategoryToolbarsBehavior(mgr, mainWindow);
  }
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildToolsMenu(QMenu& menu)
{
  new pqCreateCustomFilterReaction(
    menu.addAction("Create Custom Filter...") << pqSetName("actionToolsCreateCustomFilter"));
  new pqCameraLinkReaction(
    menu.addAction("Add Camera Link...") << pqSetName("actionToolsAddCameraLink"));
  new pqLinkSelectionReaction(
    menu.addAction("Link with Selection") << pqSetName("actionToolsLinkSelection"));
  menu.addSeparator();
  new pqManageCustomFiltersReaction(
    menu.addAction("Manage Custom Filters...") << pqSetName("actionToolsManageCustomFilters"));
  new pqManageLinksReaction(
    menu.addAction("Manage Links...") << pqSetName("actionToolsManageLinks"));
  //<addaction name="actionToolsAddCameraLink" />
  // Add support for importing plugins only if ParaView was built shared.
  new pqManagePluginsReaction(
    menu.addAction("Manage Plugins...") << pqSetName("actionManage_Plugins"));

  QMenu* dummyMenu = new QMenu();
  pqProxyGroupMenuManager* mgr = new pqProxyGroupMenuManager(dummyMenu, "ParaViewFilters", false);
  mgr->addProxyDefinitionUpdateListener("filters");

  QAction* manageFavoritesAction = menu.addAction("Manage Favorites...")
    << pqSetName("actionManage_Favorites");
  new pqManageFavoritesReaction(manageFavoritesAction, mgr);

  new pqCustomizeShortcutsReaction(
    menu.addAction("Customize Shortcuts...") << pqSetName("actionCustomize"));

  menu.addSeparator(); // --------------------------------------------------

  //<addaction name="actionToolsDumpWidgetNames" />
  new pqTestingReaction(menu.addAction("Record Test...") << pqSetName("actionToolsRecordTest"),
    pqTestingReaction::RECORD);
  new pqTestingReaction(menu.addAction("Play Test...") << pqSetName("actionToolsPlayTest"),
    pqTestingReaction::PLAYBACK, Qt::QueuedConnection);
  new pqTestingReaction(menu.addAction("Lock View Size") << pqSetName("actionTesting_Window_Size"),
    pqTestingReaction::LOCK_VIEW_SIZE);
  new pqTestingReaction(
    menu.addAction("Lock View Size Custom...") << pqSetName("actionTesting_Window_Size_Custom"),
    pqTestingReaction::LOCK_VIEW_SIZE_CUSTOM);
  menu.addSeparator();
  new pqTimerLogReaction(menu.addAction("Timer Log") << pqSetName("actionToolsTimerLog"));
  menu.addSeparator(); // --------------------------------------------------

#if VTK_MODULE_ENABLE_ParaView_pqPython
  menu.addSeparator(); // --------------------------------------------------
  new pqTraceReaction(menu.addAction("Start Trace") << pqSetName("actionToolsStartStopTrace"),
    "Start Trace", "Stop Trace");
#endif
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildViewMenu(QMenu& menu, QMainWindow& mainWindow)
{
  new pqViewMenuManager(&mainWindow, &menu);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(QMenu& menu)
{
  // Build the context menu manually so we can insert submenus where needed.
  QAction* actionPBOpen = new QAction(menu.parent());
  actionPBOpen->setObjectName(QStringLiteral("actionPBOpen"));
  QIcon icon4;
  icon4.addFile(
    QStringLiteral(":/pqWidgets/Icons/pqOpen24.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBOpen->setIcon(icon4);
  actionPBOpen->setShortcutContext(Qt::WidgetShortcut);
  actionPBOpen->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Open", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
  actionPBOpen->setToolTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Open", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
  actionPBOpen->setStatusTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Open", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBShowAll = new QAction(menu.parent());
  actionPBShowAll->setObjectName(QStringLiteral("actionPBShowAll"));
  QIcon showAllIcon;
  showAllIcon.addFile(
    QStringLiteral(":/pqWidgets/Icons/pqEyeball.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBShowAll->setIcon(showAllIcon);
  actionPBShowAll->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Show All", Q_NULLPTR));
#ifndef QT_NO_STATUSTIP
  actionPBShowAll->setStatusTip(QApplication::translate(
    "pqPipelineBrowserContextMenu", "Shoo all source outputs in the pipeline", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBHideAll = new QAction(menu.parent());
  actionPBHideAll->setObjectName(QStringLiteral("actionPBHideAll"));
  QIcon hideAllIcon;
  hideAllIcon.addFile(
    QStringLiteral(":/pqWidgets/Icons/pqEyeballClosed.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBHideAll->setIcon(hideAllIcon);
  actionPBHideAll->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Hide All", Q_NULLPTR));
#ifndef QT_NO_STATUSTIP
  actionPBHideAll->setStatusTip(QApplication::translate(
    "pqPipelineBrowserContextMenu", "Hide all source outputs in the pipeline", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBCopy = new QAction(menu.parent());
  actionPBCopy->setObjectName(QStringLiteral("actionPBCopy"));
  QIcon icon2;
  icon2.addFile(
    QStringLiteral(":/pqWidgets/Icons/pqCopy22.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBCopy->setIcon(icon2);
  actionPBCopy->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Copy", Q_NULLPTR));
#ifndef QT_NO_STATUSTIP
  actionPBCopy->setStatusTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Copy Properties", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBPaste = new QAction(menu.parent());
  actionPBPaste->setObjectName(QStringLiteral("actionPBPaste"));
  QIcon icon3;
  icon3.addFile(
    QStringLiteral(":/pqWidgets/Icons/pqPaste22.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBPaste->setIcon(icon3);
  actionPBPaste->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Paste", Q_NULLPTR));
#ifndef QT_NO_STATUSTIP
  actionPBPaste->setStatusTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Paste Properties", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBChangeInput = new QAction(menu.parent());
  actionPBChangeInput->setObjectName(QStringLiteral("actionPBChangeInput"));
  actionPBChangeInput->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Change &Input...", Q_NULLPTR));
  actionPBChangeInput->setIconText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Change Input...", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
  actionPBChangeInput->setToolTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Change a Filter's Input", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
  actionPBChangeInput->setStatusTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Change a Filter's Input", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBReloadFiles = new QAction(menu.parent());
  actionPBReloadFiles->setObjectName(QStringLiteral("actionPBReloadFiles"));
  actionPBReloadFiles->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Reload Files", Q_NULLPTR));
  actionPBReloadFiles->setIconText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Reload Files", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
  actionPBReloadFiles->setToolTip(QApplication::translate("pqPipelineBrowserContextMenu",
    "Reload data files in case they were changed externally.", Q_NULLPTR));
#endif // QT_NO_TOOLTIP

  QAction* actionPBIgnoreTime = new QAction(menu.parent());
  actionPBIgnoreTime->setObjectName(QStringLiteral("actionPBIgnoreTime"));
  actionPBIgnoreTime->setCheckable(true);
  actionPBIgnoreTime->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Ignore Time", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
  actionPBIgnoreTime->setToolTip(QApplication::translate("pqPipelineBrowserContextMenu",
    "Disregard this source/filter's time from animations", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
  actionPBIgnoreTime->setStatusTip(QApplication::translate("pqPipelineBrowserContextMenu",
    "Disregard this source/filter's time from animations", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QAction* actionPBDelete = new QAction(menu.parent());
  actionPBDelete->setObjectName(QStringLiteral("actionPBDelete"));
  QIcon icon;
  icon.addFile(
    QStringLiteral(":/QtWidgets/Icons/pqDelete24.png"), QSize(), QIcon::Normal, QIcon::Off);
  actionPBDelete->setIcon(icon);
  actionPBDelete->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Delete", Q_NULLPTR));
#ifndef QT_NO_STATUSTIP
  actionPBDelete->setStatusTip(
    QApplication::translate("pqPipelineBrowserContextMenu", "Delete", Q_NULLPTR));
#endif // QT_NO_STATUSTIP

  QByteArray signalName = QMetaObject::normalizedSignature("deleteKey()");
  if (menu.parent()->metaObject()->indexOfSignal(signalName) != -1)
  {
    // Trigger a delete when the user requests a delete.
    QObject::connect(
      menu.parent(), SIGNAL(deleteKey()), actionPBDelete, SLOT(trigger()), Qt::QueuedConnection);
  }

  QAction* actionPBCreateCustomFilter = new QAction(menu.parent());
  actionPBCreateCustomFilter->setObjectName(QStringLiteral("actionPBCreateCustomFilter"));
  actionPBCreateCustomFilter->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "&Create Custom Filter...", Q_NULLPTR));

  QAction* actionPBLinkSelection = new QAction(menu.parent());
  actionPBLinkSelection->setObjectName(QStringLiteral("actionPBLinkSelection"));
  actionPBLinkSelection->setText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Link with selection", Q_NULLPTR));
  actionPBLinkSelection->setIconText(
    QApplication::translate("pqPipelineBrowserContextMenu", "Link with selection", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
  actionPBLinkSelection->setToolTip(QApplication::translate("pqPipelineBrowserContextMenu",
    "Link this source and current selected source as a selection link", Q_NULLPTR));
#endif // QT_NO_TOOLTIP

  menu.addAction(actionPBOpen);
  menu.addAction(actionPBShowAll);
  menu.addAction(actionPBHideAll);
  menu.addAction(actionPBCopy);
  menu.addAction(actionPBPaste);
  menu.addAction(actionPBChangeInput);
  QMenu* addFilterMenu = menu.addMenu("Add Filter");
  pqParaViewMenuBuilders::buildFiltersMenu(
    *addFilterMenu, nullptr, true /*hide disabled*/, false /*quickLaunchable*/);
  menu.addAction(actionPBReloadFiles);
  menu.addAction(actionPBIgnoreTime);
  menu.addAction(actionPBDelete);
  menu.addAction(actionPBCreateCustomFilter);
  menu.addAction(actionPBLinkSelection);

  // And here the reactions come in handy! Just reuse the reaction used for
  // File | Open.
  new pqLoadDataReaction(actionPBOpen);
  new pqShowHideAllReaction(actionPBShowAll, pqShowHideAllReaction::ActionType::Show);
  new pqShowHideAllReaction(actionPBHideAll, pqShowHideAllReaction::ActionType::Hide);
  new pqCopyReaction(actionPBCopy);
  new pqCopyReaction(actionPBPaste, true);
  new pqChangePipelineInputReaction(actionPBChangeInput);
  new pqReloadFilesReaction(actionPBReloadFiles);
  new pqIgnoreSourceTimeReaction(actionPBIgnoreTime);
  new pqDeleteReaction(actionPBDelete);
  new pqCreateCustomFilterReaction(actionPBCreateCustomFilter);
  new pqLinkSelectionReaction(actionPBLinkSelection);
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildMacrosMenu(QMenu& menu)
{
  Q_UNUSED(menu);
#if VTK_MODULE_ENABLE_ParaView_pqPython
  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager = pqPVApplicationCore::instance()->pythonManager();
  if (manager)
  {
    new pqMacroReaction(menu.addAction("Add new macro...") << pqSetName("actionMacroCreate"));
    QMenu* editMenu = menu.addMenu("Edit...");
    QMenu* deleteMenu = menu.addMenu("Delete...");
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
  QString documentationPath(vtkPVFileInformation::GetParaViewDocDirectory().c_str());
  QString paraViewGettingStartedFile = documentationPath + "/GettingStarted.pdf";

  // Getting Started with ParaView
  new pqDesktopServicesReaction(QUrl::fromLocalFile(paraViewGettingStartedFile),
    (menu.addAction(QIcon(":/pqWidgets/Icons/pdf.png"), "Getting Started with ParaView")
                                  << pqSetName("actionGettingStarted")));

  // ParaView Guide
  QAction* guide = menu.addAction(QIcon(":/pqWidgets/Icons/pdf.png"), "ParaView Guide");
  guide->setObjectName("actionGuide");
  guide->setShortcut(QKeySequence::HelpContents);
  QString guideURL = QString("https://www.paraview.org/paraview-downloads/"
                             "download.php?submit=Download&version=v%1.%2&type=binary&os="
                             "Sources&downloadFile=ParaViewGuide-%1.%2.%3.pdf")
                       .arg(vtkSMProxyManager::GetVersionMajor())
                       .arg(vtkSMProxyManager::GetVersionMinor())
                       .arg(vtkSMProxyManager::GetVersionPatch());
  new pqDesktopServicesReaction(QUrl(guideURL), guide);

#ifdef PARAVIEW_USE_QTHELP
  // Help
  QAction* help = menu.addAction("Reader, Filter, and Writer Reference") << pqSetName("actionHelp");
  new pqHelpReaction(help);
#endif

  // -----------------
  menu.addSeparator();

  // ParaView Tutorial
  QString tutorialURL = QString("https://www.paraview.org/paraview-downloads/"
                                "download.php?submit=Download&version=v%1.%2&type=binary&os="
                                "Sources&downloadFile=ParaViewTutorial-%1.%2.%3.pdf")
                          .arg(vtkSMProxyManager::GetVersionMajor())
                          .arg(vtkSMProxyManager::GetVersionMinor())
                          .arg(vtkSMProxyManager::GetVersionPatch());
  new pqDesktopServicesReaction(
    QUrl(tutorialURL), (menu.addAction(QIcon(":/pqWidgets/Icons/pdf.png"), "ParaView Tutorial")
                         << pqSetName("actionTutorialNotes")));

  // Sandia National Labs Tutorials
  new pqDesktopServicesReaction(QUrl("http://www.paraview.org/Wiki/SNL_ParaView_4_Tutorials"),
    (menu.addAction("Sandia National Labs Tutorials") << pqSetName("actionSNLTutorial")));

  // Example Data Sets

  // Example Visualizations
  new pqExampleVisualizationsDialogReaction(
    menu.addAction("Example Visualizations") << pqSetName("ExampleVisualizations"));

  // -----------------
  menu.addSeparator();

  // ParaView Web Site
  new pqDesktopServicesReaction(QUrl("http://www.paraview.org"),
    (menu.addAction("ParaView Web Site") << pqSetName("actionWebSite")));

  // ParaView Wiki
  new pqDesktopServicesReaction(QUrl("http://www.paraview.org/Wiki/ParaView"),
    (menu.addAction("ParaView Wiki") << pqSetName("actionWiki")));

  // ParaView Community Support
  new pqDesktopServicesReaction(QUrl("http://www.paraview.org/community-support/"),
    (menu.addAction("ParaView Community Support") << pqSetName("actionCommunitySupport")));

  // ParaView Release Notes
  QString versionString(PARAVIEW_VERSION_FULL);
  int indexOfHyphen = versionString.indexOf('-');
  if (indexOfHyphen > -1)
  {
    versionString = versionString.left(indexOfHyphen);
  }
  versionString.replace('.', '-');
  new pqDesktopServicesReaction(
    QUrl("https://blog.kitware.com/paraview-" + versionString + "-release-notes/"),
    (menu.addAction("Release Notes") << pqSetName("actionReleaseNotes")));

  // -----------------
  menu.addSeparator();

  // Professional Support
  new pqDesktopServicesReaction(QUrl("http://www.kitware.com/products/paraviewpro.html"),
    (menu.addAction("Professional Support") << pqSetName("actionProfessionalSupport")));

  // Professional Training
  new pqDesktopServicesReaction(QUrl("http://www.kitware.com/products/protraining.php"),
    (menu.addAction("Professional Training") << pqSetName("actionTraining")));

  // Online Tutorials
  new pqDesktopServicesReaction(QUrl("http://www.paraview.org/tutorials/"),
    (menu.addAction("Online Tutorials") << pqSetName("actionTutorials")));

  // Online Blogs
  new pqDesktopServicesReaction(QUrl("https://blog.kitware.com/tag/ParaView/"),
    (menu.addAction("Online Blogs") << pqSetName("actionBlogs")));

#if !defined(__APPLE__)
  // -----------------
  menu.addSeparator();
#endif

  // About
  new pqAboutDialogReaction(menu.addAction("About...") << pqSetName("actionAbout"));
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildToolbars(QMainWindow& mainWindow)
{
  QToolBar* mainToolBar = new pqMainControlsToolbar(&mainWindow)
    << pqSetName("MainControlsToolbar");
  mainToolBar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, mainToolBar);

  QToolBar* vcrToolbar = new pqVCRToolbar(&mainWindow) << pqSetName("VCRToolbar");
  vcrToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, vcrToolbar);

  QToolBar* timeToolbar = new pqAnimationTimeToolbar(&mainWindow)
    << pqSetName("currentTimeToolbar");
  timeToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, timeToolbar);

  QToolBar* customViewpointsToolbar = new pqCustomViewpointsToolbar(&mainWindow)
    << pqSetName("customViewpointsToolbar");
  customViewpointsToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, customViewpointsToolbar);

  mainWindow.addToolBarBreak();

  QToolBar* colorToolbar = new pqColorToolbar(&mainWindow) << pqSetName("variableToolbar");
  colorToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, colorToolbar);

  QToolBar* reprToolbar = new pqRepresentationToolbar(&mainWindow)
    << pqSetName("representationToolbar");
  reprToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, reprToolbar);

  QToolBar* cameraToolbar = new pqCameraToolbar(&mainWindow) << pqSetName("cameraToolbar");
  cameraToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, cameraToolbar);

  QToolBar* axesToolbar = new pqAxesToolbar(&mainWindow) << pqSetName("axesToolbar");
  axesToolbar->layout()->setSpacing(0);
  mainWindow.addToolBar(Qt::TopToolBarArea, axesToolbar);

#if VTK_MODULE_ENABLE_ParaView_pqPython
  // Give the macros menu to the pqPythonMacroSupervisor
  pqPythonManager* manager =
    qobject_cast<pqPythonManager*>(pqApplicationCore::instance()->manager("PYTHON_MANAGER"));
  if (manager)
  {
    QToolBar* macrosToolbar = new QToolBar("Macros Toolbars", &mainWindow)
      << pqSetName("MacrosToolbar");
    manager->addWidgetForRunMacros(macrosToolbar);
    mainWindow.addToolBar(Qt::TopToolBarArea, macrosToolbar);
  }
#endif
}

//-----------------------------------------------------------------------------
void pqParaViewMenuBuilders::buildCatalystMenu(QMenu& menu, QWidget* exportConfiguration)
{
  new pqCatalystConnectReaction(menu.addAction("Connect...") << pqSetName("actionCatalystConnect"));
  new pqCatalystPauseSimulationReaction(
    menu.addAction("Pause Simulation") << pqSetName("actionCatalystPauseSimulation"));

  new pqCatalystContinueReaction(menu.addAction("Continue") << pqSetName("actionCatalystContinue"));

  new pqCatalystSetBreakpointReaction(
    menu.addAction("Set Breakpoint") << pqSetName("actionCatalystSetBreakpoint"));

  new pqCatalystRemoveBreakpointReaction(
    menu.addAction("Remove Breakpoint") << pqSetName("actionCatalystRemoveBreakpoint"));

#if VTK_MODULE_ENABLE_ParaView_pqPython
#define SHOWNEWCATALYSTGUI 1
#if SHOWNEWCATALYSTGUI
  menu.addSeparator(); // --------------------------------------------------
  // QAction* cexport = menu.addAction("Configure Exports"); //WTH won't this show up on mac?
  // QAction* cexport = menu.addAction("Setup Exports"); //or this on mac?
  QAction* cexport = menu.addAction("Define Exports")
    << pqSetName("actionCatalystConfigure"); // but this is OK?
  QObject::connect(cexport, SIGNAL(triggered()), exportConfiguration, SLOT(show()));

  QAction* gcatalyst = menu.addAction("Export Catalyst Script")
    << pqSetName("actionExportCatalyst");
  new pqCatalystExportReaction(gcatalyst);
  QAction* gimmediate = menu.addAction("Export Now") << pqSetName("actionExportImmediate");
  new pqImmediateExportReaction(gimmediate);
  QAction* gtemporal = menu.addAction("Export Temporal Script")
    << pqSetName("actionExportTemporal");
  new pqTemporalExportReaction(gtemporal);

#else
  (void)exportConfiguration; // avoid unreferenced parameter comp warning
#endif

#define SHOWOLDCATALYSTGUI 1
#if SHOWOLDCATALYSTGUI
  menu.addSeparator(); // --------------------------------------------------
  QAction* csg = menu.addAction("Generate Script -deprecated") << pqSetName("Export State");
  new pqCatalystScriptGeneratorReaction(csg);

  pqSGWritersMenuManager* menuMgr =
    new pqSGWritersMenuManager(&menu, "&Writers", "CatalystWritersMenu", nullptr);
  menuMgr->createMenu();
#endif
#else
  (void)exportConfiguration; // avoid unreferenced parameter comp warning
#endif
}
