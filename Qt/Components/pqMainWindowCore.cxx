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

#include "pqMainWindowCore.h"
#include <vtkPQConfig.h>

#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QToolBar>
#include <QtDebug>
#include <QList>
#include <QDir>
#include <QMainWindow>

#include "pqActiveView.h"
#include "pqActiveServer.h"
#include "pqAnimationManager.h"
#include "pqAnimationPanel.h"
#include "pqApplicationCore.h"
#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterDefinitionWizard.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqDataInformationWidget.h"
#include "pqDisplayColorWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqElementInspectorWidget.h"
#include "pqEnterIdsDialog.h"
#include "pqEnterPointsDialog.h"
#include "pqEnterThresholdsDialog.h"
#include "pqLinksManager.h"
#include "pqLookmarkBrowser.h"
#include "pqLookmarkInspector.h"
#include "pqLookmarkBrowserModel.h"
#include "pqLookmarkDefinitionWizard.h"
#include "pqMainWindowCore.h"
#include "pqMultiViewFrame.h"
#include "pqMultiView.h"
#include "pqObjectInspectorDriver.h"
#include "pqObjectInspectorWidget.h"
#include "pqPendingDisplayManager.h"
#include "pqPendingDisplayManager.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineBuilder.h"
#include "pqPipelineDisplay.h"
#include "pqPipelineFilter.h"
#include "pqPipelineMenu.h"
#include "pqPipelineSource.h"
#include "pqPlotViewModule.h"
#include "pqPQLookupTableManager.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqProgressManager.h"
#include "pqProxyTabWidget.h"
#include "pqReaderFactory.h"
#include "pqRenderViewModule.h"
#include "pqViewManager.h"
#include "pqSelectionManager.h"
#include "pqSelectReaderDialog.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerStartupBrowser.h"
#include "pqSettingsDialog.h"
#include "pqSettingsDialog.h"
#include "pqSettings.h"
#include "pqSMAdaptor.h"
#include "pqSMAdaptor.h"
#include "pqSourceProxyInfo.h"
#include "pqStateLoader.h"
#include "pqTimerLogDisplay.h"
#include "pqToolTipTrapper.h"
#include "pqVCRController.h"
#include "pqWriterFactory.h"
#include "pqPluginDialog.h"
#include "pqPluginManager.h"
#include "pqActionGroupInterface.h"

#include <pqFileDialog.h>
#include <pqObjectNaming.h>
#include <pqProgressWidget.h>
#include <pqServerResources.h>
#include <pqSetData.h>
#include <pqSetName.h>
#include <pqCoreTestUtility.h>
#include <pqUndoStack.h>
#include "QtTestingConfigure.h"

#ifdef PARAVIEW_EMBED_PYTHON
#include <pqPythonDialog.h>
#endif // PARAVIEW_EMBED_PYTHON

#include <QVTKWidget.h>

#include <vtkDataObject.h>
#include <vtkProcessModule.h>
#include <vtkPVOptions.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSmartPointer.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderModuleProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <vtkToolkits.h>

#include "assert.h"

///////////////////////////////////////////////////////////////////////////
// pqMainWindowCore::pqImplementation

/// Private implementation details for pqMainWindowCore
class pqMainWindowCore::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    Parent(parent),
    MultiViewManager(parent),
    Lookmarks(0),
    CustomFilters(new pqCustomFilterManagerModel(parent)),
    CustomFilterManager(0),
    LookupTableManager(new pqPQLookupTableManager(parent)),
    ObjectInspectorDriver(0),
    RecentFiltersMenu(0),
    FilterMenu(0),
    PipelineMenu(0),
    PipelineBrowser(0),
    VariableToolbar(0),
    CustomFilterToolbar(0),
    ToolTipTrapper(0),
    InCreateSource(false),
    LinksManager(0),
    TimerLog(0)
  {
#ifdef PARAVIEW_EMBED_PYTHON
  this->PythonDialog = 0;
#endif // PARAVIEW_EMBED_PYTHON
  this->MultiViewManager.setObjectName("MultiViewManager");
  }

  ~pqImplementation()
  {
    delete this->ToolTipTrapper;
    delete this->PipelineMenu;
    delete this->CustomFilterManager;
    delete this->CustomFilters;
    delete this->Lookmarks;
    delete this->LookupTableManager;
  }

  QWidget* const Parent;
  pqViewManager MultiViewManager;
  pqVCRController VCRController;
  pqSelectionManager SelectionManager;
  pqElementInspectorWidget* ElementInspector;
  pqLookmarkBrowser* LookmarkBrowser;
  pqLookmarkInspector* LookmarkInspector;
  pqLookmarkBrowserModel* Lookmarks;
  pqCustomFilterManagerModel* const CustomFilters;
  pqCustomFilterManager* CustomFilterManager;
  pqPQLookupTableManager* LookupTableManager;
  pqObjectInspectorDriver* ObjectInspectorDriver;
 
  QMenu* RecentFiltersMenu; 
  QMenu* FilterMenu;
  QMenu* AlphabeticalMenu;
  QList<QString> RecentFilterList;

  pqPipelineMenu* PipelineMenu;
  pqPipelineBrowser *PipelineBrowser;
  QToolBar* VariableToolbar;
  QToolBar* CustomFilterToolbar;
  QList<QToolBar*> PluginToolBars;
  
  pqToolTipTrapper* ToolTipTrapper;

  bool InCreateSource;
  
  QPointer<pqProxyTabWidget> ProxyPanel;
  QPointer<pqAnimationManager> AnimationManager;
  QPointer<pqLinksManager> LinksManager;
  QPointer<pqTimerLogDisplay> TimerLog;

#ifdef PARAVIEW_EMBED_PYTHON
  QPointer<pqPythonDialog> PythonDialog;
#endif // PARAVIEW_EMBED_PYTHON

  pqCoreTestUtility TestUtility;
  pqActiveServer ActiveServer;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindowCore

pqMainWindowCore::pqMainWindowCore(QWidget* parent_widget) :
  Implementation(new pqImplementation(parent_widget))
{
  this->setObjectName("MainWindowCore");
  
  pqApplicationCore* const core = pqApplicationCore::instance();
  core->setLookupTableManager(this->Implementation->LookupTableManager);
  QObject::connect(this, SIGNAL(postAccept()),
    this->Implementation->LookupTableManager, 
    SLOT(updateLookupTableScalarRanges()));

  // Connect the view manager to the pqActiveView.
  QObject::connect(&this->Implementation->MultiViewManager,
    SIGNAL(activeViewModuleChanged(pqGenericViewModule*)),
    &pqActiveView::instance(), SLOT(setCurrent(pqGenericViewModule*)));

  // Listen to the active render module changed signals.
  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqGenericViewModule*)),
    this, SLOT(onActiveViewChanged(pqGenericViewModule*)));
  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqGenericViewModule*)),
    &this->selectionManager(), SLOT(setActiveView(pqGenericViewModule*)));
    
  // Listen for compound proxy register events.
  pqServerManagerObserver *observer =
      pqApplicationCore::instance()->getPipelineData();
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)),
      this->Implementation->CustomFilters, SLOT(addCustomFilter(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
      this->Implementation->CustomFilters, SLOT(removeCustomFilter(QString)));

  // Listen to selection changed events.
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  this->connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(onSelectionChanged()));
  this->connect(selection,
      SIGNAL(selectionChanged(const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(onSelectionChanged()));

  // Update enable state when pending displays state changes.
  this->connect(core->getPendingDisplayManager(), SIGNAL(pendingDisplays(bool)),
      this, SLOT(onPendingDisplayChanged(bool)));

  this->connect(core->getServerManagerModel(), 
    SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onServerCreation(pqServer*)));

  this->connect(core, 
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerCreationFinished(pqServer*)));

  this->connect(core->getServerManagerModel(),
      SIGNAL(aboutToRemoveServer(pqServer*)),
      this, SLOT(onRemovingServer(pqServer*)));
  this->connect(core->getServerManagerModel(),
      SIGNAL(finishedRemovingServer()),
      this, SLOT(onSelectionChanged()));

  // Add a source to the pending display list when a new one is created.
  // The pending display should be created inside the undo set.
  this->connect(core, SIGNAL(finishSourceCreation(pqPipelineSource*)),
      this, SLOT(onSourceCreation(pqPipelineSource*)));
  this->connect(core->getServerManagerModel(),
      SIGNAL(preSourceRemoved(pqPipelineSource*)),
      core->getPendingDisplayManager(), 
      SLOT(removePendingDisplayForSource(pqPipelineSource*)));

  this->connect(core, SIGNAL(finishedAddingSource(pqPipelineSource*)),
      this, SLOT(onSourceCreationFinished(pqPipelineSource*)));
  this->connect(core, SIGNAL(aboutToRemoveSource(pqPipelineSource*)),
      this, SLOT(onRemovingSource(pqPipelineSource*)));

  // Listen for the signal that the lookmark button for a given view was pressed
  QObject::connect(
    &this->Implementation->MultiViewManager, SIGNAL(createLookmark(pqGenericViewModule*)),
    this,
    SLOT(onToolsCreateLookmark(pqGenericViewModule*)));

  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(serverManagerExtensionLoaded()),
                this,
                SLOT(refreshFiltersMenu()));
  
  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(guiInterfaceLoaded(QObject*)),
                this,
                SLOT(addPluginActions(QObject*)));

/*
  this->installEventFilter(this);
*/
  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    &this->Implementation->MultiViewManager, SLOT(setActiveServer(pqServer*)));

  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    core->getUndoStack(), SLOT(setActiveServer(pqServer*))); 

  // set up state loader.
  pqStateLoader* loader = pqStateLoader::New();
  loader->SetMainWindowCore(this);
  core->setStateLoader(loader);
  loader->Delete();
}

//-----------------------------------------------------------------------------
pqMainWindowCore::~pqMainWindowCore()
{
  delete Implementation;
}

pqViewManager& pqMainWindowCore::multiViewManager()
{
  return this->Implementation->MultiViewManager;
}

pqSelectionManager& pqMainWindowCore::selectionManager()
{
  return this->Implementation->SelectionManager;
}

pqVCRController& pqMainWindowCore::VCRController()
{
  return this->Implementation->VCRController;
}

void pqMainWindowCore::setSourceMenu(QMenu* menu)
{
  if(menu)
    {
    QObject::connect(menu, SIGNAL(triggered(QAction*)), 
      this, SLOT(onCreateSource(QAction*)));

    menu->clear();
    
    menu->addAction("2D Glyph") 
      << pqSetName("2D Glyph") << pqSetData("GlyphSource2D");
    menu->addAction("3D Text") 
      << pqSetName("3D Text") << pqSetData("VectorText");
    menu->addAction("Text")
      <<pqSetData("Text") << pqSetData("TextSource");
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

void pqMainWindowCore::refreshFiltersMenu()
{
  vtkSMProxyManager* manager = vtkSMObject::GetProxyManager();
  manager->InstantiateGroupPrototypes("filters");
  if(this->Implementation->FilterMenu)
    {
    this->Implementation->FilterMenu->clear();

    // Update the menu items for the server and compound filters too.

    QStringList::Iterator iter;

    this->Implementation->RecentFiltersMenu = 
      this->Implementation->FilterMenu->addMenu("&Recent") 
      << pqSetName("Recent");
    this->restoreRecentFilterMenu();
    


    //Common Filters
    QStringList commonFilters;
    commonFilters<<"Clip";
    commonFilters<<"Cut";
    commonFilters<<"Threshold";
    commonFilters<<"Contour";
    commonFilters<<"StreamTracer";

    QMenu *commonMenu = this->Implementation->FilterMenu->addMenu("&Common") 
      << pqSetName("Common");
    for(iter = commonFilters.begin(); iter != commonFilters.end(); ++iter)
      {
      QString proxyName = (*iter);
      vtkSMProxy* proxy = manager->GetProxy(
        "filters_prototypes", proxyName.toAscii().data());
      QString proxyLabel = proxyName;
      if (proxy && proxy->GetXMLLabel())
        {
        proxyLabel = proxy->GetXMLLabel();
        }
      QAction* action = commonMenu->addAction(proxyLabel) << pqSetName(proxyName)
        << pqSetData(proxyName);
      action->setEnabled(false);
      }

    this->Implementation->AlphabeticalMenu = 
      this->Implementation->FilterMenu->addMenu("&Alphabetical") 
      << pqSetName("Alphabetical");


    int numFilters = manager->GetNumberOfProxies("filters_prototypes");
    QMap<QString, QString> sortingMap;
    for(int i=0; i<numFilters; i++)
      {
      QStringList categoryList;
      QString proxyName = manager->GetProxyName("filters_prototypes",i);
      vtkSMProxy* proxy = manager->GetProxy(
        "filters_prototypes", proxyName.toAscii().data());
      QString proxyLabel = proxyName;
      if (proxy && proxy->GetXMLLabel())
        {
        proxyLabel = proxy->GetXMLLabel();
        }
      sortingMap[proxyLabel] = proxyName;
      }
    for (QMap<QString, QString>::iterator iter2 = sortingMap.begin();
      iter2 != sortingMap.end(); ++iter2)
      {
      //I am hiding the min max filter because it is intended for python 
      //scripts and its output is not 'correct' on parallel runs when it
      //uses the standard paraview display pipeline.
      if (QString::compare(iter2.key(), "MinMax") != 0)
        {
        QAction* action = 
          this->Implementation->AlphabeticalMenu->addAction(iter2.key()) 
            << pqSetName(iter2.value()) << pqSetData(iter2.value());
        action->setEnabled(false);
        }
      }
    }

  this->updateFiltersMenu();
}

void pqMainWindowCore::setFilterMenu(QMenu* menu)
{
  if(this->Implementation->FilterMenu)
    {
    QObject::disconnect(this->Implementation->FilterMenu, 
      SIGNAL(triggered(QAction*)), 
      this, SLOT(onCreateFilter(QAction*)));

    QObject::disconnect(this->Implementation->FilterMenu,
      SIGNAL(triggered(QAction*)),
      this, SLOT(updateRecentFilterMenu(QAction*)));

    this->Implementation->FilterMenu->clear();
    }

  this->Implementation->FilterMenu = menu;

  if(this->Implementation->FilterMenu)
    {
    QObject::connect(this->Implementation->FilterMenu, 
      SIGNAL(triggered(QAction*)), 
      this, SLOT(onCreateFilter(QAction*)));

    QObject::connect(this->Implementation->FilterMenu,
      SIGNAL(triggered(QAction*)),
      this, SLOT(updateRecentFilterMenu(QAction*)),
      Qt::QueuedConnection);

    this->refreshFiltersMenu();
    }
}

pqPipelineMenu& pqMainWindowCore::pipelineMenu()
{
  if(!this->Implementation->PipelineMenu)
    {
    this->Implementation->PipelineMenu = new pqPipelineMenu(this);
    this->Implementation->PipelineMenu->setObjectName("PipelineMenu");
    }

  return *this->Implementation->PipelineMenu;
}

pqPipelineBrowser* pqMainWindowCore::pipelineBrowser()
{
  return this->Implementation->PipelineBrowser;
}

void pqMainWindowCore::setupPipelineBrowser(QDockWidget* dock_widget)
{
  this->Implementation->PipelineBrowser = new pqPipelineBrowser(dock_widget);
  this->Implementation->PipelineBrowser->setObjectName("pipelineBrowser");
    
  dock_widget->setWidget(this->Implementation->PipelineBrowser);

  QObject::connect(
    &pqActiveView::instance(),
    SIGNAL(changed(pqGenericViewModule*)),
    this->Implementation->PipelineBrowser,
    SLOT(setViewModule(pqGenericViewModule*)));
}

pqProxyTabWidget* pqMainWindowCore::setupProxyTabWidget(QDockWidget* dock_widget)
{
  pqProxyTabWidget* const proxyPanel = 
    new pqProxyTabWidget(dock_widget);
  this->Implementation->ProxyPanel = proxyPanel;

  pqObjectInspectorWidget* object_inspector = proxyPanel->getObjectInspector();
    
  dock_widget->setWidget(proxyPanel);

  pqUndoStack* const undoStack = pqApplicationCore::instance()->getUndoStack();
  
  // Connect Accept/reset signals.
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()),
    undoStack,
    SLOT(accept()));
    
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()),
    &this->Implementation->SelectionManager,
    SLOT(clearSelection()));

  QObject::connect(
    object_inspector, 
    SIGNAL(postaccept()),
    undoStack,
    SLOT(endUndoSet()));

  QObject::connect(
    object_inspector,
    SIGNAL(postaccept()),
    this,
    SLOT(onPostAccept()));

  QObject::connect(
    object_inspector,
    SIGNAL(accepted()), 
    this,
    SLOT(createPendingDisplays()));
    
  QObject::connect(
    pqApplicationCore::instance()->getPendingDisplayManager(),
    SIGNAL(pendingDisplays(bool)),
    object_inspector,
    SLOT(forceModified(bool)));

  // Use the server manager selection model to determine which page
  // should be shown.
  pqObjectInspectorDriver *driver = this->getObjectInspectorDriver();
  QObject::connect(
    driver,
    SIGNAL(sourceChanged(pqProxy *)),
    proxyPanel,
    SLOT(setProxy(pqProxy *)));
  
  QObject::connect(
    &pqActiveView::instance(),
    SIGNAL(changed(pqGenericViewModule*)),
    proxyPanel,
    SLOT(setView(pqGenericViewModule*)));

  return proxyPanel;
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
    SLOT(accept()));
    
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()),
    &this->Implementation->SelectionManager,
    SLOT(clearSelection()));

  QObject::connect(
    object_inspector, 
    SIGNAL(postaccept()),
    undoStack,
    SLOT(endUndoSet()));

  QObject::connect(
    object_inspector,
    SIGNAL(postaccept()),
    this,
    SLOT(onPostAccept()));

  QObject::connect(
    object_inspector,
    SIGNAL(accepted()), 
    this,
    SLOT(createPendingDisplays()));
    
  QObject::connect(
    pqApplicationCore::instance()->getPendingDisplayManager(),
    SIGNAL(pendingDisplays(bool)),
    object_inspector,
    SLOT(forceModified(bool)));

  // Use the server manager selection model to determine which page
  // should be shown.
  pqObjectInspectorDriver *driver = this->getObjectInspectorDriver();
  QObject::connect(
    driver,
    SIGNAL(sourceChanged(pqProxy *)),
    object_inspector,
    SLOT(setProxy(pqProxy *)));
  
  QObject::connect(
    &pqActiveView::instance(),
    SIGNAL(changed(pqGenericViewModule*)),
    object_inspector,
    SLOT(setView(pqGenericViewModule*)));

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
    SIGNAL(undone()),
    statistics_view,
    SLOT(refreshData()));
    
  QObject::connect(
    undo_stack,
    SIGNAL(redone()),
    statistics_view,
    SLOT(refreshData()));

  QObject::connect(
    this,
    SIGNAL(postAccept()),
    statistics_view,
    SLOT(refreshData()));    
  
  QObject::connect(
    pqApplicationCore::instance(),
    SIGNAL(stateLoaded()),
    statistics_view,
    SLOT(refreshData()));
     
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupElementInspector(QDockWidget* dock_widget)
{
  pqElementInspectorWidget* const element_inspector = 
    new pqElementInspectorWidget(dock_widget);

  QObject::connect(&this->Implementation->SelectionManager,
                   SIGNAL(selectionChanged(pqSelectionManager*)),
                   element_inspector,
                   SLOT(onSelectionChanged(pqSelectionManager*)));
    
  dock_widget->setWidget(element_inspector);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupLookmarkBrowser(QDockWidget* dock_widget)
{
  if(!this->Implementation->Lookmarks)
    {
    this->Implementation->Lookmarks = new pqLookmarkBrowserModel(this);
    }
  this->Implementation->LookmarkBrowser = 
    new pqLookmarkBrowser(this->Implementation->Lookmarks, dock_widget);
  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    this->Implementation->LookmarkBrowser, SLOT(setActiveServer(pqServer*)),
    Qt::QueuedConnection);

  dock_widget->setWidget(this->Implementation->LookmarkBrowser);
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::setupLookmarkInspector(QDockWidget* dock_widget)
{
  if(!this->Implementation->Lookmarks)
    {
    this->Implementation->Lookmarks = new pqLookmarkBrowserModel(this);
    }

  this->Implementation->LookmarkInspector = 
    new pqLookmarkInspector(this->Implementation->Lookmarks, dock_widget);

  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    this->Implementation->LookmarkInspector, SLOT(setActiveServer(pqServer*)),
    Qt::QueuedConnection);

  if(this->Implementation->LookmarkBrowser)
    {
    QObject::connect(
      this->Implementation->LookmarkBrowser->getSelectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection & )),
      this->Implementation->LookmarkInspector,
      SLOT(onLookmarkSelectionChanged(const QItemSelection &)));
      //SLOT(onLookmarkSelectionChanged(const QItemSelection &, const QItemSelection & )));
    }
  dock_widget->setWidget(this->Implementation->LookmarkInspector);
}

//-----------------------------------------------------------------------------
pqAnimationManager* pqMainWindowCore::getAnimationManager()
{
  if (!this->Implementation->AnimationManager)
    {
    this->Implementation->AnimationManager = new pqAnimationManager(this);
    QObject::connect(
      &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
      this->Implementation->AnimationManager, 
      SLOT(onActiveServerChanged(pqServer*)), Qt::QueuedConnection);

    QObject::connect(this->Implementation->AnimationManager,
      SIGNAL(activeSceneChanged(pqAnimationScene*)),
      this, SLOT(onActiveSceneChanged(pqAnimationScene*)));
    this->Implementation->AnimationManager->setViewWidget(
      &this->multiViewManager());
    }
  return this->Implementation->AnimationManager;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupAnimationPanel(QDockWidget* dock_widget)
{
  pqAnimationPanel* const panel = new pqAnimationPanel(dock_widget);
  pqUndoStack* const undoStack = pqApplicationCore::instance()->getUndoStack();
  
  pqAnimationManager* mgr = this->getAnimationManager();
  QObject::connect(mgr, SIGNAL(activeSceneChanged(pqAnimationScene*)),
    &this->VCRController(), SLOT(setAnimationScene(pqAnimationScene*)));

  QObject::connect(panel, SIGNAL(beginUndoSet(const QString&)),
    undoStack, SLOT(beginUndoSet(QString)));
  QObject::connect(panel, SIGNAL(endUndoSet()),
    undoStack, SLOT(endUndoSet()));
  
  panel->setManager(mgr);
  dock_widget->setWidget(panel);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupVariableToolbar(QToolBar* toolbar)
{
  this->Implementation->VariableToolbar = toolbar;
  
  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(
    toolbar)
    << pqSetName("displayColor");
    
  toolbar->addWidget(display_color);

  QObject::connect(this->getObjectInspectorDriver(),
    SIGNAL(displayChanged(pqConsumerDisplay *, pqGenericViewModule *)),
    display_color, SLOT(setDisplay(pqConsumerDisplay *)));
  
  QObject::connect(
    this,
    SIGNAL(postAccept()),
    display_color,
    SLOT(reloadGUI()));
}

//-----------------------------------------------------------------------------
pqObjectInspectorDriver* pqMainWindowCore::getObjectInspectorDriver()
{
  if(!this->Implementation->ObjectInspectorDriver)
    {
    this->Implementation->ObjectInspectorDriver =
        new pqObjectInspectorDriver(this);
    this->Implementation->ObjectInspectorDriver->setSelectionModel(
        pqApplicationCore::instance()->getSelectionModel());
    this->connect(&pqActiveView::instance(),
        SIGNAL(changed(pqGenericViewModule *)),
        this->Implementation->ObjectInspectorDriver,
        SLOT(setActiveView(pqGenericViewModule *)));
    }

  return this->Implementation->ObjectInspectorDriver;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupRepresentationToolbar(QToolBar* toolbar)
{
  pqDisplayRepresentationWidget* display_representation = new pqDisplayRepresentationWidget(
    toolbar)
    << pqSetName("displayRepresentation");

  toolbar->addWidget(display_representation);

  QObject::connect(this->getObjectInspectorDriver(),
    SIGNAL(displayChanged(pqConsumerDisplay *, pqGenericViewModule *)),
    display_representation, SLOT(setDisplay(pqConsumerDisplay *)));

  QObject::connect(this, SIGNAL(postAccept()),
    display_representation, SLOT(reloadGUI()));
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

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupProgressBar(QStatusBar* toolbar)
{
  pqProgressWidget* const progress_bar = new pqProgressWidget(toolbar);
  toolbar->addPermanentWidget(progress_bar);
  progress_bar->enableProgress(false);

  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();

  QObject::connect(progress_manager, SIGNAL(enableProgress(bool)),
    progress_bar, SLOT(enableProgress(bool)));
    
  QObject::connect(progress_manager, SIGNAL(progress(const QString&, int)),
    progress_bar, SLOT(setProgress(const QString&, int)));

  QObject::connect(progress_manager, SIGNAL(enableAbort(bool)),
    progress_bar, SLOT(enableAbort(bool)));

  QObject::connect(progress_bar, SIGNAL(abortPressed()),
    progress_manager, SLOT(triggerAbort()));

  progress_manager->addNonBlockableObject(progress_bar);
  progress_manager->addNonBlockableObject(progress_bar->getAbortButton());
}

//-----------------------------------------------------------------------------
bool pqMainWindowCore::compareView(
  const QString& referenceImage,
  double threshold, 
  ostream& output,
  const QString& tempDirectory)
{
  pqRenderViewModule* renModule = qobject_cast<pqRenderViewModule*>(pqActiveView::instance().current());

  if (!renModule)
    {
    output << "ERROR: Could not locate the render module." << endl;
    return false;
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
  bool ret = pqCoreTestUtility::CompareImage(render_window, referenceImage, 
    threshold, output, tempDirectory);
  renModule->getWidget()->resize(cur_size);
  renModule->render();
  return ret;
}

void pqMainWindowCore::initializeStates()
{
  emit this->enableFileOpen(true);

  emit this->enableFileLoadServerState(true);
  
  emit this->enableFileSaveServerState(false);
  emit this->enableFileSaveData(false);
  emit this->enableFileSaveScreenshot(false);

  emit this->enableFileSaveAnimation(false);
  emit this->enableFileSaveGeometry(false);

  emit this->enableServerConnect(true);
  emit this->enableServerDisconnect(false);

  emit this->enableSourceCreate(true);
  emit this->enableFilterCreate(false);

  emit this->enableVariableToolbar(false);
  emit this->enableSelectionToolbar(false);

  emit this->enableCameraUndo(false);
  emit this->enableCameraRedo(false);
  emit this->cameraUndoLabel("");
  emit this->cameraRedoLabel("");
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfServers() == 0)
    {
    pqServerStartupBrowser* const server_browser = new pqServerStartupBrowser(
      pqApplicationCore::instance()->serverStartups(),
      *pqApplicationCore::instance()->settings(),
      this->Implementation->Parent);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), 
      this, SLOT(onFileOpen(pqServer*)), Qt::QueuedConnection);
    server_browser->setModal(true);
    server_browser->show();
    }
  else
    {
    pqServer *server = this->getActiveServer();
    if(server)
      {
      this->onFileOpen(server);
      }
    else
      {
      qDebug() << "No active server selected.";
      }
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
  pqFileDialog* const file_dialog = new pqFileDialog(server, 
    this->Implementation->Parent, tr("Open File:"), QString(), filters);
    
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileOpenDialog");
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileOpen(const QStringList&)));
  file_dialog->setModal(true); 
  file_dialog->show(); 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen(const QStringList& files)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer *server = this->getActiveServer();
  for(int i = 0; i != files.size(); ++i)
    {
    pqPipelineSource* reader = this->createReaderOnActiveServer(files[i]);
    if (!reader)
      {
      pqSelectReaderDialog prompt(files[i], server,
                                  qobject_cast<QWidget*>(this->parent()));
      if(prompt.exec() == QDialog::Accepted)
        {
        QString whichReader = prompt.getReader();
        reader = core->createReaderOnServer(files[i], server, whichReader);
        }
      else
        {
        continue;
        }
      }
      
    // Add this to the list of recent server resources ...
    if(reader)
      {
      pqServerResource resource = server->getResource();
      resource.setPath(files[i]);
      core->serverResources().add(resource);
      core->serverResources().save(*core->settings());
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  int num_servers = core->getServerManagerModel()->getNumberOfServers();
  if (num_servers > 0)
    {
    pqServer* server = this->getActiveServer();
    if (!server)
      {
      qDebug() << "No active server. Cannot load state.";
      return;
      }

    this->onFileLoadServerState(server);
    }
  else
    {
    pqServerStartupBrowser* const server_browser = new pqServerStartupBrowser(
      pqApplicationCore::instance()->serverStartups(),
      *pqApplicationCore::instance()->settings(),
      this->Implementation->Parent);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    QObject::connect(server_browser, SIGNAL(serverConnected(pqServer*)), 
      this, SLOT(onFileLoadServerState(pqServer*)), Qt::QueuedConnection);
    server_browser->setModal(true);
    server_browser->show();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState(pqServer*)
{
  QString filters;
  filters += "ParaView state file (*.pvsm)";
  filters += ";;All files (*)";

  pqFileDialog *fileDialog = new pqFileDialog(NULL,
      this->Implementation->Parent, tr("Open Server State File:"), QString(), filters);
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("FileLoadServerStateDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)),
      this, SLOT(onFileLoadServerState(const QStringList&)));
  fileDialog->setModal(true);
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState(const QStringList& files)
{
  pqServer *server = this->getActiveServer();
  for(int i = 0; i != files.size(); ++i)
    {
    // Read in the xml file to restore.
    vtkPVXMLParser *xmlParser = vtkPVXMLParser::New();
    xmlParser->SetFileName(files[i].toAscii().data());
    xmlParser->Parse();

    // Get the root element from the parser.
    vtkPVXMLElement *root = xmlParser->GetRootElement();
    if (root)
      {
      pqApplicationCore::instance()->loadState(root, server);
                                              
      // Add this to the list of recent server resources ...
      pqServerResource resource;
      resource.setScheme("session");
      resource.setPath(files[i]);
      resource.setSessionServer(server->getResource());
      pqApplicationCore::instance()->serverResources().add(resource);
      pqApplicationCore::instance()->serverResources().save(*pqApplicationCore::instance()->settings());
      }
    else
      {
      qCritical("Root does not exist. Either state file could not be opened "
                "or it does not contain valid xml");
      }

    xmlParser->Delete();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveServerState()
{
  QString filters;
  filters += "ParaView state file (*.pvsm)";
  filters += ";;All files (*)";

  pqFileDialog* const file_dialog = new pqFileDialog(NULL,
    this->Implementation->Parent, tr("Save Server State:"), QString(), filters);
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileSaveServerStateDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveServerState(const QStringList&)));
  file_dialog->setModal(true);
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveServerState(const QStringList& files)
{
  vtkPVXMLElement *root = vtkPVXMLElement::New();
  root->SetName("ParaView");
  pqApplicationCore::instance()->saveState(root);
  //this->Implementation->MultiViewManager.saveState(root);
  this->multiViewManager().saveState(root);


  // Print the xml to the requested file(s).
  pqServer *server = this->getActiveServer();
  for(int i = 0; i != files.size(); ++i)
    {
    ofstream os(files[i].toAscii().data(), ios::out);
    root->PrintXML(os, vtkIndent());
    
    // Add this to the list of recent server resources ...
    pqServerResource resource;
    resource.setScheme("session");
    resource.setPath(files[i]);
    resource.setSessionServer(server->getResource());
    pqApplicationCore::instance()->serverResources().add(resource);
    pqApplicationCore::instance()->serverResources().save(*pqApplicationCore::instance()->settings());
    }

  root->Delete();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveData()
{
  pqPipelineSource* source = this->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source, cannot save data.";
    return;
    }

  // Get the list of writers that can write the output from the given source.
  QString filters = 
    pqApplicationCore::instance()->getWriterFactory()->getSupportedFileTypes(
      source);

  pqFileDialog file_dialog(source->getServer(),
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
  pqPipelineSource* source = this->getActiveSource();
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
    ->SetElement(0, files[0].toAscii().data());

  // TODO: We can popup a wizard or something for setting the properties
  // on the writer.
  vtkSMProxyProperty::SafeDownCast(writer->GetProperty("Input"))->AddProxy(
    source->getProxy());
  writer->UpdateVTKObjects();

  writer->UpdatePipeline();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveScreenshot()
{
  pqGenericViewModule* view = pqActiveView::instance().current();
  if(!view)
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
  filters += ";;PDF file (*.pdf)";
  filters += ";;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(NULL,
    this->Implementation->Parent, tr("Save Screenshot:"), QString(), filters);
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileSaveScreenshotDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveScreenshot(const QStringList&)));
  file_dialog->setModal(true);
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveScreenshot(const QStringList& files)
{
  pqGenericViewModule* view = pqActiveView::instance().current();
  if(!view)
    {
    qDebug() << "Cannnot save image. No active view module.";
    return;
    }
  for(int i = 0; i != files.size(); ++i)
    {
    if(!view->saveImage(0, 0, files[i]))
      {
      qCritical() << "Save Image failed.";
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveAnimation()
{
  pqAnimationManager* mgr = this->getAnimationManager();
  if (!mgr || !mgr->getActiveScene())
    {
    qDebug() << "Cannot save animation since no active scene is present.";
    return;
    }
  QString filters = "";

#ifdef VTK_USE_MPEG2_ENCODER
  filter += "MPEG files (*.mpg);;";
#endif
#ifdef _WIN32
  filters += "AVI files (*.avi);;";
#else
# ifdef VTK_USE_FFMPEG_ENCODER
  filters += "AVI files (*.avi);;";
# endif
#endif
  filters +="JPEG images (*.jpg);;TIFF images (*.tif);;PNG images (*.png);;";
  filters +="All files(*)";
  pqFileDialog* const file_dialog = new pqFileDialog(NULL,
    this->Implementation->Parent, tr("Save Animation:"), QString(), filters);
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveAnimation(const QStringList&)));
  file_dialog->setModal(true);
  file_dialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveAnimation(const QStringList& files)
{
  pqAnimationManager* mgr = this->getAnimationManager();
  if (!mgr || !mgr->getActiveScene())
    {
    qDebug() << "Cannot save animation since no active scene is present.";
    return;
    }

  // This is essential since we don't want the view frame
  // decorations to apper in out animation.
  this->multiViewManager().hideDecorations();
  if (!mgr->saveAnimation(files[0]))
    {
    // qDebug() << "Animation save failed!";
    }
  this->multiViewManager().showDecorations();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onSaveGeometry()
{
  pqAnimationManager* mgr = this->getAnimationManager();
  if (!mgr || !mgr->getActiveScene())
    {
    qDebug() << "Cannot save animation geometry since no active scene is present.";
    return;
    }
  pqGenericViewModule* view = pqActiveView::instance().current();
  if (!view)
    {
    qDebug() << "Cannot save animation geometry since no active view.";
    return;
    }

  QString filters = "ParaView Data files (*.pvd);;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(NULL,
    this->Implementation->Parent, tr("Save Animation Geometry"), QString(), filters);
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onSaveGeometry(const QStringList&)));
  file_dialog->setModal(true);
  file_dialog->show();  
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onSaveGeometry(const QStringList& files)
{
  pqAnimationManager* mgr = this->getAnimationManager();
  if (!mgr || !mgr->getActiveScene())
    {
    qDebug() << "Cannot save animation since no active scene is present.";
    return;
    }
  pqGenericViewModule* view = pqActiveView::instance().current();
  if (!view)
    {
    qDebug() << "Cannot save animation geometry since no active view.";
    return;
    }

  if (!mgr->saveGeometry(files[0], view))
    {
    qDebug() << "Animation save geometry failed!";
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditCameraUndo()
{
  pqRenderViewModule* view = qobject_cast<pqRenderViewModule*>(
    pqActiveView::instance().current());
  if (!view)
    {
    qDebug() << "No active render module, cannot undo camera.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->undo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditCameraRedo()
{
  pqRenderViewModule* view = qobject_cast<pqRenderViewModule*>(
    pqActiveView::instance().current());
  if (!view)
    {
    qDebug() << "No active render module, cannot redo camera.";
    return;
    }
  pqUndoStack* stack = view->getInteractionUndoStack();
  stack->redo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerConnect()
{
  pqServerStartupBrowser* const server_browser = new pqServerStartupBrowser(
    pqApplicationCore::instance()->serverStartups(),
    *pqApplicationCore::instance()->settings(),
    this->Implementation->Parent);
  server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
  server_browser->setModal(true);
  server_browser->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerDisconnect()
{
  pqServer* server = this->getActiveServer();
  if (server)
    {
    pqApplicationCore::instance()->removeServer(server);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsCreateCustomFilter()
{
  // Get the selected sources from the application core. Notify the user
  // if the selection is empty.
  QWidget *mainWin = this->Implementation->Parent;
  const pqServerManagerSelection *selections =
    pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  if(selections->size() == 0)
    {
    QMessageBox::warning(mainWin, "Create Custom Filter Error",
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
    QMessageBox::warning(mainWin, "Create Custom Filter Error",
        "The selected objects cannot be used to make a custom filter.\n"
        "To create a new custom filter, select the sources and "
        "filters you want.\nThen, launch the creation wizard.",
        QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton);
    return;
    }

  pqCustomFilterDefinitionWizard wizard(&custom, mainWin);
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

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsManageCustomFilters()
{
  if(!this->Implementation->CustomFilterManager)
    {
    this->Implementation->CustomFilterManager =
      new pqCustomFilterManager(this->Implementation->CustomFilters,
        this->Implementation->Parent);
    }

  this->Implementation->CustomFilterManager->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsCreateLookmark()
{
  // Create a lookmark of the currently active view
  this->onToolsCreateLookmark(pqActiveView::instance().current());
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsCreateLookmark(pqGenericViewModule *view)
{
  // right now we only support Lookmarks of render modules
  pqRenderViewModule* const render_module = qobject_cast<pqRenderViewModule*>(view);
  if(!render_module)
    {
    qCritical() << "Cannnot create Lookmark. No active render module.";
    return;
    }

  pqLookmarkDefinitionWizard wizard(render_module, this->Implementation->Lookmarks, this->Implementation->Parent);
  if(wizard.exec() == QDialog::Accepted)
    {
    wizard.createLookmark();
    QString lookmarkName = wizard.getLookmarkName();
    this->Implementation->LookmarkBrowser->selectLookmark(lookmarkName);
    this->Implementation->LookmarkBrowser->parentWidget()->show();
    this->Implementation->LookmarkInspector->parentWidget()->show();
    }
}


//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsRecordTest()
{
  QString filters;
#ifdef QT_TESTING_WITH_XML
  filters += "XML Files (*.xml);;";
#endif
#ifdef QT_TESTING_WITH_PYTHON
  filters += "Python Files (*.py);;";
#endif
  filters += "All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(NULL,
      this->Implementation->Parent, tr("Record Test"), QString(), filters);
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("ToolsRecordTestDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(onToolsRecordTest(const QStringList &)));
  fileDialog->setModal(true);
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsRecordTest(const QStringList &fileNames)
{
  if(fileNames.empty())
    {
    return;
    }

  this->Implementation->TestUtility.recordTests(fileNames[0]);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsRecordTestScreenshot()
{
  if(!qobject_cast<pqRenderViewModule*>(pqActiveView::instance().current()))
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
  pqFileDialog *fileDialog = new pqFileDialog(NULL,
      this->Implementation->Parent, tr("Save Test Screenshot"), QString(),
      filters);
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("RecordTestScreenshotDialog");
  fileDialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList &)), 
      this, SLOT(onToolsRecordTestScreenshot(const QStringList &)));
  fileDialog->setModal(true);
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsRecordTestScreenshot(const QStringList &fileNames)
{
  pqRenderViewModule* const render_module = qobject_cast<pqRenderViewModule*>(
    pqActiveView::instance().current());
  if(!render_module)
    {
    qCritical() << "Cannnot save image. No active render module.";
    return;
    }

  QVTKWidget* const widget = qobject_cast<QVTKWidget*>(render_module->getWidget());
  assert(widget);

  QSize old_size = widget->size();
  widget->resize(300,300);

  QStringList::ConstIterator iter = fileNames.begin();
  for( ; iter != fileNames.end(); ++iter)
    {
    if(!pqCoreTestUtility::SaveScreenshot(
        widget->GetRenderWindow(), *iter))
      {
      qCritical() << "Save Image failed.";
      }
    }

  widget->resize(old_size);
  render_module->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsPlayTest()
{
  QString filters;
#ifdef QT_TESTING_WITH_XML
  filters += "XML Files (*.xml);;";
#endif
#ifdef QT_TESTING_WITH_PYTHON
  filters += "Python Files (*.py);;";
#endif
  filters += "All Files (*)";
  pqFileDialog *fileDialog = new pqFileDialog(NULL,
      this->Implementation->Parent, tr("Play Test"), QString(), filters);
  fileDialog->setAttribute(Qt::WA_DeleteOnClose);
  fileDialog->setObjectName("ToolsPlayTestDialog");
  fileDialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fileDialog, SIGNAL(filesSelected(const QStringList&)), 
      this, SLOT(onToolsPlayTest(const QStringList&)));
  fileDialog->setModal(true);
  fileDialog->show();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsPlayTest(const QStringList &fileNames)
{
  if(1 == fileNames.size())
    {
    this->Implementation->TestUtility.playTests(fileNames[0]);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsTimerLog()
{
  if(!this->Implementation->TimerLog)
    {
    this->Implementation->TimerLog
      = new pqTimerLogDisplay(this->Implementation->Parent);
    this->Implementation->TimerLog->setAttribute(Qt::WA_QuitOnClose, false);
    }
  this->Implementation->TimerLog->show();
  this->Implementation->TimerLog->raise();
  this->Implementation->TimerLog->activateWindow();
  this->Implementation->TimerLog->refresh();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsOutputWindow()
{
  vtkProcessModuleGUIHelper *helper
    = vtkProcessModule::GetProcessModule()->GetGUIHelper();
  pqProcessModuleGUIHelper *pqHelper
    = pqProcessModuleGUIHelper::SafeDownCast(helper);
  if (!pqHelper)
    {
    qWarning("Could not get the pqProcessModuleGUIHelper");
    }
  else
    {
    pqHelper->showOutputWindow();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsPythonShell()
{
#ifdef PARAVIEW_EMBED_PYTHON
  if (!this->Implementation->PythonDialog)
    {
    const char* argv0 = vtkProcessModule::GetProcessModule()->
      GetOptions()->GetArgv0();
    this->Implementation->PythonDialog = 
      new pqPythonDialog(this->Implementation->Parent, 1, (char**)&argv0);
    this->Implementation->PythonDialog->setAttribute(Qt::WA_QuitOnClose, false);
    }
  this->Implementation->PythonDialog->show();
  this->Implementation->PythonDialog->raise();
  this->Implementation->PythonDialog->activateWindow();
 
#else // PARAVIEW_EMBED_PYTHON
  QMessageBox::information(NULL, "ParaView", "Python Shell not available");
#endif // PARAVIEW_EMBED_PYTHON
}

//-----------------------------------------------------------------------------
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
void pqMainWindowCore::onEditSettings()
{
  pqSettingsDialog dialog(this->Implementation->Parent);
  dialog.setRenderModule(qobject_cast<pqRenderViewModule*>(
      pqActiveView::instance().current()));
  dialog.exec();
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
    pqServerStartupBrowser* const server_browser = new pqServerStartupBrowser(
      pqApplicationCore::instance()->serverStartups(),
      *pqApplicationCore::instance()->settings(),
      this->Implementation->Parent);
    server_browser->setAttribute(Qt::WA_DeleteOnClose);  // auto delete when closed
    server_browser->exec();
    }
  
  if (this->getActiveServer())
    {
    QString sourceName = action->data().toString();
    if (!this->createSourceOnActiveServer(sourceName))
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
  if (!this->createFilterForActiveSource(filterName))
    {
    qCritical() << "Filter could not be created.";
    } 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::updateRecentFilterMenu(QAction* action)
{
  if(!action)
    {
    return;
    }

  QString filterName = action->data().toString();
  int idx=this->Implementation->RecentFilterList.indexOf(filterName);
  if(idx!=-1)
    {
    this->Implementation->RecentFilterList.removeAt(idx);
    }

  this->Implementation->RecentFilterList.push_front(filterName);
  if(this->Implementation->RecentFilterList.size()>10)
    {
    this->Implementation->RecentFilterList.removeLast();
    }



  this->Implementation->RecentFiltersMenu->clear();


  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  QList<QString>::iterator begin,end;
  begin=this->Implementation->RecentFilterList.begin();
  end=this->Implementation->RecentFilterList.end();
  for(;begin!=end;++begin)
    {
    QString proxyLabel = (*begin);
    vtkSMProxy* proxy = pxm->GetProxy(
      "filters_prototypes", (*begin).toAscii().data());
    if (proxy && proxy->GetXMLLabel())
      {
      proxyLabel = proxy->GetXMLLabel();
      }
    QAction* recentA = 
      this->Implementation->RecentFiltersMenu->addAction(proxyLabel) 
      << pqSetName(*begin) << pqSetData(*begin);
    recentA->setEnabled(false);
    }

  this->saveRecentFilterMenu();
}


static const char* RecentFilterMenuSettings[] = {
  "FilterOne",
  "FilterTwo",
  "FilterThree",
  "FilterFour",
  "FilterFive",
  "FilterSix",
  "FilterSeven",
  "FilterEight",
  "FilterNine",
  "FilterTen",
  NULL  // keep last
};


//-----------------------------------------------------------------------------
void pqMainWindowCore::saveRecentFilterMenu()
{
  pqSettings* settings = pqApplicationCore::instance()->settings();
  const char** str;

  QList<QString>::iterator begin,end;
  begin=this->Implementation->RecentFilterList.begin();
  end=this->Implementation->RecentFilterList.end();

  for(str=RecentFilterMenuSettings; *str != NULL; str++)
  {
    if(begin!=end)
    {
      QString key = QString("recentFilterMenu/") + *str;
      settings->setValue(key, *begin);
      begin++;    
    }

  }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::restoreRecentFilterMenu()
{
  this->Implementation->RecentFiltersMenu->clear();

  // Now load default values from the QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();

  const char** str;

  for(str=RecentFilterMenuSettings; *str != NULL; str++)
  {
    QString key = QString("recentFilterMenu/") + *str;
    if (settings->contains(key))
    {
      QString filterName=settings->value(key).toString();

      int idx=this->Implementation->RecentFilterList.indexOf(filterName);
      if(idx!=-1)
        {
        this->Implementation->RecentFilterList.removeAt(idx);
        }

      this->Implementation->RecentFilterList.push_back(filterName);
      if(this->Implementation->RecentFilterList.size()>10)
        {
        this->Implementation->RecentFilterList.removeLast();
        }
    }
  }


   this->Implementation->RecentFiltersMenu->clear();


   vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
   QList<QString>::iterator begin,end;
   begin=this->Implementation->RecentFilterList.begin();
   end=this->Implementation->RecentFilterList.end();
   for(;begin!=end;++begin)
     {
      QString proxyLabel = (*begin);
      vtkSMProxy* proxy = pxm->GetProxy(
        "filters_prototypes", (*begin).toAscii().data());
      if (proxy && proxy->GetXMLLabel())
        {
        proxyLabel = proxy->GetXMLLabel();
        }
     QAction* recentA = 
       this->Implementation->RecentFiltersMenu->addAction(proxyLabel) 
       << pqSetName(*begin) << pqSetData(*begin);
     recentA->setEnabled(false);
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
  if (!this->createCompoundSource(sourceName))
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
void pqMainWindowCore::onSelectionChanged()
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = this->getActiveServer();

  pqApplicationCore *core = pqApplicationCore::instance();
  int numServers = core->getServerManagerModel()->getNumberOfServers();
  pqGenericViewModule* view = pqActiveView::instance().current();
  pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(view);
  bool pendingDisplays = 
      core->getPendingDisplayManager()->getNumberOfPendingDisplays() > 0;

  // Update the filters menu.
  if(!pendingDisplays)
    {
    this->updateFiltersMenu();
    }

  // Update the server connect/disconnect actions.
  emit this->enableServerConnect(numServers == 0);
  emit this->enableServerDisconnect(server != 0);

  // Update various actions that depend on pending displays.
  this->updatePendingActions(server, source, numServers, pendingDisplays);

  // Update the reset center action.
  emit this->enableResetCenter(source != 0 && renderModule != 0);

  // Update the save screenshot action.
  emit this->enableFileSaveScreenshot(server != 0 && view != 0);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onPendingDisplayChanged(bool pendingDisplays)
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = this->getActiveServer(); 

  emit this->enableFileOpen(!pendingDisplays);

  pqApplicationCore *core = pqApplicationCore::instance();
  int numServers = core->getServerManagerModel()->getNumberOfServers();
  this->updatePendingActions(server, source, numServers, pendingDisplays);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveViewChanged(pqGenericViewModule *view)
{
  pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(view);

  // Update the selection toolbar.
  emit this->enableSelectionToolbar(renderModule != 0);

  // Get the active source and server.
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = this->getActiveServer();

  // Update the reset center action.
  emit this->enableResetCenter(source != 0 && renderModule != 0);

  // Update the show center axis action.
  emit this->enableShowCenterAxis(renderModule != 0);

  // Update the save screenshot action.
  emit this->enableFileSaveScreenshot(server != 0 && view != 0);

  // Update the animation manager if it exists.
  if(this->Implementation->AnimationManager)
    {
    pqAnimationScene *scene =
        this->Implementation->AnimationManager->getActiveScene();
    emit this->enableFileSaveGeometry(scene != 0 && renderModule != 0);
    }

  // Update the view undo/redo state.
  this->updateViewUndoRedo(renderModule);
  if(renderModule)
    {
    // Make sure the render module undo stack is connected.
    this->connect(renderModule->getInteractionUndoStack(),
        SIGNAL(stackChanged(bool, QString, bool, QString)),
        this, SLOT(onActiveViewUndoChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveViewUndoChanged()
{
  pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(
      pqActiveView::instance().current());
  if(renderModule && renderModule == this->sender())
    {
    this->updateViewUndoRedo(renderModule);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveSceneChanged(pqAnimationScene *scene)
{
  pqRenderViewModule *renderModule = qobject_cast<pqRenderViewModule *>(
      pqActiveView::instance().current());
  emit this->enableFileSaveAnimation(scene != 0);
  emit this->enableFileSaveGeometry(scene != 0 && renderModule != 0);
}

//-----------------------------------------------------------------------------
pqServerManagerModelItem *pqMainWindowCore::getActiveObject() const
{
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *selected = selection->selectedItems();
  if(selected->size() == 1)
    {
    item = selected->first();
    }
  else if(selected->size() > 1)
    {
    item = selection->currentItem();
    if(item && !selection->isSelected(item))
      {
      item = 0;
      }
    }

  return item;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::updatePendingActions(pqServer *server,
    pqPipelineSource *source, int numServers, bool pendingDisplays)
{
  // Update the file menu actions.
  emit this->enableFileLoadServerState(!pendingDisplays &&
      (!numServers || server != 0));
  emit this->enableFileSaveServerState(!pendingDisplays && server !=0);
  emit this->enableFileSaveData(!pendingDisplays && source);

  // Update the source and filter menus.
  emit this->enableSourceCreate(!pendingDisplays &&
      (numServers == 0 || server != 0));
  emit this->enableFilterCreate(!pendingDisplays &&
      source != 0 && server != 0);

  // Update the variable toolbar.
  emit this->enableVariableToolbar(source != 0 && !pendingDisplays);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::updateViewUndoRedo(pqRenderViewModule *renderModule)
{
  bool can_undo_camera = false;
  bool can_redo_camera = false;
  QString undo_camera_label;
  QString redo_camera_label;

  if(renderModule)
    {
    pqUndoStack* const stack = renderModule->getInteractionUndoStack();
    if (stack && stack->canUndo())
      {
      can_undo_camera = true;
      undo_camera_label = "Interaction";
      }
    if (stack && stack->canRedo())
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

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerCreation(pqServer* server)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  this->Implementation->ActiveServer.setCurrent(server);

  // Create a render module.
  core->getPipelineBuilder()->createView(server);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerCreationFinished(pqServer *server)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  core->getSelectionModel()->setCurrentItem(server,
      pqServerManagerSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onRemovingServer(pqServer *server)
{
  // Make sure the server and its sources are not selected.
  pqServerManagerSelection toDeselect;
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerSelectionModel *selection = core->getSelectionModel();
  toDeselect.append(server);
  QList<pqPipelineSource*> sources =
      core->getServerManagerModel()->getSources(server);
  QList<pqPipelineSource*>::Iterator iter = sources.begin();
  for( ; iter != sources.end(); ++iter)
    {
    toDeselect.append(*iter);
    }

  selection->select(toDeselect, pqServerManagerSelectionModel::Deselect);
  if(selection->currentItem() == server)
    {
    if(selection->selectedItems()->size() > 0)
      {
      selection->setCurrentItem(selection->selectedItems()->last(),
            pqServerManagerSelectionModel::NoUpdate);
      }
    else
      {
      selection->setCurrentItem(0, pqServerManagerSelectionModel::NoUpdate);
      }
    }

  this->Implementation->ActiveServer.setCurrent(0);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onSourceCreationFinished(pqPipelineSource *source)
{
  if(this->Implementation->ProxyPanel)
    {
    // Make sure the property tab is showing since the accept/reset
    // buttons are on that panel.
    this->Implementation->ProxyPanel->setCurrentIndex(
        pqProxyTabWidget::PROPERTIES);
    }

  // Set the new source as the current selection.
  pqApplicationCore::instance()->getSelectionModel()->setCurrentItem(source,
      pqServerManagerSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onRemovingSource(pqPipelineSource *source)
{
  // If the source is selected, remove it from the selection.
  pqApplicationCore *core = pqApplicationCore::instance();
  pqServerManagerSelectionModel *selection = core->getSelectionModel();
  if(selection->isSelected(source))
    {
    if(selection->selectedItems()->size() > 1)
      {
      // Deselect the source.
      selection->select(source, pqServerManagerSelectionModel::Deselect);

      // If the source is the current item, change the current item.
      if(selection->currentItem() == source)
        {
        selection->setCurrentItem(selection->selectedItems()->last(),
            pqServerManagerSelectionModel::NoUpdate);
        }
      }
    else
      {
      // If the item is a filter and has only one input, set the
      // input as the current item. Otherwise, select the server.
      pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter *>(source);
      if(filter && filter->getInputCount() == 1)
        {
        selection->setCurrentItem(filter->getInput(0),
            pqServerManagerSelectionModel::ClearAndSelect);
        }
      else
        {
        selection->setCurrentItem(source->getServer(),
            pqServerManagerSelectionModel::ClearAndSelect);
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onSourceCreation(pqPipelineSource *source)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  core->getPendingDisplayManager()->addPendingDisplayForSource(source);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onPostAccept()
{
  this->updateFiltersMenu();

  emit this->postAccept();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::updateFiltersMenu()
{
  QMenu* const menu = this->Implementation->FilterMenu;
  if(!menu)
    {
    return;
    }

  // Get the list of selected sources. Make sure the list contains
  // only valid sources.
  const pqServerManagerSelection *selected =
      pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  if(selected->size() == 0)
    {
    // Filters need an input to be created.
    menu->setEnabled(false);
    return;
    }

  pqPipelineSource *source = 0;
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelection::ConstIterator iter = selected->begin();
  for( ; iter != selected->end(); ++iter)
    {
    item = *iter;
    pqServer *server = dynamic_cast<pqServer *>(item);
    if(server)
      {
      // A server is not a valid input.
      menu->setEnabled(false);
      return;
      }

    source = dynamic_cast<pqPipelineSource *>(item);
    if(!source || !source->getProxy())
      {
      // Unsupported/unknown input type or missing proxy.
      menu->setEnabled(false);
      return;
      }
    }

  // Get the list of available filters.
  QList<QString> supportedFilters;
  pqApplicationCore::instance()->getPipelineBuilder()->
    getSupportedProxies("filters", source->getServer(), supportedFilters);

  // Iterate over all filters in the menu and see if they can be
  // applied to the current source(s).
  bool some_enabled = false;
  vtkSMProxyManager *proxyManager = vtkSMProxyManager::GetProxyManager();
  QList<QAction *> menu_actions = menu->findChildren<QAction *>();
  QList<QAction *>::Iterator action = menu_actions.begin();
  for( ; action != menu_actions.end(); ++action)
    {
    QString filterName = (*action)->data().toString();
    if (filterName.isEmpty())
      {
      continue;
      }

    (*action)->setEnabled(false);
    if (!supportedFilters.contains(filterName))
      {
      // skip filters not supported by the server.
      continue;
      }

    vtkSMProxy* output = proxyManager->GetProxy("filters_prototypes",
      filterName.toAscii().data());
    if (!output)
      {
      continue;
      }

    vtkSMInputProperty *input = vtkSMInputProperty::SafeDownCast(
      output->GetProperty("Input"));
    if(input)
      {
      if(!input->GetMultipleInput() && selected->size() > 1)
        {
        continue;
        }

      input->RemoveAllUncheckedProxies();
      for(iter = selected->begin(); iter != selected->end(); ++iter)
        {
        item = *iter;
        source = dynamic_cast<pqPipelineSource *>(item);
        input->AddUncheckedProxy(source->getProxy());
        }

      if(input->IsInDomains())
        {
        (*action)->setEnabled(true);
        some_enabled = true;
        }
      }
    }

  menu->setEnabled(some_enabled);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::getActiveSource()
{
  return dynamic_cast<pqPipelineSource *>(this->getActiveObject());
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::getRootSources(QList<pqPipelineSource*> *sources, pqPipelineSource *src)
{
  pqPipelineFilter *filter = dynamic_cast<pqPipelineFilter*>(src);
  if(!filter || filter->getInputCount()==0)
    {
    sources->push_back(src);
    return;
    }
  for(int i=0; i<filter->getInputCount(); i++)
    {
    this->getRootSources(sources, filter->getInput(i));
    }
}


//-----------------------------------------------------------------------------
pqServer* pqMainWindowCore::getActiveServer()
{
  return this->Implementation->ActiveServer.current();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::removeActiveSource()
{
  pqPipelineSource* source = this->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source to remove.";
    return;
    }
  pqApplicationCore::instance()->removeSource(source);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::removeActiveServer()
{
  pqServer* server = this->getActiveServer();
  if (!server)
    {
    qDebug() << "No active server to remove.";
    return;
    }
  pqApplicationCore::instance()->removeServer(server);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createSourceOnActiveServer(
  const QString& xmlname)
{
  return pqApplicationCore::instance()->createSourceOnServer(xmlname, 
                 this->getActiveServer());
}


//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createFilterForActiveSource(
  const QString& xmlname)
{
  // Get the list of selected sources.
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerSelection selected =
      *core->getSelectionModel()->selectedItems();
  pqPipelineSource* source = 0;
  pqPipelineSource* filter = 0;
  pqServerManagerModelItem* item = 0;
  pqServerManagerSelection::ConstIterator iter = selected.begin();
  if(iter != selected.end())
    {
    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);
    filter = core->createFilterForSource(xmlname, source);
    ++iter;
    }

  pqPipelineBuilder* builder = core->getPipelineBuilder();
  for( ; filter && iter != selected.end(); ++iter)
    {
    item = *iter;
    source = dynamic_cast<pqPipelineSource*>(item);
    builder->addConnection(source, filter);
    }

  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createCompoundSource(
  const QString& name)
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = dynamic_cast<pqServer *>(item);
  if(!server && source)
    {
    server = source->getServer();
    }

  pqPipelineSource* cp = pqApplicationCore::instance()->createCompoundFilter(
      name, server, source);

  cp->getProxy()->UpdateVTKObjects();
  
  return cp;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createReaderOnActiveServer(
  const QString& filename)
{
  return pqApplicationCore::instance()->createReaderOnServer(filename,
     this->getActiveServer());
}

void pqMainWindowCore::disableAutomaticDisplays()
{
  QObject::disconnect(pqApplicationCore::instance(),
    SIGNAL(finishSourceCreation(pqPipelineSource*)),
    this, SLOT(onSourceCreation(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::createPendingDisplays()
{
  pqGenericViewModule* view = pqActiveView::instance().current();
  pqApplicationCore::instance()->getPendingDisplayManager()->
    createPendingDisplays(view);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetCamera()
{
  pqRenderViewModule* ren = qobject_cast<pqRenderViewModule*>(pqActiveView::instance().current());
  if (ren)
    {
    ren->resetCamera();
    ren->render();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z)
{
  pqRenderViewModule* ren = qobject_cast<pqRenderViewModule*>(pqActiveView::instance().current());
  if (ren)
    {
    vtkSMRenderModuleProxy* proxy = ren->getRenderModuleProxy();
    proxy->SynchronizeCameraProperties();
 
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraPosition"), 0, 0);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraPosition"), 1, 0);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraPosition"), 2, 0);

    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraFocalPoint"), 0, look_x);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraFocalPoint"), 1, look_y);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraFocalPoint"), 2, look_z);

    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraViewUp"), 0, up_x);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraViewUp"), 1, up_y);
    pqSMAdaptor::setMultipleElementProperty(
      proxy->GetProperty("CameraViewUp"), 2, up_z);
    proxy->UpdateVTKObjects();

    ren->resetCamera();
    ren->render();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionPosX()
{
  this->resetViewDirection(1, 0, 0, 0, 0, 1);
}
//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionNegX()
{
  this->resetViewDirection(-1, 0, 0, 0, 0, 1);

}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionPosY()
{
  this->resetViewDirection(0, 1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionNegY()
{
  this->resetViewDirection(0, -1, 0, 0, 0, 1);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionPosZ()
{
  this->resetViewDirection(0, 0, 1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirectionNegZ()
{
  this->resetViewDirection(0, 0, -1, 0, 1, 0);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::enableTestingRenderWindowSize(bool enable)
{
  this->setMaxRenderWindowSize(
    enable? QSize(300, 300) : QSize(-1, -1));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setMaxRenderWindowSize(const QSize& size)
{
  this->Implementation->MultiViewManager.setMaxViewWindowSize(size);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::createBarCharView()
{
    pqApplicationCore::instance()->getPipelineBuilder()->createView(
      this->getActiveServer(), "BarChart");
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::createXYPlotView()
{
  pqApplicationCore::instance()->getPipelineBuilder()->createView(
    this->getActiveServer(), "XYPlot");
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::createTableView()
{
  pqApplicationCore::instance()->getPipelineBuilder()->createView(
    this->getActiveServer(), "TableView");
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetCenterOfRotationToCenterOfCurrentData()
{
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(
    pqActiveView::instance().current());
  if (!rm)
    {
    qDebug() << "No active render module. Cannot reset center of rotation.";
    return;
    }
  pqPipelineSource* source = this->getActiveSource();
  if (!source)
    {
    qDebug() << "No active source. Cannot reset center of rotation.";
    return;
    }
  pqPipelineDisplay* display = qobject_cast<pqPipelineDisplay*>(
    source->getDisplay(rm));
  if (!display)
    {
    //qDebug() << "Active source not shown in active view. Cannot reset center.";
    return;
    }
  double bounds[6];
  if (display->getDataBounds(bounds))
    {
    double center[3];
    center[0] = (bounds[1]+bounds[0])/2.0;
    center[1] = (bounds[3]+bounds[2])/2.0;
    center[2] = (bounds[5]+bounds[4])/2.0;
    rm->setCenterOfRotation(center);
    rm->render();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setCenterAxesVisibility(bool visible)
{
  pqRenderViewModule* rm = qobject_cast<pqRenderViewModule*>(
    pqActiveView::instance().current());
  if (!rm)
    {
    qDebug() << "No active render module. setCenterAxesVisibility failed.";
    return;
    }
  rm->setCenterAxesVisibility(visible);
  rm->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsManageLinks()
{
  if(this->Implementation->LinksManager)
    {
    this->Implementation->LinksManager->raise();
    this->Implementation->LinksManager->activateWindow();
    }
  else
    {
    this->Implementation->LinksManager = new
      pqLinksManager(this->Implementation->Parent);
    this->Implementation->LinksManager->setWindowTitle("Link Manager");
    this->Implementation->LinksManager->setAttribute(Qt::WA_DeleteOnClose);
    this->Implementation->LinksManager->show();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onManagePlugins()
{
  pqPluginDialog diag(this->getActiveServer(), this->Implementation->Parent);
  diag.exec();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEnterSelectionIds()
{
  //pop up the dialog to ask for ids to select
  pqEnterIdsDialog* const points_dialog = new pqEnterIdsDialog( 
    this->Implementation->Parent);
    
  points_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(
    points_dialog, 
    SIGNAL(idsEntered(int)), 
    this, 
    SLOT(onIdsEntered(int))
    );
  points_dialog->setModal(true); 
  points_dialog->show(); 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onIdsEntered(int id)
{
  //take the ids selected in the dialog and tell paraview to extract them
  this->Implementation->SelectionManager.setIds(id);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEnterSelectionPoints()
{
  //pop up the dialog to ask for points to select
  pqEnterPointsDialog* const points_dialog = new pqEnterPointsDialog( 
    this->Implementation->Parent);
    
  points_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(
    points_dialog, 
    SIGNAL(pointsEntered(double , double , double )), 
    this, 
    SLOT(onPointsEntered(double , double , double ))
    );
  points_dialog->setModal(true); 
  points_dialog->show(); 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onPointsEntered(double X, double Y, double Z)
{
  //take the points selected in the dialog and tell paraview to extract them
  this->Implementation->SelectionManager.setPoints(X,Y,Z);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEnterSelectionThresholds()
{
  //pop up the dialog to ask for thresholds to select
  pqEnterThresholdsDialog* const points_dialog = new pqEnterThresholdsDialog( 
    this->Implementation->Parent);
    
  points_dialog->setAttribute(Qt::WA_DeleteOnClose);
  QObject::connect(
    points_dialog, 
    SIGNAL(thresholdsEntered(double , double)), //toil and trouble 
    this, 
    SLOT(onThresholdsEntered(double , double))
    );
  points_dialog->setModal(true); 
  points_dialog->show(); 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onThresholdsEntered(double min, double max)
{
  //take the thresholds selected in the dialog and tell paraview to extract them
  this->Implementation->SelectionManager.setThresholds(min, max);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addPluginActions(QObject* iface)
{
  pqActionGroupInterface* actionGroup =
    qobject_cast<pqActionGroupInterface*>(iface);
  if(actionGroup)
    {
    this->addPluginActions(actionGroup);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addPluginActions(pqActionGroupInterface* iface)
{
  QString name = iface->groupName();
  QStringList splitName = name.split('/', QString::SkipEmptyParts);

  QMainWindow* mw = qobject_cast<QMainWindow*>(this->Implementation->Parent);
  if(!mw)
    {
    QWidgetList allWidgets = QApplication::topLevelWidgets();
    QWidgetList::iterator iter;
    for(iter = allWidgets.begin(); !mw && iter != allWidgets.end(); ++iter)
      {
      mw = qobject_cast<QMainWindow*>(*iter);
      }
    }

  if(!mw)
    {
    qWarning("Could not find MainWindow for actions group");
    return;
    }

  if(splitName.size() == 2 && splitName[0] == "ToolBar")
    {
    QToolBar* tb = new QToolBar(splitName[1], mw);
    tb->setObjectName(splitName[1]);
    tb->addActions(iface->actionGroup()->actions());
    mw->addToolBar(tb);
    this->Implementation->PluginToolBars.append(tb);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::removePluginToolBars()
{
  qDeleteAll(this->Implementation->PluginToolBars);
  this->Implementation->PluginToolBars.clear();
}

