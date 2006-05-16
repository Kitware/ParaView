/*=========================================================================

   Program:   ParaQ
   Module:    pqMainWindow.cxx

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
#include "pqFileDialogEventPlayer.h"
#include "pqFileDialogEventTranslator.h"
#include "pqMainWindow.h"
#include "pqMultiViewFrame.h"
#include "pqMultiView.h"
#include "pqNameCount.h"
#include "pqObjectInspectorWidget.h"
#include "pqParts.h"
#include "pqPicking.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineData.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineModel.h"
#include "pqPipelineSource.h"
#include "pqPipelineServer.h"
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
#include <pqEventTranslator.h>
#include <pqFileDialog.h>
#include <pqImageComparison.h>
#include <pqLocalFileDialogModel.h>
#include <pqObjectNaming.h>
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
#include <vtkSMUndoStack.h>

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
#include <QToolButton>
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
    Pipeline(0),
    PipelineBrowser(0),
    ActiveView(0),
    ElementInspectorDock(0),
    CompoundProxyToolBar(0),
    VariableSelectorToolBar(0),
    UndoRedoToolBar(0),
    ProxyInfo(0),
    VTKConnector(0),
    Inspector(0),
    UndoStack(0)
  {
    this->UndoStack = vtkSMUndoStack::New();
    vtkSMProxyManager::GetProxyManager()->SetUndoStack(this->UndoStack);
  }
  
  ~pqImplementation()
  {
    // Clean up the model before deleting the adaptor
    delete this->Inspector;
    this->Inspector = 0;
    
    delete this->PipelineBrowser;
    this->PipelineBrowser = 0;

    delete this->ProxyInfo;
    this->ProxyInfo = 0;

    // Clean up the pipeline before the view manager.
    delete this->Pipeline;
    this->Pipeline = 0;
    
    delete this->MultiViewManager;
    this->MultiViewManager = 0;

    delete this->CurrentServer;
    this->CurrentServer = 0;
    
    this->UndoStack->Delete();
    this->UndoStack = 0;
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
  pqMultiView* MultiViewManager;
  QAction* ServerDisconnectAction;
  pqPipelineData *Pipeline;
  pqPipelineBrowser *PipelineBrowser;
  pqMultiViewFrame* ActiveView;
  QDockWidget *ElementInspectorDock;
  QToolBar* CompoundProxyToolBar;
  QToolBar* VariableSelectorToolBar;
  QToolBar* UndoRedoToolBar;
  pqSourceProxyInfo* ProxyInfo;
  vtkSmartPointer<vtkEventQtSlotConnect> VTKConnector;
  pqObjectInspectorWidget* Inspector;
  vtkSMUndoStack* UndoStack;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindow

pqMainWindow::pqMainWindow() :
  Implementation(new pqImplementation())
{
  this->setObjectName("MainWindow");
  this->setWindowTitle("ParaQ");

  this->menuBar() << pqSetName("MenuBar");

  // Set up the main ParaQ items along with the central widget.
  this->Implementation->Pipeline = new pqPipelineData();
  this->Implementation->ProxyInfo = new pqSourceProxyInfo();
  this->Implementation->VTKConnector = vtkSmartPointer<vtkEventQtSlotConnect>::New();

  this->Implementation->MultiViewManager = new pqMultiView(this) << pqSetName("MultiViewManager");
  //this->Implementation->MultiViewManager->hide();  // workaround for flickering in Qt 4.0.1 & 4.1.0
  this->setCentralWidget(this->Implementation->MultiViewManager);
  QObject::connect(this->Implementation->MultiViewManager, SIGNAL(frameAdded(pqMultiViewFrame*)),
      this, SLOT(onNewQVTKWidget(pqMultiViewFrame*)));
  QObject::connect(this->Implementation->MultiViewManager, SIGNAL(frameRemoved(pqMultiViewFrame*)),
      this, SLOT(onDeleteQVTKWidget(pqMultiViewFrame*)));
  //this->Implementation->MultiViewManager->show();  // workaround for flickering in Qt 4.0.1 & 4.1.0

  // Listen for the pipeline's server signals.
  QObject::connect(this->Implementation->Pipeline, SIGNAL(serverAdded(pqServer *)),
      this, SLOT(onAddServer(pqServer *)));

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
    << pqSetName("CompoundFilter")
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenCompoundFilterWizard()));
  compoundFilterAction->setEnabled(false);

  menu->addAction(tr("&Link Editor..."))
    << pqSetName("LinkEditor")
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenLinkEditor()));

  menu->addSeparator();
  
  menu->addAction(tr("Validate Widget Names"))
    << pqSetName("Validate")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onValidateWidgetNames()));
  
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
  QDockWidget* const pipeline_dock = new QDockWidget("Pipeline Inspector", this)
    << pqSetName("PipelineInspectorDock");
    
  pipeline_dock->setAllowedAreas(
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);
  pipeline_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  // Make sure the pipeline data instance is created before the
  // pipeline list model. This ensures that the connections will
  // work.
  this->Implementation->PipelineBrowser = new pqPipelineBrowser(pipeline_dock);
  this->Implementation->PipelineBrowser->setObjectName("PipelineList");
  pipeline_dock->setWidget(this->Implementation->PipelineBrowser);

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, pipeline_dock, QIcon(":pqWidgets/pqPipelineList22.png"), visible);

  this->connect(this->Implementation->PipelineBrowser, SIGNAL(proxySelected(vtkSMProxy*)), this, SIGNAL(proxyChanged(vtkSMProxy*)));
  this->connect(this->Implementation->PipelineBrowser, SIGNAL(proxySelected(vtkSMProxy*)), this, SLOT(onProxySelected(vtkSMProxy*)));
}

void pqMainWindow::createStandardObjectInspector(bool visible)
{
  QDockWidget* const object_inspector_dock = new QDockWidget("Object Inspector", this)
    << pqSetName("ObjectInspectorDock");
    
  object_inspector_dock->setAllowedAreas(
      Qt::LeftDockWidgetArea |
      Qt::RightDockWidgetArea);
  object_inspector_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  this->Implementation->Inspector = new pqObjectInspectorWidget(object_inspector_dock);
  object_inspector_dock->setWidget(this->Implementation->Inspector);
  if(this->Implementation->PipelineBrowser)
    {
    connect(this->Implementation->PipelineBrowser, SIGNAL(proxySelected(vtkSMProxy *)),
        this->Implementation->Inspector, SLOT(setProxy(vtkSMProxy *)));
    }

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, object_inspector_dock, QIcon(), visible);
}

void pqMainWindow::createStandardElementInspector(bool visible)
{
  this->Implementation->ElementInspectorDock = new QDockWidget("Element Inspector View", this)
    << pqSetName("ElementInspectorDock");
    
  this->Implementation->ElementInspectorDock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);
  this->Implementation->ElementInspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  pqElementInspectorWidget* const element_inspector = new pqElementInspectorWidget(this->Implementation->ElementInspectorDock);
  this->Implementation->ElementInspectorDock->setWidget(element_inspector);

  this->addStandardDockWidget(Qt::BottomDockWidgetArea, this->Implementation->ElementInspectorDock, QIcon(), visible);

}

void pqMainWindow::createStandardVCRToolBar()
{
  QToolBar* const toolbar = new QToolBar(tr("VCR Controls"), this)
    << pqSetName("VCRControlsToolBar");
  
  pqPlayControlsWidget* const vcr_controls = new pqPlayControlsWidget(toolbar)
    << pqSetName("VCRControls");
  
  toolbar->addWidget(vcr_controls)
    << pqSetName("VCRControls");

  this->connect(vcr_controls, SIGNAL(first()), SLOT(onFirstTimeStep()));
  this->connect(vcr_controls, SIGNAL(back()), SLOT(onPreviousTimeStep()));
  this->connect(vcr_controls, SIGNAL(forward()), SLOT(onNextTimeStep()));
  this->connect(vcr_controls, SIGNAL(last()), SLOT(onLastTimeStep()));

  this->addToolBar(Qt::TopToolBarArea, toolbar);
}
//-----------------------------------------------------------------------------
void pqMainWindow::createUndoRedoToolBar()
{
  QToolBar* const toolbar = new QToolBar(tr("Undo Redo Controls"), this)
    << pqSetName("UndoRedoToolBar");
  this->Implementation->UndoRedoToolBar = toolbar;
  this->addToolBar(Qt::TopToolBarArea, toolbar);
  QAction *undoAction = new QAction(toolbar) << pqSetName("UndoButton");
  //QToolButton* const undoButton = new QToolButton(toolbar) << pqSetName("UndoButton");
  //undoButton->setAutoRaise(true);
  undoAction->setToolTip("Undo");
  undoAction->setIcon(QIcon(":pqWidgets/pqUndo22.png"));
  undoAction->setEnabled(false);
  //toolbar->addWidget(undoButton);
  toolbar->addAction(undoAction);

  QAction *redoAction = new QAction(toolbar) << pqSetName("RedoButton");
  //QToolButton* const redoButton = new QToolButton(toolbar) << pqSetName("RedoButton");
  //redoButton->setAutoRaise(true);
  redoAction->setToolTip("Redo");
  redoAction->setIcon(QIcon(":pqWidgets/pqRedo22.png"));
  redoAction->setEnabled(false);
  //toolbar->addWidget(redoButton);
  toolbar->addAction(redoAction);

  //this->connect(undoButton, SIGNAL(clicked()), SLOT(onUndo()));
  //this->connect(redoButton, SIGNAL(clicked()), SLOT(onRedo()));
  this->connect(undoAction, SIGNAL(triggered()), SLOT(onUndo()));
  this->connect(redoAction, SIGNAL(triggered()), SLOT(onRedo()));

  this->Implementation->VTKConnector->Connect(this->Implementation->UndoStack,
    vtkCommand::ModifiedEvent, this, SLOT(onUndoRedoStackChanged(vtkObject*, 
        unsigned long, void*, void*, vtkCommand*)), NULL, 1.0);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardVariableToolBar()
{
  this->Implementation->VariableSelectorToolBar = new QToolBar(tr("Variables"), this)
    << pqSetName("VariableSelectorToolBar");
    
  pqVariableSelectorWidget* varSelector = new pqVariableSelectorWidget(this->Implementation->VariableSelectorToolBar)
    << pqSetName("VariableSelector");
    
  this->Implementation->VariableSelectorToolBar->addWidget(varSelector);

  this->connect(this->Implementation->PipelineBrowser, SIGNAL(proxySelected(vtkSMProxy*)), this, SLOT(onUpdateVariableSelector(vtkSMProxy*)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), this, SIGNAL(variableChanged(pqVariableType, const QString&)));
  this->connect(varSelector, SIGNAL(variableChanged(pqVariableType, const QString&)), this, SLOT(onVariableChanged(pqVariableType, const QString&)));
    
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->VariableSelectorToolBar);
}

void pqMainWindow::createStandardCompoundProxyToolBar()
{
  this->Implementation->CompoundProxyToolBar = new QToolBar(tr("Compound Proxies"), this)
    << pqSetName("CompoundProxyToolBar");
    
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->CompoundProxyToolBar);
  this->connect(this->Implementation->CompoundProxyToolBar, SIGNAL(actionTriggered(QAction*)), SLOT(onCreateCompoundProxy(QAction*)));

  // Workaround for file new crash.
  this->Implementation->PipelineBrowser->setFocus();
}

QMenu* pqMainWindow::fileMenu()
{
  if(!this->Implementation->FileMenu)
    {
    this->Implementation->FileMenu = this->menuBar()->addMenu(tr("&File"))
      << pqSetName("FileMenu");
    }
    
  return this->Implementation->FileMenu;
}

QMenu* pqMainWindow::viewMenu()
{
  if(!this->Implementation->ViewMenu)
    {
    this->Implementation->ViewMenu = this->menuBar()->addMenu(tr("&View"))
      << pqSetName("ViewMenu");
    }
    
  return this->Implementation->ViewMenu;
}

QMenu* pqMainWindow::serverMenu()
{
  if(!this->Implementation->ServerMenu)
    {
    this->Implementation->ServerMenu = this->menuBar()->addMenu(tr("&Server"))
      << pqSetName("ServerMenu");
    }
    
  return this->Implementation->ServerMenu;
}

QMenu* pqMainWindow::sourcesMenu()
{
  if(!this->Implementation->SourcesMenu)
    {
    this->Implementation->SourcesMenu = this->menuBar()->addMenu(tr("&Sources"))
      << pqSetName("SourcesMenu");
    }
    
  return this->Implementation->SourcesMenu;
}

QMenu* pqMainWindow::filtersMenu()
{
  if(!this->Implementation->FiltersMenu)
    {
    this->Implementation->FiltersMenu = this->menuBar()->addMenu(tr("&Filters"))
      << pqSetName("FiltersMenu");
    }
    
  return this->Implementation->FiltersMenu;
}

QMenu* pqMainWindow::toolsMenu()
{
  if(!this->Implementation->ToolsMenu)
    {
    this->Implementation->ToolsMenu = this->menuBar()->addMenu(tr("&Tools"))
      << pqSetName("ToolsMenu");
    }
    
  return this->Implementation->ToolsMenu;
}

QMenu* pqMainWindow::helpMenu()
{
  if(!this->Implementation->HelpMenu)
    {
    this->Implementation->HelpMenu = this->menuBar()->addMenu(tr("&Help"))
      << pqSetName("HelpMenu");
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
  /*
  if(this->Implementation->CurrentServer)
    {
    if(this->Implementation->CompoundProxyToolBar)
      {
      this->Implementation->CompoundProxyToolBar->clear();
      }
    this->Implementation->Pipeline->removeServer(this->Implementation->CurrentServer);
    delete this->Implementation->CurrentServer;
    }
  */
  
  delete this->Implementation->CurrentServer;
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
    if(this->Implementation->PipelineBrowser)
      {
      this->Implementation->PipelineBrowser->selectServer(
          this->Implementation->CurrentServer);
      }

    this->onNewQVTKWidget(qobject_cast<pqMultiViewFrame *>(
        this->Implementation->MultiViewManager->widgetOfIndex(pqMultiView::Index())));
    }
  emit serverChanged(this->Implementation->CurrentServer);
}

void pqMainWindow::onFileNew()
{
  // Clean up the pipeline.
  if(this->Implementation->Pipeline)
    {
    this->Implementation->Pipeline->getModel()->clearPipelines();
    this->Implementation->Pipeline->getNameCount()->Reset();
    }

  // Reset the multi-view. Use the removed widget list to clean
  // up the render modules. Then, delete the widgets.
  this->Implementation->ActiveView = 0;
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

  // Clean up the current server.
  delete this->Implementation->CurrentServer;
  this->Implementation->CurrentServer = 0;

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
    server_browser->setModal(true);
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

  // TODO: handle more than exodus
  QString filters;
  filters += "Exodus files (*.g *.e *.ex2 *.ex2v2 *.exo *.gen *.exoII *.0 *.00 *.000 *.0000)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(NULL, Server), 
    this, tr("Open File:"), QString(), filters);
  file_dialog->setObjectName("FileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineBrowser)
    {
    return;
    }

  QVTKWidget *win = 0;
  pqServer *server = this->Implementation->PipelineBrowser->getCurrentServer()->GetServer();
  if(this->Implementation->ActiveView)
    {
    win = qobject_cast<QVTKWidget *>(this->Implementation->ActiveView->mainWidget());
    }

  if(server)
    {
    vtkSMProxy* source = 0;
    vtkSMRenderModuleProxy* rm = this->Implementation->Pipeline->getRenderModule(win);
    for(int i = 0; i != Files.size(); ++i)
      {
      source = this->createReader(Files[i], server);
      source->UpdateVTKObjects();
      this->Implementation->Pipeline->setVisible(
          this->Implementation->Pipeline->createAndRegisterDisplay(source, rm), true);
      }

    rm->ResetCamera();
    win->update();

    // Select the latest source in the pipeline inspector.
    if(source)
      this->Implementation->PipelineBrowser->selectProxy(source);
    }
}

void pqMainWindow::onFileOpenServerState()
{
  QString filters;
  filters += "ParaView state file (*.pvs)";
  filters += ";;All files (*)";

  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      this, tr("Open Server State File:"), QString(), filters);
  fileDialog->setObjectName("FileOpenServerStateDialog");
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
    vtkPVXMLElement *element = pqXMLUtil::FindNestedElementByName(root,
        "MainWindow");
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
    vtkSMObject::GetProxyManager()->LoadState(root);
    }

  xmlParser->Delete();
}

void pqMainWindow::onFileSaveServerState()
{
  QString filters;
  filters += "ParaView state file (*.pvs)";
  filters += ";;All files (*)";

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this, tr("Save Server State:"), QString(), filters);
  file_dialog->setObjectName("FileSaveServerStateDialog");
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
  element->SetName("MainWindow");
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
    this->Implementation->Pipeline->getModel()->saveState(root, this->Implementation->MultiViewManager);
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

  QString filters;
  filters += "PNG image (*.png)";
  filters += ";;BMP image (*.bmp)";
  filters += ";;TIFF image (*.tif)";
  filters += ";;PPM image (*.ppm)";
  filters += ";;JPG image (*.jpg)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this, tr("Save Screenshot:"), QString(), filters);
  file_dialog->setObjectName("FileSaveScreenshotDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveScreenshot(const QStringList& Files)
{
  vtkRenderWindow* const render_window =
    this->Implementation->ActiveView ? qobject_cast<QVTKWidget*>(this->Implementation->ActiveView->mainWidget())->GetRenderWindow() : 0;

  // All tests need a 300x300 render window size
  QSize cur_size = this->Implementation->ActiveView->mainWidget()->size();
  this->Implementation->ActiveView->mainWidget()->resize(300,300);

  for(int i = 0; i != Files.size(); ++i)
    {
    if(!pqImageComparison::SaveScreenshot(render_window, Files[i]))
      QMessageBox::critical(this, tr("Save Screenshot:"), tr("Error saving file"));
    }

  this->Implementation->ActiveView->mainWidget()->resize(cur_size);
  render_window->Render();
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
  QSize cur_size = this->Implementation->ActiveView->mainWidget()->size();
  this->Implementation->ActiveView->mainWidget()->resize(300,300);
  bool ret = pqImageComparison::CompareImage(render_window, ReferenceImage, 
    Threshold, Output, TempDirectory);
  this->Implementation->ActiveView->mainWidget()->resize(cur_size);
  render_window->Render();
  return ret;
}

void pqMainWindow::onServerConnect()
{
  this->setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onServerConnect(pqServer*)));
  server_browser->setModal(true);
  server_browser->show();
}

void pqMainWindow::onServerConnect(pqServer* Server)
{
  this->setServer(Server);
}

void pqMainWindow::onServerDisconnect()
{
  if(this->Implementation->CurrentServer)
    {
    this->Implementation->Pipeline->removeServer(this->Implementation->CurrentServer);
    delete this->Implementation->CurrentServer;
    this->Implementation->CurrentServer = 0;
    }
}

void pqMainWindow::onUpdateSourcesFiltersMenu(pqServer*)
{
  this->Implementation->FiltersMenu->clear();
  this->Implementation->SourcesMenu->clear();

  // Update the menu items for the server and compound filters too.
  QAction* compoundFilterAction = this->Implementation->ToolsMenu->findChild<QAction*>(
      "CompoundFilter");
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
          QMenu *menu = this->Implementation->FiltersMenu->addMenu(*iter) << pqSetName(*iter);
          categories.insert(*iter, menu);
          if((*iter) == "&Alphabetical" || (*iter) == "Alphabetical")
            {
            alphabetical = menu;
            }
          }
        }
      }

    vtkSMProxyManager* manager = vtkSMObject::GetProxyManager();
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
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineBrowser)
    {
    return 0;
    }

  pqServer *server = this->Implementation->PipelineBrowser->getCurrentServer()->GetServer();
  this->Implementation->UndoStack->BeginUndoSet(
      server->GetConnectionID(), "CreateSource");

  vtkSMProxy* source = this->Implementation->Pipeline->createAndRegisterSource(
      source_name.toAscii().data(), server);
  if(source && this->Implementation->ActiveView)
    {
    QVTKWidget *win = qobject_cast<QVTKWidget *>(
        this->Implementation->ActiveView->mainWidget());
    vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->getRenderModule(win);
    this->Implementation->Pipeline->setVisible(
        this->Implementation->Pipeline->createAndRegisterDisplay(source, rm), true);
    rm->ResetCamera();
    win->update();
    }

  this->Implementation->PipelineBrowser->selectProxy(source);

  this->Implementation->UndoStack->EndUndoSet();
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
  if(!action || !this->Implementation->Pipeline || !this->Implementation->PipelineBrowser)
    {
    return;
    }
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->Implementation->PipelineBrowser->getSelectedProxy();
  pqServer *server = this->Implementation->PipelineBrowser->getListModel()->getServerFor(current)->GetServer();
  if(current && server)
    {
    vtkSMProxy *source = 0;
    vtkSMProxy *next = this->Implementation->PipelineBrowser->getNextProxy();
    if(next)
      {
      source = this->Implementation->Pipeline->createAndRegisterFilter(filterName, server);
      this->Implementation->Pipeline->addConnection(current, source);
      this->Implementation->Pipeline->removeConnection(current, next);
      this->Implementation->Pipeline->addConnection(source, next);
      }
    else
      {
      source = this->Implementation->Pipeline->createAndRegisterFilter(filterName, server);
      this->Implementation->Pipeline->addConnection(current, source);
      }

    if(source && this->Implementation->ActiveView)
      {
      QVTKWidget *win = qobject_cast<QVTKWidget *>(
          this->Implementation->ActiveView->mainWidget());
      vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->getRenderModule(win);

      // Only turn on visibility for added filters?
      this->Implementation->Pipeline->setVisible(
          this->Implementation->Pipeline->createAndRegisterDisplay(source, rm), next == 0);
      rm->ResetCamera();
      win->update();
      }

    this->Implementation->PipelineBrowser->selectProxy(source);
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

void pqMainWindow::onValidateWidgetNames()
{
  pqObjectNaming::Validate(*this);
}

void pqMainWindow::onRecordTest()
{
  QString filters;
  filters += "XML files (*.xml)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this, tr("Record Test:"), QString(), filters);
  file_dialog->setObjectName("ToolsRecordTestDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onRecordTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onRecordTest(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    pqEventTranslator* const translator = new pqEventTranslator();
    translator->addWidgetEventTranslator(new pqFileDialogEventTranslator());
    translator->addDefaultWidgetEventTranslators();
    
    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(translator, Files[i], this);
    dialog->show();
    }
}

void pqMainWindow::onPlayTest()
{
  QString filters;
  filters += "XML files (*.xml)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new
    pqLocalFileDialogModel(), this, tr("Play Test:"), QString(), filters);
  file_dialog->setObjectName("ToolsPlayTestDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onPlayTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onPlayTest(const QStringList& Files)
{
  QApplication::processEvents();

  pqEventPlayer player;
  player.addWidgetEventPlayer(new pqFileDialogEventPlayer());
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
  if(!action || !this->Implementation->Pipeline || !this->Implementation->PipelineBrowser)
    {
    return;
    }

  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->Implementation->PipelineBrowser->getSelectedProxy();
  pqServer *server = this->Implementation->PipelineBrowser->getListModel()->getServerFor(current)->GetServer();
  if(current && server)
    {
    vtkSMProxy *source = 0;
    vtkSMProxy *next = this->Implementation->PipelineBrowser->getNextProxy();
    if(next)
      {
      source = this->Implementation->Pipeline->createAndRegisterBundle(filterName, server);
      this->Implementation->Pipeline->addConnection(current, source);
      this->Implementation->Pipeline->removeConnection(current, next);
      this->Implementation->Pipeline->addConnection(source, next);
      }
    else
      {
      source = this->Implementation->Pipeline->createAndRegisterBundle(filterName, server);
      this->Implementation->Pipeline->addConnection(current, source);
      }

    if(source && this->Implementation->ActiveView)
      {
      QVTKWidget *win = qobject_cast<QVTKWidget *>(
          this->Implementation->ActiveView->mainWidget());
      vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->getRenderModule(win);

      // Only turn on visibility for added filters?
      this->Implementation->Pipeline->setVisible(
          this->Implementation->Pipeline->createAndRegisterDisplay(source, rm), next == 0);
      rm->ResetCamera();
      win->update();
      }

    this->Implementation->PipelineBrowser->selectProxy(source);
    }
}

void pqMainWindow::onCompoundProxyAdded(const QString&, const QString& proxy)
{
  this->Implementation->CompoundProxyToolBar->addAction(QIcon(":/pqWidgets/pqBundle32.png"), proxy) 
    << pqSetName(proxy) << pqSetData(proxy);
}

void pqMainWindow::onAddServer(pqServer *server)
{
  // When restoring a state file, the PipelineData object will
  // create the pqServer. Make sure the CurrentServer gets set.
  if(server && !this->Implementation->CurrentServer)
    {
    this->Implementation->CurrentServer = server;
    emit serverChanged(this->Implementation->CurrentServer);
    }
}

void pqMainWindow::onRemoveServer(pqServer *server)
{
  if(!this->Implementation->MultiViewManager || !this->Implementation->Pipeline)
    {
    return;
    }

  // Get the server object from the pipeline model.
  pqPipelineServer *serverObject =
      this->Implementation->Pipeline->getModel()->getServerFor(server);
  if(!serverObject)
    {
    return;
    }

  // Clean up all the views associated with the server.
  QWidget *widget = 0;
  int total = serverObject->GetWindowCount();
  this->Implementation->MultiViewManager->blockSignals(true);
  for(int i = 0; i < total; i++)
    {
    widget = serverObject->GetWindow(i);

    // Clean up the render module.
    this->cleanUpWindow(qobject_cast<QVTKWidget *>(widget));

    // Remove the window from the multi-view.
    this->Implementation->MultiViewManager->removeWidget(widget->parentWidget());
    }

  this->Implementation->MultiViewManager->blockSignals(false);
}

void pqMainWindow::onAddWindow(QWidget *win)
{
  if(!win)
    {
    return;
    }

  QVTKWidget *widget = qobject_cast<QVTKWidget *>(win);
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
    this->Implementation->Pipeline->getModel()->removeWindow(win);
    vtkSMRenderModuleProxy *rm = this->Implementation->Pipeline->removeViewMapping(win);
    if(rm)
      {
      rm->Delete();
      }

    if(win->parentWidget() == this->Implementation->ActiveView)
      {
      this->Implementation->ActiveView = 0;
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
  selector->addVariable(VARIABLE_TYPE_NONE, "");

  if(!this->Implementation->CurrentProxy)
    return;
    
  pqPipelineSource* pqObject = pqPipelineData::instance()->getModel()->getSourceFor(p);
  vtkSMDisplayProxy* display = pqObject->GetDisplay()->GetDisplayProxy(0);
  if(!display)
    return;

  pqPipelineSource* reader = pqObject;
  pqPipelineFilter* filter = dynamic_cast<pqPipelineFilter*>(reader);
  while(filter && filter->GetInputCount() > 0)
    {
    reader = filter->GetInput(0);
    filter = dynamic_cast<pqPipelineFilter*>(reader);
    }

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

  // Update the variable selector to display the current proxy color
  QVariant scalarColor = pqSMAdaptor::getElementProperty(display, display->GetProperty("ScalarVisibility"));
  
  vtkSMIntVectorProperty* scalar_visibility = vtkSMIntVectorProperty::SafeDownCast(display->GetProperty("ScalarVisibility"));
  if(scalar_visibility && scalar_visibility->GetElement(0))
    {
    vtkSMStringVectorProperty* d_svp = vtkSMStringVectorProperty::SafeDownCast(display->GetProperty("ColorArray"));
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(display->GetProperty("ScalarMode"));
    int fieldtype = ivp->GetElement(0);
    if(fieldtype == vtkSMDataObjectDisplayProxy::CELL_FIELD_DATA)
      {
      selector->chooseVariable(VARIABLE_TYPE_CELL, d_svp->GetElement(0));
      }
    else if(fieldtype == vtkSMDataObjectDisplayProxy::POINT_FIELD_DATA)
      {
      selector->chooseVariable(VARIABLE_TYPE_NODE, d_svp->GetElement(0));
      }
    }
  else
    {
    selector->chooseVariable(VARIABLE_TYPE_NONE, "");
    }
}

void pqMainWindow::onVariableChanged(pqVariableType type, const QString& name)
{
  if(this->Implementation->CurrentProxy)
    {
    pqPipelineSource* o = pqPipelineData::instance()->getModel()->getSourceFor(this->Implementation->CurrentProxy);
    vtkSMDisplayProxy* display = o->GetDisplay()->GetDisplayProxy(0);
    QWidget* widget = o->GetDisplay()->GetDisplayWindow(0);

    switch(type)
      {
      case VARIABLE_TYPE_NONE:
        pqPart::Color(display, NULL, 0);
        break;
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
    pqPipelineSource *source = pipeline->getModel()->getSourceFor(
        this->Implementation->CurrentProxy);
    if(source)
      {
      source->GetDisplay()->UpdateWindows();
      }
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
    pqPipelineSource *source = pipeline->getModel()->getSourceFor(
        this->Implementation->CurrentProxy);
    if(source)
      {
      source->GetDisplay()->UpdateWindows();
      }
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
    pqPipelineSource *source = pipeline->getModel()->getSourceFor(
        this->Implementation->CurrentProxy);
    if(source)
      {
      source->GetDisplay()->UpdateWindows();
      }
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
    pqPipelineSource *source = pipeline->getModel()->getSourceFor(
        this->Implementation->CurrentProxy);
    if(source)
      {
      source->GetDisplay()->UpdateWindows();
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onUndoRedoStackChanged(vtkObject*, unsigned long, void*, 
  void*,  vtkCommand*)
{
  QAction* undoButton = 
    this->Implementation->UndoRedoToolBar->findChild<QAction*>("UndoButton");

  QAction* redoButton = 
    this->Implementation->UndoRedoToolBar->findChild<QAction*>("RedoButton");
  if (undoButton)
    {
    undoButton->setEnabled(this->Implementation->UndoStack->CanUndo()); 
    }

  if (redoButton)
    {
    redoButton->setEnabled(this->Implementation->UndoStack->CanRedo());
    }
}


//-----------------------------------------------------------------------------
void pqMainWindow::onUndo()
{
  this->Implementation->UndoStack->Undo();
  QVTKWidget *win = qobject_cast<QVTKWidget *>(this->Implementation->ActiveView->mainWidget());
  if (win)
    {
    win->update();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRedo()
{
  this->Implementation->UndoStack->Redo();
  QVTKWidget *win = qobject_cast<QVTKWidget *>(this->Implementation->ActiveView->mainWidget());
  if (win)
    {
    win->update();
    }
}

//-----------------------------------------------------------------------------
vtkSMProxy* pqMainWindow::createReader(const QString &file, pqServer* server)
{
  vtkSMProxy* source = this->Implementation->Pipeline->createAndRegisterSource("ExodusReader", server);
  pqSMAdaptor::setElementProperty(source, source->GetProperty("FileName"), file);
  pqSMAdaptor::setElementProperty(source, source->GetProperty("FilePrefix"), file);
  pqSMAdaptor::setElementProperty(source, source->GetProperty("FilePattern"), "%s");
  return source;
}

//-----------------------------------------------------------------------------
pqPipelineData* pqMainWindow::getPipeline()
{
  return this->Implementation->Pipeline;
}

//-----------------------------------------------------------------------------
vtkSMDisplayProxy* pqMainWindow::createDisplay(vtkSMProxy* source)
{
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineBrowser)
    {
    return 0;
    }

  QVTKWidget *win = 0;
  pqServer *server = this->Implementation->PipelineBrowser->getCurrentServer()->GetServer();
  if(this->Implementation->ActiveView)
    {
    win = qobject_cast<QVTKWidget *>(this->Implementation->ActiveView->mainWidget());
    }

  vtkSMDisplayProxy* display = 0;

  if(server)
    {
    vtkSMRenderModuleProxy* rm = this->Implementation->Pipeline->getRenderModule(win);
    source->UpdateVTKObjects();
    display = this->Implementation->Pipeline->createAndRegisterDisplay(source, rm);
    this->Implementation->Pipeline->setVisible(display, true);

    rm->ResetCamera();
    win->update();

    // Select the latest source in the pipeline inspector.
    if(source)
      {
      this->Implementation->PipelineBrowser->selectProxy(source);
      }
    }
  return display;
}

//-----------------------------------------------------------------------------
QVTKWidget* pqMainWindow::getVTKWindow()
{
  pqServer* server = this->getServer();
  if ( !server )
    {
    return  0;
    }

  if(!this->Implementation->ActiveView)
    {
    return 0;
    }
  return qobject_cast<QVTKWidget *>(this->Implementation->ActiveView->mainWidget());
}

//-----------------------------------------------------------------------------
vtkSMRenderModuleProxy* pqMainWindow::getRenderModule()
{
  QVTKWidget* win = this->getVTKWindow();
  if ( !win )
    {
    return  0;
    }

  return this->Implementation->Pipeline->getRenderModule(win);
}

//-----------------------------------------------------------------------------
pqServer* pqMainWindow::getServer()
{
  if(!this->Implementation->Pipeline || !this->Implementation->PipelineBrowser ||
    !this->Implementation->PipelineBrowser->getCurrentServer())
    {
    return 0;
    }

  return this->Implementation->PipelineBrowser->getCurrentServer()->GetServer();
}
