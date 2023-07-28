// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
#include "myMainWindow.h"
#include "ui_myMainWindow.h"

#include <pqApplicationCore.h>
#include <pqCategoryToolbarsBehavior.h>
#include <pqColorToolbar.h>
#include <pqDeleteReaction.h>
#include <pqHelpReaction.h>
#include <pqLoadDataReaction.h>
#include <pqParaViewBehaviors.h>
#include <pqParaViewMenuBuilders.h>
#include <pqRepresentationToolbar.h>

//-----------------------------------------------------------------------------
class myMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
myMainWindow::myMainWindow()
  : Internals(new pqInternals())
{
  // Setup default GUI layout.
  this->Internals->setupUi(this);

  // Setup the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup color editor
  // Provide access to the color-editor panel for the application and hide it
  pqApplicationCore::instance()->registerManager(
    "COLOR_EDITOR_PANEL", this->Internals->colorMapEditorDock);
  this->Internals->colorMapEditorDock->hide();

  // Create a custom file menu with only Open and close
  QList<QAction*> actionList = this->Internals->menu_File->actions();
  QAction* action = actionList.at(0);
  new pqLoadDataReaction(action);
  QObject::connect(
    actionList.at(1), SIGNAL(triggered()), QApplication::instance(), SLOT(closeAllWindows()));

  // Build the filters menu
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser->contextMenu());

  // Add the ColorToolbar
  QToolBar* colorToolbar = new pqColorToolbar(this);
  colorToolbar->layout()->setSpacing(0);
  this->addToolBar(colorToolbar);

  // Add the Representation Toolbar
  QToolBar* reprToolbar = new pqRepresentationToolbar(this);
  reprToolbar->setObjectName("Representation");
  reprToolbar->layout()->setSpacing(0);
  this->addToolBar(reprToolbar);

  // Enable help from the properties panel.
  // This is not really working as the documentation is not built in this app
  QObject::connect(this->Internals->proxyTabWidget, &pqPropertiesPanel::helpRequested,
    &pqHelpReaction::showProxyHelp);

  // hook delete to pqDeleteReaction.
  QAction* tempDeleteAction = new QAction(this);
  pqDeleteReaction* handler = new pqDeleteReaction(tempDeleteAction);
  handler->connect(this->Internals->proxyTabWidget, SIGNAL(deleteRequested(pqProxy*)),
    SLOT(deleteSource(pqProxy*)));

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(this, this);
}

//-----------------------------------------------------------------------------
myMainWindow::~myMainWindow() = default;
