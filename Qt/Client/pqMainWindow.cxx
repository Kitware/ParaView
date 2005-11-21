/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqAboutDialog.h"
#include "pqCommandDispatcher.h"
#include "pqCommandDispatcherManager.h"
#include "pqConfig.h"
#include "pqConnect.h"
#include "pqFileDialog.h"
#include "pqLocalFileDialogModel.h"
#include "pqMainWindow.h"
#include "pqObjectInspector.h"
#include "pqObjectInspectorDelegate.h"
#include "pqParts.h"
#include "pqRefreshToolbar.h"
#include "pqRenderViewProxy.h"
#include "vtkSMMultiViewRenderModuleProxy.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqSetData.h"
#include "pqSetName.h"
#include "pqSMAdaptor.h"
#include "pqPipelineData.h"
#include "pqMultiViewManager.h"
#include "pqMultiViewFrame.h"

#ifdef PARAQ_EMBED_PYTHON
#include "pqPythonDialog.h"
#endif // PARAQ_EMBED_PYTHON

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
#include <vtkWindowToImageFilter.h>
#include <vtkBMPWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkPNMWriter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkErrorCode.h>
#include <vtkSmartPointer.h>
#include <vtkPNGReader.h>
#include <vtkImageDifference.h>
#include <vtkImageShiftScale.h>

#include <QVTKWidget.h>

#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqRecordEventsDialog.h>

pqMainWindow::pqMainWindow() :
  CurrentServer(0),
  RefreshToolbar(0),
  PropertyToolbar(0),
  MultiViewManager(0),
  ServerDisconnectAction(0),
  Inspector(0),
  InspectorDelegate(0),
  InspectorDock(0),
  InspectorView(0),
  ActiveView(0)
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
 
  // Setup the refresh toolbar.
  QObject::connect(&pqCommandDispatcherManager::Instance(), SIGNAL(updateWindows()), this, SLOT(onUpdateWindows()));
  this->RefreshToolbar = new pqRefreshToolbar(this);
  this->addToolBar(this->RefreshToolbar);

  // Create the object inspector model.
  this->Inspector = new pqObjectInspector(this);
  if(this->Inspector)
    this->Inspector->setObjectName("Inspector");

  this->InspectorDelegate = new pqObjectInspectorDelegate(this);

  // Add the object inspector dock window.
  this->InspectorDock = new QDockWidget("Object Inspector", this);
  if(this->InspectorDock)
    {
    this->InspectorDock->setObjectName("InspectorDock");
    this->InspectorDock->setAllowedAreas(
        Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    this->InspectorDock->setFeatures(QDockWidget::AllDockWidgetFeatures);
    this->InspectorView = new QTreeView(this->InspectorDock);
    if(this->InspectorView)
      {
      this->InspectorView->setObjectName("InspectorView");
      this->InspectorView->setAlternatingRowColors(true);
      this->InspectorView->header()->hide();
      this->InspectorDock->setWidget(this->InspectorView);
      this->InspectorView->setModel(this->Inspector);
      if(this->InspectorDelegate)
        this->InspectorView->setItemDelegate(this->InspectorDelegate);
      }

    this->addDockWidget(Qt::LeftDockWidgetArea, this->InspectorDock);
    }

  this->setServer(0);
  this->Adaptor = new pqSMAdaptor;  // should go in pqServer?

}

pqMainWindow::~pqMainWindow()
{
  // Clean up the model before deleting the adaptor.
  if(this->Inspector)
  {
    if(this->InspectorView)
      this->InspectorView->setModel(0);
    delete this->Inspector;
  }
  
  // clean up multiview before server
  if(this->MultiViewManager)
    {
    delete this->MultiViewManager;
    this->MultiViewManager = 0;
    }

  delete this->PropertyToolbar;
  delete this->RefreshToolbar;
  delete this->CurrentServer;
  delete this->Adaptor;
}

void pqMainWindow::setServer(pqServer* Server)
{
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

    this->MultiViewManager = new pqMultiViewManager(this) << pqSetName("MultiViewManager");
    QObject::connect(this->MultiViewManager, SIGNAL(frameAdded(pqMultiViewFrame*)), this, SLOT(onNewQVTKWidget(pqMultiViewFrame*)));
    QObject::connect(this->MultiViewManager, SIGNAL(frameRemoved(pqMultiViewFrame*)), this, SLOT(onDeleteQVTKWidget(pqMultiViewFrame*)));
    this->setCentralWidget(this->MultiViewManager);
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
  if(!this->ActiveView)
    return;
    
  QVTKWidget* w = qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget());
  vtkSMRenderModuleProxy* rm = this->GraphicsViews[w];

  for(int i = 0; i != Files.size(); ++i)
    {
    QString file = Files[i];
    
    vtkSMProxy* source = this->CurrentServer->GetPipelineData()->newSMProxy("sources", "ExodusReader");
    this->CurrentServer->GetProxyManager()->RegisterProxy("paraq", "source1", source);
    source->Delete();
    Adaptor->setProperty(source->GetProperty("FileName"), file);
    Adaptor->setProperty(source->GetProperty("FilePrefix"), file);
    Adaptor->setProperty(source->GetProperty("FilePattern"), "%s");
    source->UpdateVTKObjects();
    
    pqAddPart(rm, vtkSMSourceProxy::SafeDownCast(source));
    if(this->Inspector)
      this->Inspector->setProxy(this->Adaptor, source);
    }

  rm->ResetCamera();
  w->update();
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
    ofstream file(Files[i].toAscii().data());
    file << "<ServerState>" << "\n";
    this->CurrentServer->GetProxyManager()->SaveState("test", &file, 0);
    file << "</ServerState>" << "\n";
    }
}

void pqMainWindow::onFileSaveScreenshot()
{
  if(!this->CurrentServer)
    {
    QMessageBox::critical(this, tr("Save Screenshot:"), tr("No server connections to save"));
    return;
    }

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save Screenshot:"), this, "fileSaveDialog");
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

template<typename WriterT>
bool saveImage(vtkWindowToImageFilter* Capture, const QFileInfo& File)
{
  WriterT* const writer = WriterT::New();
  writer->SetInput(Capture->GetOutput());
  writer->SetFileName(File.filePath().toAscii().data());
  writer->Write();
  const bool result = writer->GetErrorCode() == vtkErrorCode::NoError;
  writer->Delete();
  
  return result;
}

void pqMainWindow::onFileSaveScreenshot(const QStringList& Files)
{
  /*
  vtkWindowToImageFilter* const capture = vtkWindowToImageFilter::New();
  capture->SetInput(this->Window->GetRenderWindow());
  capture->Update();

  for(int i = 0; i != Files.size(); ++i)
    {
    const QFileInfo file(Files[i]);
    
    bool success = false;
    
    if(file.completeSuffix() == "bmp")
      success = saveImage<vtkBMPWriter>(capture, file);
    else if(file.completeSuffix() == "tif")
      success = saveImage<vtkTIFFWriter>(capture, file);
    else if(file.completeSuffix() == "ppm")
      success = saveImage<vtkPNMWriter>(capture, file);
    else if(file.completeSuffix() == "png")
      success = saveImage<vtkPNGWriter>(capture, file);
    else if(file.completeSuffix() == "jpg")
      success = saveImage<vtkJPEGWriter>(capture, file);
      
    if(!success)
      QMessageBox::critical(this, tr("Save Screenshot:"), tr("Error saving file"));
    }
    
  capture->Delete();
  */
}

bool pqMainWindow::compareView(const QString& ReferenceImage, double Threshold, ostream& Output)
{
  // Verify the reference image exists
  if(!QFileInfo(ReferenceImage).exists())
    {
    Output << "<DartMeasurement name=\"ImageNotFound\" type=\"text/string\">";
    Output << ReferenceImage.toAscii().data();
    Output << "</DartMeasurement>" << endl;

    return false;
    }

  // Load the reference image
  vtkSmartPointer<vtkPNGReader> reference_image = vtkSmartPointer<vtkPNGReader>::New();
  reference_image->SetFileName(ReferenceImage.toAscii().data()); 
  reference_image->Update();
  
  // Get a screenshot
  vtkSmartPointer<vtkWindowToImageFilter> screenshot = vtkSmartPointer<vtkWindowToImageFilter>::New();
  if(this->ActiveView)
    {
    screenshot->SetInput(qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget())->GetRenderWindow());
    }
  screenshot->Update();

  // Compute the difference between the reference image and the screenshot
  vtkSmartPointer<vtkImageDifference> difference = vtkSmartPointer<vtkImageDifference>::New();
  difference->SetInput(screenshot->GetOutput());
  difference->SetImage(reference_image->GetOutput());
  difference->Update();

  Output << "<DartMeasurement name=\"ImageError\" type=\"numeric/double\">";
  Output << difference->GetThresholdedError();
  Output << "</DartMeasurement>" << endl;

  Output << "<DartMeasurement name=\"ImageThreshold\" type=\"numeric/double\">";
  Output << Threshold;
  Output << "</DartMeasurement>" << endl;

  // If the difference didn't exceed the threshold, we're done
  if(difference->GetThresholdedError() <= Threshold)
    return true;

/*
  // Write the reference image to a file  resample_image->SetInput(reference_image->GetOutput());
  const QString reference_file = "c:\\reference.png";
  vtkSmartPointer<vtkPNGWriter> reference_writer = vtkSmartPointer<vtkPNGWriter>::New();
  reference_writer->SetInput(reference_image->GetOutput());
  reference_writer->SetFileName(reference_file.toAscii().data());
  reference_writer->Write();

  // Write the screenshot to a file
  const QString screenshot_file = "c:\\screenshot.png";
  vtkSmartPointer<vtkPNGWriter> screenshot_writer = vtkSmartPointer<vtkPNGWriter>::New();
  screenshot_writer->SetInput(screenshot->GetOutput());
  screenshot_writer->SetFileName(screenshot_file.toAscii().data());
  screenshot_writer->Write();

  // Write the difference to a file, increasing the contrast to make discrepancies stand out
  vtkSmartPointer<vtkImageShiftScale> scale_image = vtkSmartPointer<vtkImageShiftScale>::New();
  scale_image->SetShift(0);
  scale_image->SetScale(10);
  scale_image->SetInput(difference->GetOutput());
  
  const QString difference_file = "c:\\difference.png";
  vtkSmartPointer<vtkPNGWriter> difference_writer = vtkSmartPointer<vtkPNGWriter>::New();
  difference_writer->SetInput(scale_image->GetOutput());
  difference_writer->SetFileName(difference_file.toAscii().data());
  difference_writer->Write();

  Output << "<DartMeasurementFile name=\"ValidImage\" type=\"image/jpeg\">";
  Output << ReferenceImage.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;
 
  Output << "<DartMeasurementFile name=\"TestImage\" type=\"image/jpeg\">";
  Output << screenshot_file.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;
  
  Output << "<DartMeasurementFile name=\"DifferenceImage\" type=\"image/jpeg\">";
  Output << difference_file.toAscii().data();
  Output << "</DartMeasurementFile>" << endl;
*/

  return false;
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
  if(!action || !this->ActiveView)
    {
    return;
    }

  QByteArray sourceName = action->data().toString().toAscii();

  vtkSMProxy* source = this->CurrentServer->GetPipelineData()->newSMProxy("sources", sourceName);
  
  //TEMP
  if(this->Inspector)
    this->Inspector->setProxy(this->Adaptor, source);
  
  QVTKWidget* w = qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget());
  vtkSMRenderModuleProxy* rm = this->GraphicsViews[w];
  pqAddPart(rm, vtkSMSourceProxy::SafeDownCast(source));
  rm->ResetCamera();
  w->update();
}

void pqMainWindow::onCreateFilter(QAction* action)
{
  if(!action || !this->ActiveView)
    {
    return;
    }
  
  QByteArray filterName = action->data().toString().toAscii();

  vtkSMSourceProxy* cp = this->CurrentServer->GetPipelineData()->currentProxy();
  vtkSMProxy* source = this->CurrentServer->GetPipelineData()->newSMProxy("filters", filterName);
  vtkSMSourceProxy* sp = vtkSMSourceProxy::SafeDownCast(source);
  this->CurrentServer->GetPipelineData()->addInput(sp, cp);
  //TEMP
  if(this->Inspector)
    this->Inspector->setProxy(this->Adaptor, sp);
  
  QVTKWidget* w = qobject_cast<QVTKWidget*>(this->ActiveView->mainWidget());
  vtkSMRenderModuleProxy* rm = this->GraphicsViews[w];
  pqAddPart(rm, sp);
  rm->ResetCamera();
  w->update();
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

  this->GraphicsViews.insert(w, view);

  QSignalMapper* sm = new QSignalMapper(parent);
  sm->setMapping(parent, parent);
  QObject::connect(parent, SIGNAL(activeChanged(bool)), sm, SLOT(map()));
  QObject::connect(sm, SIGNAL(mapped(QWidget*)), this, SLOT(onFrameActive(QWidget*)));

  parent->setActive(1);
  
}

void pqMainWindow::onDeleteQVTKWidget(pqMultiViewFrame* parent)
{
  QVTKWidget* w = qobject_cast<QVTKWidget*>(parent->mainWidget());
  QMap<QVTKWidget*, vtkSMRenderModuleProxy*>::iterator iter;
  iter = this->GraphicsViews.find(w);
  
  // delete render module
  (*iter)->Delete();

  if(this->ActiveView == parent)
    {
    this->ActiveView = 0;
    }

  this->GraphicsViews.erase(iter);
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

