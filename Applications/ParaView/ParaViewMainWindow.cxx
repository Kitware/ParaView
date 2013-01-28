/*=========================================================================

   Program: ParaView
   Module:    ParaViewMainWindow.cxx

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
#include "vtkPVConfig.h"
#ifndef BUILD_SHARED_LIBS
# ifdef PARAVIEW_ENABLE_PYTHON
#   include "paraviewpythonmodules.h"
# endif
#endif

#include "ParaViewMainWindow.h"
#include "ui_ParaViewMainWindow.h"

#include "pqActiveObjects.h"
#include "pqHelpReaction.h"
#include "pqObjectInspectorWidget.h"
#include "pqOptions.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqPropertiesPanel.h"
#include "vtkProcessModule.h"
#include "vtkPVPlugin.h"

#ifndef BUILD_SHARED_LIBS
# include "pvStaticPluginsInit.h"
#endif


#include "ParaViewDocumentationInitializer.h"

class ParaViewMainWindow::pqInternals : public Ui::pqClientMainWindow
{
};

//-----------------------------------------------------------------------------
ParaViewMainWindow::ParaViewMainWindow()
{
#ifndef BUILD_SHARED_LIBS
#ifdef PARAVIEW_ENABLE_PYTHON
  CMakeLoadAllPythonModules();
#endif
#endif
  // init the ParaView embedded documentation.
  PARAVIEW_DOCUMENTATION_INIT();

  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.
  this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->Internals->animationViewDock->hide();
  this->Internals->statisticsDock->hide();
  this->Internals->selectionInspectorDock->hide();
  this->Internals->comparativePanelDock->hide();
  this->Internals->collaborationPanelDock->hide();
  this->Internals->memoryInspectorDock->hide();
  this->Internals->multiBlockInspectorDock->hide();
  this->tabifyDockWidget(this->Internals->animationViewDock,
    this->Internals->statisticsDock);

  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());

  if(!options->GetUseOldPanels())
    {
    this->removeDockWidget(this->Internals->objectInspectorDock);
    this->removeDockWidget(this->Internals->displayDock);
    delete this->Internals->objectInspectorDock;
    delete this->Internals->displayDock;
    this->Internals->objectInspectorDock = 0;
    this->Internals->displayDock = 0;

    this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->informationDock);
    this->Internals->propertiesDock->show();
    this->Internals->propertiesDock->raise();

    // Enable help from the properties panel.
    QObject::connect(this->Internals->propertiesPanel,
                     SIGNAL(helpRequested(const QString&, const QString&)),
                     this, SLOT(showHelpForProxy(const QString&, const QString&)));
    }
  else
    {
    this->removeDockWidget(this->Internals->propertiesDock);
    delete this->Internals->propertiesDock;
    this->Internals->propertiesDock = 0;

    this->tabifyDockWidget(this->Internals->objectInspectorDock, this->Internals->displayDock);
    this->tabifyDockWidget(this->Internals->objectInspectorDock, this->Internals->informationDock);
    this->Internals->objectInspectorDock->raise();

    // Enable help from the object inspector.
    QObject::connect(this->Internals->objectInspector,
      SIGNAL(helpRequested(const QString&, const QString&)),
      this, SLOT(showHelpForProxy(const QString&, const QString&)));
    }

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(
    *this->Internals->pipelineBrowser);

  pqParaViewMenuBuilders::buildToolbars(*this);

  // Setup the View menu. This must be setup after all toolbars and dockwidgets
  // have been created.
  pqParaViewMenuBuilders::buildViewMenu(*this->Internals->menu_View, *this);

  // Setup the menu to show macros.
  pqParaViewMenuBuilders::buildMacrosMenu(*this->Internals->menu_Macros);

  // Setup the help menu.
  pqParaViewMenuBuilders::buildHelpMenu(*this->Internals->menu_Help);

  // Final step, define application behaviors. Since we want all ParaView
  // behaviors, we use this convenience method.
  new pqParaViewBehaviors(this, this);

  // load static plugins
#ifndef BUILD_SHARED_LIBS
  paraview_static_plugins_init();
#endif
}

//-----------------------------------------------------------------------------
ParaViewMainWindow::~ParaViewMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showHelpForProxy(const QString& groupname, const
  QString& proxyname)
{
  pqHelpReaction::showProxyHelp(groupname, proxyname);
}
