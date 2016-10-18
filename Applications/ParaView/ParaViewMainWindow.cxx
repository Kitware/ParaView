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
#ifdef PARAVIEW_ENABLE_PYTHON
extern "C" {
void vtkPVInitializePythonModules();
}
#endif

#include "ParaViewMainWindow.h"
#include "ui_ParaViewMainWindow.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqDeleteReaction.h"
#ifdef PARAVIEW_USE_QTHELP
#include "pqHelpReaction.h"
#endif
#include "pqLoadDataReaction.h"
#include "pqOptions.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqWelcomeDialog.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVPlugin.h"
#include "vtkProcessModule.h"
#include "vtkSMSettings.h"

#ifndef BUILD_SHARED_LIBS
#include "pvStaticPluginsInit.h"
#endif

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

#ifdef PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
#include "ParaViewDocumentationInitializer.h"
#endif

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonDebugLeaksView.h"
typedef pqPythonDebugLeaksView DebugLeaksViewType;
#else
#include "vtkQtDebugLeaksView.h"
typedef vtkQtDebugLeaksView DebugLeaksViewType;
#endif

class ParaViewMainWindow::pqInternals : public Ui::pqClientMainWindow
{
public:
  bool FirstShow;
  pqInternals()
    : FirstShow(true)
  {
  }
};

//-----------------------------------------------------------------------------
ParaViewMainWindow::ParaViewMainWindow()
{
  // the debug leaks view should be constructed as early as possible
  // so that it can monitor vtk objects created at application startup.
  if (getenv("PV_DEBUG_LEAKS_VIEW"))
  {
    vtkQtDebugLeaksView* leaksView = new DebugLeaksViewType(this);
    leaksView->setWindowFlags(Qt::Window);
    leaksView->show();
  }

#ifdef PARAVIEW_ENABLE_PYTHON
  vtkPVInitializePythonModules();
#endif

#ifdef PARAVIEW_ENABLE_EMBEDDED_DOCUMENTATION
  // init the ParaView embedded documentation.
  PARAVIEW_DOCUMENTATION_INIT();
#endif

  this->Internals = new pqInternals();
  this->Internals->setupUi(this);

  // Setup default GUI layout.
  this->setTabPosition(Qt::LeftDockWidgetArea, QTabWidget::North);

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  this->tabifyDockWidget(this->Internals->colorMapEditorDock, this->Internals->memoryInspectorDock);
  this->tabifyDockWidget(this->Internals->colorMapEditorDock, this->Internals->timeInspectorDock);
  this->tabifyDockWidget(
    this->Internals->colorMapEditorDock, this->Internals->comparativePanelDock);
  this->tabifyDockWidget(
    this->Internals->colorMapEditorDock, this->Internals->collaborationPanelDock);

  this->Internals->selectionDisplayDock->hide();
  this->Internals->animationViewDock->hide();
  this->Internals->statisticsDock->hide();
  this->Internals->comparativePanelDock->hide();
  this->Internals->collaborationPanelDock->hide();
  this->Internals->memoryInspectorDock->hide();
  this->Internals->multiBlockInspectorDock->hide();
  this->Internals->colorMapEditorDock->hide();
  this->Internals->timeInspectorDock->hide();

  this->tabifyDockWidget(this->Internals->animationViewDock, this->Internals->statisticsDock);

  // setup properties dock
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->viewPropertiesDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->displayPropertiesDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->informationDock);

  vtkSMSettings* settings = vtkSMSettings::GetInstance();

  int propertiesPanelMode = settings->GetSettingAsInt(
    ".settings.GeneralSettings.PropertiesPanelMode", vtkPVGeneralSettings::ALL_IN_ONE);
  switch (propertiesPanelMode)
  {
    case vtkPVGeneralSettings::SEPARATE_DISPLAY_PROPERTIES:
      delete this->Internals->viewPropertiesPanel;
      delete this->Internals->viewPropertiesDock;
      this->Internals->viewPropertiesPanel = NULL;
      this->Internals->viewPropertiesDock = NULL;

      this->Internals->propertiesPanel->setPanelMode(
        pqPropertiesPanel::SOURCE_PROPERTIES | pqPropertiesPanel::VIEW_PROPERTIES);
      break;

    case vtkPVGeneralSettings::SEPARATE_VIEW_PROPERTIES:
      delete this->Internals->displayPropertiesPanel;
      delete this->Internals->displayPropertiesDock;
      this->Internals->displayPropertiesPanel = NULL;
      this->Internals->displayPropertiesDock = NULL;

      this->Internals->propertiesPanel->setPanelMode(
        pqPropertiesPanel::SOURCE_PROPERTIES | pqPropertiesPanel::DISPLAY_PROPERTIES);
      break;

    case vtkPVGeneralSettings::ALL_SEPARATE:
      this->Internals->propertiesPanel->setPanelMode(pqPropertiesPanel::SOURCE_PROPERTIES);
      break;

    case vtkPVGeneralSettings::ALL_IN_ONE:
    default:
      delete this->Internals->viewPropertiesPanel;
      delete this->Internals->viewPropertiesDock;
      this->Internals->viewPropertiesPanel = NULL;
      this->Internals->viewPropertiesDock = NULL;

      delete this->Internals->displayPropertiesPanel;
      delete this->Internals->displayPropertiesDock;
      this->Internals->displayPropertiesPanel = NULL;
      this->Internals->displayPropertiesDock = NULL;
      break;
  }

  this->Internals->propertiesDock->show();
  this->Internals->propertiesDock->raise();

  // Enable help from the properties panel.
  QObject::connect(this->Internals->propertiesPanel,
    SIGNAL(helpRequested(const QString&, const QString&)), this,
    SLOT(showHelpForProxy(const QString&, const QString&)));

  /// hook delete to pqDeleteReaction.
  QAction* tempDeleteAction = new QAction(this);
  pqDeleteReaction* handler = new pqDeleteReaction(tempDeleteAction);
  handler->connect(this->Internals->propertiesPanel, SIGNAL(deleteRequested(pqPipelineSource*)),
    SLOT(deleteSource(pqPipelineSource*)));

  // setup color editor
  /// Provide access to the color-editor panel for the application.
  pqApplicationCore::instance()->registerManager(
    "COLOR_EDITOR_PANEL", this->Internals->colorMapEditorDock);

  // Populate application menus with actions.
  pqParaViewMenuBuilders::buildFileMenu(*this->Internals->menu_File);
  pqParaViewMenuBuilders::buildEditMenu(*this->Internals->menu_Edit);

  // Populate sources menu.
  pqParaViewMenuBuilders::buildSourcesMenu(*this->Internals->menuSources, this);

  // Populate filters menu.
  pqParaViewMenuBuilders::buildFiltersMenu(*this->Internals->menuFilters, this);

  // Populate Tools menu.
  pqParaViewMenuBuilders::buildToolsMenu(*this->Internals->menuTools);

  // Populate Catalyst menu.
  pqParaViewMenuBuilders::buildCatalystMenu(*this->Internals->menu_Catalyst);

  // setup the context menu for the pipeline browser.
  pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(*this->Internals->pipelineBrowser);

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
}

//-----------------------------------------------------------------------------
ParaViewMainWindow::~ParaViewMainWindow()
{
  delete this->Internals;
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showHelpForProxy(const QString& groupname, const QString& proxyname)
{
#ifdef PARAVIEW_USE_QTHELP
  pqHelpReaction::showProxyHelp(groupname, proxyname);
#endif
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::dragEnterEvent(QDragEnterEvent* evt)
{
  evt->acceptProposedAction();
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::dropEvent(QDropEvent* evt)
{
  QList<QUrl> urls = evt->mimeData()->urls();
  if (urls.isEmpty())
  {
    return;
  }

  QList<QString> files;

  foreach (QUrl url, urls)
  {
    if (!url.toLocalFile().isEmpty())
    {
      files.append(url.toLocalFile());
    }
  }

  // If we have no file we return
  if (files.empty() || files.first().isEmpty())
  {
    return;
  }
  pqLoadDataReaction::loadData(files);
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showEvent(QShowEvent* evt)
{
  this->Superclass::showEvent(evt);
  if (this->Internals->FirstShow)
  {
    this->Internals->FirstShow = false;
    pqApplicationCore* core = pqApplicationCore::instance();
    if (!core->getOptions()->GetDisableRegistry())
    {
      if (core->settings()->value("GeneralSettings.ShowWelcomeDialog", true).toBool())
      {
        pqTimer::singleShot(1000, this, SLOT(showWelcomeDialog()));
      }
    }
  }
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showWelcomeDialog()
{
  pqWelcomeDialog dialog(this);
  dialog.exec();
}
