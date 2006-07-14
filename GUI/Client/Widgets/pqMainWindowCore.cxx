/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowCore.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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

#include "pqApplicationCore.h"
#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterDefinitionWizard.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqDataInformationWidget.h"
#include "pqDisplayColorWidget.h"
#include "pqElementInspectorWidget.h"
#include "pqMainWindowCore.h"
#include "pqMultiView.h"
#include "pqMultiViewFrame.h"
#include "pqObjectInspectorWidget.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineMenu.h"
#include "pqPipelineSource.h"
#include "pqReaderFactory.h"
#include "pqRenderModule.h"
#include "pqRenderWindowManager.h"
#include "pqSelectionManager.h"
#include "pqServer.h"
#include "pqServerBrowser.h"
#include "pqServerFileDialogModel.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqSettings.h"
#include "pqSimpleAnimationManager.h"
#include "pqSourceProxyInfo.h"
#include "pqToolTipTrapper.h"
#include "pqVCRController.h"
#include "pqWriterFactory.h"

#include <pqConnect.h>
#include <pqEventPlayer.h>
#include <pqEventPlayerXML.h>
#include <pqEventTranslator.h>
#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>
#include <pqObjectNaming.h>
#include <pqProgressBar.h>
#include <pqRecordEventsDialog.h>
#include <pqSetData.h>
#include <pqSetName.h>
#include <pqTestUtility.h>
#include <pqUndoStack.h>

#ifdef PARAVIEW_EMBED_PYTHON
#include <pqPythonDialog.h>
#endif // PARAVIEW_EMBED_PYTHON

#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolBar>
#include <QtDebug>

#include <QVTKWidget.h>

#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>
#include <vtkSmartPointer.h>

///////////////////////////////////////////////////////////////////////////
// pqMainWindowCore::pqImplementation

/// Private implementation details for pqMainWindowCore
class pqMainWindowCore::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    Parent(parent),
    RenderWindowManager(new pqRenderWindowManager(parent)),
    MultiViewManager(parent),
    CustomFilters(new pqCustomFilterManagerModel()),
    CustomFilterManager(0),
    RecentFilesMenu(0),
    FilterMenu(0),
    PipelineMenu(0),
    VariableToolbar(0),
    CustomFilterToolbar(0),
    ToolTipTrapper(0),
    IgnoreBrowserSelectionChanges(false)
  {
  }
  
  ~pqImplementation()
  {
    delete this->ToolTipTrapper;
    delete this->PipelineMenu;
    delete this->CustomFilterManager;
    delete this->CustomFilters;
  }

  QWidget* const Parent;
  pqRenderWindowManager* const RenderWindowManager;
  pqMultiView MultiViewManager;
  pqVCRController VCRController;
  pqSelectionManager SelectionManager;
  pqCustomFilterManagerModel* const CustomFilters;
  pqCustomFilterManager* CustomFilterManager;
 
  QMenu* RecentFilesMenu; 
  QStringList RecentFilesList;
  
  QMenu* FilterMenu;
  pqPipelineMenu* PipelineMenu;
  QToolBar* VariableToolbar;
  QToolBar* CustomFilterToolbar;
  
  pqToolTipTrapper* ToolTipTrapper;

  bool IgnoreBrowserSelectionChanges;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindowCore

pqMainWindowCore::pqMainWindowCore(QWidget* parent_widget) :
  Implementation(new pqImplementation(parent_widget))
{
  this->setObjectName("MainWindowCore");

  QObject::connect(&this->Implementation->MultiViewManager, 
                   SIGNAL(frameAdded(pqMultiViewFrame*)), 
                   this->Implementation->RenderWindowManager, 
                   SLOT(onFrameAdded(pqMultiViewFrame*)));

  QObject::connect(&this->Implementation->MultiViewManager, 
                   SIGNAL(frameRemoved(pqMultiViewFrame*)), 
                   this->Implementation->RenderWindowManager, 
                   SLOT(onFrameRemoved(pqMultiViewFrame*)));

  pqApplicationCore* const core = pqApplicationCore::instance();

  this->Implementation->RecentFilesList
    = pqApplicationCore::instance()->settings()->recentFilesList();

  // Initialize supported file types.
  core->getReaderFactory()->loadFileTypes(":/pqWidgets/XML/ParaViewReaders.xml");
  core->getWriterFactory()->loadFileTypes(":/pqWidgets/XML/ParaViewWriters.xml");
  
  // Connect the renderModule manager with the view manager so that 
  // new render modules are created as new frames are added.
  QObject::connect(
    this->Implementation->RenderWindowManager, 
    SIGNAL(activeRenderModuleChanged(pqRenderModule*)),
    core, 
    SLOT(setActiveRenderModule(pqRenderModule*)));

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
                   this, SLOT(onInitializeStates()));

  // Update enable state when the active view changes.
  QObject::connect(core, SIGNAL(activeRenderModuleChanged(pqRenderModule*)),
                   this, SLOT(onActiveRenderModuleChanged(pqRenderModule*)));
                   
/*
  this->installEventFilter(this);
*/
}

//-----------------------------------------------------------------------------
pqMainWindowCore::~pqMainWindowCore()
{
  delete Implementation;
}

pqMultiView& pqMainWindowCore::multiViewManager()
{
  return this->Implementation->MultiViewManager;
}

pqRenderWindowManager& pqMainWindowCore::renderWindowManager()
{
  return *this->Implementation->RenderWindowManager;
}

pqSelectionManager& pqMainWindowCore::selectionManager()
{
  return this->Implementation->SelectionManager;
}

pqVCRController& pqMainWindowCore::VCRController()
{
  return this->Implementation->VCRController;
}

void pqMainWindowCore::setRecentFilesMenu(QMenu* menu)
{
  this->Implementation->RecentFilesMenu = menu;
  this->updateRecentFilesMenu(true);
}

void pqMainWindowCore::setSourceMenu(QMenu* menu)
{
  if(menu)
    {
    menu << pqConnect(SIGNAL(triggered(QAction*)), 
      this, SLOT(onCreateSource(QAction*)));

    menu->clear();
    
    QMenu* released = menu->addMenu("Released")
      << pqSetName("Released");
      
    released->addAction("Cone") 
      << pqSetName("Cone") << pqSetData("ConeSource");

    menu->addAction("2D Glyph") 
      << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    menu->addAction("3D Text") 
      << pqSetName("3D Text") << pqSetData("VectorText");
    menu->addAction("Arrow")
      << pqSetName("Arrow") << pqSetData("ArrowSource");
    menu->addAction("Axes") 
      << pqSetName("Axes") << pqSetData("Axes");
    menu->addAction("Box") 
      << pqSetName("Box") << pqSetData("CubeSource");
    menu->addAction("Cone") 
      << pqSetName("Cone") << pqSetData("ConeSource");
    menu->addAction("Cylinder")
      << pqSetName("Cylinder") << pqSetData("CylinderSource");
    menu->addAction("Hierarchical Fractal") 
      << pqSetName("Hierarchical Fractal") << pqSetData("HierarchicalFractal");
    menu->addAction("Line") 
      << pqSetName("Line") << pqSetData("LineSource");
    menu->addAction("Mandelbrot") 
      << pqSetName("Mandelbrot") << pqSetData("ImageMandelbrotSource");
    menu->addAction("Plane") 
      << pqSetName("Plane") << pqSetData("PlaneSource");
    menu->addAction("Sphere") 
      << pqSetName("Sphere") << pqSetData("SphereSource");
    menu->addAction("Superquadric") 
      << pqSetName("Superquadric") << pqSetData("SuperquadricSource");
    menu->addAction("Wavelet") 
      << pqSetName("Wavelet") << pqSetData("RTAnalyticSource");
    }
}

void pqMainWindowCore::setFilterMenu(QMenu* menu)
{
  this->Implementation->FilterMenu = menu;
  if(this->Implementation->FilterMenu)
    {
    this->Implementation->FilterMenu << pqConnect(SIGNAL(triggered(QAction*)), 
      this, SLOT(onCreateFilter(QAction*)));

    this->Implementation->FilterMenu->clear();

    // Update the menu items for the server and compound filters too.
    QMenu *alphabetical = this->Implementation->FilterMenu;
    QMap<QString, QMenu *> categories;

    QStringList::Iterator iter;
    pqSourceProxyInfo proxyInfo;

    //Released Filters
    QStringList releasedFilters;
    releasedFilters<<"Clip";
    releasedFilters<<"Cut";
    releasedFilters<<"Threshold";

    QMenu *releasedMenu = this->Implementation->FilterMenu->addMenu("Released") << pqSetName("Released");
    for(iter = releasedFilters.begin(); iter != releasedFilters.end(); ++iter)
      {
      QAction* action = releasedMenu->addAction(*iter) << pqSetName(*iter)
        << pqSetData(*iter);
      action->setEnabled(false);
      }

    // Load in the filter information.
    QFile filterInfo(":/pqWidgets/XML/ParaViewFilters.xml");
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
        this->Implementation->FilterMenu->addSeparator();
        }
      else
        {
        QMenu *_menu = this->Implementation->FilterMenu->addMenu(*iter) 
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
}

pqPipelineMenu& pqMainWindowCore::pipelineMenu()
{
  if(!this->Implementation->PipelineMenu)
    {
    this->Implementation->PipelineMenu = new pqPipelineMenu(this);
    this->Implementation->PipelineMenu->setObjectName("PipelineMenu");

    // Disable the add filter action to start with.
    QAction *action = this->Implementation->PipelineMenu->getMenuAction(
        pqPipelineMenu::AddFilterAction);
    action->setEnabled(false);

    // TEMP: Load in the filter information.
    QFile filterInfo(":/pqWidgets/XML/ParaViewFilters.xml");
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
    
  return *this->Implementation->PipelineMenu;
}

void pqMainWindowCore::setupPipelineBrowser(QDockWidget* dock_widget)
{
  pqPipelineBrowser* const pipeline_browser = new pqPipelineBrowser(dock_widget)
    << pqSetName("pipelineBrowser");
    
  dock_widget->setWidget(pipeline_browser);

  this->connect(
    pipeline_browser,
    SIGNAL(selectionChanged(pqServerManagerModelItem*)), 
    this,
    SLOT(onBrowserSelectionChanged(pqServerManagerModelItem*)));

  QObject::connect(
    this,
    SIGNAL(select(pqServerManagerModelItem*)),
    pipeline_browser,
    SLOT(select(pqServerManagerModelItem*)));
}

pqObjectInspectorWidget* pqMainWindowCore::setupObjectInspector(QDockWidget* dock_widget)
{
  pqObjectInspectorWidget* const object_inspector = 
    new pqObjectInspectorWidget(dock_widget);
    
  dock_widget->setWidget(object_inspector);

  pqUndoStack* const undoStack = pqApplicationCore::instance()->getUndoStack();
  
  // Connect Accept/reset signals.
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()),
    undoStack,
    SLOT(Accept()));
    
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()),
    &this->Implementation->SelectionManager,
    SLOT(clearSelection()));

  QObject::connect(
    object_inspector, 
    SIGNAL(postaccept()),
    undoStack,
    SLOT(EndUndoSet()));

  QObject::connect(
    object_inspector,
    SIGNAL(postaccept()),
    this,
    SLOT(onPostAccept()));

  QObject::connect(
    object_inspector,
    SIGNAL(accepted()), 
    pqApplicationCore::instance(),
    SLOT(createPendingDisplays()));
    
  QObject::connect(
    pqApplicationCore::instance(),
    SIGNAL(pendingDisplays(bool)),
    object_inspector,
    SLOT(forceModified(bool)));

  QObject::connect(
    this,
    SIGNAL(activeSourceChanged(vtkSMProxy*)),
    object_inspector,
    SLOT(setProxy(vtkSMProxy*)));

  return object_inspector;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupStatisticsView(QDockWidget* dock_widget)
{
  pqDataInformationWidget* const statistics_view =
    new pqDataInformationWidget(dock_widget)
    << pqSetName("statisticsView");
    
  dock_widget->setWidget(statistics_view);

  pqUndoStack* const undo_stack = pqApplicationCore::instance()->getUndoStack();
  // Undo/redo operations can potentially change data information,
  // hence we must refresh the data on undo/redo.
  QObject::connect(
    undo_stack,
    SIGNAL(Undone()),
    statistics_view,
    SLOT(refreshData()));
    
  QObject::connect(
    undo_stack,
    SIGNAL(Redone()),
    statistics_view,
    SLOT(refreshData()));

  QObject::connect(
    this,
    SIGNAL(postAccept()),
    statistics_view,
    SLOT(refreshData()));    
}

void pqMainWindowCore::setupElementInspector(QDockWidget* dock_widget)
{
  pqElementInspectorWidget* const element_inspector = 
    new pqElementInspectorWidget(dock_widget);
    
  dock_widget->setWidget(element_inspector);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupVariableToolbar(QToolBar* toolbar)
{
  this->Implementation->VariableToolbar = toolbar;
  
  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(
    toolbar)
    << pqSetName("displayColor");
    
  toolbar->addWidget(display_color);

  QObject::connect(
    pqApplicationCore::instance(),
    SIGNAL(activeSourceChanged(pqPipelineSource*)),
    display_color,
    SLOT(updateVariableSelector(pqPipelineSource*)));
  
  QObject::connect(
    this,
    SIGNAL(postAccept()),
    display_color,
    SLOT(reloadGUI()));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupCustomFilterToolbar(QToolBar* toolbar)
{
  this->Implementation->CustomFilterToolbar = toolbar;

  this->connect(toolbar, 
    SIGNAL(actionTriggered(QAction*)), SLOT(onCreateCompoundProxy(QAction*)));
  // Listen for compound proxy register events.
  pqServerManagerObserver *observer =
      pqApplicationCore::instance()->getPipelineData();
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)),
      this, SLOT(onCompoundProxyAdded(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
      this, SLOT(onCompoundProxyRemoved(QString)));

/*
  // Workaround for file new crash.
  this->Implementation->PipelineBrowser->setFocus();
*/
}

void pqMainWindowCore::setupProgressBar(QStatusBar* toolbar)
{
  pqProgressBar* const progress_bar = new pqProgressBar(toolbar);
  toolbar->addPermanentWidget(progress_bar);
  progress_bar->hide();

  QObject::connect(
    pqApplicationCore::instance(),
    SIGNAL(enableProgress(bool)),
    progress_bar,
    SLOT(setVisible(bool)));
    
  QObject::connect(
    pqApplicationCore::instance(), 
    SIGNAL(progress(const QString&, int)),
    progress_bar, 
    SLOT(setProgress(const QString&, int)));
}

//-----------------------------------------------------------------------------
bool pqMainWindowCore::compareView(
  const QString& referenceImage,
  double threshold, 
  ostream& output,
  const QString& tempDirectory)
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

void pqMainWindowCore::initializeStates()
{
  this->onInitializeStates();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileNew()
{
  pqServer* server = pqApplicationCore::instance()->getActiveServer();
  if (server)
    {
    pqApplicationCore::instance()->removeServer(server);
    }
  this->onInitializeStates();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfServers() == 0)
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this->Implementation->Parent);
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
void pqMainWindowCore::onFileOpen(pqServer* server)
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
    this->Implementation->Parent, tr("Open File:"), QString(), filters);
  file_dialog->setObjectName("FileOpenDialog");
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->show(); 
  QApplication::processEvents();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen(const QStringList& files)
{
  pqApplicationCore* core =pqApplicationCore::instance();

  for(int i = 0; i != files.size(); ++i)
    {
    pqPipelineSource* reader = 
      core->createReaderOnActiveServer(files[i]);
    if (!reader)
      {
      qDebug() << "Failed to create reader for : " << files[i];
      continue;
      }

    this->addRecentFile(files[i]);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState()
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
    this->onFileLoadServerState(server);
    }
  else
    {
    pqServerBrowser* const server_browser = new pqServerBrowser(this->Implementation->Parent);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    // let the regular onServerConnect() operation be performed as well.
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
      SLOT(onServerConnect(pqServer*)));
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), 
      this, SLOT(onFileLoadServerState(pqServer*)));
    server_browser->setModal(true);
    server_browser->show();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState(pqServer*)
{
  QString filters;
  filters += "ParaView state file (*.pvs)";
  filters += ";;All files (*)";

  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      this->Implementation->Parent, tr("Open Server State File:"), QString(), filters);
  fileDialog->setObjectName("FileLoadServerStateDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onFileLoadServerState(const QStringList&)));
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState(const QStringList& Files)
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

void pqMainWindowCore::onFileSaveServerState()
{
  QString filters;
  filters += "ParaView state file (*.pvs)";
  filters += ";;All files (*)";

  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this->Implementation->Parent, tr("Save Server State:"), QString(), filters);
  file_dialog->setObjectName("FileSaveServerStateDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->show();
}

void pqMainWindowCore::onFileSaveServerState(const QStringList& Files)
{
  if(Files.size() == 0)
    {
    return;
    }

  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("ParaView");
  pqApplicationCore::instance()->saveState(root);

  // Print the xml to the requested file(s).
  for(int i = 0; i != Files.size(); ++i)
    {
    ofstream os(Files[i].toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    }

  root->Delete();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveData()
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
    this->Implementation->Parent, tr("Save File:"), QString(), filters);
  file_dialog.setObjectName("FileSaveDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  file_dialog.setAttribute(Qt::WA_DeleteOnClose, false);
  QObject::connect(&file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveData(const QStringList&)));
  file_dialog.exec();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveData(const QStringList& files)
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
void pqMainWindowCore::onFileSaveScreenshot()
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
    this->Implementation->Parent, tr("Save Screenshot:"), QString(), filters);
  file_dialog->setObjectName("FileSaveScreenshotDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveScreenshot(const QStringList& files)
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
void pqMainWindowCore::onFileSaveAnimation()
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
  filters +=";;JPEG images (*.jpg);;TIFF images (*.tif);;PNG images (*.png)";
  filters +=";;All files(*)";
  pqFileDialog* const file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), 
    this->Implementation->Parent, tr("Save Animation:"), QString(), filters);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveAnimation(const QStringList&)));
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveAnimation(const QStringList& files)
{
  pqPipelineSource* source = pqApplicationCore::instance()->getActiveSource();
  if (!source)
    {
    qDebug() << "Cannot save animation, no reader selected.";
    return;
    }
  pqSimpleAnimationManager manager(this);
  if (!manager.createTimestepAnimation(source, files[0]))
    {
    qDebug()<< "Animation not saved successfully.";
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditCameraUndo()
{
  pqRenderModule* view = pqApplicationCore::instance()->getActiveRenderModule();
  if (!view)
    {
    qDebug() << "No active render module, cannot undo camera.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->Undo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditCameraRedo()
{
  pqRenderModule* view = pqApplicationCore::instance()->getActiveRenderModule();
  if (!view)
    {
    qDebug() << "No active render module, cannot redo camera.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->Redo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerConnect()
{
  pqApplicationCore::instance()->setActiveServer(0);
  
  pqServerBrowser* const server_browser = new pqServerBrowser(
    this->Implementation->Parent);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
    SLOT(onServerConnect(pqServer*)));
  server_browser->setModal(true);
  server_browser->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerConnect(pqServer* server)
{
  // bring the server object under the window so that when the
  // window is destroyed the server will get destroyed too.
  server->setParent(this);

  // Make the newly created server the active server.
  pqApplicationCore::instance()->setActiveServer(server);
  // Create a render module.
  this->Implementation->RenderWindowManager->onFrameAdded(
    qobject_cast<pqMultiViewFrame *>(
      this->Implementation->MultiViewManager.widgetOfIndex(
        pqMultiView::Index())));
        
  this->updateRecentFilesMenu(true);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerDisconnect()
{
  // for now, they both are same.
  this->onFileNew();
}

void pqMainWindowCore::onToolsCreateCustomFilter()
{
  // Get the selected sources from the application core. Notify the user
  // if the selection is empty.
  QWidget *activeWindow = QApplication::activeWindow();
  const pqServerManagerSelection *selections =
    pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  if(selections->size() == 0)
    {
    QMessageBox::warning(activeWindow, "Create Custom Filter Error",
        "No pipeline objects are selected.\n"
        "To create a new custom filter, select the sources and "
        "filters you want.\nThen, launch the creation wizard.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
    }

  // Create a custom filter definition model with the pipeline
  // selection. The model only accepts pipeline sources. Notify the
  // user if the model is empty.
  pqCustomFilterDefinitionModel custom(this);
  custom.setContents(selections);
  if(!custom.hasChildren(QModelIndex()))
    {
    QMessageBox::warning(activeWindow, "Create Custom Filter Error",
        "The selected objects cannot be used to make a custom filter.\n"
        "To create a new custom filter, select the sources and "
        "filters you want.\nThen, launch the creation wizard.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
    }

  pqCustomFilterDefinitionWizard wizard(&custom, activeWindow);
  wizard.setCustomFilterList(this->Implementation->CustomFilters);
  if(wizard.exec() == QDialog::Accepted)
    {
    // Create a new compound proxy from the custom filter definition.
    wizard.createCustomFilter();
    QString customName = wizard.getCustomFilterName();

    // Launch the custom filter manager in case the user wants to save
    // the compound proxy definition. Select the new custom filter for
    // the user.
    this->onToolsManageCustomFilters();
    this->Implementation->CustomFilterManager->selectCustomFilter(customName);
    }
}

void pqMainWindowCore::onToolsManageCustomFilters()
{
  if(!this->Implementation->CustomFilterManager)
    {
    this->Implementation->CustomFilterManager =
      new pqCustomFilterManager(this->Implementation->CustomFilters,
        QApplication::activeWindow());
    }

  this->Implementation->CustomFilterManager->show();
}

void pqMainWindowCore::onToolsDumpWidgetNames()
{
  QStringList names;
  pqObjectNaming::DumpHierarchy(names);
  names.sort();
  
  for(int i = 0; i != names.size(); ++i)
    {
    qDebug() << names[i];
    }
}

void pqMainWindowCore::onToolsRecordTest()
{
  QString filters;
  filters += "XML Files (*.xml)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Record Test"), QString(), filters);
  fileDialog->setObjectName("ToolsRecordTestDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(onToolsRecordTest(const QStringList &)));
  fileDialog->show();
}

void pqMainWindowCore::onToolsRecordTest(const QStringList &fileNames)
{
  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    pqEventTranslator* const translator = new pqEventTranslator();
    pqTestUtility::Setup(*translator);

    pqRecordEventsDialog* const dialog = new pqRecordEventsDialog(
        translator, *iter, QApplication::activeWindow());
    dialog->show();
    }
}

void pqMainWindowCore::onToolsRecordTestScreenshot()
{
  if(!pqApplicationCore::instance()->getActiveRenderModule())
    {
    qDebug() << "Cannnot save image. No active render module.";
    return;
    }

  QString filters;
  filters += "PNG Image (*.png)";
  filters += ";;BMP Image (*.bmp)";
  filters += ";;TIFF Image (*.tif)";
  filters += ";;PPM Image (*.ppm)";
  filters += ";;JPG Image (*.jpg)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Save Test Screenshot"), QString(),
      filters);
  fileDialog->setObjectName("RecordTestScreenshotDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(onToolsRecordTestScreenshot(const QStringList &)));
  fileDialog->show();
}

void pqMainWindowCore::onToolsRecordTestScreenshot(const QStringList &fileNames)
{
  pqRenderModule* render_module =
      pqApplicationCore::instance()->getActiveRenderModule();
  if(!render_module)
    {
    qCritical() << "Cannnot save image. No active render module.";
    return;
    }

  QSize old_size = render_module->getWidget()->size();
  render_module->getWidget()->resize(300,300);

  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    if(!pqTestUtility::SaveScreenshot(
        render_module->getWidget()->GetRenderWindow(), *iter))
      {
      qCritical() << "Save Image failed.";
      }
    }

  render_module->getWidget()->resize(old_size);
  render_module->render();
}

void pqMainWindowCore::onToolsPlayTest()
{
  QString filters;
  filters += "XML Files (*.xml)";
  filters += ";;All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(new pqLocalFileDialogModel(),
      QApplication::activeWindow(), tr("Play Test"), QString(), filters);
  fileDialog->setObjectName("ToolsPlayTestDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)), 
      this, SLOT(onToolsPlayTest(const QStringList&)));
  fileDialog->show();
}

void pqMainWindowCore::onToolsPlayTest(const QStringList &fileNames)
{
  QApplication::processEvents();

  pqEventPlayer player;
  pqTestUtility::Setup(player);

  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    pqEventPlayerXML xml_player;
    xml_player.playXML(player, *iter);
    }
}

void pqMainWindowCore::onToolsPythonShell()
{
#ifdef PARAVIEW_EMBED_PYTHON
  pqPythonDialog* const dialog = new pqPythonDialog(this->Implementation->Parent);
  dialog->show();
#endif // PARAVIEW_EMBED_PYTHON
}

void pqMainWindowCore::onHelpEnableTooltips(bool enabled)
{
  if(enabled)
    {
    delete this->Implementation->ToolTipTrapper;
    this->Implementation->ToolTipTrapper = 0;
    }
  else
    {
    this->Implementation->ToolTipTrapper = new pqToolTipTrapper();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCreateSource(QAction* action)
{
  if(!action)
    {
    return;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfServers() == 0)
    {
    // We need to create a new connection.
    pqServerBrowser* const server_browser = new pqServerBrowser(this->Implementation->Parent);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    // let the regular onServerConnect() operation be performed as well.
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), this, 
      SLOT(onServerConnect(pqServer*)));
    server_browser->exec();
    }
  if (core->getActiveServer())
    {
    QString sourceName = action->data().toString();
    if (!core->createSourceOnActiveServer(sourceName))
      {
      qCritical() << "Source could not be created.";
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCreateFilter(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString filterName = action->data().toString();
  if (!pqApplicationCore::instance()->createFilterForActiveSource(filterName))
    {
    qCritical() << "Filter could not be created.";
    } 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCreateCompoundProxy(QAction* action)
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
void pqMainWindowCore::onCompoundProxyAdded(QString proxy)
{
  if(this->Implementation->CustomFilterToolbar)
    {
    this->Implementation->CustomFilterToolbar->addAction(
      QIcon(":/pqWidgets/Icons/pqBundle32.png"), proxy) 
      << pqSetName(proxy) << pqSetData(proxy);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCompoundProxyRemoved(QString proxy)
{
  // Remove the action associated with the compound proxy.
  if(this->Implementation->CustomFilterToolbar)
    {
    QAction *action =
      this->Implementation->CustomFilterToolbar->findChild<QAction *>(proxy);
    if(action)
      {
      this->Implementation->CustomFilterToolbar->removeAction(action);
      delete action;
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveSourceChanged(pqPipelineSource* src)
{
  emit this->activeSourceChanged(src ? src->getProxy() : 0);

  // Update the filters menu if there are no sources waiting for displays.
  // Updating the filters menu will cause the execution of a filter because
  // it's output is needed to check filter matches. We do not want the
  // filter to execute prematurely.
  if (pqApplicationCore::instance()->getNumberOfSourcesPendingDisplays() == 0)
    {
    this->updateFiltersMenu(src);
    }

  this->onInitializeStates();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveServerChanged(pqServer* )
{
  this->onInitializeStates();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveRenderModuleChanged(pqRenderModule* rm)
{
  this->onInitializeStates();
  
/*
  if (rm && rm->getWidget())
    {
    rm->getWidget()->installEventFilter(this);
    }
*/

  if(rm)
    {
    QObject::connect(
      rm->getInteractionUndoStack(),
      SIGNAL(StackChanged(bool, QString, bool, QString)),
      this,
      SLOT(onInitializeInteractionStates()));
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCoreActiveChanged()
{
  if(this->Implementation->IgnoreBrowserSelectionChanges)
    {
    return;
    }

  this->Implementation->IgnoreBrowserSelectionChanges = true;
    
  pqServer* const activeServer =
    pqApplicationCore::instance()->getActiveServer();
  pqPipelineSource* const activeSource = 
    pqApplicationCore::instance()->getActiveSource();
 
  pqServerManagerModelItem* item = activeSource;
  if(!item)
    {
    item = activeServer;
    }

  emit this->select(item);
  
  this->Implementation->IgnoreBrowserSelectionChanges = false;
}

/*
bool pqMainWindowCore::eventFilter(QObject* watched, QEvent* e)
{
  if (e->type() == QEvent::KeyPress)
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
        QAction*  selectAction = 
          this->Implementation->SelectionToolBar->findChild<QAction*>(
            "SelectButton");
        if (this->Implementation->SelectionToolBar->isEnabled())
          {
          selectAction->trigger();
          }
        }
      return true;
      }
    }

  return QMainWindow::eventFilter(watched, e);
}
*/

//-----------------------------------------------------------------------------
void pqMainWindowCore::onInitializeStates()
{
  pqServer* const server = pqApplicationCore::instance()->getActiveServer();
  pqPipelineSource *source = pqApplicationCore::instance()->getActiveSource();
  pqRenderModule* rm = pqApplicationCore::instance()->getActiveRenderModule();
  
  const int num_servers = pqApplicationCore::instance()->
    getServerManagerModel()->getNumberOfServers();

  const bool pending_displays = 
    (pqApplicationCore::instance()->getNumberOfSourcesPendingDisplays() > 0);

  emit this->enableFileOpen(!pending_displays);

  this->updateRecentFilesMenu(!pending_displays && (!num_servers || server !=0));
  
  emit this->enableFileLoadServerState(
    !pending_displays && (!num_servers || server !=0));
  
  emit this->enableFileSaveServerState(
    !pending_displays && server !=0);
  
  emit this->enableFileSaveData(!pending_displays && source);

  emit this->enableFileSaveScreenshot(server != 0 && rm != 0);

  emit this->enableFileSaveAnimation(
    pqSimpleAnimationManager::canAnimate(
          pqApplicationCore::instance()->getActiveSource()));

  emit this->enableServerConnect(num_servers == 0);
    
  emit this->enableServerDisconnect(server != 0);

  emit this->enableSourceCreate(
    (num_servers == 0 || server != 0) && !pending_displays);

  emit this->enableFilterCreate(
    source != 0 && server != 0 && !pending_displays);

  emit this->enableVariableToolbar(
    source != 0 && !pending_displays);

  emit this->enableSelectionToolbar(rm ? true : false);

  this->onInitializeInteractionStates();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onInitializeInteractionStates()
{
  bool can_undo_camera = false;
  bool can_redo_camera = false;
  QString undo_camera_label = "";
  QString redo_camera_label = "";

  if(pqRenderModule* const rm =
    pqApplicationCore::instance()->getActiveRenderModule())
    {
    pqUndoStack* const stack = rm->getInteractionUndoStack();
    if (stack && stack->CanUndo())
      {
      can_undo_camera = true;
      undo_camera_label = "Interaction";
      }
    if (stack && stack->CanRedo())
      {
      can_redo_camera = true;
      redo_camera_label = "Interaction";
      }
    }

  emit this->enableCameraUndo(can_undo_camera);
  emit this->enableCameraRedo(can_redo_camera);
  emit this->cameraUndoLabel(undo_camera_label);
  emit this->cameraRedoLabel(redo_camera_label);
}

void pqMainWindowCore::onPostAccept()
{
  this->updateFiltersMenu(pqApplicationCore::instance()->getActiveSource());
  
  emit this->postAccept();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onBrowserSelectionChanged(pqServerManagerModelItem* item)
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
void pqMainWindowCore::updateFiltersMenu(pqPipelineSource* source)
{
  if(this->Implementation->PipelineMenu)
    {
    QAction *addFilter = this->Implementation->PipelineMenu->getMenuAction(
        pqPipelineMenu::AddFilterAction);
    if(addFilter)
      {
      addFilter->setEnabled(source != 0);
      }
    }

  QMenu* const menu = this->Implementation->FilterMenu;
  if(!menu)
    {
    return;
    }
 
  // Iterate over all filters in the menu and see if they are
  // applicable to the current source.
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
void pqMainWindowCore::addRecentFile(const QString& fileName)
{
  this->Implementation->RecentFilesList.removeAll(fileName);
  this->Implementation->RecentFilesList.push_front(fileName);
  pqApplicationCore::instance()->settings()->setRecentFilesList(
    this->Implementation->RecentFilesList);
  while(this->Implementation->RecentFilesList.size() > 10)
    {
    this->Implementation->RecentFilesList.removeLast();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::updateRecentFilesMenu(bool enabled)
{
  if(QMenu* const rfMenu = this->Implementation->RecentFilesMenu)
    {
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
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onRecentFileOpen()
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
