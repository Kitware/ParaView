/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowCore.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
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

=========================================================================*/

#include "pqMainWindowCore.h"
#include <vtkPQConfig.h>

#include <QAction>
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
#include <QDoubleSpinBox>
#include <QMenuBar>

#include "pqActionGroupInterface.h"
#include "pqActiveChartOptions.h"
#include "pqActiveRenderViewOptions.h"
#include "pqActiveServer.h"
#include "pqActiveTwoDRenderViewOptions.h"
#include "pqActiveView.h"
#include "pqActiveViewOptionsManager.h"
#include "pqAnimationManager.h"
#include "pqAnimationViewWidget.h"
#include "pqApplicationCore.h"
#include "pqApplicationOptionsDialog.h"
#include "pqBarChartView.h"
#include "pqBarChartViewContextMenuHandler.h"
#include "pqBoxChartViewContextMenuHandler.h"
#include "pqCameraDialog.h"
#include "pqColorScaleToolbar.h"
#include "pqCloseViewUndoElement.h"
#include "pqCustomFilterDefinitionModel.h"
#include "pqCustomFilterDefinitionWizard.h"
#include "pqCustomFilterManager.h"
#include "pqCustomFilterManagerModel.h"
#include "pqDataInformationWidget.h"
#include "pqDisplayColorWidget.h"
#include "pqDisplayRepresentationWidget.h"
#include "pqDockWindowInterface.h"
#include "pqFilterInputDialog.h"
#include "pqFiltersMenuManager.h"
#include "pqSourcesMenuManager.h"
#include "pqHelperProxyRegisterUndoElement.h"
#include "pqLineChartView.h"
#include "pqLineChartViewContextMenuHandler.h"
#include "pqLinksManager.h"
#include "pqLookmarkBrowser.h"
#include "pqLookmarkBrowserModel.h"
#include "pqLookmarkDefinitionWizard.h"
#include "pqLookmarkInspector.h"
#include "pqLookmarkManagerModel.h"
#include "pqLookmarkModel.h"
#include "pqLookmarkToolbar.h"
#include "pqMainWindowCore.h"
#include "pqMultiViewFrame.h"
#include "pqMultiView.h"
#include "pqObjectBuilder.h"
#include "pqObjectInspectorDriver.h"
#include "pqObjectInspectorWidget.h"
#include "pqOptions.h"
#include "pqOutputPort.h"
#include "pqPendingDisplayManager.h"
#include "pqPickHelper.h"
#include "pqPipelineBrowser.h"
#include "pqPipelineFilter.h"
#include "pqPipelineMenu.h"
#include "pqPipelineModel.h"
#include "pqPipelineRepresentation.h"
#include "pqPluginDialog.h"
#include "pqPluginManager.h"
#include "pqPQLookupTableManager.h"
#include "pqProcessModuleGUIHelper.h"
#include "pqProgressManager.h"
#include "pqProxyTabWidget.h"

#ifdef PARAVIEW_ENABLE_PYTHON
#include "pqPythonManager.h"
#include "pqPythonDialog.h"
#endif // PARAVIEW_ENABLE_PYTHON

#include "pqReaderFactory.h"
#include "pqRenderView.h"
#include "pqRubberBandHelper.h"
#include "pqSelectionInspectorPanel.h"
#include "pqSelectionManager.h"
#include "pqSelectReaderDialog.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"
#include "pqServerManagerObserver.h"
#include "pqServerManagerSelectionModel.h"
#include "pqServerStartupBrowser.h"
#include "pqServerStartup.h"
#include "pqServerStartups.h"
#include "pqSettings.h"
#include "pqSimpleServerStartup.h"
#include "pqSMAdaptor.h"
#include "pqSplitViewUndoElement.h"
#include "pqSpreadSheetView.h"
#include "pqSpreadSheetViewDecorator.h"
#include "pqStackedChartViewContextMenuHandler.h"
#include "pqStateLoader.h"
#include "pqTimeKeeper.h"
#include "pqTimerLogDisplay.h"
#include "pqToolTipTrapper.h"
#include "pqTwoDRenderView.h"
#include "pqUndoStackBuilder.h"
#include "pqVCRController.h"
#include "pqViewContextMenuManager.h"
#include "pqView.h"
#include "pqViewManager.h"
#include "pqViewMenu.h"
#include "pqWriterFactory.h"
#include "pqSaveSnapshotDialog.h"
#include "pqQuickLaunchDialog.h"
#include "pqViewOptionsInterface.h"
#include "pqViewExporterManager.h"
#include "pqImageUtil.h"

#include <pqFileDialog.h>
#include <pqObjectNaming.h>
#include <pqProgressWidget.h>
#include <pqServerResources.h>
#include <pqSetData.h>
#include <pqSetName.h>
#include <pqCoreTestUtility.h>
#include <pqUndoStack.h>
#include <pqWriterDialog.h>
#include "QtTestingConfigure.h"

#include <QVTKWidget.h>

#include <vtkDataObject.h>
#include <vtkImageData.h>
#include <vtkProcessModule.h>
#include <vtkPVDisplayInformation.h>
#include <vtkPVOptions.h>
#include <vtkPVXMLElement.h>
#include <vtkPVXMLParser.h>
#include <vtkSmartPointer.h>
#include <vtkSMDoubleRangeDomain.h>
#include <vtkSMDoubleVectorProperty.h>
#include <vtkSMInputProperty.h>
#include <vtkSMIntVectorProperty.h>
#include <vtkSMProxyIterator.h>
#include <vtkSMProxyManager.h>
#include <vtkSMProxyProperty.h>
#include <vtkSMRenderViewProxy.h>
#include <vtkSMSourceProxy.h>
#include <vtkSMStringVectorProperty.h>

#include <vtkToolkits.h>

#include <vtkstd/algorithm>
#include <vtkstd/map>
#include <vtkstd/set>
#include <vtkstd/string>
#include <vtkstd/vector>

#include <cassert>
#include <ctime>

// If CrashRecovery is set under Edit->Settings, then before each 
// "Apply" a state file is saved to this file.
const char CrashRecoveryStateFile[]=".PV3CrashRecoveryState.pvsm";

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
    ActiveViewOptions(0),
    ViewContextMenu(0),
    PipelineMenu(0),
    PipelineBrowser(0),
    VariableToolbar(0),
    LookmarkToolbar(0),
    ToolTipTrapper(0),
    InCreateSource(false),
    ColorScale(0),
    LinksManager(0),
    TimerLog(0), 
    QuickLaunchDialog(parent)
  {
  this->MultiViewManager.setObjectName("MultiViewManager");
  this->CameraDialog = 0;
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

  void mySetParent(QWidget *parent) 
  {
    this->Parent = parent;
    this->MultiViewManager.setParent(parent);
    this->CustomFilters->setParent(parent);
    this->LookupTableManager->setParent(parent);
  }

  QWidget* Parent;
  pqViewManager MultiViewManager;
  pqVCRController VCRController;
  pqSelectionManager SelectionManager;
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonManager PythonManager;
#endif // PARAVIEW_ENABLE_PYTHON
  pqLookmarkManagerModel* LookmarkManagerModel;
  pqLookmarkBrowser* LookmarkBrowser;
  pqLookmarkInspector* LookmarkInspector;
  QString CurrentToolbarLookmark;
  pqLookmarkBrowserModel* Lookmarks;
  pqCustomFilterManagerModel* const CustomFilters;
  pqCustomFilterManager* CustomFilterManager;
  pqPQLookupTableManager* LookupTableManager;
  pqObjectInspectorDriver* ObjectInspectorDriver;
  pqActiveViewOptionsManager *ActiveViewOptions;
  pqViewContextMenuManager *ViewContextMenu;
  pqReaderFactory ReaderFactory;
  pqWriterFactory WriterFactory;
  pqPendingDisplayManager PendingDisplayManager;
  pqRubberBandHelper RenderViewSelectionHelper;
  pqPickHelper RenderViewPickHelper;
  pqViewExporterManager ViewExporterManager;
  QPointer<pqUndoStack> UndoStack;
 
  QPointer<pqFiltersMenuManager> FiltersMenuManager;
  QPointer<pqSourcesMenuManager> SourcesMenuManager;
  QPointer<pqViewMenu> ToolbarMenu;
  QPointer<pqViewMenu> DockWindowMenu;

  pqPipelineMenu* PipelineMenu;
  pqPipelineBrowser *PipelineBrowser;
  QToolBar* VariableToolbar;
  QToolBar* LookmarkToolbar;
  QList<QObject*> PluginToolBars;
  
  pqToolTipTrapper* ToolTipTrapper;
  
  QPointer<pqCameraDialog> CameraDialog;

  bool InCreateSource;

  QPointer<pqColorScaleToolbar> ColorScale;
  
  QPointer<pqProxyTabWidget> ProxyPanel;
  QPointer<pqAnimationManager> AnimationManager;
  QPointer<pqLinksManager> LinksManager;
  QPointer<pqTimerLogDisplay> TimerLog;
  QPointer<pqApplicationOptionsDialog> ApplicationSettings;

  pqCoreTestUtility TestUtility;
  pqActiveServer ActiveServer;
  pqQuickLaunchDialog QuickLaunchDialog;
  
  QPointer<pqPipelineSource> PreviouslySelectedSource;
};

///////////////////////////////////////////////////////////////////////////
// pqMainWindowCore

pqMainWindowCore::pqMainWindowCore(QWidget* parent_widget)
{
  this->Implementation = new pqImplementation(parent_widget);
  this->constructorHelper(parent_widget);
}

pqMainWindowCore::pqMainWindowCore()
{
  this->Implementation = new pqImplementation(NULL);
  this->constructorHelper(NULL);
}

void pqMainWindowCore::setParent(QWidget* newParent) 
{
  this->Implementation->mySetParent(newParent);
}

void pqMainWindowCore::constructorHelper(QWidget *parent_widget)
{
  this->setObjectName("MainWindowCore");
  
  pqApplicationCore* const core = pqApplicationCore::instance();
  pqObjectBuilder* const builder = core->getObjectBuilder();

  core->setLookupTableManager(this->Implementation->LookupTableManager);

  // Register some universally accessible managers.
  core->registerManager("PENDING_DISPLAY_MANAGER", 
    &this->Implementation->PendingDisplayManager);
  core->registerManager("MULTIVIEW_MANAGER",
    &this->Implementation->MultiViewManager);
  core->registerManager("SELECTION_MANAGER",
    &this->Implementation->SelectionManager);

#ifdef PARAVIEW_ENABLE_PYTHON
  this->Implementation->PythonManager.setParentForPythonDialog(parent_widget);
#endif // PARAVIEW_ENABLE_PYTHON

  // Set up the context menu manager.
  this->getViewContextMenuManager();

  // Connect the view manager to the pqActiveView.
  QObject::connect(&this->Implementation->MultiViewManager,
    SIGNAL(activeViewChanged(pqView*)),
    &pqActiveView::instance(), SLOT(setCurrent(pqView*)));
  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    &this->Implementation->MultiViewManager, SLOT(setActiveView(pqView*)));

  // Connect the view manager's camera button.
  QObject::connect(&this->Implementation->MultiViewManager,
    SIGNAL(triggerCameraAdjustment(pqView*)),
    this, SLOT(showCameraDialog(pqView*)));

  this->Implementation->MultiViewManager.setViewOptionsManager(
    this->getActiveViewOptionsManager());

  // Listen to the active render module changed signals.
  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this, SLOT(onActiveViewChanged(pqView*)));

  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    &this->selectionManager(), SLOT(setActiveView(pqView*)));
    
  // Listen for compound proxy register events.
  pqServerManagerObserver *observer =
      pqApplicationCore::instance()->getServerManagerObserver();
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)),
      this->Implementation->CustomFilters, SLOT(addCustomFilter(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
      this->Implementation->CustomFilters, SLOT(removeCustomFilter(QString)));
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)),
                this, SIGNAL(refreshFiltersMenu()));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
                this, SIGNAL(refreshFiltersMenu()));
  this->connect(observer, SIGNAL(compoundProxyDefinitionRegistered(QString)),
                this, SIGNAL(refreshSourcesMenu()));
  this->connect(observer, SIGNAL(compoundProxyDefinitionUnRegistered(QString)),
                this, SIGNAL(refreshSourcesMenu()));
  // Now that the connections are set up, import custom filters from settings
  this->Implementation->CustomFilters->importCustomFiltersFromSettings();

  // Set up connection with selection helpers for all views.
  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    &this->Implementation->RenderViewSelectionHelper, SLOT(setView(pqView*)));

  // BUG #5924. Disable selection with picking the center of rotation.
  QObject::connect(
    &this->Implementation->RenderViewPickHelper, SIGNAL(startPicking()),
    &this->Implementation->RenderViewSelectionHelper, SLOT(DisabledPush()));
  QObject::connect(
    &this->Implementation->RenderViewPickHelper, SIGNAL(stopPicking()),
    &this->Implementation->RenderViewSelectionHelper, SLOT(DisabledPop()));

  // Connect up the pqLookmarkManagerModel and pqLookmarkBrowserModel
  this->Implementation->LookmarkManagerModel = new pqLookmarkManagerModel(this);

  this->Implementation->Lookmarks = new pqLookmarkBrowserModel(
    this->Implementation->LookmarkManagerModel,parent_widget);
  QObject::connect(this->Implementation->LookmarkManagerModel,
      SIGNAL(lookmarkAdded(pqLookmarkModel*)),
      this->Implementation->Lookmarks,
      SLOT(addLookmark(pqLookmarkModel*)));
  QObject::connect(this->Implementation->LookmarkManagerModel,
      SIGNAL(lookmarkRemoved(const QString&)),
      this->Implementation->Lookmarks,
      SLOT(removeLookmark(const QString&)));
  QObject::connect(this->Implementation->LookmarkManagerModel,
      SIGNAL(lookmarkModified(pqLookmarkModel*)),
      this->Implementation->Lookmarks,
      SLOT(onLookmarkModified(pqLookmarkModel*)));
  QObject::connect(this->Implementation->Lookmarks,
      SIGNAL(lookmarkRemoved(const QString&)),
      this->Implementation->LookmarkManagerModel,
      SLOT(removeLookmark(const QString&)));
  QObject::connect(this->Implementation->Lookmarks,
      SIGNAL(importLookmarks(const QStringList&)),
      this->Implementation->LookmarkManagerModel,
      SLOT(importLookmarksFromFiles(const QStringList&)));
  QObject::connect(this->Implementation->Lookmarks,
      SIGNAL(exportLookmarks(const QList<pqLookmarkModel*>&,const QStringList&)),
      this->Implementation->LookmarkManagerModel,
      SLOT(exportLookmarksToFiles(const QList<pqLookmarkModel*>&,const QStringList&)));

  // Listen to selection changed events.
  // These are queued connections, since while changes are happening the SM
  // may not be in a good state to check which filters should be enabled 
  // etc etc.
  // As a general policy, GUI updates must be QueuedConnection. This policy
  // does not apply to core layer i.e. creation of pqProxies etc.
  pqServerManagerSelectionModel *selection =
      pqApplicationCore::instance()->getSelectionModel();
  this->connect(selection, SIGNAL(currentChanged(pqServerManagerModelItem*)),
      this, SLOT(onSelectionChanged()), Qt::QueuedConnection);
  this->connect(selection,
      SIGNAL(selectionChanged(
          const pqServerManagerSelection&, const pqServerManagerSelection&)),
      this, SLOT(onSelectionChanged()), Qt::QueuedConnection);

  // Update enable state when pending displays state changes.
  this->connect(
    &this->Implementation->PendingDisplayManager, SIGNAL(pendingDisplays(bool)),
    this, SLOT(onPendingDisplayChanged(bool)));

  this->connect(core->getServerManagerModel(), 
    SIGNAL(serverAdded(pqServer*)),
    this, SLOT(onServerCreation(pqServer*)));

  this->connect(core->getObjectBuilder(), 
    SIGNAL(finishedAddingServer(pqServer*)),
    this, SLOT(onServerCreationFinished(pqServer*)));

  this->connect(core->getServerManagerModel(),
      SIGNAL(aboutToRemoveServer(pqServer*)),
      this, SLOT(onRemovingServer(pqServer*)));
  this->connect(core->getServerManagerModel(),
      SIGNAL(finishedRemovingServer()),
      this, SLOT(onSelectionChanged()));

  this->connect(builder, SIGNAL(sourceCreated(pqPipelineSource*)),
    this, SLOT(onSourceCreationFinished(pqPipelineSource*)),
    Qt::QueuedConnection);

  this->connect(builder, SIGNAL(filterCreated(pqPipelineSource*)),
    this, SLOT(onSourceCreationFinished(pqPipelineSource*)),
    Qt::QueuedConnection);

  this->connect(builder, 
    SIGNAL(readerCreated(pqPipelineSource*, const QString&)),
    this, SLOT(onSourceCreationFinished(pqPipelineSource*)),
    Qt::QueuedConnection);

  this->connect(builder, 
    SIGNAL(readerCreated(pqPipelineSource*, const QStringList&)),
    this, SLOT(onReaderCreated(pqPipelineSource*, const QStringList&)));

  this->connect(builder, SIGNAL(sourceCreated(pqPipelineSource*)),
    this, SLOT(onSourceCreation(pqPipelineSource*)));

  this->connect(builder, SIGNAL(filterCreated(pqPipelineSource*)),
    this, SLOT(onSourceCreation(pqPipelineSource*)));

  this->connect(builder, 
    SIGNAL(readerCreated(pqPipelineSource*, const QString&)),
    this, SLOT(onSourceCreation(pqPipelineSource*)));

  this->connect(builder, SIGNAL(destroying(pqPipelineSource*)),
    this, SLOT(onRemovingSource(pqPipelineSource*)));

  this->connect(builder, SIGNAL(proxyCreated(pqProxy*)),
    this, SLOT(onProxyCreation(pqProxy*)));

  this->connect(builder, SIGNAL(viewCreated(pqView*)),
    this, SLOT(onViewCreated(pqView*)));

  // Listen for the signal that the lookmark button for a given view was pressed
  this->connect(&this->Implementation->MultiViewManager, 
                SIGNAL(createLookmark(QWidget*)), //pqGenericViewModule*)),
                this,
                SLOT(onToolsCreateLookmark(QWidget*))); //pqGenericViewModule*)));

  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(serverManagerExtensionLoaded()),
                this,
                SIGNAL(refreshFiltersMenu()));
  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(serverManagerExtensionLoaded()),
                this,
                SIGNAL(refreshSourcesMenu()));
  
  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(guiInterfaceLoaded(QObject*)),
                this, SLOT(addPluginInterface(QObject*)));
  this->connect(pqApplicationCore::instance()->getPluginManager(),
                SIGNAL(guiExtensionLoaded()),
                this, SLOT(extensionLoaded()));

/*
  this->installEventFilter(this);
*/
  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    &this->Implementation->MultiViewManager, SLOT(setActiveServer(pqServer*)));

  // setup Undo Stack.
  pqUndoStackBuilder* usBuilder = pqUndoStackBuilder::New();
  this->Implementation->UndoStack = new pqUndoStack(false, usBuilder, this);
  usBuilder->Delete();

  pqSplitViewUndoElement* svu_elem = pqSplitViewUndoElement::New();
  this->Implementation->UndoStack->registerElementForLoader(svu_elem);
  svu_elem->Delete();

  pqCloseViewUndoElement* cvu_elem = pqCloseViewUndoElement::New();
  this->Implementation->UndoStack->registerElementForLoader(cvu_elem);
  cvu_elem->Delete();

  this->Implementation->PendingDisplayManager.setUndoStack(
    this->Implementation->UndoStack);
  this->Implementation->MultiViewManager.setUndoStack(
    this->Implementation->UndoStack);

  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    this->Implementation->UndoStack, SLOT(setActiveServer(pqServer*))); 

  // clear undo stack when state is loaded.
  QObject::connect(core, SIGNAL(stateLoaded()),
    this->Implementation->UndoStack, SLOT(clear()));

  QObject::connect(
    &this->Implementation->VCRController, SIGNAL(beginNonUndoableChanges()),
    this->Implementation->UndoStack, SLOT(beginNonUndoableChanges()));
  QObject::connect(
    &this->Implementation->VCRController, SIGNAL(endNonUndoableChanges()),
    this->Implementation->UndoStack, SLOT(endNonUndoableChanges()));

  core->setUndoStack(this->Implementation->UndoStack);

  // set up state loader.
  pqStateLoader* loader = pqStateLoader::New();
  loader->SetMainWindowCore(this);
  core->setStateLoader(loader);
  loader->Delete();

  // Set up a callback to before further intialization once the application
  // event loop starts.
  QTimer::singleShot(100, this, SLOT(applicationInitialize()));

  // Instantiate prototypes for sources and filters. These are used
  // in populating sources and filters menus.
  vtkSMProxyManager* pxm = vtkSMObject::GetProxyManager();
  pxm->InstantiateGroupPrototypes("sources");
  pxm->InstantiateGroupPrototypes("filters");


  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    &this->Implementation->RenderViewPickHelper, SLOT(setView(pqView*)));

  // BUG #5924. Don't want to let picking be enabled when selecting.
  QObject::connect(
    &this->Implementation->RenderViewSelectionHelper, SIGNAL(startSelection()),
    &this->Implementation->RenderViewPickHelper, SLOT(DisabledPush()));
  QObject::connect(
    &this->Implementation->RenderViewSelectionHelper, SIGNAL(stopSelection()),
    &this->Implementation->RenderViewPickHelper, SLOT(DisabledPop()));

  QObject::connect(&this->Implementation->RenderViewPickHelper, 
                   SIGNAL(pickFinished(double, double, double)),
                   this, 
                   SLOT(pickCenterOfRotationFinished(double, double, double)));

  QObject::connect(&this->Implementation->RenderViewPickHelper,
    SIGNAL(enabled(bool)), 
    this, SIGNAL(enablePickCenter(bool)));
  QObject::connect(&this->Implementation->RenderViewPickHelper,
    SIGNAL(picking(bool)), 
    this, SIGNAL(pickingCenter(bool)));

  // Make the view manager non-blockable so that none of the views are disabled.
  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();
  progress_manager->addNonBlockableObject(
    &this->Implementation->MultiViewManager);

  /// Set up the view exporter.
  QObject::connect(&this->Implementation->ViewExporterManager,
    SIGNAL(exportable(bool)), 
    this, SIGNAL(enableExport(bool)));

  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
    &this->Implementation->ViewExporterManager,
    SLOT(setView(pqView*)));

  QObject::connect(pqApplicationCore::instance()->getPluginManager(),
    SIGNAL(serverManagerExtensionLoaded()),
    &this->Implementation->ViewExporterManager,
    SLOT(refresh()));

  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
      &this->Implementation->PendingDisplayManager,
      SLOT(setActiveView(pqView*)));

  // Register the color scale editor manager with the application so it
  // can be used by the display panels.
  core->registerManager("COLOR_SCALE_EDITOR",
      this->getColorScaleEditorManager());

  // the most recently used file extensions
  this->restoreSettings();
}

//-----------------------------------------------------------------------------
pqMainWindowCore::~pqMainWindowCore()
{
  // Paraview is closing all is well, remove the crash
  // recovery file.
  if (QFile::exists(CrashRecoveryStateFile))
    {
    QFile::remove(CrashRecoveryStateFile);
    }

  this->saveSettings();
  delete Implementation;
}

//-----------------------------------------------------------------------------
pqViewManager& pqMainWindowCore::multiViewManager()
{
  return this->Implementation->MultiViewManager;
}

//-----------------------------------------------------------------------------
pqSelectionManager& pqMainWindowCore::selectionManager()
{
  return this->Implementation->SelectionManager;
}

//-----------------------------------------------------------------------------
pqVCRController& pqMainWindowCore::VCRController()
{
  return this->Implementation->VCRController;
}

//-----------------------------------------------------------------------------
pqRubberBandHelper* pqMainWindowCore::renderViewSelectionHelper() const
{
  return &this->Implementation->RenderViewSelectionHelper;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setSourceMenu(QMenu* menu)
{
  delete this->Implementation->SourcesMenuManager;
  this->Implementation->SourcesMenuManager = 0;
  if (menu)
    {
    pqSourcesMenuManager*fmm = new pqSourcesMenuManager(menu);
    fmm->setXMLGroup("sources");
    QDir custom(":/CustomResources");
    if (custom.exists("CustomSources.xml"))
      {
      fmm->setFilteringXMLDir(":/CustomResources");
      }
    else
      {
      fmm->setFilteringXMLDir(":/ParaViewResources");
      }
    fmm->setElementTagName("Source");
    fmm->setRecentlyUsedMenuSize(0);
    QObject::connect(fmm, SIGNAL(selected(const QString&)),
      this, SLOT(onCreateSource(const QString&)));
    QObject::connect(this, SIGNAL(refreshSourcesMenu()),
      fmm, SLOT(update()));
    QObject::connect(this, SIGNAL(enableSourceCreate(bool)),
      fmm, SLOT(setEnabled(bool)));
    this->Implementation->SourcesMenuManager= fmm;
    fmm->initialize();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setFilterMenu(QMenu* menu)
{
  delete this->Implementation->FiltersMenuManager;
  this->Implementation->FiltersMenuManager = 0;
  if (menu)
    {
    pqFiltersMenuManager *fmm = new pqFiltersMenuManager(menu);
    fmm->setXMLGroup("filters");
    QDir custom(":/CustomResources");
    if (custom.exists("CustomFilters.xml"))
      {
      fmm->setFilteringXMLDir(":/CustomResources");
      }
    else
      {
      fmm->setFilteringXMLDir(":/ParaViewResources");
      }
    fmm->setElementTagName("Filter");
    fmm->setRecentlyUsedMenuSize(10);
    QObject::connect(fmm, SIGNAL(selected(const QString&)),
      this, SLOT(onCreateFilter(const QString&)),
      Qt::QueuedConnection);
    QObject::connect(this, SIGNAL(refreshFiltersMenu()),
      fmm, SLOT(update()));
    QObject::connect(this, SIGNAL(enableFilterCreate(bool)),
      fmm, SLOT(setEnabled(bool)));
    this->Implementation->FiltersMenuManager = fmm;
    fmm->initialize();
    
    }
}

//-----------------------------------------------------------------------------
/// Provides access to the menu manager used for the filters menu.
pqProxyMenuManager* pqMainWindowCore::filtersMenuManager() const
{
  return this->Implementation->FiltersMenuManager;
}

//-----------------------------------------------------------------------------
/// Provides access to the menu manager used for the sources menu.
pqProxyMenuManager* pqMainWindowCore::sourcesMenuManager() const
{
  return this->Implementation->SourcesMenuManager;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setToolbarMenu(pqViewMenu *menu)
{
  this->Implementation->ToolbarMenu = menu;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setDockWindowMenu(pqViewMenu *menu)
{
  this->Implementation->DockWindowMenu = menu;
}

//-----------------------------------------------------------------------------
pqPipelineMenu& pqMainWindowCore::pipelineMenu()
{
  if(!this->Implementation->PipelineMenu)
    {
    this->Implementation->PipelineMenu = new pqPipelineMenu(this);
    this->Implementation->PipelineMenu->setObjectName("PipelineMenu");
    }

  return *this->Implementation->PipelineMenu;
}

//-----------------------------------------------------------------------------
pqPipelineBrowser* pqMainWindowCore::pipelineBrowser()
{
  return this->Implementation->PipelineBrowser;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupPipelineBrowser(QDockWidget* dock_widget)
{
  this->Implementation->PipelineBrowser = new pqPipelineBrowser(dock_widget);
  this->Implementation->PipelineBrowser->setObjectName("pipelineBrowser");
    
  dock_widget->setWidget(this->Implementation->PipelineBrowser);

  QObject::connect(
    &pqActiveView::instance(), SIGNAL(changed(pqView*)),
    this->Implementation->PipelineBrowser, SLOT(setView(pqView*)));

  // Connect undo/redo.
  QObject::connect(
    this->Implementation->PipelineBrowser, SIGNAL(beginUndo(const QString&)),
    this->Implementation->UndoStack, SLOT(beginUndoSet(const QString&)));
  QObject::connect(
    this->Implementation->PipelineBrowser, SIGNAL(endUndo()),
    this->Implementation->UndoStack, SLOT(endUndoSet()));
}

//-----------------------------------------------------------------------------
pqProxyTabWidget* pqMainWindowCore::setupProxyTabWidget(QDockWidget* dock_widget)
{
  pqProxyTabWidget* const proxyPanel = 
    new pqProxyTabWidget(dock_widget);
  this->Implementation->ProxyPanel = proxyPanel;

  pqObjectInspectorWidget* object_inspector = proxyPanel->getObjectInspector();
    
  dock_widget->setWidget(proxyPanel);

  //QObject::connect(object_inspector, 
  //                 SIGNAL(preaccept()),
  //                 &this->Implementation->SelectionManager, 
  //                 SLOT(clearSelection()));
  QObject::connect(object_inspector, 
                   SIGNAL(accepted()),
                   this->Implementation->LookupTableManager, 
                   SLOT(updateLookupTableScalarRanges()));
  QObject::connect(object_inspector, SIGNAL(postaccept()),
                   this,             SLOT(onPostAccept()));
  QObject::connect(object_inspector, SIGNAL(accepted()), 
                   &this->Implementation->PendingDisplayManager,
                   SLOT(createPendingDisplays()));

  // Save crash recovery state on "Apply" before changes
  // are made, this grabs the last known good state.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  bool useCrashRecovery=settings->value("crashRecovery",false).toBool();
  if (useCrashRecovery)
    {
    QObject::connect(object_inspector, SIGNAL(preaccept()),
                     this, SLOT(onFileSaveRecoveryState()));
    }

  // Use the server manager selection model to determine which page
  // should be shown.
  pqObjectInspectorDriver *driver = this->getObjectInspectorDriver();
  QObject::connect(driver,     SIGNAL(outputPortChanged(pqOutputPort*)),
                   proxyPanel, SLOT(setOutputPort(pqOutputPort*)));
  QObject::connect(driver, SIGNAL(representationChanged(pqDataRepresentation*, pqView*)),
                   proxyPanel, SLOT(setRepresentation(pqDataRepresentation*)));
  QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
                   proxyPanel, SLOT(setView(pqView*)));

  return proxyPanel;
}

pqObjectInspectorWidget* pqMainWindowCore::setupObjectInspector(QDockWidget* dock_widget)
{
  pqObjectInspectorWidget* const object_inspector = 
    new pqObjectInspectorWidget(dock_widget);

  dock_widget->setWidget(object_inspector);

  //QObject::connect(object_inspector,
  //                 SIGNAL(preaccept()),
  //                 &this->Implementation->SelectionManager,
  //                 SLOT(clearSelection()));
  QObject::connect(object_inspector, SIGNAL(postaccept()),
                   this,             SLOT(onPostAccept()));
  QObject::connect(object_inspector, SIGNAL(accepted()), 
                   this,             SLOT(createPendingDisplays()));

  // Use the server manager selection model to determine which page
  // should be shown.
  pqObjectInspectorDriver *driver = this->getObjectInspectorDriver();
  QObject::connect(driver,           SIGNAL(sourceChanged(pqProxy *)),
                   object_inspector, SLOT(setProxy(pqProxy *)));
    QObject::connect(&pqActiveView::instance(), SIGNAL(changed(pqView*)),
                   object_inspector, SLOT(setView(pqView*)));

  return object_inspector;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupStatisticsView(QDockWidget* dock_widget)
{
  pqDataInformationWidget* const statistics_view =
    new pqDataInformationWidget(dock_widget)
    << pqSetName("statisticsView");
    
  dock_widget->setWidget(statistics_view);
}

//-----------------------------------------------------------------------------
pqAnimationViewWidget* pqMainWindowCore::setupAnimationView(QDockWidget* dock_widget)
{
  pqAnimationViewWidget* const animation_view =
    new pqAnimationViewWidget(dock_widget)
    << pqSetName("animationView");
  
  pqAnimationManager* mgr = this->getAnimationManager();
  QObject::connect(mgr, SIGNAL(activeSceneChanged(pqAnimationScene*)), 
                   animation_view, SLOT(setScene(pqAnimationScene*)));
  dock_widget->setWidget(animation_view);
  return animation_view;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupSelectionInspector(QDockWidget* dock_widget)
{
  pqSelectionInspectorPanel* const selection_inspector = 
    new pqSelectionInspectorPanel(dock_widget)
    << pqSetName("selectionInspectorPanel");

  QObject::connect(
    &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
    selection_inspector, SLOT(setServer(pqServer*)));

  selection_inspector->setSelectionManager(&this->Implementation->SelectionManager);

  //QObject::connect(this, SIGNAL(postAccept()),
  //  selection_inspector, SLOT(refresh()));

  //QObject::connect(core, SIGNAL(finishedAddingServer(pqServer*)),
  //  selection_inspector, SLOT(setServer(pqServer*)));

  dock_widget->setWidget(selection_inspector);
}

//-----------------------------------------------------------------------------
pqLookmarkManagerModel* pqMainWindowCore::getLookmarkManagerModel()
{
  return this->Implementation->LookmarkManagerModel;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupLookmarkBrowser(QDockWidget* dock_widget)
{
  this->Implementation->LookmarkBrowser = 
    new pqLookmarkBrowser(this->Implementation->Lookmarks, dock_widget);

  QObject::connect(this->Implementation->LookmarkBrowser,
                   SIGNAL(loadLookmark(const QString&)),
                   this,
                   SLOT(onLoadLookmark(const QString&)));

  dock_widget->setWidget(this->Implementation->LookmarkBrowser);
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::setupLookmarkInspector(QDockWidget* dock_widget)
{
  this->Implementation->LookmarkInspector = 
    new pqLookmarkInspector(this->Implementation->LookmarkManagerModel, 
                            dock_widget);
  this->Implementation->LookmarkInspector->setObjectName("lookmarkInspector");

  QObject::connect(this->Implementation->LookmarkInspector,
                   SIGNAL(removeLookmark(const QString&)),
                   this->Implementation->LookmarkManagerModel,
                   SLOT(removeLookmark(const QString&)));
  QObject::connect(this->Implementation->LookmarkInspector,
                   SIGNAL(loadLookmark(const QString&)),
                   this,SLOT(onLoadLookmark(const QString&)));
  QObject::connect(this->Implementation->LookmarkBrowser,
                   SIGNAL(selectedLookmarksChanged(const QStringList &)),
                   this->Implementation->LookmarkInspector,
                   SLOT(onLookmarkSelectionChanged(const QStringList &)));

  dock_widget->setWidget(this->Implementation->LookmarkInspector);
}

//-----------------------------------------------------------------------------
pqAnimationManager* pqMainWindowCore::getAnimationManager()
{
  if (!this->Implementation->AnimationManager)
    {
    this->Implementation->AnimationManager = new pqAnimationManager(
      this->Implementation->Parent);
    QObject::connect(
      &this->Implementation->ActiveServer, SIGNAL(changed(pqServer*)),
      this->Implementation->AnimationManager, 
      SLOT(onActiveServerChanged(pqServer*)));

    QObject::connect(this, SIGNAL(applicationSettingsChanged()),
                     this->Implementation->AnimationManager,
                     SLOT(updateApplicationSettings()));

    QObject::connect(this->Implementation->AnimationManager,
                     SIGNAL(activeSceneChanged(pqAnimationScene*)),
                     this, 
                     SLOT(onActiveSceneChanged(pqAnimationScene*)));
    QObject::connect(this->Implementation->AnimationManager, 
                     SIGNAL(activeSceneChanged(pqAnimationScene*)),
                     &this->VCRController(), 
                     SLOT(setAnimationScene(pqAnimationScene*)));

    this->Implementation->AnimationManager->setViewWidget(
      &this->multiViewManager());

    QObject::connect(this->Implementation->AnimationManager,
                     SIGNAL(beginNonUndoableChanges()),
                     this->Implementation->UndoStack, 
                     SLOT(beginNonUndoableChanges()));
    QObject::connect(this->Implementation->AnimationManager,
                     SIGNAL(endNonUndoableChanges()),
                     this->Implementation->UndoStack, 
                     SLOT(endNonUndoableChanges()));

    QObject::connect(this->Implementation->AnimationManager,
                     SIGNAL(disconnectServer()),
                     this, 
                     SLOT(onServerDisconnect()), 
                     Qt::QueuedConnection);
    }
  return this->Implementation->AnimationManager;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupVariableToolbar(QToolBar* toolbar)
{
  this->Implementation->VariableToolbar = toolbar;
  
  pqDisplayColorWidget* display_color = new pqDisplayColorWidget(toolbar)
    << pqSetName("displayColor");

  toolbar->addWidget(display_color);

  QObject::connect(this->getObjectInspectorDriver(),
                   SIGNAL(representationChanged(pqDataRepresentation*, pqView*)),
                   display_color, 
                   SLOT(setRepresentation(pqDataRepresentation*)));

  this->getColorScaleEditorManager()->setColorWidget(display_color);
}

pqColorScaleToolbar* pqMainWindowCore::getColorScaleEditorManager()
{
  if(!this->Implementation->ColorScale)
    {
    this->Implementation->ColorScale = 
      new pqColorScaleToolbar(this->Implementation->Parent);
    this->connect(this->getObjectInspectorDriver(),
        SIGNAL(representationChanged(pqDataRepresentation*, pqView*)),
        this->Implementation->ColorScale, 
        SLOT(setActiveRepresentation(pqDataRepresentation*)));
    }

  return this->Implementation->ColorScale;
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
                  SIGNAL(changed(pqView*)),
                  this->Implementation->ObjectInspectorDriver,
                  SLOT(setActiveView(pqView*)));
    }

  return this->Implementation->ObjectInspectorDriver;
}

//-----------------------------------------------------------------------------
pqActiveViewOptionsManager* pqMainWindowCore::getActiveViewOptionsManager()
{
  if(!this->Implementation->ActiveViewOptions)
    {
    this->Implementation->ActiveViewOptions =
        new pqActiveViewOptionsManager(this->Implementation->Parent);
    this->Implementation->ActiveViewOptions->setActiveView(
        pqActiveView::instance().current());
    this->connect(&pqActiveView::instance(), SIGNAL(changed(pqView *)),
        this->Implementation->ActiveViewOptions, SLOT(setActiveView(pqView *)));

    this->Implementation->ActiveViewOptions->setRenderViewOptions(
      new pqActiveRenderViewOptions(this->Implementation->ActiveViewOptions));

    pqActiveChartOptions *chartOptions = new pqActiveChartOptions(
      this->Implementation->ActiveViewOptions);
    this->Implementation->ActiveViewOptions->registerOptions(
      pqBarChartView::barChartViewType(), chartOptions);
    this->Implementation->ActiveViewOptions->registerOptions(
      pqLineChartView::lineChartViewType(), chartOptions);

    pqActiveTwoDRenderViewOptions* twoDOptions = new pqActiveTwoDRenderViewOptions(
      this->Implementation->ActiveViewOptions);
    this->Implementation->ActiveViewOptions->registerOptions(
      pqTwoDRenderView::twoDRenderViewType(), twoDOptions);
    }

  return this->Implementation->ActiveViewOptions;
}

//-----------------------------------------------------------------------------
pqViewContextMenuManager* pqMainWindowCore::getViewContextMenuManager()
{
  if(!this->Implementation->ViewContextMenu)
    {
    this->Implementation->ViewContextMenu = new pqViewContextMenuManager(this);
    pqServerManagerModel* smModel = 
      pqApplicationCore::instance()->getServerManagerModel();
    QObject::connect(smModel, SIGNAL(viewAdded(pqView*)),
      this->Implementation->ViewContextMenu, SLOT(setupContextMenu(pqView*)));
    QObject::connect(smModel, SIGNAL(viewRemoved(pqView*)),
      this->Implementation->ViewContextMenu, SLOT(cleanupContextMenu(pqView*)));

    // Bar chart
    pqBarChartViewContextMenuHandler *barChart =
      new pqBarChartViewContextMenuHandler(
      this->Implementation->ViewContextMenu);
    barChart->setOptionsManager(this->getActiveViewOptionsManager());
    this->connect(barChart, SIGNAL(screenshotRequested()),
      this, SLOT(onFileSaveScreenshot()));
    this->Implementation->ViewContextMenu->registerHandler(
      pqBarChartView::barChartViewType(), barChart);

    // Line chart
    pqLineChartViewContextMenuHandler *lineChart =
      new pqLineChartViewContextMenuHandler(
      this->Implementation->ViewContextMenu);
    lineChart->setOptionsManager(this->getActiveViewOptionsManager());
    this->connect(lineChart, SIGNAL(screenshotRequested()),
      this, SLOT(onFileSaveScreenshot()));
    this->Implementation->ViewContextMenu->registerHandler(
      pqLineChartView::lineChartViewType(), lineChart);

    // TODO: Stacked chart
    pqStackedChartViewContextMenuHandler *stackedChart =
      new pqStackedChartViewContextMenuHandler(
      this->Implementation->ViewContextMenu);
    stackedChart->setOptionsManager(this->getActiveViewOptionsManager());
    this->connect(stackedChart, SIGNAL(screenshotRequested()),
      this, SLOT(onFileSaveScreenshot()));
    //this->Implementation->ViewContextMenu->registerHandler(
    //  pqStackedChartView::stackedChartViewType(), stackedChart);

    // TODO: Statistical box chart
    pqBoxChartViewContextMenuHandler *boxChart =
      new pqBoxChartViewContextMenuHandler(
      this->Implementation->ViewContextMenu);
    boxChart->setOptionsManager(this->getActiveViewOptionsManager());
    this->connect(boxChart, SIGNAL(screenshotRequested()),
      this, SLOT(onFileSaveScreenshot()));
    //this->Implementation->ViewContextMenu->registerHandler(
    //  pqBoxChartView::boxChartViewType(), boxChart);
    }

  return this->Implementation->ViewContextMenu;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupRepresentationToolbar(QToolBar* toolbar)
{
  pqDisplayRepresentationWidget* display_representation = new pqDisplayRepresentationWidget(
    toolbar)
    << pqSetName("displayRepresentation");

  toolbar->addWidget(display_representation);

  QObject::connect(this->getObjectInspectorDriver(),
                   SIGNAL(representationChanged(pqDataRepresentation*, pqView*)),
                   display_representation, 
                   SLOT(setRepresentation(pqDataRepresentation*)));

  QObject::connect(this,                   SIGNAL(postAccept()),
                   display_representation, SLOT(reloadGUI()));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupCommonFiltersToolbar(QToolBar* toolbar)
{
  // use QActions from Filters -> Common
  if (this->Implementation->FiltersMenuManager)
    {
    QList<QAction*> actions = 
      this->Implementation->FiltersMenuManager->menu()->actions();
    foreach(QAction* action, actions)
      {
      QMenu* menu = action->menu();
      if(menu && action->text().remove('&') == "Common")
        {
        toolbar->addActions(menu->actions());
        break;
        }
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupLookmarkToolbar(QToolBar* toolbar)
{
  this->Implementation->LookmarkToolbar = toolbar;

  // add in existing lookmarks first
  for(int i=0; 
      i<this->Implementation->LookmarkManagerModel->getNumberOfLookmarks();
      i++)
    {
    pqLookmarkModel *lmk = 
      this->Implementation->LookmarkManagerModel->getLookmark(i);
    this->Implementation->LookmarkToolbar->addAction(
      QIcon(QPixmap::fromImage(lmk->getIcon())), lmk->getName()) 
      << pqSetName(lmk->getName()) << pqSetData(lmk->getName());
    }

  // connect up toolbar with lookmark manager events
  QObject::connect(toolbar, SIGNAL(actionTriggered(QAction*)), 
                   this,    SLOT(onLoadToolbarLookmark(QAction*)));
  QObject::connect(toolbar, 
                   SIGNAL(customContextMenuRequested(const QPoint &)),
                   this, 
                   SLOT(showLookmarkToolbarContextMenu(const QPoint &)));
  QObject::connect(this->Implementation->LookmarkManagerModel, 
                   SIGNAL(lookmarkAdded(const QString&, const QImage&)),
                   this, 
                   SLOT(onLookmarkAdded(const QString&, const QImage&)));
  QObject::connect(this->Implementation->LookmarkManagerModel, 
                   SIGNAL(lookmarkRemoved(const QString&)),
                   this, 
                   SLOT(onLookmarkRemoved(const QString&)));
  QObject::connect(this->Implementation->LookmarkManagerModel, 
                   SIGNAL(lookmarkNameChanged(const QString&, const QString&)),
                   this, 
                   SLOT(onLookmarkNameChanged(const QString&, const QString&)));
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::showLookmarkToolbarContextMenu(const QPoint &menuPos)
{
  QMenu menu;
  menu.setObjectName("ToolbarLookmarkMenu");

  // Create the actions that are not lookmark-specific
  QAction *actionDisplayBrowser = new QAction("Lookmark Browser",
    this->Implementation->LookmarkToolbar);
  QObject::connect(actionDisplayBrowser, 
                   SIGNAL(triggered()), 
                   this->Implementation->LookmarkBrowser->parentWidget(), 
                   SLOT(show()));
  menu.addAction(actionDisplayBrowser);
  QAction *actionNew = new QAction("New",
    this->Implementation->LookmarkToolbar);
  QObject::connect(actionNew, SIGNAL(triggered()), 
      this, SLOT(onToolsCreateLookmark()));
  menu.addAction(actionNew);

  // Create the lookmark-specific toolbar context menu actions if the mouse 
  // event was over a lookmark
  QAction *lmkAction = 
    this->Implementation->LookmarkToolbar->actionAt(menuPos);
  if(lmkAction)
    {
    this->Implementation->CurrentToolbarLookmark = lmkAction->data().toString();
    if(this->Implementation->CurrentToolbarLookmark.isNull() || 
      this->Implementation->CurrentToolbarLookmark.isEmpty())
      {
      return;
      }

    QAction *actionEdit = new QAction("Edit",
      this->Implementation->LookmarkToolbar);
    QObject::connect(actionEdit, SIGNAL(triggered()), 
        this, SLOT(onEditToolbarLookmark()));
    menu.addAction(actionEdit);
  
    //this->Implementation->LookmarkToolbarContextMenuActions.push_back(
    //actionEdit);
    QAction *actionRemove = new QAction("Delete",
      this->Implementation->LookmarkToolbar);
    //this->Implementation->LookmarkToolbarContextMenuActions.push_back(
    //actionRemove);
    QObject::connect(actionRemove, SIGNAL(triggered()), 
        this, SLOT(onRemoveToolbarLookmark()));
    menu.addAction(actionRemove);
    }

  menu.exec(this->Implementation->LookmarkToolbar->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onLookmarkAdded(const QString &name, const QImage &icon)
{
  this->Implementation->LookmarkToolbar->addAction(
    QIcon(QPixmap::fromImage(icon)), name) 
    << pqSetName(name) << pqSetData(name);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onRemoveToolbarLookmark()
{
  if(this->Implementation->CurrentToolbarLookmark.isNull() || 
    this->Implementation->CurrentToolbarLookmark.isEmpty())
    {
    return;
    }

  this->Implementation->LookmarkManagerModel->removeLookmark(
    this->Implementation->CurrentToolbarLookmark);
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::onLookmarkRemoved(const QString &name)
{
  // Remove the action associated with the lookmark.
  QAction *action = 
    this->Implementation->LookmarkToolbar->findChild<QAction *>(name);
  if(action)
    {
    this->Implementation->LookmarkToolbar->removeAction(action);
    delete action;
    }
}

void pqMainWindowCore::onLookmarkNameChanged(const QString &oldName, 
                                             const QString &newName)
{
  QAction *action = 
    this->Implementation->LookmarkToolbar->findChild<QAction *>(oldName);
  if(action)
    {
    action << pqSetName(newName);
    action << pqSetData(newName);
    action->setText(newName);
    action->setIconText(newName);
    action->setToolTip(newName);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditToolbarLookmark()
{
  if(this->Implementation->CurrentToolbarLookmark.isNull() || 
    this->Implementation->CurrentToolbarLookmark.isEmpty())
    {
    return;
    }

  this->Implementation->LookmarkBrowser->getSelectionModel()->clear();
  this->Implementation->LookmarkBrowser->getSelectionModel()->setCurrentIndex(
    this->Implementation->Lookmarks->getIndexFor(
      this->Implementation->CurrentToolbarLookmark),QItemSelectionModel::SelectCurrent);
  this->Implementation->LookmarkInspector->parentWidget()->show();
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::onLoadToolbarLookmark(QAction *action)
{
  if(!action)
    {
    return;
    }

  this->onLoadLookmark(action->data().toString());
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::onLoadLookmark(const QString &name)
{
  // If no sources are selected, the lookmark has multiple inputs,
  //    or there are more selected sources than the lookmark has inputs,
  //    prompt the user to specify which source(s) to use.
  // Otherwise apply the lookmark to the selected source(s)

  pqApplicationCore* core = pqApplicationCore::instance();
  const pqServerManagerSelection *selections =
    core->getSelectionModel()->selectedItems();

  // Construct a list of the sources 

  QList<pqPipelineSource*> sources;
  pqPipelineSource *src;
  for (int i=0; i<selections->size(); i++)
    {
    pqServerManagerModelItem *item = selections->at(i);
    if( (src = dynamic_cast<pqPipelineSource*>(item)) )
      {
      sources.push_back(src);
      }
    }

  this->Implementation->UndoStack->beginUndoSet(
    QString("Load Lookmark %1").arg(this->Implementation->CurrentToolbarLookmark));

  pqObjectBuilder* builder = core->getObjectBuilder();
  pqView *view = pqActiveView::instance().current();
  if (!view)
    {
    view = builder->createView(pqRenderView::renderViewType(), this->getActiveServer());
    }

  this->Implementation->LookmarkManagerModel->loadLookmark(this->getActiveServer(), 
    view, &sources, name);

  this->Implementation->UndoStack->endUndoSet();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupProgressBar(QStatusBar* toolbar)
{
  pqProgressWidget* const progress_bar = new pqProgressWidget(toolbar);
  toolbar->addPermanentWidget(progress_bar);

  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();

  QObject::connect(progress_manager, SIGNAL(enableProgress(bool)),
                   progress_bar,     SLOT(enableProgress(bool)));
    
  QObject::connect(progress_manager, SIGNAL(progress(const QString&, int)),
                   progress_bar,     SLOT(setProgress(const QString&, int)));

  QObject::connect(progress_manager, SIGNAL(enableAbort(bool)),
                   progress_bar,      SLOT(enableAbort(bool)));

  QObject::connect(progress_bar,     SIGNAL(abortPressed()),
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
  pqView* curView = pqActiveView::instance().current();

  if (!curView)
    {
    output << "ERROR: Could not locate the active view." << endl;
    return false;
    }

  // All tests need a 300x300 render window size.
  QSize cur_size = curView->getWidget()->size();
  curView->getWidget()->resize(300,300);
  vtkImageData* test_image = curView->captureImage(1);

  if (!test_image)
    {
    output << "ERROR: Failed to capture snapshot." << endl;
    return false;
    }

  // The returned image will have extents translated to match the view position,
  // we shift them back.
  int viewPos[2];
  curView->getViewProxy()->GetViewPosition(viewPos);
  // Update image extents based on ViewPosition
  int extents[6];
  test_image->GetExtent(extents);
  for (int cc=0; cc < 4; cc++)
    {
    extents[cc] -= viewPos[cc/2];
    }
  test_image->SetExtent(extents);

  bool ret = pqCoreTestUtility::CompareImage(test_image, referenceImage, 
    threshold, output, tempDirectory);
  test_image->Delete();
  curView->getWidget()->resize(cur_size);
  curView->render();
  return ret;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::initializeStates()
{
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

  emit this->enableCameraUndo(false);
  emit this->enableCameraRedo(false);
  emit this->cameraUndoLabel("");
  emit this->cameraRedoLabel("");
}

//-----------------------------------------------------------------------------
bool pqMainWindowCore::makeServerConnectionIfNoneExists()
{
  if (this->getActiveServer())
    {
    return true;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfItems<pqServer*>() != 0)
    {
    // cannot really happen, however, if no active server, yet
    // server connection exists, we don't try to make a new server connection.
    return false;
    }

  // It is possible that we are waiting for a reverse connection to connect
  // (this happends when playing back tests esp.). So wait until that reverse
  // connection stuff is done with
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  while (pm->IsAcceptingConnections())
    {
    pqEventDispatcher::processEventsAndWait(10);
    }

  if (core->getServerManagerModel()->getNumberOfItems<pqServer*>() != 0)
    {
    // the waiting resulted in a successful connection, return true.
    return true;
    }

  return this->makeServerConnection();
}

//-----------------------------------------------------------------------------
bool pqMainWindowCore::makeServerConnection()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerStartupBrowser server_browser (core->serverStartups(), 
    this->Implementation->Parent);
  QStringList ignoreList;
  ignoreList << "builtin";
  server_browser.setIgnoreList(ignoreList);
  server_browser.exec();
  return (this->getActiveServer() != NULL);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::restoreSettings()
{
  // Load the most recently used file extensions from QSettings, if available.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  
  if ( settings->contains("extensions/ScreenshotExtension") )
    {
    this->ScreenshotExtension =
       settings->value("extensions/ScreenshotExtension").toString();
    }
  else
    {
    this->ScreenshotExtension = QString();
    }
  
  if ( settings->contains("extensions/DataExtension") )
    {
    this->DataExtension = settings->value("extensions/DataExtension").toString();
    }
  else
    {
    this->DataExtension = QString();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::saveSettings()
{
  // Save the most recently used file extensions to QSettings.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("extensions/ScreenshotExtension", this->ScreenshotExtension);
  settings->setValue("extensions/DataExtension",       this->DataExtension);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::makeDefaultConnectionIfNoneExists()
{
  if (this->getActiveServer())
    {
    return ;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  if (core->getServerManagerModel()->getNumberOfItems<pqServer*>() != 0)
    {
    // cannot really happen, however, if no active server, yet
    // server connection exists, we don't try to make a new server connection.
    return ;
    }

  pqServerResource resource = pqServerResource("builtin:");
  core->getObjectBuilder()->createServer(resource);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen()
{
  this->makeServerConnectionIfNoneExists();
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

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileOpen(pqServer* server)
{
  QString filters = this->Implementation->ReaderFactory.getSupportedFileTypes(server);
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
  this->createReaderOnActiveServer(files);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileLoadServerState()
{
  this->makeServerConnectionIfNoneExists();
  pqApplicationCore* core = pqApplicationCore::instance();
  int num_servers = core->getServerManagerModel()->getNumberOfItems<pqServer*>();
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
    qDebug() << "No server connection present.";
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
                   this,       SLOT(onFileLoadServerState(const QStringList&)));
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
  // NOTE: Of the two operations , building the XML tree
  // and writing it to disk, building is the more expensive.
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
    pqApplicationCore::instance()->serverResources().save(
      *pqApplicationCore::instance()->settings());
    }

  root->Delete();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveRecoveryState()
{
  QStringList stateFileName;
  stateFileName << CrashRecoveryStateFile;
  this->onFileSaveServerState(stateFileName);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveData()
{
  pqOutputPort* port = qobject_cast<pqOutputPort*>(this->getActiveObject());
  if (!port)
    {
    pqPipelineSource* source = this->getActiveSource();
    if (source)
      {
      port = source->getOutputPort(0);
      }
    }

  if (!port)
    {
    qDebug() << "No active source, cannot save data.";
    return;
    }

  // Get the list of writers that can write the output from the given source.
  QString filters = 
    this->Implementation->WriterFactory.getSupportedFileTypes(port);

  pqFileDialog file_dialog(port->getServer(),
    this->Implementation->Parent, tr("Save File:"), QString(), filters);
  file_dialog.setRecentlyUsedExtension(this->DataExtension);
  file_dialog.setObjectName("FileSaveDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  QObject::connect(&file_dialog, SIGNAL(filesSelected(const QStringList&)), 
    this, SLOT(onFileSaveData(const QStringList&)));
  
  if ( file_dialog.exec() == QDialog::Accepted )
    {
    QString selectedFile = file_dialog.getSelectedFiles()[0];
    QFileInfo fileInfo  = QFileInfo( selectedFile );
    this->DataExtension = QString("*.") + fileInfo.suffix();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveData(const QStringList& files)
{
  pqOutputPort* port = qobject_cast<pqOutputPort*>(this->getActiveObject());
  if (!port)
    {
    pqPipelineSource* source = this->getActiveSource();
    if (source)
      {
      port = source->getOutputPort(0);
      }
    }

  if (!port)
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
  proxy.TakeReference(
    this->Implementation->WriterFactory.newWriter(files[0], port));

  vtkSMSourceProxy* writer = vtkSMSourceProxy::SafeDownCast(proxy);
  if (!writer)
    {
    qDebug() << "Failed to create writer for: " << files[0];
    return;
    }

  if (writer->IsA("vtkSMPSWriterProxy") && port->getServer()->getNumberOfPartitions() > 1)
    {
    pqOptions* options = pqOptions::SafeDownCast(
      vtkProcessModule::GetProcessModule()->GetOptions());
    // HACK: To avoid showing the dialog when running tests. We need a better
    // way to deciding that a test is running.
    if (options->GetTestFiles().size() == 0)
      {
      QMessageBox::StandardButton result = 
        QMessageBox::question(
          this->Implementation->Parent,
          "Serial Writer Warning",
          "This writer will collect all of the data to the first node before "
          "writing because it does not support parallel IO. This may cause the "
          "first node to run out of memory if the data is large. "
          "Are you sure you want to continue?",
          QMessageBox::Ok | QMessageBox::Cancel,
          QMessageBox::Cancel);
      if (result == QMessageBox::Cancel)
        {
        return;
        }
      }
    }

  // The "FileName" and "Input" properties of the writer are set here.
  // All others will be editable from the properties dialog.

  vtkSMStringVectorProperty *filenameProperty = 
    vtkSMStringVectorProperty::SafeDownCast(writer->GetProperty("FileName"));
  filenameProperty->SetElement(0, files[0].toAscii().data());

  pqSMAdaptor::setInputProperty(writer->GetProperty("Input"),
    port->getSource()->getProxy(),
    port->getPortNumber());

  pqWriterDialog dialog(writer);

  // Check to see if this writer has any properties that can be configured by 
  // the user. If it does, display the dialog.
  if(dialog.hasConfigurableProperties())
    {
    dialog.exec();
    if(dialog.result() == QDialog::Rejected)
      {
      // The user pressed Cancel so don't write
      return;
      }
    }

  writer->UpdateVTKObjects();

  writer->UpdatePipeline();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileSaveScreenshot()
{
  pqView* view = pqActiveView::instance().current();
  if(!view)
    {
    qDebug() << "Cannnot save image. No active view.";
    return;
    }

  pqSaveSnapshotDialog ssDialog(this->Implementation->Parent);
  ssDialog.setViewSize(view->getSize());
  ssDialog.setAllViewsSize(this->multiViewManager().clientSize());

  if (ssDialog.exec() != QDialog::Accepted)
    {
    return;
    }

  QString filters;
  filters += "PNG image (*.png)";
  filters += ";;BMP image (*.bmp)";
  filters += ";;TIFF image (*.tif)";
  filters += ";;PPM image (*.ppm)";
  filters += ";;JPG image (*.jpg)";
  filters += ";;PDF file (*.pdf)";
  pqFileDialog file_dialog(NULL,
    this->Implementation->Parent, tr("Save Screenshot:"), QString(), filters);
  file_dialog.setRecentlyUsedExtension(this->ScreenshotExtension);
  file_dialog.setObjectName("FileSaveScreenshotDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() != QDialog::Accepted)
    {
    return;
    }

  QSize chosenSize = ssDialog.viewSize();
  QString palette = ssDialog.palette();

  // temporarily load the color palette chosen by the user.
  vtkSmartPointer<vtkPVXMLElement> currentPalette;
  pqApplicationCore* core = pqApplicationCore::instance();
  if (!palette.isEmpty())
    {
    currentPalette.TakeReference(core->getCurrrentPalette());
    core->loadPalette(palette);
    }
  vtkSmartPointer<vtkImageData> img;
  QString file = file_dialog.getSelectedFiles()[0];
  QFileInfo fileInfo = QFileInfo( file );
  this->ScreenshotExtension = QString("*.") + fileInfo.suffix();

  int stereo = ssDialog.getStereoMode();
  QList<pqView*> views;
  if (stereo)
    {
    pqRenderViewBase::setStereo(stereo);
    }

  if (ssDialog.saveAllViews())
    {
    img.TakeReference(this->multiViewManager().captureImage( 
        chosenSize.width(), chosenSize.height()));
    }
  else
    {
    img.TakeReference(view->captureImage(chosenSize));
    }

  if (img.GetPointer() == NULL)
    {
    qCritical() << "Save Image failed.";
    }
  else
    {
    pqImageUtil::saveImage(img, file, ssDialog.quality());
    }

  if (stereo)
    {
    pqRenderViewBase::setStereo(0);
    core->render();
    }

  // restore palette.
  if (!palette.isEmpty())
    {
    core->loadPalette(currentPalette);
    }

}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onFileExport()
{
  QString filters = 
    this->Implementation->ViewExporterManager.getSupportedFileTypes();
  if (filters.isEmpty())
    {
    qDebug() << "Cannot export current view.";
    return;
    }

  pqFileDialog file_dialog(NULL,
    this->Implementation->Parent, tr("Save File:"), QString(), filters);
  file_dialog.setObjectName("FileExportDialog");
  file_dialog.setFileMode(pqFileDialog::AnyFile);
  if (file_dialog.exec() == QDialog::Accepted &&
    file_dialog.getSelectedFiles().size() > 0)
    {
    if (!this->Implementation->ViewExporterManager.write(
        file_dialog.getSelectedFiles()[0]))
      {
      qCritical() << "Failed to export correctly.";
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
  mgr->saveAnimation();
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
  pqView* view = pqActiveView::instance().current();
  if (!view)
    {
    qDebug() << "Cannot save animation geometry since no active view.";
    return;
    }

  QString filters = "ParaView Data files (*.pvd);;All files (*)";
  pqFileDialog* const file_dialog = new pqFileDialog(
    this->getActiveServer(),
    this->Implementation->Parent, 
    tr("Save Animation Geometry"), 
    QString(), 
    filters);
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setObjectName("FileSaveAnimationDialog");
  file_dialog->setFileMode(pqFileDialog::AnyFile);
  QObject::connect(file_dialog, SIGNAL(filesSelected(const QStringList&)), 
                   this,        SLOT(onSaveGeometry(const QStringList&)));
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
  pqView* view = pqActiveView::instance().current();
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
  pqRenderView* view = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if (!view)
    {
    qDebug() << "No active render module, cannot undo camera.";
    return;
    }
  view->undo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditCameraRedo()
{
  pqRenderView* view = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if (!view)
    {
    qDebug() << "No active render module, cannot redo camera.";
    return;
    }
  view->redo();
  view->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerConnect()
{
  pqServer* server = this->getActiveServer();

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();

  if (server && smmodel->findItems<pqPipelineSource*>(server).size() > 0)
    {
    int ret = QMessageBox::warning(this->Implementation->Parent, 
      tr("Disconnect from current server?"),
      tr("Before connecting to a new server, \n"
        "the current connection will be closed and \n"
        "the state will be discarded.\n\n"
        "Are you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
      {
      return;
      }
    }

  this->makeServerConnection();

  // If for some reason,  the connect failed,
  // we create a default builtin connection.
  this->makeDefaultConnectionIfNoneExists();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerDisconnect()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqServerManagerModel* smmodel = core->getServerManagerModel();
  pqServer* server = this->getActiveServer();

  if (server && smmodel->findItems<pqPipelineSource*>(server).size() > 0)
    {
    int ret = QMessageBox::warning(this->Implementation->Parent, 
      tr("Disconnect from current server?"),
      tr("The current connection will be closed and \n"
        "the state will be discarded.\n\n"
        "Are you sure you want to continue?"),
      QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::No)
      {
      return;
      }
    }

  if (server)
    {
    core->getObjectBuilder()->removeServer(server);
    }
  QList<QWidget*> removed;
  this->Implementation->MultiViewManager.reset(removed);
  foreach (QWidget* widget, removed)
    {
    delete widget;
    }

  pqEventDispatcher::processEventsAndWait(1);

  // Always have a builtin connection connected.
  this->makeDefaultConnectionIfNoneExists();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::ignoreTimesFromSelectedSources(bool ignore)
{
  this->Implementation->UndoStack->beginUndoSet(
    QString("Toggle Ignore Time"));
  const pqServerManagerSelection *selections =
    pqApplicationCore::instance()->getSelectionModel()->selectedItems();
  foreach (pqServerManagerModelItem* item, (*selections))
    {
    pqOutputPort* port = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = port? port->getSource():
      qobject_cast<pqPipelineSource*>(item);
    if (source)
      {
      if (ignore)
        {
        source->getServer()->getTimeKeeper()->removeSource(source);
        }
      else
        {
        source->getServer()->getTimeKeeper()->addSource(source);
        }
      }
    }
  this->Implementation->UndoStack->endUndoSet();
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
void pqMainWindowCore::onToolsCreateLookmark(QWidget* widget)
{
  pqMultiViewFrame* frame= qobject_cast<pqMultiViewFrame*>(widget);
  if(frame)
    {
    // Create a lookmark of the currently active view
    this->onToolsCreateLookmark(
        this->Implementation->MultiViewManager.getView(frame));
    }

}
//-----------------------------------------------------------------------------
void pqMainWindowCore::onToolsCreateLookmark(pqView *view)
{
  // right now we only support Lookmarks of render modules
  if(!view->supportsLookmarks())
    {
    qCritical() << "This view type does not support lookmarks.";
    return;
    }

  pqLookmarkDefinitionWizard wizard(this->Implementation->LookmarkManagerModel, 
                                    view, 
                                    this->Implementation->Parent);
  if(wizard.exec() == QDialog::Accepted)
    {
    wizard.createLookmark();
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
  filters += "XML Files (*.xml);;";
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
  if(!qobject_cast<pqRenderView*>(pqActiveView::instance().current()))
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
  pqRenderView* const render_module = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if(!render_module)
    {
    qCritical() << "Cannnot save image. No active render module.";
    return;
    }

  QVTKWidget* const widget = 
    qobject_cast<QVTKWidget*>(render_module->getWidget());
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
  filters += "XML Files (*.xml);;";
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
                   this,       SLOT(onToolsPlayTest(const QStringList&)));
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
#ifdef PARAVIEW_ENABLE_PYTHON
  pqPythonDialog* dialog = this->Implementation->PythonManager.pythonShellDialog();
  dialog->show();
  dialog->raise();
  dialog->activateWindow();
#else // PARAVIEW_ENABLE_PYTHON
  QMessageBox::information(NULL, "ParaView", "Python Shell not available");
#endif // PARAVIEW_ENABLE_PYTHON
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onHelpEnableTooltips(bool enabled)
{
  if (enabled)
    {
    delete this->Implementation->ToolTipTrapper;
    this->Implementation->ToolTipTrapper = 0;
    }
  else
    {
    this->Implementation->ToolTipTrapper = new pqToolTipTrapper();
    }

  // Save in settings.
  pqSettings* settings = pqApplicationCore::instance()->settings();
  settings->setValue("/EnableTooltips", enabled);
  emit this->enableTooltips(enabled);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditViewSettings()
{
  pqActiveViewOptionsManager *manager = this->getActiveViewOptionsManager();
  manager->showOptions();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onEditSettings()
{
  // Setup the applications dialog (if it hasn't been built already)
  this->setupApplicationSettingsDialog();

  // Show the dialog
  this->Implementation->ApplicationSettings->show();
  this->Implementation->ApplicationSettings->raise();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addApplicationSettings(pqOptionsContainer *options)
{
  // Setup the applications dialog (if it hasn't been built already)
  this->setupApplicationSettingsDialog();

  // Add the options
  this->Implementation->ApplicationSettings->addOptions(options);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::setupApplicationSettingsDialog()
{
  // Create the application settings dialog if it does not exist.
  if(!this->Implementation->ApplicationSettings)
    {
    this->Implementation->ApplicationSettings = 
      new pqApplicationOptionsDialog(this->Implementation->Parent);
    this->Implementation->ApplicationSettings->setObjectName("ApplicationSettings");
    this->Implementation->ApplicationSettings->setAttribute(Qt::WA_QuitOnClose, false);
    QObject::connect(this->Implementation->ApplicationSettings,
                     SIGNAL(appliedChanges()),
                     this, SIGNAL(applicationSettingsChanged()));
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCreateSource(const QString& name)
{
  this->makeServerConnectionIfNoneExists();
  
  if (this->getActiveServer())
    {
    if (!this->createSourceOnActiveServer(name))
      {
      qCritical() << "Source could not be created.";
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onCreateFilter(const QString& filterName)
{
  if (!this->createFilterForActiveSource(filterName))
    {
    qCritical() << "Filter could not be created.";
    } 
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onSelectionChanged()
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
  pqPipelineSource *source = opPort? opPort->getSource() : 
    qobject_cast<pqPipelineSource*>(item);
  pqServer *server = this->getActiveServer();

  pqApplicationCore *core = pqApplicationCore::instance();
  int numServers = core->getServerManagerModel()->getNumberOfItems<pqServer*>();
  pqView* view = pqActiveView::instance().current();
  pqRenderView* renderView = qobject_cast<pqRenderView*>(view);
  bool pendingDisplays = 
    this->Implementation->PendingDisplayManager.getNumberOfPendingDisplays() > 0;

  if (this->Implementation->PreviouslySelectedSource)
    {
    QObject::disconnect(this->Implementation->PreviouslySelectedSource, 
        SIGNAL(dataUpdated(pqPipelineSource*)),
        this->Implementation->FiltersMenuManager,
        SLOT(updateEnableState()));
    }
  this->Implementation->PreviouslySelectedSource = source;

  if (source)
    {
    QObject::connect(source, SIGNAL(dataUpdated(pqPipelineSource*)),
        this->Implementation->FiltersMenuManager, SLOT(updateEnableState()));
    }
  
  // Update the server connect/disconnect actions.
  // emit this->enableServerConnect(numServers == 0); -- it's always possible to
  //      create a new connection, it just implies that we'll disconnect before
  //      connecting to the new one.
  emit this->enableServerDisconnect(server != 0);

  // Update various actions that depend on pending displays.
  this->updatePendingActions(server, source, numServers, pendingDisplays);

  // Update the reset center action.
  emit this->enableResetCenter(source != 0 && renderView != 0);

  // Update the save screenshot action.
  emit this->enableFileSaveScreenshot(server != 0 && view != 0);

  // Update the animation manager if it exists.
  if(this->Implementation->AnimationManager)
    {
    // Update the animation manager. Setting the active server will
    // change the active scene.
    this->Implementation->AnimationManager->onActiveServerChanged(server);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onPendingDisplayChanged(bool pendingDisplays)
{
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = this->getActiveServer(); 

  pqApplicationCore *core = pqApplicationCore::instance();
  int numServers = core->getServerManagerModel()->getNumberOfItems<pqServer*>();
  this->updatePendingActions(server, source, numServers, pendingDisplays);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveViewChanged(pqView* view)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(view);

  // Get the active source and server.
  pqServerManagerModelItem *item = this->getActiveObject();
  pqPipelineSource *source = dynamic_cast<pqPipelineSource *>(item);
  pqServer *server = this->getActiveServer();

  // Update the reset center action.
  emit this->enableResetCenter(source != 0 && renderView != 0);

  // Update the show center axis action.
  emit this->enableShowCenterAxis(renderView != 0);

  // Update the save screenshot action.
  emit this->enableFileSaveScreenshot(server != 0 && view != 0);

  // Update the animation manager if it exists.
  if(this->Implementation->AnimationManager)
    {
    pqAnimationScene *scene =
        this->Implementation->AnimationManager->getActiveScene();
    emit this->enableFileSaveGeometry(scene != 0 && renderView != 0);
    }

  // Update the view undo/redo state.
  this->updateViewUndoRedo(renderView);
  if(renderView)
    {
    // Make sure the render module undo stack is connected.
    this->connect(renderView, SIGNAL(canUndoChanged(bool)),
        this, SLOT(onActiveViewUndoChanged()));
    }

  if(this->Implementation->CameraDialog)
    {
    this->showCameraDialog(view);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveViewUndoChanged()
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(
      pqActiveView::instance().current());
  if(renderView && renderView == this->sender())
    {
    this->updateViewUndoRedo(renderView);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onActiveSceneChanged(pqAnimationScene *scene)
{
  pqRenderView* renderView = qobject_cast<pqRenderView*>(
      pqActiveView::instance().current());
  emit this->enableFileSaveAnimation(scene != 0);
  emit this->enableFileSaveGeometry(scene != 0 && renderView != 0);
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
void pqMainWindowCore::updateViewUndoRedo(pqRenderView* renderView)
{
  bool can_undo_camera = false;
  bool can_redo_camera = false;
  QString undo_camera_label;
  QString redo_camera_label;

  if(renderView)
    {
    if (renderView->canUndo())
      {
      can_undo_camera = true;
      undo_camera_label = "Interaction";
      }
    if (renderView->canRedo())
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

  // Check if it is possible to access display on the server. If not, we show a
  // message.
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  vtkPVDisplayInformation* di = vtkPVDisplayInformation::New();
  pm->GatherInformation(server->GetConnectionID(),
    vtkProcessModule::RENDER_SERVER, di, pm->GetProcessModuleID());
  if (!di->GetCanOpenDisplay())
    {
    QMessageBox::warning(this->Implementation->Parent, 
      tr("Server DISPLAY not accessible"),
      tr("Display is not accessible on the server side.\n"
        "Remote rendering will be disabled."),
      QMessageBox::Ok);
    }
  di->Delete();
  pqSettings* settings = core->settings();
  QString curView = settings->value("/defaultViewType",
    pqRenderView::renderViewType()).toString();

  if (curView != "None" && !curView.isEmpty()) 
    {
    // When a server is created, we create a new render view for it.
    if(pqView* view = core->getObjectBuilder()->createView(curView, server))
      {
      view->render();
      }
    }

  // Show warning dialogs before server times out.
  QObject::connect(server, SIGNAL(fiveMinuteTimeoutWarning()), 
    this, SLOT(fiveMinuteTimeoutWarning()));
  QObject::connect(server, SIGNAL(finalTimeoutWarning()), 
    this, SLOT(finalTimeoutWarning()));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onServerCreationFinished(pqServer *server)
{
  pqApplicationCore *core = pqApplicationCore::instance();
  core->getSelectionModel()->setCurrentItem(server,
      pqServerManagerSelectionModel::ClearAndSelect);

  this->Implementation->UndoStack->clear();
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
      core->getServerManagerModel()->findItems<pqPipelineSource*>(server);
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
/// Called when a new reader is created by the GUI.
void pqMainWindowCore::onReaderCreated(pqPipelineSource* reader, 
  const QStringList& files)
{
  if (!reader || files.size() == 0)
    {
    return;
    }

  pqApplicationCore* core = pqApplicationCore::instance();
  pqServer* server = reader->getServer();

  // Add this to the list of recent server resources ...
  pqServerResource resource = server->getResource();
  resource.setPath(files[0]);
  resource.addData("readergroup", reader->getProxy()->GetXMLGroup());
  resource.addData("reader", reader->getProxy()->GetXMLName());
  resource.addData("extrafilesCount", QString("%1").arg(files.size()-1));
  for (int cc=1; cc < files.size(); cc++)
    {
    resource.addData(QString("file.%1").arg(cc-1), files[cc]);
    }
  core->serverResources().add(resource);
  core->serverResources().save(*core->settings());
}

//-----------------------------------------------------------------------------
// Called when any pqProxy or subclass is created,
// We update the undo stack to include an element
// which will manage the helper proxies correctly.
void pqMainWindowCore::onProxyCreation(pqProxy* proxy)
{
  if (proxy->getHelperProxies().size() > 0)
    {
    pqHelperProxyRegisterUndoElement* elem = 
      pqHelperProxyRegisterUndoElement::New();
    elem->RegisterHelperProxies(proxy);
    this->Implementation->UndoStack->addToActiveUndoSet(elem);
    elem->Delete();
    }
}


// Go upstream till we find an input that has timesteps and hide its time.
static void pqMainWindowCoreHideInputTimes(pqPipelineFilter* filter,
  bool hide)
{
  if (!filter)
    {
    return;
    }
  QList<pqOutputPort*> inputs = filter->getAllInputs();
  foreach (pqOutputPort* input, inputs)
    {
    pqPipelineSource* source = input->getSource();
    if (   source->getProxy()->GetProperty("TimestepValues")
        || source->getProxy()->GetProperty("TimeRange") )
      {
      if (hide)
        {
        source->getServer()->getTimeKeeper()->removeSource(source);
        }
      else
        {
        source->getServer()->getTimeKeeper()->addSource(source);
        }
      }
    else
      {
      pqMainWindowCoreHideInputTimes(
        qobject_cast<pqPipelineFilter*>(source), hide);
      }
    }
}

//-----------------------------------------------------------------------------
/// Called when a new source/filter/reader is created
/// by the GUI. Unlike  onSourceCreationFinished
/// this is not connected with Qt::QueuedConnection
/// hence is called immediately when a source is
/// created.
void pqMainWindowCore::onSourceCreation(pqPipelineSource *source)
{
  this->Implementation->PendingDisplayManager.addPendingDisplayForSource(
    source);
  
  // If the newly created source is a filter has TimestepValues or TimeRange
  // then we assume that this is a "temporal" filter which may distort the
  // time. So we hide the timesteps from all the inputs.
  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (filter && (   filter->getProxy()->GetProperty("TimestepValues")
                 || filter->getProxy()->GetProperty("TimeRange") ))
    {
    pqMainWindowCoreHideInputTimes(filter, true);
    }
}

//-----------------------------------------------------------------------------
/// Called when a new source/filter/reader is created
/// by the GUI. This slot is connected with 
/// Qt::QueuedConnection.
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
  pqApplicationCore *core = pqApplicationCore::instance();
  core->getSelectionModel()->setCurrentItem(source,
      pqServerManagerSelectionModel::ClearAndSelect);
}

//-----------------------------------------------------------------------------
// This method is called only when the gui intiates the removal of the source.
void pqMainWindowCore::onRemovingSource(pqPipelineSource *source)
{
  // FIXME: updating of selection must happen even is the source is removed
  // from python script or undo redo.
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

  QList<pqView*> views = source->getViews();

  pqPipelineFilter* filter = qobject_cast<pqPipelineFilter*>(source);
  if (filter)
    {
    // Make all inputs visible in views that the removed source
    // is currently visible in.
    QList<pqOutputPort*> inputs = filter->getInputs();
    foreach(pqView* view, views)
      {
      pqDataRepresentation* src_disp = source->getRepresentation(view);
      if (!src_disp || !src_disp->isVisible())
        {
        continue;
        }
      // For each input, if it is not visible in any of the views
      // that the delete filter is visible, we make the input visible.
      for(int cc=0; cc < inputs.size(); ++cc)
        {
        pqPipelineSource* input = inputs[cc]->getSource();
        pqDataRepresentation* input_disp = input->getRepresentation(view);
        if (input_disp && !input_disp->isVisible())
          {
          input_disp->setVisible(true);
          }
        }
      }

    if (   filter->getProxy()->GetProperty("TimestepValues")
        || filter->getProxy()->GetProperty("TimeRange") )
      {
      pqMainWindowCoreHideInputTimes(filter, false);
      }
    }

  foreach (pqView* view, views)
    {
    // this triggers an eventually render call.
    view->render();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onViewCreated(pqView* view)
{
  pqPipelineSource* source = 0;
  pqSpreadSheetView* spreadSheet = qobject_cast<pqSpreadSheetView*>(view);
  if (spreadSheet)
    {
    new pqSpreadSheetViewDecorator(spreadSheet);

    if ((source = this->getActiveSource()) != 0 &&
      !this->Implementation->PendingDisplayManager.isPendingDisplay(source))
      {
      // If a new spreadsheet view is created, we show the active source in it by
      // default.
      pqApplicationCore::instance()->getObjectBuilder()->createDataRepresentation(
        source->getOutputPort(0), view);
      // trigger an eventual-render.
      view->render();
      }
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::onPostAccept()
{
  emit this->postAccept();
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::getActiveSource()
{
  pqServerManagerModelItem* item = this->getActiveObject();
  if (item && qobject_cast<pqPipelineSource*>(item))
    {
    return static_cast<pqPipelineSource*>(item);
    }
  else if (item && qobject_cast<pqOutputPort*>(item))
    {
    pqOutputPort* port = static_cast<pqOutputPort*>(item);
    return port->getSource();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::getRootSources(QList<pqPipelineSource*> *sources, 
                                      pqPipelineSource *src)
{
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter*>(src);
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
pqServer* pqMainWindowCore::getActiveServer() const
{
  return this->Implementation->ActiveServer.current();
}

//-----------------------------------------------------------------------------
pqActiveServer* pqMainWindowCore::getActiveServerTracker() const
{
  return &this->Implementation->ActiveServer;
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
  pqApplicationCore::instance()->getObjectBuilder()->destroy(source);
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
  pqApplicationCore::instance()->getObjectBuilder()->removeServer(server);
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createSourceOnActiveServer(
  const QString& xmlname)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();  

  this->Implementation->UndoStack->beginUndoSet(
    QString("Create '%1'").arg(xmlname));
  pqPipelineSource* source =
    builder->createSource("sources", xmlname, this->getActiveServer());
  this->Implementation->UndoStack->endUndoSet();

  return source;
}


//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createFilterForActiveSource(
  const QString& xmlname)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqObjectBuilder* builder = core->getObjectBuilder();  

  vtkSMProxyManager* pxm = vtkSMProxyManager::GetProxyManager();
  vtkSMProxy* prototype = 
    pxm->GetPrototypeProxy("filters", xmlname.toAscii().data());
  if (!prototype)
    {
    qCritical() << "Unknown proxy type: " << xmlname;
    return 0;
    }

  // Get the list of selected sources.
  pqServerManagerSelection selected =
      *core->getSelectionModel()->selectedItems();


  QMap<QString, QList<pqOutputPort*> > namedInputs;
  QList<pqOutputPort*> selectedOutputPorts;

  // Determine the list of selected output ports.
  foreach (pqServerManagerModelItem* item, selected)
    {
    pqOutputPort* opPort = qobject_cast<pqOutputPort*>(item);
    pqPipelineSource* source = qobject_cast<pqPipelineSource*>(item);
    if (opPort)
      {
      selectedOutputPorts.push_back(opPort);
      }
    else if (source)
      {
      selectedOutputPorts.push_back(source->getOutputPort(0));
      }
    }

  QList<const char*> inputPortNames = pqPipelineFilter::getInputPorts(prototype);
  namedInputs[inputPortNames[0]] = selectedOutputPorts;

  // If the filter has more than 1 input ports, we are simply going to ask the 
  // user to make selection for the inputs for each port. We may change that in 
  // future to be smarter.
  int numInputPorts = inputPortNames.size();
  if (pqPipelineFilter::getRequiredInputPorts(prototype).size() > 1)
    {
    vtkSmartPointer<vtkSMProxy> filterProxy;
    filterProxy.TakeReference(pxm->NewProxy("filters", xmlname.toAscii().data()));
    filterProxy->SetConnectionID(this->getActiveServer()->GetConnectionID());

    // Create a dummy pqPipelineFilter which we can use to
    // pass on to the pqFilterInputDialog.
    pqPipelineFilter* filter = new pqPipelineFilter(xmlname,
      filterProxy, this->getActiveServer(), this);
    
    pqFilterInputDialog dialog(this->Implementation->Parent);
    dialog.setObjectName("SelectInputDialog");

    pqServerManagerModel *smModel =
        pqApplicationCore::instance()->getServerManagerModel();
    pqPipelineModel *model = new pqPipelineModel(*smModel);
    model->addSource(filter);
    foreach (pqOutputPort *outputPort, selectedOutputPorts)
      {
      model->addConnection(outputPort->getSource(), filter,
          outputPort->getPortNumber());
      }

    dialog.setModelAndFilter(model, filter, namedInputs);
    if (QDialog::Accepted != dialog.exec())
      {
      // User aborted creation.
      delete model;
      delete filter;
      return 0; 
      }

    for (int cc=0; cc < numInputPorts; cc++)
      {
      QString portName = filter->getInputPortName(cc);
      namedInputs[portName] = dialog.getFilterInputs(portName);
      }

    delete model;
    delete filter;
    }

  this->Implementation->UndoStack->beginUndoSet(
    QString("Create '%1'").arg(xmlname));
  pqPipelineSource* filter = builder->createFilter("filters", xmlname, 
    namedInputs, this->getActiveServer());
  this->Implementation->UndoStack->endUndoSet();

  return filter;
}

//-----------------------------------------------------------------------------
pqPipelineSource* pqMainWindowCore::createReaderOnActiveServer(
  const QStringList& files)
{
  if (files.empty())
    {
    return 0;
    }

  pqServer* server = this->getActiveServer();
  if (!server)
    {
    qCritical() << "Cannot create reader without an active server.";
    return 0;
    }

  pqReaderFactory *readerFactory = &this->Implementation->ReaderFactory;
  // For performance, only check if the first file is readable.
  for (int i=0; i < 1 /*files.size()*/; i++)
    {
    if (!readerFactory->checkIfFileIsReadable(files[i], server))
      {
      qWarning() << "File '" << files[i] << "' cannot be read.";
      return 0;
      }
    }

  // Determine reader type based on first file. For now, we are relying
  // on the user to avoid mixing file types.
  QString filename = files[0];
  QString readerType = readerFactory->getReaderType(filename, server);
  if (readerType.isEmpty())
    {
    // The reader factory could not determine the type of reader to create for the
    // file. Ask the user.
    pqSelectReaderDialog prompt(filename, server, 
      readerFactory, this->Implementation->Parent);
    if(prompt.exec() == QDialog::Accepted)
      {
      readerType = prompt.getReader();
      }
    else
      {
      // User didn't choose any reader.
      return NULL;
      }
    }

  this->Implementation->UndoStack->beginUndoSet(
    QString("Create 'Reader'")); /// FIXME
  pqPipelineSource* reader = readerFactory->createReader(
    files, readerType, server);
  this->Implementation->UndoStack->endUndoSet();

  return reader;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::disableAutomaticDisplays()
{
  QObject::disconnect(pqApplicationCore::instance(),
    SIGNAL(finishSourceCreation(pqPipelineSource*)),
    this, SLOT(onSourceCreation(pqPipelineSource*)));
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetCamera()
{
  pqView* view = pqActiveView::instance().current();
  if (view)
    {
    view->resetDisplay();
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z)
{
  pqRenderView* ren = qobject_cast<pqRenderView*>(pqActiveView::instance().current());
  if (ren)
    {
    ren->resetViewDirection(look_x, look_y, look_z,
      up_x, up_y, up_z);
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
void pqMainWindowCore::pickCenterOfRotation(bool begin)
{
 if (!qobject_cast<pqRenderView*>(pqActiveView::instance().current()))
    {
    return;
    }

  if (begin)
    {
    this->Implementation->RenderViewPickHelper.beginPick();  
    }
  else
    {
    this->Implementation->RenderViewPickHelper.endPick();
    }
}


//-----------------------------------------------------------------------------
void pqMainWindowCore::pickCenterOfRotationFinished(double x, double y, double z)
{
  this->Implementation->RenderViewPickHelper.endPick();  

  pqRenderView* rm = qobject_cast<pqRenderView*>(
    pqActiveView::instance().current());
  if (!rm)
    {
    qDebug() << "No active render module. Cannot reset center of rotation.";
    return;
    }

  double center[3];
  center[0] = x;
  center[1] = y;
  center[2] = z;

  rm->setCenterOfRotation(center);
  rm->render();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::resetCenterOfRotationToCenterOfCurrentData()
{
  pqRenderView* rm = qobject_cast<pqRenderView*>(
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

  pqPipelineRepresentation* repr = qobject_cast<pqPipelineRepresentation*>(
    source->getRepresentation(rm));
  if (!repr)
    {
    //qDebug() << "Active source not shown in active view. Cannot set center.";
    return;
    }

  double bounds[6];
  if (repr->getDataBounds(bounds))
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
  pqRenderView* rm = qobject_cast<pqRenderView*>(
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
void pqMainWindowCore::extensionLoaded()
{
  // plugins may contain new entries for menus
  if(this->Implementation->FiltersMenuManager)
    this->Implementation->FiltersMenuManager->update();

  if(this->Implementation->SourcesMenuManager)
    this->Implementation->SourcesMenuManager->update();
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addPluginInterface(QObject* iface)
{
  pqActionGroupInterface* actionGroup =
    qobject_cast<pqActionGroupInterface*>(iface);
  pqDockWindowInterface* dockWindow =
    qobject_cast<pqDockWindowInterface*>(iface);

  if(actionGroup)
    {
    this->addPluginActions(actionGroup);
    }
  else if(dockWindow)
    {
    this->addPluginDockWindow(dockWindow);
    }

  pqViewOptionsInterface* viewOptions =
    qobject_cast<pqViewOptionsInterface*>(iface);
  if(viewOptions)
    {
    foreach(QString viewtype, viewOptions->viewTypes())
      {

      // Try to create active view options
      pqActiveViewOptions* o =
        viewOptions->createActiveViewOptions(viewtype, 
          this->Implementation->ActiveViewOptions);
      if(o)
        {
        this->Implementation->ActiveViewOptions->registerOptions(
          viewtype, o);
        }

        // Try to create global view options
        pqOptionsContainer* globalOptions =
        viewOptions->createGlobalViewOptions(viewtype, 
          this->Implementation->ApplicationSettings);
        if(globalOptions)
          {
          this->addApplicationSettings(globalOptions);
          }

      }
    }
}

//-----------------------------------------------------------------------------
QMainWindow* pqMainWindowCore::findMainWindow()
{
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
  return mw;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addPluginActions(pqActionGroupInterface* iface)
{
  QString name = iface->groupName();
  QStringList splitName = name.split('/', QString::SkipEmptyParts);

  QMainWindow* mw = this->findMainWindow();
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

    // Add the toolbar to the view menu.
    if(this->Implementation->ToolbarMenu)
      {
      this->Implementation->ToolbarMenu->addWidget(tb, splitName[1]);
      }
    }
  else if(splitName.size() == 2 && splitName[0] == "MenuBar")
    {
    QMenu *menu = NULL;
    QList<QAction *> menuBarActions = mw->menuBar()->actions();
    foreach(QAction *existingMenuAction, menuBarActions)
      {
      QString menuName = existingMenuAction->text();
      menuName.remove('&');
      if (menuName == splitName[1])
        {
        menu = existingMenuAction->menu();
        break;
        }
      }
    if (menu)
      {
      // Add to existing menu.
      QAction *a;
      a = menu->addSeparator();
      this->Implementation->PluginToolBars.append(a);
      foreach(a, iface->actionGroup()->actions())
        {
        menu->addAction(a);
        this->Implementation->PluginToolBars.append(a);
        }
      }
    else
      {
      // Create new menu.
      menu = new QMenu(splitName[1], mw);
      menu->setObjectName(splitName[1]);
      menu->addActions(iface->actionGroup()->actions());
      mw->menuBar()->addMenu(menu);
      this->Implementation->PluginToolBars.append(menu);
      }
    }
  else if (splitName.size())
    {
    QString msg = 
      QString("Do not know what action group \"%1\" is").arg(splitName[0]);
    qWarning(msg.toAscii().data());
    }
  else 
    {
    qWarning("Action group doesn't have an identifier.");
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::addPluginDockWindow(pqDockWindowInterface* iface)
{
  QMainWindow* mw = this->findMainWindow();
  if(!mw)
    {
    qWarning("Could not find MainWindow for dock window");
    return;
    }

  // Get the dock area.
  QString area = iface->dockArea();
  Qt::DockWidgetArea dArea = Qt::LeftDockWidgetArea;
  if(area.compare("Right", Qt::CaseInsensitive) == 0)
    {
    dArea = Qt::RightDockWidgetArea;
    }
  else if(area.compare("Top", Qt::CaseInsensitive) == 0)
    {
    dArea = Qt::TopDockWidgetArea;
    }
  else if(area.compare("Bottom", Qt::CaseInsensitive) == 0)
    {
    dArea = Qt::BottomDockWidgetArea;
    }

  // Create the dock window.
  QDockWidget *dock = iface->dockWindow(mw);
  mw->addDockWidget(dArea, dock);

  // Add the dock window to the view menu.
  if(this->Implementation->DockWindowMenu)
    {
    this->Implementation->DockWindowMenu->addWidget(dock, dock->windowTitle());
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::removePluginToolBars()
{
  qDeleteAll(this->Implementation->PluginToolBars);
  this->Implementation->PluginToolBars.clear();
}

//-----------------------------------------------------------------------------
pqUndoStack* pqMainWindowCore::getApplicationUndoStack() const
{
  return this->Implementation->UndoStack;
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::applicationInitialize()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  pqOptions* options = pqOptions::SafeDownCast(
    vtkProcessModule::GetProcessModule()->GetOptions());

  // check for --server.
  const char* serverresource_name = options->GetServerResourceName();
  if (serverresource_name)
    {
    pqServerStartup* startUp = 
      core->serverStartups().getStartup(serverresource_name);
    if (startUp)
      {
      pqSimpleServerStartup starter;
      starter.startServerBlocking(*startUp);
      }
    }

  if (!this->getActiveServer())
    {
    if (serverresource_name)
      {
      qCritical() << "Could not connect to requested server \"" 
        << serverresource_name 
        << "\". Creating default builtin connection.";
      }
    this->makeDefaultConnectionIfNoneExists();
    }
  // Now we are assured that some default server connection has been made
  // (either the one requested by the user on the command line or simply the
  // default one).
    
  // check for --data option.
  if (options->GetParaViewDataName())
    {
    // We don't directly set the data file name instead use the dialog. This
    // makes it possible to select a file group.
    pqFileDialog* dialog = new pqFileDialog(
      this->getActiveServer(),
      this->Implementation->Parent, 
      tr("Internal Open File"), QString(),
      QString());
    dialog->setFileMode(pqFileDialog::ExistingFiles);
    dialog->selectFile(options->GetParaViewDataName());
    QStringList selectedFiles = dialog->getSelectedFiles();
    delete dialog;

    //QStringList files;
    //files.push_back(options->GetParaViewDataName());
    this->createReaderOnActiveServer(selectedFiles);
    }
  else if (options->GetStateFileName())
    {
    // check for --state option. (Bug #5711)
    // NOTE: --data and --state cannnot be specifed at the same time.
    QStringList files;
    files.push_back(options->GetStateFileName());
    this->onFileLoadServerState(files);
    }
  
  pqSettings* settings = pqApplicationCore::instance()->settings();
  if (settings->contains("/EnableTooltips"))
    {
    this->onHelpEnableTooltips(settings->value("/EnableTooltips").toBool());
    }
  else
    {
    this->onHelpEnableTooltips(true);
    }

  // Look for a crash recovery state file, nag user and
  // load if desired.
  bool recoveryEnabled=settings->value("crashRecovery",false).toBool();
  if (recoveryEnabled
      && QFile::exists(CrashRecoveryStateFile))
    {
    int recover
      = QMessageBox::question(
                0,
                "ParaView3",
                "A crash recovery state file has been found.\n"
                "Would you like to restore ParaView to its pre-crash state?",
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
    if (recover==QMessageBox::Yes)
      {
      QStringList fileName;
      fileName << CrashRecoveryStateFile;
      this->onFileLoadServerState(fileName);
      }
    QFile::remove(CrashRecoveryStateFile);
    }
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::showCameraDialog(pqView* view)
{
  if(!view)
    {
    if(this->Implementation->CameraDialog)
      {
      this->Implementation->CameraDialog->SetCameraGroupsEnabled(false);
      }
    return;
    }
  pqRenderView* renModule = qobject_cast<pqRenderView*>(view);

  if (!renModule)
    {
    if(this->Implementation->CameraDialog)
      {
      this->Implementation->CameraDialog->SetCameraGroupsEnabled(false);
      }
    return;
    }

  if(!this->Implementation->CameraDialog)
    {
    this->Implementation->CameraDialog = new pqCameraDialog(
      this->Implementation->Parent);
    this->Implementation->CameraDialog->setWindowTitle("Adjust Camera");
    this->Implementation->CameraDialog->setAttribute(Qt::WA_DeleteOnClose);
    this->Implementation->CameraDialog->setRenderModule(renModule);
    this->Implementation->CameraDialog->show();
    }
  else
    {
    this->Implementation->CameraDialog->SetCameraGroupsEnabled(true);
    this->Implementation->CameraDialog->setRenderModule(renModule);
    this->Implementation->CameraDialog->raise();
    this->Implementation->CameraDialog->activateWindow();
    }

}

//-----------------------------------------------------------------------------
void pqMainWindowCore::fiveMinuteTimeoutWarning()
{
  QMessageBox::warning(this->Implementation->Parent,
    tr("Server Timeout Warning"),
    tr("The server connection will timeout under 5 minutes.\n"
    "Please save your work."),
    QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::finalTimeoutWarning()
{
  QMessageBox::critical(this->Implementation->Parent,
    tr("Server Timeout Warning"),
    tr("The server connection will timeout shortly.\n"
    "Please save your work."),
    QMessageBox::Ok);
}

//-----------------------------------------------------------------------------
// update the state of the \c node if node is not an ancestor of any of the
// non-blockable widgets. If so, then it recurses over all its children.
static void selectiveEnabledInternal(QWidget* node, 
  QList<QPointer<QObject> >& nonblockable, bool enable)
{
  if (!node)
    {
    return;
    }
  if (nonblockable.size() == 0)
    {
    node->setEnabled(enable);
    return;
    }

  foreach (QObject* objElem, nonblockable)
    {
    QWidget* elem = qobject_cast<QWidget*>(objElem);
    if (elem)
      {
      if (node == elem)
        {
        // this is a non-blockable wiget. Don't change it's enable state.
        nonblockable.removeAll(elem);
        return;
        }

      if (node->isAncestorOf(elem))
        {
        // iterate over all children and selectively disable each.
        QList<QObject*> children = node->children();
        for (int cc=0; cc < children.size(); cc++)
          {
          QWidget* child = qobject_cast<QWidget*>(children[cc]);
          if (child)
            {
            ::selectiveEnabledInternal(child, nonblockable, enable);
            }
          }
        return;
        }
      }
    }

  // implies node is not an ancestor of any of the nonblockable widgets,
  // we can simply update its enable state.
  node->setEnabled(enable);
}
//-----------------------------------------------------------------------------
void pqMainWindowCore::setSelectiveEnabledState(bool enable)
{
  pqProgressManager* progress_manager = 
    pqApplicationCore::instance()->getProgressManager();
  QList<QPointer<QObject> > nonblockable = progress_manager->nonBlockableObjects();

  if (nonblockable.size() == 0)
    {
    this->Implementation->Parent->setEnabled(enable);
    return;
    }

  // Do selective disbling.
  selectiveEnabledInternal(this->Implementation->Parent, nonblockable, enable);
}

//-----------------------------------------------------------------------------
void pqMainWindowCore::quickLaunch()
{
  this->Implementation->QuickLaunchDialog.setActions(
    this->Implementation->FiltersMenuManager->findChildren<QAction*>());
  this->Implementation->QuickLaunchDialog.addActions(
    this->Implementation->SourcesMenuManager->findChildren<QAction*>());
  this->Implementation->QuickLaunchDialog.exec();
}
