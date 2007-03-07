/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowCore.h

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

#ifndef _pqMainWindowCore_h
#define _pqMainWindowCore_h

#include "pqComponentsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QObject>
class QSize;
class pqAnimationManager;
class pqAnimationScene;
class pqGenericViewModule;
class pqLookmarkToolbar;
class pqMultiView;
class pqObjectInspectorDriver;
class pqObjectInspectorWidget;
class pqPipelineBrowser;
class pqPipelineMenu;
class pqPipelineSource;
class pqPlotViewModule;
class pqProxy;
class pqProxyTabWidget;
class pqRenderViewModule;
class pqSelectionManager;
class pqServer;
class pqServerManagerModelItem;
class pqToolsMenu;
class pqVCRController;
class pqViewManager;
class pqViewMenu;
class pqActionGroupInterface;

class vtkUnstructuredGrid;

class QAction;
class QDockWidget;
class QIcon;
class QMenu;
class QStatusBar;
class QToolBar;
class QWidget;

/** \brief Provides a standardized main window for ParaView applications -
application authors can derive from pqMainWindowCore and call its member functions
to use as-much or as-little of the standardized functionality as desired */

class PQCOMPONENTS_EXPORT pqMainWindowCore :
  public QObject
{
  Q_OBJECT
  
public:
  pqMainWindowCore(QWidget* parent);
  ~pqMainWindowCore();

  /// Returns a multi-view widget which can be embedded in the UI  
  pqViewManager& multiViewManager();
  /// Returns the selection manager, which handles interactive selection
  pqSelectionManager& selectionManager();
  /// Returns the VCR controller, which can control animation playback
  pqVCRController& VCRController();
  
  /// Assigns a menu to be populated with sources
  void setSourceMenu(QMenu* menu);
  /// Assigns a menu to be populated with filters
  void setFilterMenu(QMenu* menu);
  
  pqPipelineMenu& pipelineMenu();
  pqPipelineBrowser* pipelineBrowser();
  
  /// Setup a pipeline browser, attaching it to the given dock
  void setupPipelineBrowser(QDockWidget* parent);
  /// Setup a proxy tab widget, attaching it to the given dock
  pqProxyTabWidget* setupProxyTabWidget(QDockWidget* parent);
  /// Setup an object inspector, attaching it to the given dock
  pqObjectInspectorWidget* setupObjectInspector(QDockWidget* parent);
  /// Setup a statistics view, attaching it to the given dock
  void setupStatisticsView(QDockWidget* parent);
  /// Setup an element inspector, attaching it to the given dock
  void setupElementInspector(QDockWidget* parent);
  /// Setup lookmark browser, attaching it to the given dock
  void setupLookmarkBrowser(QDockWidget* parent);
  /// Setup lookmark inspector, attaching it to the given dock
  void setupLookmarkInspector(QDockWidget* parent);

  /// Setup the animation panel, attaching it to the given dock.
  void setupAnimationPanel(QDockWidget* parent);
  
  /// Setup a variable-selection toolbar
  void setupVariableToolbar(QToolBar* parent);
  /// Setup a compound-proxy toolbar
  void setupCustomFilterToolbar(QToolBar* parent);
  /// Setup a lookmark toolbar
  void setupLookmarkToolbar(pqLookmarkToolbar* parent);
  /// Setup a representation-selection toolbar
  void setupRepresentationToolbar(QToolBar* parent);
  
  /// Setup a progress bar, attaching it to the given status bar
  void setupProgressBar(QStatusBar* parent);

  /** Compares the contents of the window with the given reference image,
  returns true iff they "match" within some tolerance */
  bool compareView(
    const QString& ReferenceImage,
    double Threshold,
    ostream& Output,
    const QString& TempDirectory);
  
  /// Call this once all of your slots/signals are connected, to
  /// set the initial state of GUI components
  void initializeStates();

  /// returns the active source.
  pqPipelineSource* getActiveSource();

  /// returns the active server.
  pqServer* getActiveServer();

  void removeActiveSource();
  void removeActiveServer();
  
  // This will create a source with the given xmlname on the active server. 
  // On success returns
  // pqPipelineSource for the source proxy. The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createSourceOnActiveServer(const QString& xmlname);

  // This will create a filter and connect it to the active source.
  // The actual creation is delegated 
  // to pqPipelineBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createFilterForActiveSource( const QString& xmlname);

  // This will instantiate and register a compound proxy. A compound proxy
  // definition with the given name must have already been registered with
  // the proxy manager. If the compound proxy needs an input, the active
  // source will be used as the input. 
  pqPipelineSource* createCompoundSource(const QString& name);

  // Utility function to create a reader that reads the file on the 
  // active server. 
  pqPipelineSource* createReaderOnActiveServer( const QString& filename);
  
  /** By default, whenever a new source/filter is added to the pipeline,
  pqMainWindowCore will attach a display.  Clients that wish to manage
  their own pipeline / displays should call this method once at startup. */
  void disableAutomaticDisplays();
 
  // Returns the animation manager. If none is already created,
  // this call will create a new manager.
  pqAnimationManager* getAnimationManager();

  // Returns the object inspector driver. If the driver is not
  // created, a new one will be created and returned.
  pqObjectInspectorDriver* getObjectInspectorDriver();

  void removePluginToolBars();

signals:
  void enableFileOpen(bool);
  void enableFileLoadServerState(bool);
  void enableFileSaveServerState(bool);
  void enableFileSaveData(bool);
  void enableFileSaveScreenshot(bool);
  void enableFileSaveAnimation(bool);
  void enableFileSaveGeometry(bool);
  void enableCameraUndo(bool);
  void enableCameraRedo(bool);
  void cameraUndoLabel(const QString&);
  void cameraRedoLabel(const QString&);
  void enableServerConnect(bool);
  void enableServerDisconnect(bool);
  void enableSourceCreate(bool);
  void enableFilterCreate(bool);
  void enableVariableToolbar(bool);
  void enableSelectionToolbar(bool);
  void enableResetCenter(bool);
  void enableShowCenterAxis(bool);
  
  /** \todo Hide these private implementation details */
  void postAccept();

public slots:
  virtual void onFileOpen();
  virtual void onFileOpen(pqServer* Server);
  virtual void onFileOpen(const QStringList& Files);

  void onFileLoadServerState();
  void onFileLoadServerState(pqServer* Server);
  void onFileLoadServerState(const QStringList& Files);

  void onFileSaveServerState();
  void onFileSaveServerState(const QStringList& Files);

  void onFileSaveData();
  void onFileSaveData(const QStringList& files);

  void onFileSaveScreenshot();
  void onFileSaveScreenshot(const QStringList& Files);

  void onFileSaveAnimation();
  void onFileSaveAnimation(const QStringList& files);

  void onSaveGeometry();
  void onSaveGeometry(const QStringList& files);
  
  void onEditCameraUndo();
  void onEditCameraRedo();
  
  void onServerConnect();
  void onServerDisconnect();

  void onToolsCreateCustomFilter();
  void onToolsManageCustomFilters();
  
  // TO DO: Support lookmark of multiple views and different view types (plots, etc).
  // Right now this creates one for a single render view only.
  void onToolsCreateLookmark();
  void onToolsCreateLookmark(pqGenericViewModule*);

  // Have the main window handle all lookmark load signals (from the toolbar, inspector, browser)
  // The reason is because the lookmark needs to be passed a server to load the state on and
  // this class keeps track of the active server. It also contains code to prompt the user if there is none. 
  void onLoadLookmark(const QString &name);
  void onLoadCurrentLookmark(pqServer *server);

  // Display the Lookmark Inspector and set the current lookmark to the one with name "name"
  void onEditLookmark(const QString &name);

  void onToolsManageLinks();

  void onToolsDumpWidgetNames();
  
  void onToolsRecordTest();
  void onToolsRecordTest(const QStringList &fileNames);
  
  void onToolsRecordTestScreenshot();
  void onToolsRecordTestScreenshot(const QStringList &fileNames);
  
  void onToolsPlayTest();
  void onToolsPlayTest(const QStringList &fileNames);

  void onToolsTimerLog();
  void onToolsOutputWindow();
  
  void onToolsPythonShell();

  // pop up dialogs to let the user enter in selections manually
  void onEnterSelectionPoints();
  void onEnterSelectionIds();
  void onEnterSelectionThresholds();

  // extract whatever was manually selected
  void onPointsEntered(double X, double Y, double Z);
  void onIdsEntered(int id);
  void onThresholdsEntered(double min, double max);

  void onHelpEnableTooltips(bool enabled = true);

  void createPendingDisplays();

  // Called to show the settings dialog.
  void onEditSettings();

  // invoke the dialog to manage plugins
  void onManagePlugins();

  // Camera slots.
  void resetCamera();
  void resetViewDirectionPosX();
  void resetViewDirectionNegX();
  void resetViewDirectionPosY();
  void resetViewDirectionNegY();
  void resetViewDirectionPosZ();
  void resetViewDirectionNegZ();

  void resetViewDirection(
    double look_x, double look_y, double look_z,
    double up_x, double up_y, double up_z);

  // Create New Plot Views.
  void createBarCharView();
  void createXYPlotView();
  
  /// Create a table view
  void createTableView();

  // This option is used for testing. Sets the maximum size for
  // all render windows. When size.isEmpty() is true,
  // it resets the maximum bounds on the render windows.
  void setMaxRenderWindowSize(const QSize& size);
  void enableTestingRenderWindowSize(bool enable);

  // Resets the center of rotation to the center of the active 
  // source in the active view.
  void resetCenterOfRotationToCenterOfCurrentData();

  // Set center axes visibility on active render view.
  void setCenterAxesVisibility(bool visible);
private slots:
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);

  void onCreateCompoundProxy(QAction*);
  void onCompoundProxyAdded(QString proxy);
  void onCompoundProxyRemoved(QString proxy);

  void onSelectionChanged();
  void onPendingDisplayChanged(bool pendingDisplays);
  void onActiveViewChanged(pqGenericViewModule *view);
  void onActiveViewUndoChanged();
  void onActiveSceneChanged(pqAnimationScene *scene);

  void onServerCreationFinished(pqServer *server);
  void onRemovingServer(pqServer *server);
  void onSourceCreationFinished(pqPipelineSource *source);
  void onRemovingSource(pqPipelineSource *source);

  void onServerCreation(pqServer*);
  void onSourceCreation(pqPipelineSource *source);

  void onPostAccept();

  void updateFiltersMenu();
  void refreshFiltersMenu();
  
  void updateRecentFilterMenu(QAction* action);

  void addPluginActions(QObject* iface);
  void addPluginActions(pqActionGroupInterface* iface);

private:
  pqServerManagerModelItem *getActiveObject() const;
  void updatePendingActions(pqServer *server, pqPipelineSource *source,
      int numServers, bool pendingDisplays);
  void updateViewUndoRedo(pqRenderViewModule *renderModule);
  void saveRecentFilterMenu();
  void restoreRecentFilterMenu();
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqMainWindowCore_h
