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
#include <vtkPQConfig.h>

#include "pqMainWindow.h"

#include "pqApplicationCore.h"
#include "pqCompoundProxyWizard.h"
#include "pqDataInformationWidget.h"
#include "pqElementInspectorWidget.h"
#include "pqMultiViewFrame.h"
#include "pqMultiView.h"
#include "pqNameCount.h"
#include "pqObjectInspectorWidget.h"
#include "pqPicking.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineMenu.h"
#include "pqPipelineSource.h"
#include "pqPropertyManager.h"
#include "pqReaderFactory.h"
#include "pqRenderModule.h"
#include "pqRenderWindowManager.h"
#include "pqSelectionManager.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqServer.h"
#include "pqSimpleAnimationManager.h"
#include "pqSettings.h"
#include "pqSourceProxyInfo.h"
#include "pqTestUtility.h"
#include "pqToolTipTrapper.h"
#include "pqDisplayColorWidget.h"
#include "pqVCRController.h"
#include "pqXMLUtil.h"

#include <pqConnect.h>
#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqEventTranslator.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqObjectNaming.h>
#include <pqObjectPanel.h>
#include <pqPlayControlsWidget.h>
#include <pqRecordEventsDialog.h>
#include <pqServerManagerModel.h>
#include <pqSetData.h>
#include <pqSetName.h>
#include <pqUndoStack.h>
#include <pqWriterFactory.h>

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
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include "vtkToolkits.h"

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QProgressBar>
#include "pqProgressBar.h"
#include <QSignalMapper>
#include <QStatusBar>
#include <QtDebug>
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
    RecentFilesMenu(0),
    EditMenu(0),
    ViewMenu(0),
    ServerMenu(0),
    SourcesMenu(0),
    FiltersMenu(0),
    PipelineMenu(0),
    ToolsMenu(0),
    HelpMenu(0),
    RecentFilesListMaxmumLength(10),
    ActiveView(0),
    MultiViewManager(0),
    Inspector(0),
    PipelineBrowser(0),
    ElementInspectorDock(0),
    CompoundProxyToolBar(0),
    PropertyToolbar(0),
    UndoRedoToolBar(0),
    SelectionToolBar(0),
    VariableSelectorToolBar(0),
    VCRController(0),
    DataInformationWidget(0),
    RenderWindowManager(0),
    IgnoreBrowserSelectionChanges(false),
    SelectionManager(0),
    ToolTipTrapper(0)
  {
  }
  
  ~pqImplementation()
  {
    // Clean up the model before deleting the adaptor
    delete this->Inspector;
    this->Inspector = 0;
    
    delete this->PipelineBrowser;
    this->PipelineBrowser = 0;

    delete this->MultiViewManager;
    this->MultiViewManager = 0;

    delete this->DataInformationWidget;
    this->DataInformationWidget = 0;

    delete this->SelectionManager;
    this->SelectionManager = 0;
    
    delete this->ToolTipTrapper;
    this->ToolTipTrapper = 0;
  }

  // Stores standard menus
  QMenu* FileMenu;
  QMenu* RecentFilesMenu;
  QMenu* EditMenu;
  QMenu* ViewMenu;
  QMenu* ServerMenu;
  QMenu* SourcesMenu;
  QMenu* FiltersMenu;
  pqPipelineMenu *PipelineMenu;
  QMenu* ToolsMenu;
  QMenu* HelpMenu;
  
  /// Stores a mapping from dockable widgets to menu actions
  QMap<QDockWidget*, QAction*> DockWidgetVisibleActions;
  QStringList RecentFilesList;
  int RecentFilesListMaxmumLength;

  pqMultiViewFrame* ActiveView;
  pqMultiView* MultiViewManager;
  pqObjectInspectorWidget* Inspector;
  pqPipelineBrowser *PipelineBrowser;
  QDockWidget *ElementInspectorDock;
  QToolBar* CompoundProxyToolBar;
  QToolBar* PropertyToolbar;
  QToolBar* UndoRedoToolBar;
  QToolBar* SelectionToolBar;
  QToolBar* VariableSelectorToolBar;
  pqVCRController* VCRController;
  pqDataInformationWidget* DataInformationWidget;
  pqRenderWindowManager* RenderWindowManager;

  bool IgnoreBrowserSelectionChanges;

  pqSelectionManager* SelectionManager;
  
  pqToolTipTrapper* ToolTipTrapper;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindow

pqMainWindow::pqMainWindow() :
  Implementation(new pqImplementation())
{
  this->setObjectName("MainWindow");
  this->setWindowTitle("ParaView");
  this->Implementation->RecentFilesList
    = pqApplicationCore::instance()->settings()->recentFilesList();

  this->menuBar() << pqSetName("MenuBar");

  this->Implementation->MultiViewManager = 
    new pqMultiView(this) << pqSetName("MultiViewManager");
  this->Implementation->MultiViewManager->installEventFilter(this);

  //this->Implementation->MultiViewManager->hide();  
  // workaround for flickering in Qt 4.0.1 & 4.1.0
  this->setCentralWidget(this->Implementation->MultiViewManager);

  pqApplicationCore* core = pqApplicationCore::instance();
  
  // *  Create the pqRenderWindowManager. 
  this->Implementation->RenderWindowManager = new pqRenderWindowManager(this);


  // Connect the renderModule manager with the view manager so that 
  // new render modules are created as new frames are added.
  QObject::connect(this->Implementation->RenderWindowManager, 
                   SIGNAL(activeRenderModuleChanged(pqRenderModule*)),
                   core, 
                   SLOT(setActiveRenderModule(pqRenderModule*)));

  QObject::connect(this->Implementation->MultiViewManager, 
                   SIGNAL(frameAdded(pqMultiViewFrame*)), 
                   this->Implementation->RenderWindowManager, 
                   SLOT(onFrameAdded(pqMultiViewFrame*)));

  QObject::connect(this->Implementation->MultiViewManager, 
                   SIGNAL(frameRemoved(pqMultiViewFrame*)), 
                   this->Implementation->RenderWindowManager, 
                   SLOT(onFrameRemoved(pqMultiViewFrame*)));

  // Connect selection changed events.
  QObject::connect(core, SIGNAL(activeSourceChanged(pqPipelineSource*)),
                   this, SLOT(onActiveSourceChanged(pqPipelineSource*)));
  QObject::connect(core, SIGNAL(activeServerChanged(pqServer*)),
                   this, SLOT(onActiveServerChanged(pqServer*)));

  QObject::connect(core, SIGNAL(activeSourceChanged(pqPipelineSource*)),
                   this, SLOT(onCoreActiveChanged()));
  QObject::connect(core, SIGNAL(activeServerChanged(pqServer*)),
                   this, SLOT(onCoreActiveChanged()));

  // Update enable state when pending displays state changes.
  QObject::connect(core, SIGNAL(pendingDisplays(bool)),
                   this, SLOT(updateEnableState()));

  // Update enable state when the active view changes.
  QObject::connect(core, SIGNAL(activeRenderModuleChanged(pqRenderModule*)),
                   this, SLOT(onActiveRenderModuleChanged(pqRenderModule*)));

  pqUndoStack* undoStack = core->getUndoStack();
  // Connect undo/redo status.
  QObject::connect(
    undoStack, SIGNAL(StackChanged(bool, QString, bool, QString)), 
    this, SLOT(onUndoRedoStackChanged(bool, QString, bool, QString))); 

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Initialize supported file types.
  core->getReaderFactory()->loadFileTypes(":/pqClient/ParaQReaders.xml");
  core->getWriterFactory()->loadFileTypes(":/pqClient/ParaQWriters.xml");

  this->Implementation->SelectionManager = new pqSelectionManager;
  QObject::connect(core, 
                   SIGNAL(activeRenderModuleChanged(pqRenderModule*)),
                   this->Implementation->SelectionManager, 
                   SLOT(activeRenderModuleChanged(pqRenderModule*)));
}

//-----------------------------------------------------------------------------
pqMainWindow::~pqMainWindow()
{
  delete Implementation;
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardFileMenu()
{
  QMenu* const menu = this->fileMenu();
  QAction* action;
 
  menu->addAction(tr("&New"), this, SLOT(onFileNew()), 
    QKeySequence(Qt::CTRL + Qt::Key_N))
    << pqSetName("New");

  menu->addAction(tr("&Open"), this, SLOT(onFileOpen()), 
    QKeySequence(Qt::CTRL + Qt::Key_O))
    << pqSetName("Open");


  this->updateRecentlyLoadedFilesMenu(false);

  action = menu->addAction(tr("&Save Data"), this, SLOT(onFileSaveData()), 
    QKeySequence(Qt::CTRL + Qt::Key_S))
    << pqSetName("SaveData");
  action->setEnabled(false);

  menu->addSeparator();

  action = menu->addAction(tr("&Load Server State"))
    << pqSetName("LoadServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpenServerState()));
  // disable save/load state for the time being.
  action->setEnabled(true);

  action = menu->addAction(tr("&Save Server State"))
    << pqSetName("SaveServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));
  // disable save/load state for the time being.
  action->setEnabled(false);
  menu->addSeparator();

  action = menu->addAction(tr("Save Screenshot"))
    << pqSetName("SaveScreenshot")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveScreenshot())); 
  action->setEnabled(false);

  action = menu->addAction(tr("Save Animation"))
    << pqSetName("SaveAnimation")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveAnimation()));
  action->setEnabled(false);

  menu->addSeparator();
  
  menu->addAction(tr("E&xit"), QApplication::instance(), SLOT(quit()), 
    QKeySequence(Qt::CTRL + Qt::Key_Q))
    << pqSetName("Exit");
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardEditMenu()
{
  QMenu* const menu = this->editMenu();
  QAction* action;

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  action = menu->addAction(tr("Can't &Undo"), stack, SLOT(Undo()),
    QKeySequence(Qt::CTRL + Qt::Key_Z))
    << pqSetName("Undo");
  action->setEnabled(false);

  action = menu->addAction(tr("Can't &Redo"), stack, SLOT(Redo()),
    QKeySequence(Qt::CTRL + Qt::Key_R))
    << pqSetName("Redo");
  action->setEnabled(false);

  menu->addSeparator();

  action = menu->addAction(tr("Can't U&ndo Interaction"), this,
    SLOT(UndoActiveViewInteraction()),
    QKeySequence(Qt::CTRL + Qt::Key_B))
    << pqSetName("CameraUndo");
  action->setEnabled(false);

  action = menu->addAction(tr("Can't R&edo Interaction"), this,
    SLOT(RedoActiveViewInteraction()),
    QKeySequence(Qt::CTRL + Qt::Key_F))
    << pqSetName("CameraRedo");
  action->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardStatusBar()
{
  QStatusBar* sbar = this->statusBar();
  sbar->show();
  QProgressBar *progressBar = new pqProgressBar(sbar);
  progressBar->hide();
  sbar->addPermanentWidget(progressBar);
  QObject::connect(pqApplicationCore::instance(), SIGNAL(enableProgress(bool)),
                   progressBar, SLOT(setVisible(bool)));
  QObject::connect(pqApplicationCore::instance(), 
                   SIGNAL(progress(const QString&, int)),
                   progressBar, 
                   SLOT(setProgress(const QString&, int)));
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardViewMenu()
{
  this->viewMenu();
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardServerMenu()
{
  QMenu* const menu = this->serverMenu();
  
  menu->addAction(tr("&Connect"))
    << pqSetName("Connect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerConnect()));

  QAction* action= menu->addAction(tr("&Disconnect"))
    << pqSetName("Disconnect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerDisconnect()));
    
  action->setEnabled(false);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardSourcesMenu()
{
  QMenu* const menu = this->sourcesMenu();
  menu << pqConnect(SIGNAL(triggered(QAction*)), 
    this, SLOT(onCreateSource(QAction*)));
  
  this->buildSourcesMenu();
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardFiltersMenu()
{
  QMenu* const menu = this->filtersMenu();
  menu << pqConnect(SIGNAL(triggered(QAction*)), 
    this, SLOT(onCreateFilter(QAction*)));
  this->buildFiltersMenu();
}

void pqMainWindow::createStandardPipelineMenu()
{
  if(!this->Implementation->PipelineMenu)
    {
    this->Implementation->PipelineMenu = new pqPipelineMenu(this);
    this->Implementation->PipelineMenu->setObjectName("PipelineMenu");
    this->Implementation->PipelineMenu->addActionsToMenuBar(this->menuBar());

    // TEMP: Load in the filter information.
    QFile filterInfo(":/pqClient/ParaQFilters.xml");
    if(filterInfo.open(QIODevice::ReadOnly))
      {
      vtkSmartPointer<vtkPVXMLParser> xmlParser = 
        vtkSmartPointer<vtkPVXMLParser>::New();
      xmlParser->InitializeParser();
      QByteArray filter_data = filterInfo.read(1024);
      while(!filter_data.isEmpty())
        {
        xmlParser->ParseChunk(filter_data.data(), filter_data.length());
        filter_data = filterInfo.read(1024);
        }

      xmlParser->CleanupParser();
      filterInfo.close();

      this->Implementation->PipelineMenu->loadFilterInfo(xmlParser->GetRootElement());
      }
    }
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
  
  menu->addAction(tr("&Dump Widget Names"))
    << pqSetName("DumpWidgets")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onDumpWidgetNames()));
  
  menu->addAction(tr("&Record Test"))
    << pqSetName("Record")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTest()));

  menu->addAction(tr("Record &Test Screenshot"))
    << pqSetName("RecordTestScreenshot")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTestScreenshot()));

  menu->addAction(tr("&Play Test"))
    << pqSetName("Play")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPlayTest()));

#ifdef PARAQ_EMBED_PYTHON
  menu->addAction(tr("Python &Shell"))
    << pqSetName("PythonShell")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPythonShell()));
#endif // PARAQ_EMBED_PYTHON
}

void pqMainWindow::createStandardHelpMenu()
{
  QMenu* const menu = this->helpMenu();
  
  QAction* tooltipsAction = menu->addAction(tr("Tooltips"))
    << pqSetName("Tooltips");
    
  tooltipsAction->setCheckable(true);
  tooltipsAction->setChecked(!this->Implementation->ToolTipTrapper);
  tooltipsAction << pqConnect(SIGNAL(toggled(bool)), this, SLOT(enableTooltips(bool)));
}

void pqMainWindow::createStandardPipelineBrowser(bool visible)
{
  QDockWidget* const pipeline_dock = 
    new QDockWidget("Pipeline Inspector", this) 
    << pqSetName("PipelineInspectorDock");
    
  pipeline_dock->setAllowedAreas(
    Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  pipeline_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  this->Implementation->PipelineBrowser = new pqPipelineBrowser(pipeline_dock);
  this->Implementation->PipelineBrowser->setObjectName("PipelineList");
  pipeline_dock->setWidget(this->Implementation->PipelineBrowser);

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, pipeline_dock, 
    QIcon(":pqWidgets/pqPipelineList22.png"), visible);

  this->connect(this->Implementation->PipelineBrowser, 
    SIGNAL(selectionChanged(pqServerManagerModelItem*)), 
    this, SLOT(onBrowserSelectionChanged(pqServerManagerModelItem*)));

  QObject::connect(this, SIGNAL(select(pqServerManagerModelItem*)),
    this->Implementation->PipelineBrowser,
    SLOT(select(pqServerManagerModelItem*)));

}
//-----------------------------------------------------------------------------
void pqMainWindow::createStandardDataInformationWidget(bool visible)
{
  QDockWidget* const dock = 
    new QDockWidget("Statistics View", this) 
    << pqSetName("StatisticsViewDock");
    
  dock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  this->Implementation->DataInformationWidget = 
    new pqDataInformationWidget(dock);
  this->Implementation->DataInformationWidget->setObjectName(
    "DataInformationWidget");

  pqUndoStack* undoStack = pqApplicationCore::instance()->getUndoStack();
  // Undo/redo operations can potentially change data information,
  // hence we must refresh the data on undo/redo.
  QObject::connect(undoStack, SIGNAL(Undone()),
    this->Implementation->DataInformationWidget, SLOT(refreshData()));
  QObject::connect(undoStack, SIGNAL(Redone()),
    this->Implementation->DataInformationWidget, SLOT(refreshData()));

  dock->setWidget(this->Implementation->DataInformationWidget);
  
  this->addStandardDockWidget(Qt::BottomDockWidgetArea, dock, 
    QIcon(), visible);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardObjectInspector(bool visible)
{
  QDockWidget* const object_inspector_dock = 
    new QDockWidget("Object Inspector", this)
    << pqSetName("ObjectInspectorDock");

  object_inspector_dock->setAllowedAreas(
    Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  object_inspector_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);

  this->Implementation->Inspector = 
    new pqObjectInspectorWidget(object_inspector_dock);
  object_inspector_dock->setWidget(this->Implementation->Inspector);
  
  pqUndoStack* undoStack = pqApplicationCore::instance()->getUndoStack();
  // Connect Accept/reset signals.
  QObject::connect(
    this->Implementation->Inspector, 
    SIGNAL(preaccept()), undoStack, SLOT(Accept()));

  QObject::connect(
    this->Implementation->Inspector, 
    SIGNAL(postaccept()), undoStack, SLOT(EndUndoSet()));

  QObject::connect(
    this->Implementation->Inspector, SIGNAL(preaccept()), 
    this, SLOT(preAcceptUpdate()));

  QObject::connect(
    this->Implementation->Inspector, SIGNAL(postaccept()), 
    this, SLOT(postAcceptUpdate()));

  QObject::connect(this->Implementation->Inspector, SIGNAL(accepted()), 
    pqApplicationCore::instance(), SLOT(createPendingDisplays()));
  QObject::connect(
    pqApplicationCore::instance(), SIGNAL(pendingDisplays(bool)),
    this->Implementation->Inspector, SLOT(forceModified(bool)));

  if (this->Implementation->VariableSelectorToolBar)
    {
    // Connecting to accept/postaccept signals from a panel
    // is a hassle, can't we have something more centralized?
    pqDisplayColorWidget* varSelector = this->Implementation->
      VariableSelectorToolBar->findChild<pqDisplayColorWidget*>();
    if (varSelector)
      {
      this->connect(this->Implementation->Inspector, SIGNAL(postaccept()),
        varSelector, SLOT(reloadGUI()));
      }
    }

  this->addStandardDockWidget(Qt::LeftDockWidgetArea, 
    object_inspector_dock, QIcon(), visible);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardElementInspector(bool visible)
{
  this->Implementation->ElementInspectorDock = 
    new QDockWidget("Element Inspector View", this)
    << pqSetName("ElementInspectorDock");
    
  this->Implementation->ElementInspectorDock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);
  this->Implementation->ElementInspectorDock->setFeatures(
    QDockWidget::AllDockWidgetFeatures);

  pqElementInspectorWidget* const element_inspector = 
    new pqElementInspectorWidget(this->Implementation->ElementInspectorDock);
  this->Implementation->ElementInspectorDock->setWidget(element_inspector);

  this->addStandardDockWidget(Qt::BottomDockWidgetArea, 
    this->Implementation->ElementInspectorDock, QIcon(), visible);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardVCRToolBar()
{
  QToolBar* const toolbar = new QToolBar(tr("VCR Controls"), this)
    << pqSetName("VCRControlsToolBar");
  
  pqPlayControlsWidget* const vcr_controls = new pqPlayControlsWidget(toolbar)
    << pqSetName("VCRControls");
  
  toolbar->addWidget(vcr_controls)
    << pqSetName("VCRControls");

  this->Implementation->VCRController = new pqVCRController(this)
    << pqSetName("VCRController");
  
  this->connect(vcr_controls, SIGNAL(first()),  
    this->Implementation->VCRController, SLOT(onFirst()));
  this->connect(vcr_controls, SIGNAL(back()),   
    this->Implementation->VCRController, SLOT(onBack()));
  this->connect(vcr_controls, SIGNAL(forward()),
    this->Implementation->VCRController, SLOT(onForward()));
  this->connect(vcr_controls, SIGNAL(last()),   
    this->Implementation->VCRController, SLOT(onLast()));

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
  undoAction->setToolTip("Undo");
  undoAction->setIcon(QIcon(":pqWidgets/pqUndo22.png"));
  undoAction->setEnabled(false);
  toolbar->addAction(undoAction);

  QAction *redoAction = new QAction(toolbar) << pqSetName("RedoButton");
  redoAction->setToolTip("Redo");
  redoAction->setIcon(QIcon(":pqWidgets/pqRedo22.png"));
  redoAction->setEnabled(false);
  toolbar->addAction(redoAction);

  pqUndoStack* stack = pqApplicationCore::instance()->getUndoStack();
  this->connect(undoAction, SIGNAL(triggered()), stack, SLOT(Undo()));
  this->connect(redoAction, SIGNAL(triggered()), stack, SLOT(Redo()));
}

//-----------------------------------------------------------------------------
void pqMainWindow::createSelectionToolBar()
{
  QToolBar* const toolbar = new QToolBar(tr("Selection Controls"), this)
    << pqSetName("SelectionToolBar");
  this->Implementation->SelectionToolBar = toolbar;
  toolbar->setEnabled(false);
  this->addToolBar(Qt::TopToolBarArea, toolbar);

  QActionGroup* group = new QActionGroup(toolbar);

  QAction *interactAction = new QAction(toolbar) << pqSetName("InteractButton");
  interactAction->setToolTip("Switch to interaction mode");
  interactAction->setIcon(QIcon(":pqWidgets/pqMouseMove15.png"));
  interactAction->setCheckable(true);
  interactAction->setChecked(true);
  toolbar->addAction(interactAction);
  group->addAction(interactAction);

  QAction *selectAction = new QAction(toolbar) << pqSetName("SelectButton");
  selectAction->setToolTip("Switch to selection mode");
  selectAction->setIcon(QIcon(":pqWidgets/pqMouseSelect15.png"));
  selectAction->setCheckable(true);
  toolbar->addAction(selectAction);
  group->addAction(selectAction);

  this->connect(interactAction, 
                SIGNAL(triggered()), 
                this->Implementation->SelectionManager, 
                SLOT(switchToInteraction()));
  this->connect(selectAction, 
                SIGNAL(triggered()), 
                this->Implementation->SelectionManager, 
                SLOT(switchToSelection()));
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardVariableToolBar()
{
  this->Implementation->VariableSelectorToolBar = 
    new QToolBar(tr("Variables"), this)
    << pqSetName("VariableSelectorToolBar");
  this->Implementation->VariableSelectorToolBar->setEnabled(false);
    
  pqDisplayColorWidget* varSelector = new pqDisplayColorWidget(
    this->Implementation->VariableSelectorToolBar)
    << pqSetName("VariableSelector");
    
  this->Implementation->VariableSelectorToolBar->addWidget(varSelector);

  pqApplicationCore* core = pqApplicationCore::instance();
  QObject::connect(core, SIGNAL(activeSourceChanged(pqPipelineSource*)),
    varSelector, SLOT(updateVariableSelector(pqPipelineSource*)));
  
  this->connect(varSelector, 
                SIGNAL(variableChanged(pqVariableType, const QString&)), 
                this, 
                SIGNAL(variableChanged(pqVariableType, const QString&)));

  if (this->Implementation->Inspector)
    {
    this->connect(this->Implementation->Inspector, SIGNAL(postaccept()),
                  varSelector, SLOT(reloadGUI()));
    }
    
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->VariableSelectorToolBar);
}

//-----------------------------------------------------------------------------
void pqMainWindow::createStandardCompoundProxyToolBar()
{
  this->Implementation->CompoundProxyToolBar =
    new QToolBar(tr("Compound Proxies"), this)
    << pqSetName("CompoundProxyToolBar");
    
  this->addToolBar(Qt::TopToolBarArea, this->Implementation->CompoundProxyToolBar);
  this->connect(this->Implementation->CompoundProxyToolBar, 
    SIGNAL(actionTriggered(QAction*)), SLOT(onCreateCompoundProxy(QAction*)));

  // Workaround for file new crash.
  this->Implementation->PipelineBrowser->setFocus();
}

//-----------------------------------------------------------------------------
QMenu* pqMainWindow::fileMenu()
{
  if(!this->Implementation->FileMenu)
    {
    this->Implementation->FileMenu = this->menuBar()->addMenu(tr("&File"))
      << pqSetName("FileMenu");
    }
    
  return this->Implementation->FileMenu;
}

//-----------------------------------------------------------------------------
QMenu* pqMainWindow::recentFilesMenu()
{
  if(!this->Implementation->RecentFilesMenu)
    {
    this->Implementation->RecentFilesMenu = this->fileMenu()->addMenu(tr("&Recent Files"))
      << pqSetName("RecentFilesMenu");
    }
    
  return this->Implementation->RecentFilesMenu;
}

//-----------------------------------------------------------------------------
QMenu* pqMainWindow::editMenu()
{
  if (!this->Implementation->EditMenu)
    {
    this->Implementation->EditMenu = this->menuBar()->addMenu(tr("&Edit"))
      << pqSetName("EditMenu");
    }
  return this->Implementation->EditMenu;
}

//-----------------------------------------------------------------------------
QMenu* pqMainWindow::viewMenu()
{
  if(!this->Implementation->ViewMenu)
    {
    this->Implementation->ViewMenu = this->menuBar()->addMenu(tr("&View"))
      << pqSetName("ViewMenu");
    }
    
  return this->Implementation->ViewMenu;
}

//-----------------------------------------------------------------------------
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
    this->Implementation->SourcesMenu = this->menuBar()->addMenu(tr("S&ources"))
      << pqSetName("SourcesMenu");
    this->Implementation->SourcesMenu->setEnabled(false);
    }
    
  return this->Implementation->SourcesMenu;
}

QMenu* pqMainWindow::filtersMenu()
{
  if(!this->Implementation->FiltersMenu)
    {
    this->Implementation->FiltersMenu = this->menuBar()->addMenu(tr("F&ilters"))
      << pqSetName("FiltersMenu");
    this->Implementation->FiltersMenu->setEnabled(false);
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

void pqMainWindow::addStandardDockWidget(Qt::DockWidgetArea area, 
                                         QDockWidget* dockwidget, 
                                         const QIcon& icon, 
                                         bool visible)
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
        this->Implementation->DockWidgetVisibleActions[dockwidget]->setChecked(
          e->type() == QEvent::Show);
        }
      }
    }
  else if (e->type() == QEvent::KeyPress)
    {
    QKeyEvent* kEvent = static_cast<QKeyEvent*>(e);
    
    if (kEvent->key() == Qt::Key_S)
      {
      if (this->Implementation->SelectionManager->getMode() ==
          pqSelectionManager::SELECT)
        {
        QAction*  interact = 
          this->Implementation->SelectionToolBar->findChild<QAction*>(
            "InteractButton");
        if (this->Implementation->SelectionToolBar->isEnabled())
          {
          interact->trigger();
          }
        }
      else
        {
        QAction*  select = 
          this->Implementation->SelectionToolBar->findChild<QAction*>(
            "SelectButton");
        if (this->Implementation->SelectionToolBar->isEnabled())
          {
          select->trigger();
          }
        }
      }
    }

  return QMainWindow::eventFilter(watched, e);
}

//-----------------------------------------------------------------------------
void pqMainWindow::buildFiltersMenu()
{
  this->Implementation->FiltersMenu->clear();
  // Update the menu items for the server and compound filters too.

  QMenu *alphabetical = this->Implementation->FiltersMenu;
  QMap<QString, QMenu *> categories;

  QStringList::Iterator iter;
  pqSourceProxyInfo proxyInfo;

      
  //Released Filters
  QStringList releasedFilters;
  releasedFilters<<"Clip";
  releasedFilters<<"Cut";
  releasedFilters<<"Threshold";

  QMenu *releasedMenu = this->Implementation->FiltersMenu->addMenu("Released") << pqSetName("Released");
  for(iter = releasedFilters.begin(); iter != releasedFilters.end(); ++iter)
    {
        QAction* action = releasedMenu->addAction(*iter) << pqSetName(*iter)
          << pqSetData(*iter);
        action->setEnabled(false);
    }


  // Load in the filter information.
  QFile filterInfo(":/pqClient/ParaQFilters.xml");
  if(filterInfo.open(QIODevice::ReadOnly))
    {
    vtkSmartPointer<vtkPVXMLParser> xmlParser = 
      vtkSmartPointer<vtkPVXMLParser>::New();
    xmlParser->InitializeParser();
    QByteArray filter_data = filterInfo.read(1024);
    while(!filter_data.isEmpty())
      {
      xmlParser->ParseChunk(filter_data.data(), filter_data.length());
      filter_data = filterInfo.read(1024);
      }

    xmlParser->CleanupParser();
    filterInfo.close();

    proxyInfo.LoadFilterInfo(xmlParser->GetRootElement());
    }

  // Set up the filters menu based on the filter information.
  QStringList menuNames;
  proxyInfo.GetFilterMenu(menuNames);
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
      QMenu *_menu = this->Implementation->FiltersMenu->addMenu(*iter) 
        << pqSetName(*iter);
      categories.insert(*iter, _menu);
      if((*iter) == "&Alphabetical" || (*iter) == "Alphabetical")
        {
        alphabetical = _menu;
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
    proxyInfo.GetFilterMenuCategories(proxyName, categoryList);

    for(iter = categoryList.begin(); iter != categoryList.end(); ++iter)
      {
      QMap<QString, QMenu *>::Iterator jter = categories.find(*iter);
      if(jter != categories.end())
        {
        QAction* action = (*jter)->addAction(proxyName) << pqSetName(proxyName)
          << pqSetData(proxyName);
        action->setEnabled(false);
        }
      }

    if(alphabetical)
      {
      QAction* action = alphabetical->addAction(proxyName) << pqSetName(proxyName)
        << pqSetData(proxyName);
      action->setEnabled(false);
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::buildSourcesMenu()
{
  this->Implementation->SourcesMenu->clear();
#if 1
  // Released Sources
  QMenu *menu = this->Implementation->SourcesMenu->addMenu("Released") << pqSetName("Released");
  menu->addAction("Cone") 
    << pqSetName("Cone") << pqSetData("ConeSource");




  // hard code sources
  this->Implementation->SourcesMenu->addAction("2D Glyph") 
    << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
  this->Implementation->SourcesMenu->addAction("3D Text") 
    << pqSetName("3D Text") << pqSetData("VectorText");
  this->Implementation->SourcesMenu->addAction("Arrow")
    << pqSetName("Arrow") << pqSetData("ArrowSource");
  this->Implementation->SourcesMenu->addAction("Axes") 
    << pqSetName("Axes") << pqSetData("Axes");
  this->Implementation->SourcesMenu->addAction("Box") 
    << pqSetName("Box") << pqSetData("CubeSource");
  this->Implementation->SourcesMenu->addAction("Cone") 
    << pqSetName("Cone") << pqSetData("ConeSource");
  this->Implementation->SourcesMenu->addAction("Cylinder")
    << pqSetName("Cylinder") << pqSetData("CylinderSource");
  this->Implementation->SourcesMenu->addAction("Hierarchical Fractal") 
    << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
  this->Implementation->SourcesMenu->addAction("Line") 
    << pqSetName("Line") << pqSetData("LineSource");
  this->Implementation->SourcesMenu->addAction("Mandelbrot") 
    << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
  this->Implementation->SourcesMenu->addAction("Plane") 
    << pqSetName("Plane") << pqSetData("PlaneSource");
  this->Implementation->SourcesMenu->addAction("Sphere") 
    << pqSetName("Sphere") << pqSetData("SphereSource");
  this->Implementation->SourcesMenu->addAction("Superquadric") 
    << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
  this->Implementation->SourcesMenu->addAction("Wavelet") 
    << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
#else
  // TODO: Add a sources xml file to organize the sources instead of
  // hardcoding them.
  manager->InstantiateGroupPrototypes("sources");
  int numSources = manager->GetNumberOfProxies("sources_prototypes");
  for(int i=0; i<numSources; i++)
    {
    const char* proxyName = manager->GetProxyName("sources_prototypes",i);
    this->Implementation->SourcesMenu->addAction(proxyName) 
      << pqSetName(proxyName) << pqSetData(proxyName);
    }
#endif
}

//-----------------------------------------------------------------------------
void pqMainWindow::setServer(pqServer* server)
{
  pqApplicationCore::instance()->setActiveServer(server);
  /* 
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
    
    //this->Implementation->Pipeline->addServer(this->Implementation->CurrentServer);
    if(this->Implementation->PipelineBrowser)
      {
      this->Implementation->PipelineBrowser->select(
          this->Implementation->CurrentServer);
      }

    pqApplicationCore::instance()->getRenderWindowManager()->onFrameAdded(
      qobject_cast<pqMultiViewFrame *>(
      this->Implementation->MultiViewManager->widgetOfIndex(
        pqMultiView::Index())));
    }
  emit serverChanged(this->Implementation->CurrentServer);
  */
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileNew()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (server)
    {
    pqApplicationCore::instance()->removeServer(server);
    }
  this->updateEnableState();

  /*
  // Clean up the pipeline.
  if(this->Implementation->Pipeline)
    {
    // FIXME
    // this->Implementation->Pipeline->getModel()->clearPipelines();
    // this->Implementation->Pipeline->getNameCount()->Reset();
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
  */
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpen()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfServers() == 0)
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), 
      this, SLOT(onFileOpen(pqServer*)));
    // let the regular onServerConnect() operation be performed as well.
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
      SLOT(onServerConnect(pqServer*)));
    server_browser->setModal(true);
    server_browser->show();
    }
  else if(!core->getActiveServer())
    {
    qDebug() << "No active server selected.";
    }
  else
    {
    this->onFileOpen(core->getActiveServer());
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpen(pqServer* server)
{
  QString filters = pqApplicationCore::instance()->getReaderFactory()->
    getSupportedFileTypes(server);
  if (filters != "")
    {
    filters += ";;";
    }
  filters += "All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(
    new pqServerFileDialogModel(NULL, server), 
    this, tr("Open File:"), QString(), filters);
  file_dialog->setObjectName("FileOpenDialog");
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpen(const QStringList& files)
{
  pqApplicationCore* core =pqApplicationCore::instance();

  for(int i = 0; i != files.size(); ++i)
    {
    pqPipelineSource* reader = 
      core->createReaderOnActiveServer(files[i]/*, "ExodusReader"*/);
    if (!reader)
      {
      qDebug() << "Failed to create reader for : " << files[i];
      continue;
      }
    this->addFileToRecentList(files[i]);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveData()
{
  pqPipelineSource* source = pqApplicationCore::instance()->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source, cannot save data.";
    return;
    }

  // Get the list of writers that can write the output from the given source.
  QString filters = 
    pqApplicationCore::instance()->getWriterFactory()->getSupportedFileTypes(
      source);

  pqFileDialog file_dialog(
    new pqServerFileDialogModel(NULL, source->getServer()), 
    this, tr("Save File:"), QString(), filters);
  file_dialog.setObjectName("FileSaveDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  file_dialog.setAttribute(Qt::WA_DeleteOnClose, false);
  QObject::connect(&file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveData(const QStringList&)));
  file_dialog.exec();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveData(const QStringList& files)
{
  pqPipelineSource* source = pqApplicationCore::instance()->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source, cannot save data.";
    return;
    }
  if (files.size() == 0)
    {
    qDebug() << "No file choose to save.";
    return;
    }

  vtkSmartPointer<vtkSMProxy> proxy;
  proxy.TakeReference(pqApplicationCore::instance()->getWriterFactory()->
    newWriter(files[0], source));

  vtkSMSourceProxy* writer = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!writer)
    {
    qDebug() << "Failed to create writer for: " << files[0];
    return;
    }

  vtkSMStringVectorProperty::SafeDownCast(writer->GetProperty("FileName"))
    ->SetElement(0, files[0].toStdString().c_str());

  // TODO: We can popup a wizard or something for setting the properties
  // on the writer.
  vtkSMProxyProperty::SafeDownCast(writer->GetProperty("Input"))->AddProxy(
    pqApplicationCore::instance()->getActiveSource()->getProxy());
  writer->UpdateVTKObjects();

  writer->UpdatePipeline();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpenServerState()
{
  pqApplicationCore* core = pqApplicationCore::instance();
 int num_servers = core->getServerManagerModel()->getNumberOfServers();
  if (num_servers > 0)
    {
    if (!core->getActiveServer())
      {
      qDebug() << "No active server. Cannot load state.";
      return;
      }
    pqServer* server = core->getActiveServer();
    core->setActiveSource(NULL);
    this->onFileOpenServerState(server);
    }
  else
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    // let the regular onServerConnect() operation be performed as well.
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
      SLOT(onServerConnect(pqServer*)));
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), 
      this, SLOT(onFileOpenServerState(pqServer*)));
    server_browser->setModal(true);
    server_browser->show();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpenServerState(pqServer*)
{
  QString filters;
  filters += "ParaView state file (*.pvs)";
  filters += ";;All files (*)";

  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      this, tr("Open Server State File:"), QString(), filters);
  fileDialog->setObjectName("FileOpenServerStateDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onFileOpenServerState(const QStringList&)));
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  // Read in the xml file to restore.
  vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
  xmlParser->SetFileName(Files[0].toAscii().data());
  xmlParser->Parse();

  // Get the root element from the parser.
  vtkPVXMLElement *root = xmlParser->GetRootElement();
  if (root)
    {
    pqApplicationCore::instance()->loadState(root);
    QString name = root->GetName();
    }
  else
    {
    qCritical("Root does not exist. Either state file could not be opened "
              "or it does not contain valid xml");
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
  file_dialog->setFileMode(pqFileDialog::AnyFile);
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
  pqApplicationCore::instance()->saveState(root);

  /*
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

  */

  // Print the xml to the requested file(s).
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream os(Files[i].toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  root->Delete();
}
//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveScreenshot()
{
  pqRenderModule* rm = pqApplicationCore::instance()->getActiveRenderModule();
  if(!rm)
    {
    qDebug() << "Cannnot save image. No active render module.";
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
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveScreenshot(const QStringList& files)
{
  pqRenderModule* rm = pqApplicationCore::instance()->getActiveRenderModule();
  if(!rm)
    {
    qDebug() << "Cannnot save image. No active render module.";
    return;
    }

  for(int i = 0; i != files.size(); ++i)
    {
    if(!pqTestUtility::SaveScreenshot(rm->getWidget()->GetRenderWindow(), files[i]))
      {
      qCritical() << "Save Image failed.";
      }
    }
}
//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveAnimation()
{
  // Currently we only support saving animations in which reader's
  // timestep value changes.
  pqPipelineSource* source = pqApplicationCore::instance()->getActiveSource();
  if (!source)
    {
    qDebug() << "Cannot save animation, no reader selected.";
    return;
    }

  if (!pqSimpleAnimationManager::canAnimate(source))
    {
    qDebug() << "Cannot animate the selected source.";
    return;
    }


  QString filters = "MPEG files (*.mpg)";
#ifdef _WIN32
  filters += ";;AVI files (*.avi)";
#else
# ifdef VTK_USE_FFMPEG_ENCODER
  filters += ";;AVI files (*.avi)";
# endif
#endif
  filters +=";;JPEG images (*.jpg);; TIFF images (*.tif);; PNG images (*.png)";
  filters +=";;All files(*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this, tr("Save Animation:"), QString(), filters);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveAnimation(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onFileSaveAnimation(const QStringList& files)
{
  pqPipelineSource* source = pqApplicationCore::instance()->getActiveSource();
  if (!source)
    {
    qDebug() << "Cannot save animation, no reader selected.";
    return;
    }
  pqSimpleAnimationManager manager(this);
 cout << "status: " <<  manager.createTimestepAnimation(source, files[0]) << endl;
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRecordTestScreenshot()
{
  pqRenderModule* const render_module = pqApplicationCore::instance()->getActiveRenderModule();
  if(!render_module)
    {
    qDebug() << "Cannnot save image. No active render module.";
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
    this, tr("Save Test Screenshot:"), QString(), filters);
  file_dialog->setObjectName("RecordTestScreenshotDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onRecordTestScreenshot(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRecordTestScreenshot(const QStringList& files)
{
  pqRenderModule* render_module = pqApplicationCore::instance()->getActiveRenderModule();
  if(!render_module)
    {
    qDebug() << "Cannnot save image. No active render module.";
    return;
    }

  QSize old_size = render_module->getWidget()->size();
  render_module->getWidget()->resize(300,300);
  
  for(int i = 0; i != files.size(); ++i)
    {
    if(!pqTestUtility::SaveScreenshot(render_module->getWidget()->GetRenderWindow(), files[i]))
      {
      qCritical() << "Save Image failed.";
      }
    }
    
  render_module->getWidget()->resize(old_size);
  render_module->render();
}

//-----------------------------------------------------------------------------
bool pqMainWindow::compareView(const QString& referenceImage, double threshold, 
  ostream& output, const QString& tempDirectory)
{
  pqRenderModule* renModule = 
    pqApplicationCore::instance()->getActiveRenderModule();

  if (!renModule)
    {
    output << "ERROR: Could not locate the render module." << endl;
    }

  vtkRenderWindow* const render_window = 
    renModule->getRenderModuleProxy()->GetRenderWindow();

  if(!render_window)
    {
    output << "ERROR: Could not locate the Render Window." << endl;
    return false;
    }

  // All tests need a 300x300 render window size.
  QSize cur_size = renModule->getWidget()->size();
  renModule->getWidget()->resize(300,300);
  bool ret = pqTestUtility::CompareImage(render_window, referenceImage, 
    threshold, output, tempDirectory);
  renModule->getWidget()->resize(cur_size);
  renModule->render();
  return ret;
}

//-----------------------------------------------------------------------------
void pqMainWindow::onServerConnect()
{
  this->setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
    SLOT(onServerConnect(pqServer*)));
  server_browser->setModal(true);
  server_browser->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onServerConnect(pqServer* server)
{
  // bring the server object under the window so that when the
  // window is destroyed the server will get destroyed too.
  server->setParent(this);

  // Make the newly created server the active server.
  pqApplicationCore::instance()->setActiveServer(server);
  // Create a render module.
  this->Implementation->RenderWindowManager->onFrameAdded(
    qobject_cast<pqMultiViewFrame *>(
      this->Implementation->MultiViewManager->widgetOfIndex(
        pqMultiView::Index())));
  this->updateRecentlyLoadedFilesMenu(true);
}

//-----------------------------------------------------------------------------
void pqMainWindow::onServerDisconnect()
{
  // for now, they both are same.
  this->onFileNew();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onCreateSource(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString sourceName = action->data().toString();
  if (!pqApplicationCore::instance()->createSourceOnActiveServer(sourceName))
    {
    qCritical() << "Source could not be created.";
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString filterName = action->data().toString();
  if (!pqApplicationCore::instance()->createFilterForActiveSource(filterName))
    {
    qCritical() << "Source could not be created.";
    } 
}

//-----------------------------------------------------------------------------
void pqMainWindow::onOpenLinkEditor()
{
}

//-----------------------------------------------------------------------------
void pqMainWindow::onOpenCompoundFilterWizard()
{
  // TODO: pqCompoundProxyWizard should not need a server.
  pqCompoundProxyWizard* wizard = new pqCompoundProxyWizard(
    pqApplicationCore::instance()->getActiveServer(), this);
  wizard->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed

  this->connect(wizard, 
    SIGNAL(newCompoundProxy(const QString&, const QString&)), 
    SLOT(onCompoundProxyAdded(const QString&, const QString&)));
  wizard->show();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onDumpWidgetNames()
{
  const QString output = pqObjectNaming::DumpHierarchy();
  qDebug() << output;
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRecordTest()
{
  QString filters;
  filters += "XML files (*.xml)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this, tr("Record Test:"), QString(), filters);
  file_dialog->setObjectName("ToolsRecordTestDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onRecordTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onRecordTest(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    pqEventTranslator* const translator = new pqEventTranslator();
    pqTestUtility::Setup(*translator);
    
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
  file_dialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onPlayTest(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onPlayTest(const QStringList& Files)
{
  QApplication::processEvents();

  pqEventPlayer player;
  pqTestUtility::Setup(player);

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

void pqMainWindow::enableTooltips(bool enabled)
{
  if(enabled && !this->Implementation->ToolTipTrapper)
    {
    delete this->Implementation->ToolTipTrapper;
    this->Implementation->ToolTipTrapper = 0;
    }
  else if(this->Implementation->ToolTipTrapper)
    {
    this->Implementation->ToolTipTrapper = new pqToolTipTrapper();
    }
}

void pqMainWindow::onNewSelections(vtkSMProxy*, vtkUnstructuredGrid* selections)
{
  // Update the element inspector ...
  if(pqElementInspectorWidget* const element_inspector = 
    this->Implementation->ElementInspectorDock->findChild
    <pqElementInspectorWidget*>())
    {
    element_inspector->addElements(selections);
    }
}
//-----------------------------------------------------------------------------
void pqMainWindow::onCreateCompoundProxy(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString sourceName = action->data().toString();
  if (!pqApplicationCore::instance()->createCompoundSource(sourceName))
    {
    qCritical() << "Source could not be created.";
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onCompoundProxyAdded(const QString&, const QString& proxy)
{
  this->Implementation->CompoundProxyToolBar->addAction(
    QIcon(":/pqWidgets/pqBundle32.png"), proxy) 
    << pqSetName(proxy) << pqSetData(proxy);
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRemoveServer(pqServer *)
{
  if(!this->Implementation->MultiViewManager )
    {
    return;
    }

  /* FIXME
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
  */
}

void pqMainWindow::onAddWindow(QWidget *win)
{
  if(!win)
    {
    return;
    }

  /* FIXME
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
    */
}

//-----------------------------------------------------------------------------
void pqMainWindow::onCoreActiveChanged()
{
  if (this->Implementation->IgnoreBrowserSelectionChanges)
    {
    return;
    }

  this->Implementation->IgnoreBrowserSelectionChanges = true;
    
  pqPipelineSource* activeSource = 
    pqApplicationCore::instance()->getActiveSource();
  pqServer* activeServer = pqApplicationCore::instance()->getActiveServer();
 
  pqServerManagerModelItem* item = activeSource;
  if (!item)
    {
    item = activeServer;
    }

  emit this->select(item);
  

  this->Implementation->IgnoreBrowserSelectionChanges = false;
}

//-----------------------------------------------------------------------------
void pqMainWindow::onBrowserSelectionChanged(pqServerManagerModelItem* item)
{
  if (this->Implementation->IgnoreBrowserSelectionChanges)
    {
    return;
    }
  this->Implementation->IgnoreBrowserSelectionChanges = true;
  
  // Update the internal iVars that denote the active selections.
  pqServer* server = 0;
  pqPipelineSource* source = dynamic_cast<pqPipelineSource*>(item);
  if (source)
    {
    server = source->getServer();
    pqApplicationCore::instance()->setActiveServer(server);
    pqApplicationCore::instance()->setActiveSource(source);
    }
  else
    {
    server = dynamic_cast<pqServer*>(item);
    pqApplicationCore::instance()->setActiveSource(0);
    pqApplicationCore::instance()->setActiveServer(server);

    }
  this->Implementation->IgnoreBrowserSelectionChanges = false;
}

//-----------------------------------------------------------------------------
void pqMainWindow::onActiveSourceChanged(pqPipelineSource* src)
{
  if (this->Implementation->Inspector)
    {
    this->Implementation->Inspector->setProxy(
      (src)? src->getProxy() : NULL);
    }
  // Update the filters menu if there are no sources waiting for displays.
  // Updating the filters menu will cause the execution of a filter because
  // it's output is needed to check filter matches. We do not want the
  // filter to execute prematurely.
  if (pqApplicationCore::instance()->getNumberOfSourcesPendingDisplays() == 0)
    {
    this->updateFiltersMenu(src);
    }
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqMainWindow::preAcceptUpdate()
{
  this->setEnabled(false);
  if (this->statusBar()->isVisible())
    {
    this->statusBar()->showMessage(tr("Updating..."));
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::postAcceptUpdate()
{
  this->updateFiltersMenu(pqApplicationCore::instance()->getActiveSource());
  if (this->Implementation->DataInformationWidget)
    {
    this->Implementation->DataInformationWidget->refreshData();
    }
  this->setEnabled(true);
  if (this->statusBar()->isVisible())
    {
    this->statusBar()->showMessage(tr("Ready"), 2000);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::updateFiltersMenu(pqPipelineSource* source)
{
  if (!this->Implementation->FiltersMenu)
    {
    return;
    }
  // update the filter menu.
  if ( !this->Implementation->FiltersMenu )
    {
    return;
    }
  
  // Iterate over all filters in the menu and see if they are
  // applicable to the current source.
  QMenu* menu = this->filtersMenu();

  vtkSMProxy* input = (source)? source->getProxy() : NULL;
  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();

  if (!input)
    {
    menu->setEnabled(false);
    return;
    }

  QList<QString> supportedFilters;

  pqApplicationCore::instance()->getPipelineBuilder()->
    getSupportedProxies("filters", source->getServer(), supportedFilters);


  QList<QAction*> menu_actions = menu->findChildren<QAction*>();
  bool some_enabled = false;
  foreach(QAction* action, menu_actions)
    {
    if (!input)
      {
      action->setEnabled(false);
      continue;
      }
    QString filterName = action->data().toString();
    if (filterName.isEmpty())
      {
      continue;
      }
    if (!supportedFilters.contains(filterName))
      {
      // skip filters not supported by the server.
      action->setEnabled(false);
      continue;
      }
    vtkSMProxy* output = pxm->GetProxy("filters_prototypes",
      filterName.toStdString().c_str());
    if (!output)
      {
      action->setEnabled(false);
      continue;
      }
    vtkSMProxyProperty* smproperty = vtkSMProxyProperty::SafeDownCast(
      output->GetProperty("Input"));
    if (smproperty)
      {
      smproperty->RemoveAllUncheckedProxies();
      smproperty->AddUncheckedProxy(input);
      if (smproperty->IsInDomains())
        {
        action->setEnabled(true);
        some_enabled = true;
        continue;
        }
      }
    action->setEnabled(false);
    }

  menu->setEnabled(some_enabled);
}

//-----------------------------------------------------------------------------
void pqMainWindow::onActiveServerChanged(pqServer* )
{
  this->updateEnableState();
}

//-----------------------------------------------------------------------------
void pqMainWindow::updateEnableState()
{
  pqServer *server = pqApplicationCore::instance()->getActiveServer();
  pqPipelineSource *source = pqApplicationCore::instance()->getActiveSource();
  pqRenderModule* rm = pqApplicationCore::instance()->getActiveRenderModule();
  int num_servers = pqApplicationCore::instance()->
    getServerManagerModel()->getNumberOfServers();

  bool pending_displays = 
    (pqApplicationCore::instance()->getNumberOfSourcesPendingDisplays() > 0);

  if (this->Implementation->FileMenu)
    {
    QAction* openAction = this->fileMenu()->findChild<QAction*>("Open");
    QAction* saveDataAction = this->fileMenu()->findChild<QAction*>("SaveData");
    QAction* loadStateAction = 
      this->fileMenu()->findChild<QAction*>("LoadServerState");
    QAction* saveStateAction = 
      this->fileMenu()->findChild<QAction*>("SaveServerState");
    QAction* saveScreenshot =
      this->fileMenu()->findChild<QAction*>("SaveScreenshot");
    openAction->setEnabled(!pending_displays);
    saveDataAction->setEnabled(!pending_displays && source!=0);
    loadStateAction->setEnabled(
      !pending_displays && (!num_servers || server !=0));
    saveStateAction->setEnabled(
      !pending_displays && server !=0);
    saveScreenshot->setEnabled(server != 0 && rm != 0);
    this->updateRecentlyLoadedFilesMenu(!pending_displays && (!num_servers || server !=0));
    }

  if (this->Implementation->ServerMenu)
    {
    QAction* connectAction = this->serverMenu()->findChild<QAction*>("Connect");
    QAction* disconnectAction = 
      this->serverMenu()->findChild<QAction*>("Disconnect");
    connectAction->setEnabled(num_servers==0);
    disconnectAction->setEnabled(server != 0);
    }

  if ( this->Implementation->SourcesMenu )
    {
    this->Implementation->SourcesMenu->setEnabled(
      server != 0 && !pending_displays);
    }

  if ( this->Implementation->VariableSelectorToolBar )
    {
    this->Implementation->VariableSelectorToolBar->setEnabled(
      source != 0 && !pending_displays);
    }

  if ( this->Implementation->FiltersMenu )
    {
    this->Implementation->FiltersMenu->setEnabled(
      source != 0 && server != 0 && !pending_displays);
    }

  if (this->Implementation->ToolsMenu)
    {
    QAction* compoundFilterAction = 
      this->Implementation->ToolsMenu->findChild<QAction*>( "CompoundFilter");

    compoundFilterAction->setEnabled(server != 0 && !pending_displays);
    }

  if (this->Implementation->SelectionToolBar)
    {
    if (rm)
      {
      this->Implementation->SelectionToolBar->setEnabled(true);
      }
    else
      {
      this->Implementation->SelectionToolBar->setEnabled(false);
      }
    }
  if (this->Implementation->FileMenu)
    {
    QAction* saveAnimation = 
      this->Implementation->FileMenu->findChild<QAction*>("SaveAnimation");
    if (saveAnimation)
      {
      saveAnimation->setEnabled(pqSimpleAnimationManager::canAnimate(
          pqApplicationCore::instance()->getActiveSource()));
      }
    }

  this->updateInteractionUndoRedoState();
}

//-----------------------------------------------------------------------------
void pqMainWindow::onUndoRedoStackChanged(bool canUndo, QString undoText,
  bool canRedo, QString redoText)
{
  QAction* undoButton = 0;
  QAction* redoButton = 0;


  if (this->Implementation->UndoRedoToolBar)
    {
    undoButton = this->Implementation->UndoRedoToolBar->findChild<QAction*>(
      "UndoButton");
    redoButton = this->Implementation->UndoRedoToolBar->findChild<QAction*>(
      "RedoButton");
    if (undoButton)
      {
      undoButton->setEnabled(canUndo); 
      }

    if (redoButton)
      {
      redoButton->setEnabled(canRedo);
      }
    }

  if (this->Implementation->EditMenu)
    {
    undoButton = this->Implementation->EditMenu->findChild<QAction*>("Undo");
    if (undoButton)
      {
      undoButton->setEnabled(canUndo);
      QString text = (canUndo? "&Undo " + undoText : "Can't &Undo");
      undoButton->setText(text);
      }

    redoButton = this->Implementation->EditMenu->findChild<QAction*>("Redo");
    if (redoButton)
      {
      redoButton->setEnabled(canRedo);
      QString text = (canRedo? "&Redo " + redoText : "Can't &Redo");
      redoButton->setText(text);
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onActiveRenderModuleChanged(pqRenderModule* rm)
{
  this->updateEnableState();
  if (rm && rm->getWidget())
    {
    rm->getWidget()->installEventFilter(this);
    }
  if (rm)
    {
    QObject::connect(rm->getInteractionUndoStack(), 
      SIGNAL(StackChanged(bool, QString, bool, QString)),
        this, SLOT(updateInteractionUndoRedoState()));
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::UndoActiveViewInteraction()
{
  pqRenderModule* view = pqApplicationCore::instance()->getActiveRenderModule();
  if (!view)
    {
    qDebug() << "No active render module, cannot UndoActiveViewInteraction.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->Undo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindow::RedoActiveViewInteraction()
{
  pqRenderModule* view = pqApplicationCore::instance()->getActiveRenderModule();
  if (!view)
    {
    qDebug() << "No active render module, cannot RedoActiveViewInteraction.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->Redo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindow::updateInteractionUndoRedoState()
{
  if (this->Implementation->EditMenu)
    {
    QAction* undoAction = 
      this->Implementation->EditMenu->findChild<QAction*>("CameraUndo");
    QAction* redoAction = 
      this->Implementation->EditMenu->findChild<QAction*>("CameraRedo");

    bool canUndo = false;
    bool canRedo = false;
    QString undoMsg = "Can't U&ndo Interaction";
    QString redoMsg = "Can't R&edo Interaction";
    
    pqRenderModule* rm = pqApplicationCore::instance()->getActiveRenderModule();
    if (rm)
      {
      pqUndoStack* stack = rm->getInteractionUndoStack();
      if (stack && stack->CanUndo())
        {
        canUndo = true;
        undoMsg = "U&ndo Interaction";
        }
      if (stack && stack->CanRedo())
        {
        canRedo = true;
        redoMsg = "R&edo Interaction";
        }
      }

    undoAction->setEnabled(canUndo);
    undoAction->setText(undoMsg);
    redoAction->setEnabled(canRedo);
    redoAction->setText(redoMsg);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::addFileToRecentList(const QString& fileName)
{
  this->Implementation->RecentFilesList.removeAll(fileName);
  this->Implementation->RecentFilesList.push_front(fileName);
  pqApplicationCore::instance()->settings()->setRecentFilesList(
    this->Implementation->RecentFilesList);
  while ( this->Implementation->RecentFilesList.size() >
    this->Implementation->RecentFilesListMaxmumLength )
    {
    this->Implementation->RecentFilesList.removeLast();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::updateRecentlyLoadedFilesMenu(bool enabled)
{
  QMenu* const rfMenu = this->recentFilesMenu();
  rfMenu->clear();
  int cnt = 0;
  if ( !pqApplicationCore::instance()->getActiveServer() )
    {
    enabled = false;
    }
  foreach(QString file, this->Implementation->RecentFilesList)
    {
    QString str = "&" + QString().setNum(cnt);
    str += " " + file;
    QAction *qa = rfMenu->addAction(tr(str.toStdString().c_str()),
      this, SLOT(onRecentFileOpen()));
    qa->setData(QVariant(file));
    qa->setEnabled(enabled);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindow::onRecentFileOpen()
{
  if(!pqApplicationCore::instance()->getActiveServer())
    {
    qDebug() << "No active server selected.";
    return;
    }
  QVariant qv = qobject_cast<QAction*>(this->sender())->data();
  QString str = qv.toString();
  QStringList strList;
  strList << str;
  this->onFileOpen(strList);
}
