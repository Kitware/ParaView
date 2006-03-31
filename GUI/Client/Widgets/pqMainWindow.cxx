/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

=========================================================================*/

#include "pqCompoundProxyWizard.h"
#include "pqElementInspectorWidget.h"
#include "pqMainWindow.h"
#include "pqMultiViewFrame.h"
#include "pqMultiViewManager.h"
#include "pqNameCount.h"
#include "pqObjectInspectorWidget.h"
#include "pqParts.h"
#include "pqPicking.h"
#include "pqPipelineData.h"
#include "pqPipelineListModel.h"
#include "pqPipelineListWidget.h"
#include "pqPipelineObject.h"
#include "pqPipelineServer.h"
#include "pqPipelineWindow.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSMAdaptor.h"
#include "pqSMMultiView.h"
#include "pqSourceProxyInfo.h"
#include "pqVariableSelectorWidget.h"
#include "pqXMLUtil.h"

#include <pqConnect.h>
#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqFileDialog.h>
#include <pqImageComparison.h>
#include <pqLocalFileDialogModel.h>
#include <pqPlayControlsWidget.h>
#include <pqRecordEventsDialog.h>
#include <pqSetData.h>
#include <pqSetName.h>

#ifdef PARAQ_EMBED_PYTHON
#include <pqPythonDialog.h>
#endif // PARAQ_EMBED_PYTHON

#include <vtkEventQtSlotConnect.h>
#include <vtkPVArrayInformation.h>
#include <vtkPVDataInformation.h>
#include <vtkPVDataSetAttributesInformation.h>
#include <vtkPVGenericRenderWindowInteractor.h>
#include <vtkPVGeometryInformation.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkRenderWindow.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMIntRangeDomain.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMMultiViewRenderModuleProxy.h>
#include <vtkSMProxy.h>
#include <vtkSMProxyManager.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSignalMapper>
#include <QToolBar>
#include <QVTKWidget.h>

///////////////////////////////////////////////////////////////////////////
// pqMainWindow::pqImplementation

/// Private implementation details for pqMainWindow
class pqMainWindow::pqImplementation
{
public:
  pqImplementation() :
    FileMenu(0),
    ViewMenu(0),
    ServerMenu(0),
    SourcesMenu(0),
    FiltersMenu(0),
    ToolsMenu(0),
    HelpMenu(0),
    CurrentServer(0),
    CurrentProxy(0),
    PropertyToolbar(0),
    MultiViewManager(0),
    ServerDisconnectAction(0),
    Adaptor(0),
    Pipeline(0),
    PipelineList(0),
    ActiveView(0),
    ElementInspectorDock(0),
    CompoundProxyToolBar(0),
    VariableSelectorToolBar(0),
    ProxyInfo(0),
    VTKConnector(0),
    Inspector(0)
  {
  }
  
  ~pqImplementation()
  {
    // Clean up the model before deleting the adaptor
    delete this->Inspector;
    this->Inspector = 0;
    
    // Clean up the pipeline inspector before the views.
    delete this->PipelineList;
    this->PipelineList = 0;
    
    // clean up multiview before server
    delete this->MultiViewManager;
    this->MultiViewManager = 0;

    delete this->ProxyInfo;
    this->ProxyInfo = 0;

    this->VTKConnector->Delete();
    this->VTKConnector = 0;
  
/** \todo Workaround for shutdown crash */  
//    delete this->CurrentServer;
    this->CurrentServer = 0;
    
    delete this->Adaptor;
    this->Adaptor = 0;
  }

  // Stores standard menus
  QMenu* FileMenu;
  QMenu* ViewMenu;
  QMenu* ServerMenu;
  QMenu* SourcesMenu;
  QMenu* FiltersMenu;
  QMenu* ToolsMenu;
  QMenu* HelpMenu;
  
  /// Stores a mapping from dockable widgets to menu actions
  QMap<QDockWidget*, QAction*> DockWidgetVisibleActions;
  
  pqServer* CurrentServer;
  vtkSMProxy* CurrentProxy;

  QToolBar* PropertyToolbar;
  pqMultiViewManager* MultiViewManager;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  pqPipelineData *Pipeline;
  pqPipelineListWidget *PipelineList;
  pqMultiViewFrame* ActiveView;
  QDockWidget *ElementInspectorDock;
  QToolBar* CompoundProxyToolBar;
  QToolBar* VariableSelectorToolBar;
  pqSourceProxyInfo* ProxyInfo;
  vtkEventQtSlotConnect* VTKConnector;
  pqObjectInspectorWidget* Inspector;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindow

pqMainWindow::pqMainWindow() :
  Implementation(new pqImplementation())
{
  this->setObjectName("mainWindow");
  this->setWindowTitle("ParaQ");

  this->menuBar() << pqSetName("menuBar");

  // Set up the main ParaQ items along with the central widget.
  this->Implementation->Adaptor = new pqSMAdaptor();
  this->Implementation->Pipeline = new pqPipelineData(this);
  this->Implementation->ProxyInfo = new pqSourceProxyInfo();
  this->Implementation->VTKConnector = vtkEventQtSlotConnect::New();

  this->Implementation->MultiViewManager = new pqMultiViewManager(this) << pqSetName("MultiViewManager");
  //this->Implementation->MultiViewManager->hide();  // workaround for flickering in Qt 4.0.1 & 4.1.0
  this->setCentralWidget(this->Implementation->MultiViewManager);
  QObject::connect(this->Implementation->MultiViewManager, SIGNAL(frameAdded(pqMultiViewFrame*)),
      this, SLOT(onNewQVTKWidget(pqMultiViewFrame*)));
  QObject::connect(this->Implementation->MultiViewManager, SIGNAL(frameRemoved(pqMultiViewFrame*)),
      this, SLOT(onDeleteQVTKWidget(pqMultiViewFrame*)));
  //this->Implementation->MultiViewManager->show();  // workaround for flickering in Qt 4.0.1 & 4.1.0

  // Listen for the pipeline's server signals.
  QObject::connect(this->Implementation->Pipeline, SIGNAL(serverAdded(pqPipelineServer *)),
      this, SLOT(onAddServer(pqPipelineServer *)));
  QObject::connect(this->Implementation->Pipeline, SIGNAL(removingServer(pqPipelineServer *)),
      this, SLOT(onRemoveServer(pqPipelineServer *)));
  QObject::connect(this->Implementation->Pipeline, SIGNAL(windowAdded(pqPipelineWindow *)),
      this, SLOT(onAddWindow(pqPipelineWindow *)));

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
}

pqMainWindow::~pqMainWindow()
{
  delete Implementation;
}

void pqMainWindow::createStandardFileMenu()
{
  QMenu* const menu = this->fileMenu();
  
  menu->addAction(tr("&New"))
    << pqSetName("New")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileNew()));

  menu->addAction(tr("&Open..."))
    << pqSetName("Open")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpen()));

  menu->addAction(tr("&Load Server State..."))
    << pqSetName("LoadServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpenServerState()));

  menu->addAction(tr("&Save Server State..."))
    << pqSetName("SaveServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));

  menu->addAction(tr("Save Screenshot..."))
    << pqSetName("SaveScreenshot")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveScreenshot())); 

  menu->addSeparator();
  
  menu->addAction(tr("E&xit"))
    << pqSetName("Exit")
    << pqConnect(SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));
}

void pqMainWindow::createStandardViewMenu()
{
  this->viewMenu();
}

void pqMainWindow::createStandardServerMenu()
{
  QMenu* const menu = this->serverMenu();
  
  menu->addAction(tr("Connect..."))
    << pqSetName("Connect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerConnect()));

  this->Implementation->ServerDisconnectAction = menu->addAction(tr("Disconnect"))
    << pqSetName("Disconnect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerDisconnect()));
    
  this->Implementation->ServerDisconnectAction->setEnabled(false);
}

void pqMainWindow::createStandardSourcesMenu()
{
  QMenu* const menu = this->sourcesMenu();
  menu << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateSource(QAction*)));
  
  /** \todo split this into source and filter-specific slots */
  QObject::connect(this, SIGNAL(serverChanged(pqServer*)), SLOT(onUpdateSourcesFiltersMenu(pqServer*)));
}

void pqMainWindow::createStandardFiltersMenu()
{
  QMenu* const menu = this->filtersMenu();
  menu << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateFilter(QAction*)));
}

void pqMainWindow::createStandardToolsMenu()
{
  QMenu* const menu = this->toolsMenu();
  
  QAction* compoundFilterAction = menu->addAction(tr("&Compound Filters..."))
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenCompoundFilterWizard()))
    << pqSetName("CompoundFilterAction");
  compoundFilterAction->setEnabled(false);

  menu->addAction(tr("&Link Editor..."))
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenLinkEditor()));

  menu->addSeparator();
  menu->addAction(tr("&Record Test"))
    << pqSetName("Record")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTest()));

  menu->addAction(tr("&Play Test"))
    << pqSetName("Play")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPlayTest()));

#ifdef PARAQ_EMBED_PYTHON
  menu->addAction(tr("Python &Shell"))
    << pqSetName("PythonShell")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPythonShell()));
#endif // PARAQ_EMBED_PYTHON
}

void pqMainWindow::createStandardPipelineBrowser(bool visible)
{
  QDockWidget* const pipeline_dock = new QDockWidget("Pipeline Inspector", this);
  pipeline_dock->setObjectName("PipelineDock");
  pipeline_dock->setAllowedAreas(
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);
  pipeline_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  // Make sure the pipeline data instance is created before the
  // pipeline list model. This ensures that the connections will
  // work.
  this->Implementation->PipelineList = new pqPipelineListWidget(pipeline_dock);
  this->Implementation->PipelineList->setObjectName("PipelineList");
  pipeline_dock->setWidget(this->Implementation->PipelineList);

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, pipeline_dock, QIcon(":pqWidgets/pqPipelineList22.png"), visible);

  this->connect(this->Implementation->PipelineList, SIGNAL(proxySelected(vtkSMProxy*)), this, SIGNAL(proxyChanged(vtkSMProxy*)));
  this->connect(this->Implementation->PipelineList, SIGNAL(proxySelected(vtkSMProxy*)), this, SLOT(onProxySelected(vtkSMProxy*)));
}

void pqMainWindow::createStandardObjectInspector(bool visible)
{
  QDockWidget* const object_inspector_dock = new QDockWidget("Object Inspector", this);
  object_inspector_dock->setObjectName("InspectorDock");
  object_inspector_dock->setAllowedAreas(
      Qt::LeftDockWidgetArea |
      Qt::RightDockWidgetArea);
  object_inspector_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  this->Implementation->Inspector = new pqObjectInspectorWidget(object_inspector_dock);
  this->Implementation->Inspector->setObjectName("Inspector");
  object_inspector_dock->setWidget(this->Implementation->Inspector);
  if(this->Implementation->PipelineList)
    {
    connect(this->Implementation->PipelineList, SIGNAL(proxySelected(vtkSMProxy *)),
        this->Implementation->Inspector, SLOT(setProxy(vtkSMProxy *)));
    }

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, object_inspector_dock, QIcon(), visible);
}

void pqMainWindow::createStandardElementInspector(bool visible)
{
  this->Implementation->ElementInspectorDock = new QDockWidget("Element Inspector View", this);
  this->Implementation->ElementInspectorDock->setObjectName("ElementInspectorDock");
  this->Implementation->ElementInspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  this->Implementation->ElementInspectorDock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);

  pqElementInspectorWidget* const element_inspector = new pqElementInspectorWidget(this->Implementation->ElementInspectorDock);
  this->Implementation->ElementInspectorDock->setWidget(element_inspector);

  this->addStandardDockWidget(Qt::BottomDockWidgetArea, this->Implementation->ElementInspectorDock, QIcon(), visible);

}

void pqMainWindow::createStandardVCRToolBar()
{
  QToolBar* const toolbar = new QToolBar(tr("VCR Controls"), this) << pqSetName("VCRControlsToolBar");
  this->addToolBar(Qt::TopToolBarArea, toolbar);
  pqPlayControlsWidget* const vcr_controls = new pqPlayControlsWidget(toolbar);
  toolbar->addWidget(vcr_controls);
  
  this->connect(vcr_controls, SIGNAL(first()), SLOT(onFirstTimeStep()));
  this->connect(vcr_controls, SIGNAL(back()), SLOT(onPreviousTimeStep()));
  this->connect(vcr_controls, SIGNAL(forward()), SLOT(onNextTimeStep()));
  this->connect(vcr_controls, SIGNAL(last()), SLOT(onLastTimeStep()));
}

void pqMainWindow::createStandardVariableToolBar()
{
  this->Implementation->VariableSelectorToolBar = new QToolBar(tr("Variables"), this) << pqSetName("VariableSelectorToolBar");
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->VariableSelectorToolBar);
  pqVariableSelectorWidget* varSelector = new pqVariableSelectorWidget(this->Implementation->VariableSelectorToolBar) << pqSetName("VariableSelector");
  this->Implementation->VariableSelectorToolBar->addWidget(varSelector);

  this->connect(this->Implementation->PipelineList, SIGNAL(proxySelected(vtkSMProxy*)), this, SLOT(onUpdateVariableSelector(vtkSMProxy*)));
  
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), this, SIGNAL(variableChanged(pqVariableType, const QString&)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), this, SLOT(onVariableChanged(pqVariableType, const QString&)));
}

void pqMainWindow::createStandardCompoundProxyToolBar()
{
  this->Implementation->CompoundProxyToolBar = new QToolBar(tr("Compound Proxies"), this) << pqSetName("CompoundProxyToolBar");
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->CompoundProxyToolBar);
  this->connect(this->Implementation->CompoundProxyToolBar, SIGNAL(actionTriggered(QAction*)), SLOT(onCreateCompoundProxy(QAction*)));

  // Work around for file new crash.
  this->Implementation->PipelineList->setFocus();
}

QMenu* pqMainWindow::fileMenu()
{
  if(!this->Implementation->FileMenu)
    {
    this->Implementation->FileMenu = this->menuBar()->addMenu(tr("&File"))
      << pqSetName("fileMenu");
    }
    
  return this->Implementation->FileMenu;
}

QMenu* pqMainWindow::viewMenu()
{
  if(!this->Implementation->ViewMenu)
    {
    this->Implementation->ViewMenu = this->menuBar()->addMenu(tr("&View"))
      << pqSetName("viewMenu");
    }
    
  return this->Implementation->ViewMenu;
}

QMenu* pqMainWindow::serverMenu()
{
  if(!this->Implementation->ServerMenu)
    {
    this->Implementation->ServerMenu = this->menuBar()->addMenu(tr("&Server"))
      << pqSetName("serverMenu");
    }
    
  return this->Implementation->ServerMenu;
}

QMenu* pqMainWindow::sourcesMenu()
{
  if(!this->Implementation->SourcesMenu)
    {
    this->Implementation->SourcesMenu = this->menuBar()->addMenu(tr("&Sources"))
      << pqSetName("sourcesMenu");
    }
    
  return this->Implementation->SourcesMenu;
}

QMenu* pqMainWindow::filtersMenu()
{
  if(!this->Implementation->FiltersMenu)
    {
    this->Implementation->FiltersMenu = this->menuBar()->addMenu(tr("&Filters"))
      << pqSetName("filtersMenu");
    }
    
  return this->Implementation->FiltersMenu;
}

QMenu* pqMainWindow::toolsMenu()
{
  if(!this->Implementation->ToolsMenu)
    {
    this->Implementation->ToolsMenu = this->menuBar()->addMenu(tr("&Tools"))
      << pqSetName("toolsMenu");
    }
    
  return this->Implementation->ToolsMenu;
}

QMenu* pqMainWindow::helpMenu()
{
  if(!this->Implementation->HelpMenu)
    {
    this->Implementation->HelpMenu = this->menuBar()->addMenu(tr("&Help"))
      << pqSetName("helpMenu");
    }
    
  return this->Implementation->HelpMenu;
}

void pqMainWindow::addStandardDockWidget(Qt::DockWidgetArea area, QDockWidget* dockwidget, const QIcon& icon, bool visible)
{
  dockwidget->setVisible(visible);

  this->addDockWidget(area, dockwidget);
  
  QAction* const action = icon.isNull() ?
    this->viewMenu()->addAction(dockwidget->windowTitle()) :
    this->viewMenu()->addAction(icon, dockwidget->windowTitle());
    
  action << pqSetName(dockwidget->windowTitle());
  action->setCheckable(true);
  action->setChecked(visible);
  action << pqConnect(SIGNAL(triggered(bool)), dockwidget, SLOT(setVisible(bool)));
    
  this->Implementation->DockWidgetVisibleActions[dockwidget] = action;
    
  dockwidget->installEventFilter(this);
}

bool pqMainWindow::eventFilter(QObject* watched, QEvent* e)
{
  if(e->type() == QEvent::Hide || e->type() == QEvent::Show)
    {
    if(QDockWidget* const dockwidget = qobject_cast<QDockWidget*>(watched))
      {
      if(this->Implementation->DockWidgetVisibleActions.contains(dockwidget))
        {
        this->Implementation->DockWidgetVisibleActions[dockwidget]->setChecked(e->type() == QEvent::Show);
        }
      }
    }

  return QMainWindow::eventFilter(watched, e);
}

void pqMainWindow::setServer(pqServer* Server)
{
  if(this->Implementation->CurrentServer)
    {
    if(this->Implementation->CompoundProxyToolBar)
      {
      this->Implementation->CompoundProxyToolBar->clear();
      }
    this->Implementation->Pipeline->removeServer(this->Implementation->CurrentServer);
    delete this->Implementation->CurrentServer;
    }

  this->Implementation->CurrentServer = Server;
  if(this->Implementation->CurrentServer)
    {
    // preload compound proxies
    QDir appDir = QCoreApplication::applicationDirPath();
    if(appDir.cd("filters"))
      {
      QStringList files = appDir.entryList(QStringList() += "*.xml", QDir::Files | QDir::Readable);
      pqCompoundProxyWizard* wizard = new pqCompoundProxyWizard(this->Implementation->CurrentServer, this);
      wizard->hide();
      this->connect(wizard, SIGNAL(newCompoundProxy(const QString&, const QString&)),
                            SLOT(onCompoundProxyAdded(const QString&, const QString&)));
      wizard->onLoad(files);
      delete wizard;
      }
    
    this->Implementation->Pipeline->addServer(this->Implementation->CurrentServer);
    this->onNewQVTKWidget(qobject_cast<pqMultiViewFrame *>(
        this->Implementation->MultiViewManager->widgetOfIndex(pqMultiView::Index())));
    }

  emit serverChanged(this->Implementation->CurrentServer);
}

void pqMainWindow::onFileNew()
{
  // Reset the multi-view. Use the removed widget list to clean
  // up the render modules. Then, delete the widgets.
  if(this->Implementation->MultiViewManager)
    {
    QList<QWidget *> removed;
    pqMultiViewFrame *frame = 0;
    this->Implementation->MultiViewManager->reset(removed);
    QList<QWidget *>::Iterator iter = removed.begin();
    for( ; iter != removed.end(); ++iter)
      {
      frame = qobject_cast<pqMultiViewFrame *>(*iter);
      if(frame)
        {
        this->cleanUpWindow(qobject_cast<QVTKWidget *>(frame->mainWidget()));
        }

      delete *iter;
      }
    }

  // Clean up the pipeline.
  if(this->Implementation->Pipeline)
    {
    this->Implementation->Pipeline->clearPipeline();
    this->Implementation->Pipeline->getNameCount()->Reset();
    }

  // Clean up the current server.
  if(this->Implementation->CurrentServer)
    {
    delete this->Implementation->CurrentServer;
    this->Implementation->CurrentServer = 0;
    }

  // Call this method to ensure the menu items get updated.
  emit serverChanged(this->Implementation->CurrentServer);
}

void pqMainWindow::onFileNew(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onFileOpen()
{
  if(!this->Implementation->CurrentServer)
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpen(pqServer*)));
    server_browser->show();
    }
  else
    {
    this->onFileOpen(this->Implementation->CurrentServer);
    }
}

void pqMainWindow::onFileOpen(pqServer* Server)
{
  if(this->Implementation->CurrentServer != Server)
    setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->Implementation->CurrentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineList)
    return;
    
  QVTKWidget *win = this->Implementation->PipelineList->getCurrentWindow();
  if(win)
    {
    vtkSMSourceProxy* source = 0;
    vtkSMRenderModuleProxy* rm = this->Implementation->Pipeline->getRenderModule(win);
    for(int i = 0; i != Files.size(); ++i)
      {
      QString file = Files[i];
      
      source = this->Implementation->Pipeline->createSource("ExodusReader", win);
      this->Implementation->CurrentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
      source->Delete();
      pqSMAdaptor::setElementProperty(source, source->GetProperty("FileName"), file);
      pqSMAdaptor::setElementProperty(source, source->GetProperty("FilePrefix"), file);
      pqSMAdaptor::setElementProperty(source, source->GetProperty("FilePattern"), "%s");
      source->UpdateVTKObjects();
      this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(source), true);
      }

    rm->ResetCamera();
    win->update();

    // Select the latest source in the pipeline inspector.
    if(source)
      this->Implementation->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onFileOpenServerState()
{
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      tr("Open Server State File:"), this, "fileOpenDialog");
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onFileOpenServerState(const QStringList&)));
  fileDialog->show();
}

void pqMainWindow::onFileOpenServerState(pqServer*)
{
}

void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  // Clean up the current state.
  this->onFileNew();

  // Read in the xml file to restore.
  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
  xmlParser->SetFileName(Files[0].toAscii().data());
  xmlParser->Parse();

  // Get the root element from the parser.
  vtkPVXMLElement *root = xmlParser->GetRootElement();
  QString name = root->GetName();
  if(name == "ParaQ")
    {
    // Restore the application size and position.
    vtkPVXMLElement *element = ParaQ::FindNestedElementByName(root, "pqMainWindow");
    if(element)
      {
      int xpos = 0;
      int ypos = 0;
      if(element->GetScalarAttribute("x", &xpos) &&
          element->GetScalarAttribute("y", &ypos))
        {
        this->move(xpos, ypos);
        }

      int w = 0;
      int h = 0;
      if(element->GetScalarAttribute("width", &w) &&
          element->GetScalarAttribute("height", &h))
        {
        this->resize(w, h);
        }
      }

    // Restore the multi-view windows.
    if(this->Implementation->MultiViewManager)
      {
      this->Implementation->MultiViewManager->loadState(root);
      }

    // Restore the pipeline.
    if(this->Implementation->Pipeline)
      {
      this->Implementation->Pipeline->loadState(root, this->Implementation->MultiViewManager);
      }
    }
  else
    {
    // If the xml file is not a ParaQ file, it may be a server manager
    // state file.
    }

  xmlParser->Delete();
}

void pqMainWindow::onFileSaveServerState()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Server State:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("ParaQ");

  // Save the size and dock window information.
  vtkPVXMLElement *element = vtkPVXMLElement::New();
  element->SetName("pqMainWindow");
  QString number;
  number.setNum(this->x());
  element->AddAttribute("x", number.toAscii().data());
  number.setNum(this->y());
  element->AddAttribute("y", number.toAscii().data());
  number.setNum(this->width());
  element->AddAttribute("width", number.toAscii().data());
  number.setNum(this->height());
  element->AddAttribute("height", number.toAscii().data());
  element->AddAttribute("docking", this->saveState().data());
  root->AddNestedElement(element);
  element->Delete();

  if(this->Implementation->MultiViewManager)
    {
    this->Implementation->MultiViewManager->saveState(root);
    }

  if(this->Implementation->Pipeline)
    {
    this->Implementation->Pipeline->saveState(root, this->Implementation->MultiViewManager);
    }

  // Print the xml to the requested file(s).
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream os(Files[i].toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  root->Delete();
}

void pqMainWindow::onFileSaveScreenshot()
{
  if(!this->Implementation->CurrentServer)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No server connections to save"));
    return;
    }

  vtkRenderWindow* const render_window =
    this->Implementation->ActiveView ? qobject_cast<QVTKWidget*>(this->Implementation->ActiveView->mainWidget())->GetRenderWindow() : 0;

  if(!render_window)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No render window to save"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Screenshot:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveScreenshot(const QStringList& Files)
{
  vtkRenderWindow* const render_window =
    this->Implementation->ActiveView ? qobject_cast<QVTKWidget*>(this->Implementation->ActiveView->mainWidget())->GetRenderWindow() : 0;

  for(int i = 0; i != Files.size(); ++i)
    {
    if(!pqImageComparison::SaveScreenshot(render_window, Files[i]))
      QMessageBox::critical(this, tr("Save Screenshot:"), tr("Error saving file"));
    }
}

bool pqMainWindow::compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  vtkRenderWindow* const render_window =
    this->Implementation->ActiveView ? qobject_cast<QVTKWidget*>(this->Implementation->ActiveView->mainWidget())->GetRenderWindow() : 0;
    
  if(!render_window)
  {
    Output << "ERROR: Could not locate the Render Window." << endl;
    return false;
  }
    
  // All tests need a 300x300 render window size.
  int size[2];
  int *cur_size = render_window->GetSize();
  size[0] = cur_size[0];
  size[1] = cur_size[1];
  render_window->SetSize(300, 300);
  bool ret = pqImageComparison::CompareImage(render_window, ReferenceImage, 
    Threshold, Output, TempDirectory);
  render_window->SetSize(size);
  render_window->Render();
  return ret;
}

void pqMainWindow::onServerConnect()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onServerConnect(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onServerConnect(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onServerDisconnect()
{
  setServer(0);
}

void pqMainWindow::onUpdateSourcesFiltersMenu(pqServer*)
{
  this->Implementation->FiltersMenu->clear();
  this->Implementation->SourcesMenu->clear();

  // Update the menu items for the server and compound filters too.
  QAction* compoundFilterAction = this->Implementation->ToolsMenu->findChild<QAction*>(
      "CompoundFilterAction");
  compoundFilterAction->setEnabled(this->Implementation->CurrentServer);
  this->Implementation->ServerDisconnectAction->setEnabled(this->Implementation->CurrentServer);

  if(this->Implementation->CurrentServer)
    {
    QMenu *alphabetical = this->Implementation->FiltersMenu;
    QMap<QString, QMenu *> categories;
    QStringList::Iterator iter;
    if(this->Implementation->ProxyInfo)
      {
      // Load in the filter information if it is not present.
      if(!this->Implementation->ProxyInfo->IsFilterInfoLoaded())
        {
        QFile filterInfo(":/pqClient/ParaQFilters.xml");
        if(filterInfo.open(QIODevice::ReadOnly))
          {
          vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
          xmlParser->InitializeParser();
          QByteArray filter_data = filterInfo.read(1024);
          while(!filter_data.isEmpty())
            {
            xmlParser->ParseChunk(filter_data.data(), filter_data.length());
            filter_data = filterInfo.read(1024);
            }

          xmlParser->CleanupParser();

          filterInfo.close();
          vtkPVXMLElement *element = xmlParser->GetRootElement();
          this->Implementation->ProxyInfo->LoadFilterInfo(element);
          xmlParser->Delete();
          }
        }

      // Set up the filters menu based on the filter information.
      QStringList menuNames;
      this->Implementation->ProxyInfo->GetFilterMenu(menuNames);
      if(menuNames.size() > 0)
        {
        // Only use an alphabetical menu if requested.
        alphabetical = 0;
        }

      for(iter = menuNames.begin(); iter != menuNames.end(); ++iter)
        {
        if((*iter).isEmpty())
          {
          this->Implementation->FiltersMenu->addSeparator();
          }
        else
          {
          QMenu *menu = this->Implementation->FiltersMenu->addMenu(*iter);
          categories.insert(*iter, menu);
          if((*iter) == "&Alphabetical" || (*iter) == "Alphabetical")
            {
            alphabetical = menu;
            }
          }
        }
      }

    vtkSMProxyManager* manager = this->Implementation->CurrentServer->GetProxyManager();
    manager->InstantiateGroupPrototypes("filters");
    int numFilters = manager->GetNumberOfProxies("filters_prototypes");
    for(int i=0; i<numFilters; i++)
      {
      QStringList categoryList;
      QString proxyName = manager->GetProxyName("filters_prototypes",i);
      if(this->Implementation->ProxyInfo)
        {
        this->Implementation->ProxyInfo->GetFilterMenuCategories(proxyName, categoryList);
        }

      for(iter = categoryList.begin(); iter != categoryList.end(); ++iter)
        {
        QMap<QString, QMenu *>::Iterator jter = categories.find(*iter);
        if(jter != categories.end())
          {
          (*jter)->addAction(proxyName) << pqSetName(proxyName)
              << pqSetData(proxyName);
          }
        }

      if(alphabetical)
        {
        alphabetical->addAction(proxyName) << pqSetName(proxyName)
            << pqSetData(proxyName);
        }
      }

#if 1
    // hard code sources
    this->Implementation->SourcesMenu->addAction("2D Glyph") << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    this->Implementation->SourcesMenu->addAction("3D Text") << pqSetName("3D Text") << pqSetData("VectorText");
    this->Implementation->SourcesMenu->addAction("Arrow") << pqSetName("Arrow") << pqSetData("ArrowSource");
    this->Implementation->SourcesMenu->addAction("Axes") << pqSetName("Axes") << pqSetData("Axes");
    this->Implementation->SourcesMenu->addAction("Box") << pqSetName("Box") << pqSetData("CubeSource");
    this->Implementation->SourcesMenu->addAction("Cone") << pqSetName("Cone") << pqSetData("ConeSource");
    this->Implementation->SourcesMenu->addAction("Cylinder") << pqSetName("Cylinder") << pqSetData("CylinderSource");
    this->Implementation->SourcesMenu->addAction("Hierarchical Fractal") << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
    this->Implementation->SourcesMenu->addAction("Line") << pqSetName("Line") << pqSetData("LineSource");
    this->Implementation->SourcesMenu->addAction("Mandelbrot") << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
    this->Implementation->SourcesMenu->addAction("Plane") << pqSetName("Plane") << pqSetData("PlaneSource");
    this->Implementation->SourcesMenu->addAction("Sphere") << pqSetName("Sphere") << pqSetData("SphereSource");
    this->Implementation->SourcesMenu->addAction("Superquadric") << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
    this->Implementation->SourcesMenu->addAction("Wavelet") << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
#else
    // TODO: Add a sources xml file to organize the sources instead of
    // hardcoding them.
    manager->InstantiateGroupPrototypes("sources");
    int numSources = manager->GetNumberOfProxies("sources_prototypes");
    for(int i=0; i<numSources; i++)
      {
      const char* proxyName = manager->GetProxyName("sources_prototypes",i);
      this->Implementation->SourcesMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }
#endif
    }
}

vtkSMProxy* pqMainWindow::createSource(const QString& source_name)
{
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineList)
    return 0;

  QVTKWidget* const win = this->Implementation->PipelineList->getCurrentWindow();
  if(!win)
    return 0;
    
  vtkSMSourceProxy* source = this->Implementation->Pipeline->createSource(source_name.toAscii().data(), win);
  this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(source), true);
  vtkSMRenderModuleProxy* rm = this->Implementation->Pipeline->getRenderModule(win);
  rm->ResetCamera();
  win->update();
  this->Implementation->PipelineList->selectProxy(source);
  
  return source;
}

void pqMainWindow::onCreateSource(QAction* action)
{
  if(!action)
    return;

  QByteArray sourceName = action->data().toString().toAscii();
  this->createSource(sourceName);  
}

void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action || !this->Implementation->Pipeline || !this->Implementation->PipelineList)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->Implementation->PipelineList->getSelectedProxy();
  QVTKWidget *win = this->Implementation->PipelineList->getCurrentWindow();
  if(current && win)
    {
    vtkSMSourceProxy *source = 0;
    vtkSMProxy *next = this->Implementation->PipelineList->getNextProxy();
    if(next)
      {
      this->Implementation->PipelineList->getListModel()->beginCreateAndInsert();
      source = this->Implementation->Pipeline->createFilter(filterName, win);
      this->Implementation->Pipeline->addInput(source, current);
      this->Implementation->Pipeline->addInput(next, source);
      this->Implementation->Pipeline->removeInput(next, current);
      this->Implementation->PipelineList->getListModel()->finishCreateAndInsert();
      this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(source), false);
      }
    else
      {
      this->Implementation->PipelineList->getListModel()->beginCreateAndAppend();
      source = this->Implementation->Pipeline->createFilter(filterName, win);
      this->Implementation->Pipeline->addInput(source, current);
      this->Implementation->PipelineList->getListModel()->finishCreateAndAppend();

      // Only turn on visibility for added filters?
      this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(source), true);
      }

    vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->getRenderModule(win);
    rm->ResetCamera();
    win->update();
    this->Implementation->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onOpenLinkEditor()
{
}

void pqMainWindow::onOpenCompoundFilterWizard()
{
  pqCompoundProxyWizard* wizard = new pqCompoundProxyWizard(this->Implementation->CurrentServer, this);
  wizard->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed

  this->connect(wizard, SIGNAL(newCompoundProxy(const QString&, const QString&)), 
                        SLOT(onCompoundProxyAdded(const QString&, const QString&)));
  
  wizard->show();
}


void pqMainWindow::onRecordTest()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onRecordTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onRecordTest(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(Files[i], this);
    dialog->show();
    }
}

void pqMainWindow::onPlayTest()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onPlayTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onPlayTest(const QStringList& Files)
{
  pqEventPlayer player(*this);
  player.addDefaultWidgetEventPlayers();

  for(int i = 0; i != Files.size(); ++i)
    {
      pqEventPlayerXML xml_player;
      xml_player.playXML(player, Files[i]);
    }
}

void pqMainWindow::onPythonShell()
{
#ifdef PARAQ_EMBED_PYTHON
  pqPythonDialog* const dialog = new pqPythonDialog(this);
  dialog->show();
#endif // PARAQ_EMBED_PYTHON
}

void pqMainWindow::onNewQVTKWidget(pqMultiViewFrame* frame)
{
  QVTKWidget *widget = ParaQ::AddQVTKWidget(frame, this->Implementation->MultiViewManager,
      this->Implementation->CurrentServer);
  if(widget)
    {
    // Select the new window in the pipeline list.
    if(this->Implementation->PipelineList)
      {
      this->Implementation->PipelineList->selectWindow(widget);
      }

    QSignalMapper* sm = new QSignalMapper(frame);
    sm->setMapping(frame, frame);
    QObject::connect(frame, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
    QObject::connect(sm, SIGNAL(mapped(QWidget*)), this, SLOT(onFrameActive(QWidget*)));

    if(!this->Implementation->ActiveView)
      {
      frame->setActive(1);
      }
    }
}

void pqMainWindow::onDeleteQVTKWidget(pqMultiViewFrame* p)
{
  QVTKWidget* w = qobject_cast<QVTKWidget*>(p->mainWidget());
  this->cleanUpWindow(w);

  // Remove the window from the pipeline data structure.
  this->Implementation->Pipeline->removeWindow(w);

  if(this->Implementation->ActiveView == p)
    {
    this->Implementation->ActiveView = 0;
    }
}

void pqMainWindow::onFrameActive(QWidget* w)
{
  if(this->Implementation->ActiveView && this->Implementation->ActiveView != w)
    {
    pqMultiViewFrame* f = qobject_cast<pqMultiViewFrame*>(this->Implementation->ActiveView);
    if(f->active())
      f->setActive(0);
    }

  this->Implementation->ActiveView = qobject_cast<pqMultiViewFrame*>(w);
}

void pqMainWindow::onNewSelections(vtkSMProxy*, vtkUnstructuredGrid* selections)
{
  // Update the element inspector ...
  if(pqElementInspectorWidget* const element_inspector = this->Implementation->ElementInspectorDock->findChild<pqElementInspectorWidget*>())
    {
    element_inspector->addElements(selections);
    }
}

void pqMainWindow::onCreateCompoundProxy(QAction* action)
{
  if(!action || !this->Implementation->Pipeline || !this->Implementation->PipelineList)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->Implementation->PipelineList->getSelectedProxy();
  QVTKWidget *win = this->Implementation->PipelineList->getCurrentWindow();
  if(current && win)
    {
    vtkSMCompoundProxy *source = 0;
    vtkSMProxy *next = this->Implementation->PipelineList->getNextProxy();
    bool vis = false;
    if(next)
      {
      this->Implementation->PipelineList->getListModel()->beginCreateAndInsert();
      source = this->Implementation->Pipeline->createCompoundProxy(filterName, win);
      this->Implementation->Pipeline->addInput(source->GetMainProxy(), current);
      this->Implementation->Pipeline->addInput(next, source);
      this->Implementation->Pipeline->removeInput(next, current);
      this->Implementation->PipelineList->getListModel()->finishCreateAndInsert();
      }
    else
      {
      this->Implementation->PipelineList->getListModel()->beginCreateAndAppend();
      source = this->Implementation->Pipeline->createCompoundProxy(filterName, win);
      this->Implementation->Pipeline->addInput(source, current);
      this->Implementation->PipelineList->getListModel()->finishCreateAndAppend();
      vis = true;
      // Only turn on visibility for added filters?
      }

    if(source)
      {
      // TODO: how to decide which part of the compound proxy to display ????
      // for now just get the last one, and assuming it is a source proxy
      vtkSMProxy* sourceDisplay = NULL;
      for(int i=source->GetNumberOfProxies(); sourceDisplay == NULL && i>0; i--)
        {
        sourceDisplay = vtkSMSourceProxy::SafeDownCast(source->GetProxy(i-1));
        }
      this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(vtkSMSourceProxy::SafeDownCast(sourceDisplay), source), vis);
      //this->Implementation->Pipeline->setVisibility(this->Implementation->Pipeline->createDisplay(source), vis);
      }

    vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->getRenderModule(win);
    rm->ResetCamera();
    win->update();
    this->Implementation->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onCompoundProxyAdded(const QString&, const QString& proxy)
{
  this->Implementation->CompoundProxyToolBar->addAction(QIcon(":/pqWidgets/pqBundle32.png"), proxy) 
    << pqSetName(proxy) << pqSetData(proxy);
}

void pqMainWindow::onAddServer(pqPipelineServer *server)
{
  // When restoring a state file, the PipelineData object will
  // create the pqServer. Make sure the CurrentServer gets set.
  if(server && !this->Implementation->CurrentServer)
    {
    this->Implementation->CurrentServer = server->GetServer();
    emit serverChanged(this->Implementation->CurrentServer);
    }
}

void pqMainWindow::onRemoveServer(pqPipelineServer *server)
{
  if(!server || !this->Implementation->MultiViewManager)
    {
    return;
    }

  // Clean up all the views associated with the server.
  QWidget *widget = 0;
  pqPipelineWindow *win = 0;
  int total = server->GetWindowCount();
  this->Implementation->MultiViewManager->blockSignals(true);
  for(int i = 0; i < total; i++)
    {
    win = server->GetWindow(i);
    widget = win->GetWidget();

    // Clean up the render module.
    this->cleanUpWindow(qobject_cast<QVTKWidget *>(widget));

    // Remove the window from the multi-view.
    this->Implementation->MultiViewManager->removeWidget(widget->parentWidget());
    }

  this->Implementation->MultiViewManager->blockSignals(false);
}

void pqMainWindow::onAddWindow(pqPipelineWindow *win)
{
  if(!win)
    {
    return;
    }

  QVTKWidget *widget = qobject_cast<QVTKWidget *>(win->GetWidget());
  if(widget)
    {
    // Get the render module from the view map. Use the render module,
    // the interactor, and the qvtk widget to set up picking.
    vtkSMRenderModuleProxy* view = this->Implementation->Pipeline->getRenderModule(widget);
    vtkPVGenericRenderWindowInteractor* iren =
        vtkPVGenericRenderWindowInteractor::SafeDownCast(view->GetInteractor());
    pqPicking* picking = new pqPicking(view, widget);
    this->Implementation->VTKConnector->Connect(iren, vtkCommand::CharEvent, picking,
        SLOT(computeSelection(vtkObject*,unsigned long, void*, void*, vtkCommand*)),
        NULL, 1.0);
    QObject::connect(picking,
        SIGNAL(selectionChanged(vtkSMProxy*, vtkUnstructuredGrid*)),
        this, SLOT(onNewSelections(vtkSMProxy*, vtkUnstructuredGrid*)));
    }
}

void pqMainWindow::cleanUpWindow(QVTKWidget *win)
{
  if(win && this->Implementation->Pipeline)
    {
    // Remove the render module from the pipeline's view map and delete it.
    vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->removeViewMapping(win);
    if(rm)
      {
      rm->Delete();
      }
    }
}


void pqMainWindow::onProxySelected(vtkSMProxy* p)
{
  this->Implementation->CurrentProxy = p;
}

void pqMainWindow::onUpdateVariableSelector(vtkSMProxy* p)
{
  pqVariableSelectorWidget* selector = this->Implementation->VariableSelectorToolBar->findChild<pqVariableSelectorWidget*>();
  if(!selector)
    return;
    
  selector->clear();

  if(!this->Implementation->CurrentProxy)
    return;
    
  pqPipelineObject* pqObject = pqPipelineData::instance()->getObjectFor(p);
  vtkSMDisplayProxy* display = pqObject->GetDisplayProxy();
  if(!display)
    return;
  
  pqPipelineObject* reader = pqObject;
  while(reader->GetInput(0))
    reader = reader->GetInput(0);

  vtkPVDataInformation* geomInfo = display->GetGeometryInformation();
  vtkPVDataSetAttributesInformation* cellinfo = geomInfo->GetCellDataInformation();
  int i;
  for(i=0; i<cellinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = cellinfo->GetArrayInformation(i);
    selector->addVariable(VARIABLE_TYPE_CELL, info->GetName());
    }
  
  // also include unloaded arrays if any
  QList<QList<QVariant> > extraCellArrays = pqSMAdaptor::getSelectionProperty(reader->GetProxy(), 
                    reader->GetProxy()->GetProperty("CellArrayStatus"));
  for(i=0; i<extraCellArrays.size(); i++)
    {
    QList<QVariant> cell = extraCellArrays[i];
    if(cell[1] == false)
      {
      selector->addVariable(VARIABLE_TYPE_CELL, cell[0].toString());
      }
    }

  
  vtkPVDataSetAttributesInformation* pointinfo = geomInfo->GetPointDataInformation();
  for(i=0; i<pointinfo->GetNumberOfArrays(); i++)
    {
    vtkPVArrayInformation* info = pointinfo->GetArrayInformation(i);
    selector->addVariable(VARIABLE_TYPE_NODE, info->GetName());
    }
  
  // also include unloaded arrays if any
  QList<QList<QVariant> > extraPointArrays = pqSMAdaptor::getSelectionProperty(reader->GetProxy(), 
               reader->GetProxy()->GetProperty("PointArrayStatus"));
  for(i=0; i<extraPointArrays.size(); i++)
    {
    QList<QVariant> cell = extraPointArrays[i];
    if(cell[1] == false)
      {
      selector->addVariable(VARIABLE_TYPE_NODE, cell[0].toString());
      }
    }

  // set to the active display scalar
  vtkSMStringVectorProperty* d_svp = vtkSMStringVectorProperty::SafeDownCast(display->GetProperty("ColorArray"));
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(display->GetProperty("ScalarMode"));
  int fieldtype = ivp->GetElement(0);
  if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
    {
    selector->chooseVariable(VARIABLE_TYPE_CELL, d_svp->GetElement(0));
    }
  else
    {
    selector->chooseVariable(VARIABLE_TYPE_NODE, d_svp->GetElement(0));
    }
}

void pqMainWindow::onVariableChanged(pqVariableType type, const QString& name)
{
  if(this->Implementation->CurrentProxy)
    {
    pqPipelineObject* o = pqPipelineData::instance()->getObjectFor(this->Implementation->CurrentProxy);
    vtkSMDisplayProxy* display = o->GetDisplayProxy();
    QWidget* widget = o->GetParent()->GetWidget();

    switch(type)
      {
      case VARIABLE_TYPE_CELL:
        pqPart::Color(display, name.toAscii().data(), vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA);
        break;
      case VARIABLE_TYPE_NODE:
        pqPart::Color(display, name.toAscii().data(), vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA);
        break;
      }
          
    widget->update();
    }
}

void pqMainWindow::onFirstTimeStep()
{
  if(!this->Implementation->CurrentServer)
    return;
    
  if(!this->Implementation->CurrentProxy)
    return;

  const QString source_class = this->Implementation->CurrentProxy->GetVTKClassName();
  if(source_class != "vtkExodusReader" && source_class != "vtkPExodusReader")
    return;

  vtkSMIntVectorProperty* const timestep =
    vtkSMIntVectorProperty::SafeDownCast(this->Implementation->CurrentProxy->GetProperty("TimeStep"));
    
  if(!timestep)
    return;

  vtkSMIntRangeDomain* const timestep_range =
    vtkSMIntRangeDomain::SafeDownCast(timestep->GetDomain("range"));
    
  if(!timestep_range)
    return;

  int exists = 0;
  int first_step = timestep_range->GetMinimum(0, exists);

  timestep->SetElement(0, first_step);
  
  this->Implementation->CurrentProxy->UpdateVTKObjects();
  if(pqPipelineData *pipeline = pqPipelineData::instance())
    {
    QVTKWidget *win = pipeline->getWindowFor(this->Implementation->CurrentProxy);
    if(win)
      win->update();
    }
}

void pqMainWindow::onPreviousTimeStep()
{
  if(!this->Implementation->CurrentServer)
    return;
    
  if(!this->Implementation->CurrentProxy)
    return;

  const QString source_class = this->Implementation->CurrentProxy->GetVTKClassName();
  if(source_class != "vtkExodusReader" && source_class != "vtkPExodusReader")
    return;

  vtkSMIntVectorProperty* const timestep =
    vtkSMIntVectorProperty::SafeDownCast(this->Implementation->CurrentProxy->GetProperty("TimeStep"));
    
  if(!timestep)
    return;

  vtkSMIntRangeDomain* const timestep_range =
    vtkSMIntRangeDomain::SafeDownCast(timestep->GetDomain("range"));
    
  if(!timestep_range)
    return;

  int exists = 0;
  int first_step = timestep_range->GetMinimum(0, exists);

  timestep->SetElement(0, vtkstd::max(first_step, timestep->GetElement(0) - 1));
  
  this->Implementation->CurrentProxy->UpdateVTKObjects();
  if(pqPipelineData *pipeline = pqPipelineData::instance())
    {
    QVTKWidget *win= pipeline->getWindowFor(this->Implementation->CurrentProxy);
    if(win)
      win->update();
    }
}

void pqMainWindow::onNextTimeStep()
{
  if(!this->Implementation->CurrentServer)
    return;
    
  if(!this->Implementation->CurrentProxy)
    return;

  const QString source_class = this->Implementation->CurrentProxy->GetVTKClassName();
  if(source_class != "vtkExodusReader" && source_class != "vtkPExodusReader")
    return;

  vtkSMIntVectorProperty* const timestep =
    vtkSMIntVectorProperty::SafeDownCast(this->Implementation->CurrentProxy->GetProperty("TimeStep"));
    
  if(!timestep)
    return;

  vtkSMIntRangeDomain* const timestep_range =
    vtkSMIntRangeDomain::SafeDownCast(timestep->GetDomain("range"));
    
  if(!timestep_range)
    return;

  int exists = 0;
  int last_step = timestep_range->GetMaximum(0, exists);

  timestep->SetElement(0, vtkstd::min(last_step, timestep->GetElement(0) + 1));
  
  this->Implementation->CurrentProxy->UpdateVTKObjects();
  if(pqPipelineData *pipeline = pqPipelineData::instance())
    {
    QVTKWidget *win= pipeline->getWindowFor(this->Implementation->CurrentProxy);
    if(win)
      win->update();
    }
}

void pqMainWindow::onLastTimeStep()
{
  if(!this->Implementation->CurrentServer)
    return;
    
  if(!this->Implementation->CurrentProxy)
    return;

  const QString source_class = this->Implementation->CurrentProxy->GetVTKClassName();
  if(source_class != "vtkExodusReader" && source_class != "vtkPExodusReader")
    return;

  vtkSMIntVectorProperty* const timestep =
    vtkSMIntVectorProperty::SafeDownCast(this->Implementation->CurrentProxy->GetProperty("TimeStep"));
    
  if(!timestep)
    return;

  vtkSMIntRangeDomain* const timestep_range =
    vtkSMIntRangeDomain::SafeDownCast(timestep->GetDomain("range"));
    
  if(!timestep_range)
    return;

  int exists = 0;
  int last_step = timestep_range->GetMaximum(0, exists);

  timestep->SetElement(0, last_step);
  
  this->Implementation->CurrentProxy->UpdateVTKObjects();
  if(pqPipelineData *pipeline = pqPipelineData::instance())
    {
    QVTKWidget *win= pipeline->getWindowFor(this->Implementation->CurrentProxy);
    if(win)
      win->update();
    }
}
