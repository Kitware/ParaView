/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAboutDialog.h"
#include "pqConfig.h"
#include "pqConnect.h"
#include "pqFileDialog.h"
#include "pqHistogramWidget.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqMultiViewManager.h"
#include "pqMultiViewFrame.h"
#include "pqObjectInspector.h"
#include "pqObjectInspectorWidget.h"
#include "pqParts.h"
#include "pqPipelineData.h"
#include "pqPipelineListModel.h"
#include "pqPipelineListWidget.h"
#include "pqRenderViewProxy.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"
#include "pqPicking.h"
#include "pqDataSetModel.h"

// TEMP
#include "pqChartValue.h"
#include "pqHistogramChart.h"

#ifdef PARAQ_EMBED_PYTHON
#include "pqPythonDialog.h"
#endif // PARAQ_EMBED_PYTHON

#include <pqImageComparison.h>

#include <QApplication>
#include <QCheckBox>
#include <QDockWidget>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QToolBar>
#include <QTreeView>
#include <QTableView>
#include <QSignalMapper>

#include <vtkRenderWindow.h>
#include <vtkSMDataObjectDisplayProxy.h>
#include <vtkSMDisplayProxy.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSMXMLParser.h>
#include <vtkTesting.h>
#include <vtkPVGenericRenderWindowInteractor.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyle.h>
#include <vtkUnstructuredGrid.h>

#include <QVTKWidget.h>
#include <vtkEventQtSlotConnect.h>

#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqRecordEventsDialog.h>

pqMainWindow::pqMainWindow() :
  CurrentServer(0),
  PropertyToolbar(0),
  MultiViewManager(0),
  ServerDisconnectAction(0),
  Adaptor(0),
  Pipeline(0),
  Inspector(0),
  InspectorDock(0),
  PipelineList(0),
  PipelineDock(0),
  ChartWidget(0),
  ChartDock(0),
  ActiveView(0),
  ElementInspectorWidget(0),
  ElementInspectorDock(0)
{
  this->setObjectName("mainWindow");
  this->setWindowTitle(QByteArray("ParaQ Client") + QByteArray(" ") + QByteArray(QT_CLIENT_VERSION));

  this->menuBar() << pqSetName("menuBar");

  // File menu.
  QMenu* const fileMenu = this->menuBar()->addMenu(tr("File"))
    << pqSetName("fileMenu");
  
  fileMenu->addAction(tr("New..."))
    << pqSetName("New")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileNew()));
    
  fileMenu->addAction(tr("Open..."))
    << pqSetName("Open")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileOpen()));
    
  fileMenu->addAction(tr("Save Server State..."))
    << pqSetName("SaveServerState")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveServerState()));
   
  fileMenu->addAction(tr("Save Screenshot..."))
    << pqSetName("SaveScreenshot")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onFileSaveScreenshot())); 
  
  fileMenu->addAction(tr("Quit"))
    << pqSetName("Quit")
    << pqConnect(SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));

  // Server menu.
  QMenu* const serverMenu = this->menuBar()->addMenu(tr("Server"))
    << pqSetName("serverMenu");
  
  serverMenu->addAction(tr("Connect..."))
    << pqSetName("Connect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerConnect()));

  this->ServerDisconnectAction = serverMenu->addAction(tr("Disconnect"))
    << pqSetName("Disconnect")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onServerDisconnect()));    

  // Sources & Filters menus.
  this->SourcesMenu = this->menuBar()->addMenu(tr("Sources"))
    << pqSetName("sourcesMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateSource(QAction*)));
    
  this->FiltersMenu = this->menuBar()->addMenu(tr("Filters"))
    << pqSetName("filtersMenu")
    << pqConnect(SIGNAL(triggered(QAction*)), this, SLOT(onCreateFilter(QAction*)));
  
  QObject::connect(this, SIGNAL(serverChanged()), SLOT(onUpdateSourcesFiltersMenu()));

  // Tools menu.
  QMenu* toolsMenu = this->menuBar()->addMenu(tr("Tools")) << pqSetName("toolsMenu");
  toolsMenu->addAction(tr("Link Editor..."))
    << pqConnect(SIGNAL(triggered(bool)), this, SLOT(onOpenLinkEditor()));

  // Test menu.
  QMenu* const testsMenu = this->menuBar()->addMenu(tr("Tests"))
    << pqSetName("testsMenu");
  
  testsMenu->addAction(tr("Record"))
    << pqSetName("Record")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onRecordTest()));
    
  testsMenu->addAction(tr("Play"))
    << pqSetName("Play")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPlayTest()));
    
#ifdef PARAQ_EMBED_PYTHON

  testsMenu->addAction(tr("Python Shell"))
    << pqSetName("PythonShell")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onPythonShell()));
    
#endif // PARAQ_EMBED_PYTHON

  // Help menu.
  QMenu* const helpMenu = this->menuBar()->addMenu(tr("Help"))
    << pqSetName("helpMenu");
  
  helpMenu->addAction(QString(tr("About %1 %2")).arg("ParaQ").arg(QT_CLIENT_VERSION))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onHelpAbout()));
 
  // Create the pipeline instance.
  this->Pipeline = new pqPipelineData(this);

  // Set up the dock window corners to give the vertical docks
  // more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Add the pipeline list dock window.
  this->PipelineDock = new QDockWidget("Pipeline Inspector", this);
  if(this->PipelineDock)
    {
    this->PipelineDock->setObjectName("PipelineDock");
    this->PipelineDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->PipelineDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    // Make sure the pipeline data instance is created before the
    // pipeline list model. This ensures that the connections will
    // work.
    this->PipelineList = new pqPipelineListWidget(this->PipelineDock);
    if(this->PipelineList)
      {
      this->PipelineList->setObjectName("PipelineList");
      this->PipelineDock->setWidget(this->PipelineList);
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->PipelineDock);
    }

  // Add the object inspector dock window.
  this->InspectorDock = new QDockWidget("Object Inspector", this);
  if(this->InspectorDock)
    {
    this->InspectorDock->setObjectName("InspectorDock");
    this->InspectorDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->InspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);

    this->Inspector = new pqObjectInspectorWidget(this->InspectorDock);
    if(this->Inspector)
      {
      this->Inspector->setObjectName("Inspector");
      this->InspectorDock->setWidget(this->Inspector);
      if(this->PipelineList)
        {
        connect(this->PipelineList, SIGNAL(proxySelected(vtkSMSourceProxy *)),
            this->Inspector, SLOT(setProxy(vtkSMSourceProxy *)));
        }
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->InspectorDock);
    }

  // Add the chart dock window.
  this->ChartDock = new QDockWidget("Chart View", this);
  if(this->ChartDock)
    {
    this->ChartDock->setObjectName("ChartDock");
    this->ChartDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    this->ChartDock->setAllowedAreas(Qt::BottomDockWidgetArea |
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    this->ChartWidget = new pqHistogramWidget(this->ChartDock);
    if(this->ChartWidget)
      {
      this->ChartWidget->setObjectName("ChartWidget");
      this->ChartDock->setWidget(this->ChartWidget);

      // TEMP: Put in some fake data.
      pqChartValueList list;
      list.pushBack(pqChartValue((float)1.35));
      list.pushBack(pqChartValue((float)1.40));
      list.pushBack(pqChartValue((float)1.60));
      list.pushBack(pqChartValue((float)2.00));
      list.pushBack(pqChartValue((float)1.50));
      list.pushBack(pqChartValue((float)1.80));
      list.pushBack(pqChartValue((float)1.40));
      list.pushBack(pqChartValue((float)1.30));
      list.pushBack(pqChartValue((float)1.20));
      pqChartValue min((int)0);
      pqChartValue interval((int)10);
      this->ChartWidget->getHistogram()->setData(list, min, interval);
      }

    this->addDockWidget(Qt::BottomDockWidgetArea, this->ChartDock);
    }
  
  // Add the element inspector dock window.
  this->ElementInspectorDock = new QDockWidget("Element Inspector View", this);
  if(this->ElementInspectorDock)
    {
    this->ElementInspectorDock->setObjectName("ElementInspectorDock");
    this->ElementInspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    this->ElementInspectorDock->setAllowedAreas(Qt::BottomDockWidgetArea |
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    this->ElementInspectorWidget = new QTableView(this->ElementInspectorDock);
    if(this->ElementInspectorWidget)
      {
      this->ElementInspectorWidget->setObjectName("ElementInspectorWidget");
      this->ElementInspectorDock->setWidget(this->ElementInspectorWidget);
      }

    this->addDockWidget(Qt::BottomDockWidgetArea, this->ElementInspectorDock);
    }

  this->setServer(0);
  this->Adaptor = new pqSMAdaptor;

  this->VTKConnector = vtkEventQtSlotConnect::New();
  
}

pqMainWindow::~pqMainWindow()
{
  // Clean up the model before deleting the adaptor.
  if(this->Inspector)
    {
    delete this->Inspector;
    this->Inspector = 0;
    }
  
  // clean up multiview before server
  if(this->MultiViewManager)
    {
    delete this->MultiViewManager;
    this->MultiViewManager = 0;
    }

  this->VTKConnector->Delete();
  delete this->PropertyToolbar;
  delete this->CurrentServer;
  delete this->Adaptor;
}

void pqMainWindow::setServer(pqServer* Server)
{
  if(this->Pipeline)
    {
    this->Pipeline->clearViewMapping();
    this->Pipeline->removeServer(this->CurrentServer);
    }

  if(this->MultiViewManager)
    {
    delete this->MultiViewManager;
    this->MultiViewManager = 0;
    }

  delete this->PropertyToolbar;
  this->PropertyToolbar = 0;

  delete this->CurrentServer;
  this->CurrentServer = 0;

  if(Server)
    {
    this->CurrentServer = Server;

    this->Pipeline->addServer(this->CurrentServer);

    this->MultiViewManager = new pqMultiViewManager(this) << pqSetName("MultiViewManager");
    this->MultiViewManager->hide();  // workaround for flickering in Qt 4.0.1
    QObject::connect(this->MultiViewManager, SIGNAL(frameAdded(pqMultiViewFrame*)), this, SLOT(onNewQVTKWidget(pqMultiViewFrame*)));
    QObject::connect(this->MultiViewManager, SIGNAL(frameRemoved(pqMultiViewFrame*)), this, SLOT(onDeleteQVTKWidget(pqMultiViewFrame*)));
    this->setCentralWidget(this->MultiViewManager);
    this->MultiViewManager->show();  // workaround for flickering in Qt 4.0.1
    }
  
  this->ServerDisconnectAction->setEnabled(this->CurrentServer);
  emit serverChanged();
}

void pqMainWindow::onFileNew()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileNew(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileNew(pqServer* Server)
{
  setServer(Server);
}

void pqMainWindow::onFileOpen()
{
  if(!this->CurrentServer)
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this);
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpen(pqServer*)));
    server_browser->show();
    }
  else
    {
    this->onFileOpen(this->CurrentServer);
    }
}

void pqMainWindow::onFileOpen(pqServer* Server)
{
  if(this->CurrentServer != Server)
    setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpen(const QStringList& Files)
{
  if(!this->Pipeline || !this->PipelineList)
    return;
    
  QVTKWidget *window = this->PipelineList->getCurrentWindow();
  if(window)
    {
    vtkSMProxy* source = 0;
    vtkSMRenderModuleProxy* rm = this->Pipeline->getRenderModule(window);
    for(int i = 0; i != Files.size(); ++i)
      {
      QString file = Files[i];
      
      source = this->Pipeline->createSource("ExodusReader", window);
      this->CurrentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
      source->Delete();
      Adaptor->setProperty(source->GetProperty("FileName"), file);
      Adaptor->setProperty(source->GetProperty("FilePrefix"), file);
      Adaptor->setProperty(source->GetProperty("FilePattern"), "%s");
      source->UpdateVTKObjects();
      this->Pipeline->setVisibility(source, true);
      }

    rm->ResetCamera();
    window->update();

    // Select the latest source in the pipeline inspector.
    if(source)
      this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onFileOpenServerState()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, SLOT(onFileOpenServerState(pqServer*)));
  server_browser->show();
}

void pqMainWindow::onFileOpenServerState(pqServer* Server)
{
  setServer(Server);

  pqFileDialog* const file_dialog = new pqFileDialog(new pqServerFileDialogModel(this->CurrentServer->GetProcessModule()), tr("Open Server State File:"), this, "fileOpenDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(pnFileOpenServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileOpenServerState(const QStringList& Files)
{
}

void pqMainWindow::onFileSaveServerState()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Dump Server State:"), tr("No server connections to serialize"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Server State:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveServerState(const QStringList& Files)
{
  for(int i = 0; i != Files.size(); ++i)
    {
    this->CurrentServer->GetProxyManager()->SaveState(Files[i].toAscii().data());
    }
}

void pqMainWindow::onFileSaveScreenshot()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No server connections to save"));
    return;
    }

  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;

  if(!render_window)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No render window to save"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Screenshot:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

void pqMainWindow::onFileSaveScreenshot(const QStringList& Files)
{
  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;

  for(int i = 0; i != Files.size(); ++i)
    {
    if(!pqSaveScreenshot(render_window, Files[i]))
      QMessageBox::critical(this, tr("Save Screenshot:"), tr("Error saving file"));
    }
}

bool pqMainWindow::compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory)
{
  vtkRenderWindow* const render_window =
    this->ActiveView ? qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow() : 0;
    
  if(!render_window)
    return false;
    
  return pqCompareImage(render_window, ReferenceImage, Threshold, Output, TempDirectory);
}

void pqMainWindow::onServerConnect()
{
  setServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(this);
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

void pqMainWindow::onUpdateWindows()
{
  /*
  if(this->CurrentServer)
    this->CurrentServer->GetRenderModule()->StillRender();
    */
}

void pqMainWindow::onUpdateSourcesFiltersMenu()
{
  this->FiltersMenu->clear();
  this->SourcesMenu->clear();

  if(this->CurrentServer)
    {
    vtkSMProxyManager* manager = this->CurrentServer->GetProxyManager();
    manager->InstantiateGroupPrototypes("filters");
    int numFilters = manager->GetNumberOfProxies("filters_prototypes");
    for(int i=0; i<numFilters; i++)
      {
      const char* proxyName = manager->GetProxyName("filters_prototypes",i);
      this->FiltersMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }

#if 1
    // hard code sources
    this->SourcesMenu->addAction("2D Glyph") << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    this->SourcesMenu->addAction("3D Text") << pqSetName("3D Text") << pqSetData("VectorText");
    this->SourcesMenu->addAction("Arrow") << pqSetName("Arrow") << pqSetData("ArrowSource");
    this->SourcesMenu->addAction("Axes") << pqSetName("Axes") << pqSetData("Axes");
    this->SourcesMenu->addAction("Box") << pqSetName("Box") << pqSetData("CubeSource");
    this->SourcesMenu->addAction("Cone") << pqSetName("Cone") << pqSetData("ConeSource");
    this->SourcesMenu->addAction("Cylinder") << pqSetName("Cylinder") << pqSetData("CylinderSource");
    this->SourcesMenu->addAction("Hierarchical Fractal") << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
    this->SourcesMenu->addAction("Line") << pqSetName("Line") << pqSetData("LineSource");
    this->SourcesMenu->addAction("Mandelbrot") << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
    this->SourcesMenu->addAction("Plane") << pqSetName("Plane") << pqSetData("PlaneSource");
    this->SourcesMenu->addAction("Sphere") << pqSetName("Sphere") << pqSetData("SphereSource");
    this->SourcesMenu->addAction("Superquadric") << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
    this->SourcesMenu->addAction("Wavelet") << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
#else
    manager->InstantiateGroupPrototypes("sources");
    int numSources = manager->GetNumberOfProxies("sources_prototypes");
    for(int i=0; i<numSources; i++)
      {
      const char* proxyName = manager->GetProxyName("sources_prototypes",i);
      this->SourcesMenu->addAction(proxyName) << pqSetName(proxyName) << pqSetData(proxyName);
      }
#endif
    }
}

void pqMainWindow::onCreateSource(QAction* action)
{
  if(!action || !this->Pipeline || !this->PipelineList)
    return;

  QByteArray sourceName = action->data().toString().toAscii();
  QVTKWidget* window = this->PipelineList->getCurrentWindow();
  if(window)
    {
    vtkSMProxy* source = this->Pipeline->createSource(sourceName, window);
    this->Pipeline->setVisibility(source, true);
    vtkSMRenderModuleProxy* rm = this->Pipeline->getRenderModule(window);
    rm->ResetCamera();
    window->update();
    this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action || !this->Pipeline || !this->PipelineList)
    return;
  
  QByteArray filterName = action->data().toString().toAscii();
  vtkSMProxy *current = this->PipelineList->getSelectedProxy();
  QVTKWidget *window = this->PipelineList->getCurrentWindow();
  if(current && window)
    {
    vtkSMProxy *source = 0;
    vtkSMProxy *next = this->PipelineList->getNextProxy();
    if(next)
      {
      this->PipelineList->getListModel()->beginCreateAndInsert();
      source = this->Pipeline->createFilter(filterName, window);
      this->Pipeline->addInput(source, current);
      this->Pipeline->addInput(next, source);
      this->Pipeline->removeInput(next, current);
      this->PipelineList->getListModel()->finishCreateAndInsert();
      }
    else
      {
      this->PipelineList->getListModel()->beginCreateAndAppend();
      source = this->Pipeline->createFilter(filterName, window);
      this->Pipeline->addInput(source, current);
      this->PipelineList->getListModel()->finishCreateAndAppend();

      // Only turn on visibility for added filters?
      this->Pipeline->setVisibility(source, true);
      }

    vtkSMRenderModuleProxy *rm = this->Pipeline->getRenderModule(window);
    rm->ResetCamera();
    window->update();
    this->PipelineList->selectProxy(source);
    }
}

void pqMainWindow::onOpenLinkEditor()
{
}


void pqMainWindow::onHelpAbout()
{
  pqAboutDialog* const dialog = new pqAboutDialog(this);
  dialog->show();
}

void pqMainWindow::onRecordTest()
{
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Record Test:"), this, "fileSaveDialog");
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

class pqMultiViewRenderModuleUpdater : public QObject
{
public:
  pqMultiViewRenderModuleUpdater(vtkSMProxy* view, QWidget* topWidget, QWidget* parent)
    : QObject(parent), View(view), TopWidget(topWidget) {}

protected:
  bool eventFilter(QObject* caller, QEvent* e)
    {
    // TODO, apparently, this should watch for window position changes, not resizes
    if(e->type() == QEvent::Resize)
      {
      // find top level window;
      QWidget* me = qobject_cast<QWidget*>(caller);
      
      vtkSMIntVectorProperty* prop = 0;
      
      // set size of main window
      prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("GUISize"));
      if(prop)
        {
        prop->SetElements2(this->TopWidget->width(), this->TopWidget->height());
        }
      
      // position relative to main window
      prop = vtkSMIntVectorProperty::SafeDownCast(this->View->GetProperty("WindowPosition"));
      if(prop)
        {
        QPoint pos(0,0);
        pos = me->mapTo(this->TopWidget, pos);
        prop->SetElements2(pos.x(), pos.y());
        }
      }
    return false;
    }

  vtkSMProxy* View;
  QWidget* TopWidget;

};

void pqMainWindow::onNewQVTKWidget(pqMultiViewFrame* parent)
{
  vtkSMMultiViewRenderModuleProxy* rm = this->CurrentServer->GetRenderModule();
  vtkSMRenderModuleProxy* view = vtkSMRenderModuleProxy::SafeDownCast(rm->NewRenderModule());

  // if this property exists (server/client mode), render remotely
  // this should change to a user controlled setting, but this is here for testing
  vtkSMProperty* prop = view->GetProperty("CompositeThreshold");
  if(prop)
    {
    this->Adaptor->setProperty(prop, 0.0);  // remote render
    }
  view->UpdateVTKObjects();


  QVTKWidget* w = new QVTKWidget(parent);
  parent->setMainWidget(w);


  // gotta tell SM about window positions
  pqMultiViewRenderModuleUpdater* u = new pqMultiViewRenderModuleUpdater(view, this->MultiViewManager, w);
  w->installEventFilter(u);

  w->SetRenderWindow(view->GetRenderWindow());

  pqRenderViewProxy* vp = pqRenderViewProxy::New();
  vp->SetRenderModule(view);
  vtkPVGenericRenderWindowInteractor* iren = vtkPVGenericRenderWindowInteractor::SafeDownCast(view->GetInteractor());
  iren->SetPVRenderView(vp);
  vp->Delete();
  iren->Enable();
  
  pqPicking* picking = new pqPicking(view, w);
  this->VTKConnector->Connect(iren, vtkCommand::CharEvent, 
                              picking, SLOT(computeSelection(vtkObject*,unsigned long, void*, void*, vtkCommand*)),
                              NULL, 1.0);
  QObject::connect(picking, SIGNAL(selectionChanged(vtkSMSourceProxy*, vtkUnstructuredGrid*)),
                   this, SLOT(onNewSelections(vtkSMSourceProxy*, vtkUnstructuredGrid*)));

  // Keep a map of window to render module. Add the new window to the
  // pipeline data structure.
  this->Pipeline->addViewMapping(w, view);
  this->Pipeline->addWindow(w, this->CurrentServer);
  this->PipelineList->selectWindow(w);

  QSignalMapper* sm = new QSignalMapper(parent);
  sm->setMapping(parent, parent);
  QObject::connect(parent, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), this, SLOT(onFrameActive(QWidget*)));

  //parent->setActive(1);
  
}

void pqMainWindow::onDeleteQVTKWidget(pqMultiViewFrame* parent)
{
  QVTKWidget* w = qobject_cast<QVTKWidget*>(parent->mainWidget());
  vtkSMRenderModuleProxy* rm = this->Pipeline->removeViewMapping(w);

  // delete render module
  rm->Delete();

  // Remove the window from the pipeline data structure.
  this->Pipeline->removeWindow(w);

  if(this->ActiveView == parent)
    {
    this->ActiveView = 0;
    }
}

void pqMainWindow::onFrameActive(QWidget* w)
{
  if(this->ActiveView && this->ActiveView != w)
    {
    pqMultiViewFrame* f = qobject_cast<pqMultiViewFrame*>(this->ActiveView);
    if(f->active())
      f->setActive(0);
    }

  this->ActiveView = qobject_cast<pqMultiViewFrame*>(w);
}

void pqMainWindow::onNewSelections(vtkSMSourceProxy*, vtkUnstructuredGrid* selections)
{
  QAbstractItemModel* oldModel = this->ElementInspectorWidget->model();

  pqDataSetModel* newModel = new pqDataSetModel(this->ElementInspectorWidget);
  this->ElementInspectorWidget->setModel(newModel);
  newModel->setDataSet(selections);
  
  if(oldModel)
    {
    delete oldModel;
    }

  this->ElementInspectorWidget->update();

}


