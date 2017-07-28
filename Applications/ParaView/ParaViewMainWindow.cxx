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
#include "pqCoreUtilities.h"
#include "pqDeleteReaction.h"
#include "pqLoadDataReaction.h"
#include "pqOptions.h"
#include "pqParaViewBehaviors.h"
#include "pqParaViewMenuBuilders.h"
#include "pqSaveStateReaction.h"
#include "pqSettings.h"
#include "pqTimer.h"
#include "pqWelcomeDialog.h"
#include "vtkCommand.h"
#include "vtkPVGeneralSettings.h"
#include "vtkPVPlugin.h"
#include "vtkProcessModule.h"
#include "vtkSMSettings.h"

#ifndef BUILD_SHARED_LIBS
#include "pvStaticPluginsInit.h"
#endif

#ifdef PARAVIEW_USE_QTHELP
#include "pqHelpReaction.h"
#endif

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMessageBox>
#include <QMimeData>
#include <QTextCodec>
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
  int CurrentGUIFontSize;
  pqTimer UpdateFontSizeTimer;
  pqInternals()
    : FirstShow(true)
    , CurrentGUIFontSize(0)
  {
    this->UpdateFontSizeTimer.setInterval(0);
    this->UpdateFontSizeTimer.setSingleShot(true);
  }
};

//-----------------------------------------------------------------------------
ParaViewMainWindow::ParaViewMainWindow()
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  // force the use of UTF-8 text codec (Latin1 is default with Qt 4.8)
  // this is important for support of accentuated paths.
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
#endif

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
  this->Internals->outputWidgetDock->hide();

  // show output widget if we received an error message.
  this->connect(this->Internals->outputWidget, SIGNAL(messageDisplayed(const QString&, int)),
    SLOT(handleMessage(const QString&, int)));

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
  this->tabifyDockWidget(this->Internals->animationViewDock, this->Internals->outputWidgetDock);

  // setup properties dock
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->viewPropertiesDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->displayPropertiesDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->informationDock);
  this->tabifyDockWidget(this->Internals->propertiesDock, this->Internals->multiBlockInspectorDock);

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

  // update UI when font size changes.
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  pqCoreUtilities::connect(
    gsSettings, vtkCommand::ModifiedEvent, &this->Internals->UpdateFontSizeTimer, SLOT(start()));
  this->connect(&this->Internals->UpdateFontSizeTimer, SIGNAL(timeout()), SLOT(updateFontSize()));

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

      this->updateFontSize();
    }
  }
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::closeEvent(QCloseEvent* evt)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->settings()->value("GeneralSettings.ShowSaveStateOnExit", false).toBool())
  {
    if (QMessageBox::question(this, "Exit ParaView?",
          "Do you want to save the state before exiting ParaView?",
          QMessageBox::Save | QMessageBox::Discard) == QMessageBox::Save)
    {
      pqSaveStateReaction::saveState();
    }
  }
  evt->accept();
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::showWelcomeDialog()
{
  pqWelcomeDialog dialog(this);
  dialog.exec();
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::updateFontSize()
{
  vtkPVGeneralSettings* gsSettings = vtkPVGeneralSettings::GetInstance();
  bool overrideFontSize = gsSettings->GetGUIOverrideFont();
  int fontSize = overrideFontSize ? gsSettings->GetGUIFontSize() : 0;
  if (this->Internals->CurrentGUIFontSize != fontSize)
  {
    if (fontSize > 0) // note vtkPVGeneralSettings clamps it to [8, INT_MAX)
    {
      this->setStyleSheet(QString("* { font-size: %1pt }").arg(fontSize));
    }
    else
    {
      this->setStyleSheet(QString());
    }
    this->Internals->CurrentGUIFontSize = fontSize;
  }
}

//-----------------------------------------------------------------------------
void ParaViewMainWindow::handleMessage(const QString&, int type)
{
  QDockWidget* dock = this->Internals->outputWidgetDock;
  if (!dock->isVisible() && (type == pqOutputWidget::ERROR || type == pqOutputWidget::WARNING))
  {
    // if dock is not visible, we always pop it up as a floating dialog. This
    // avoids causing re-renders which may cause more errors and more confusion.
    QRect rectApp = this->geometry();

    QRect rectDock(
      QPoint(0, 0), QSize(static_cast<int>(rectApp.width() * 0.4), dock->sizeHint().height()));
    rectDock.moveCenter(
      QPoint(rectApp.center().x(), rectApp.bottom() - dock->sizeHint().height() / 2));
    dock->setFloating(true);
    dock->setGeometry(rectDock);
    dock->show();
  }
  if (dock->isVisible())
  {
    dock->raise();
  }
}
