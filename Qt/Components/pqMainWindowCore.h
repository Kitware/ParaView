/*=========================================================================

   Program: ParaView
   Module:    pqMainWindowCore.h

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

#ifndef _pqMainWindowCore_h
#define _pqMainWindowCore_h

#include "pqComponentsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QObject>
#include <QWidget>

class pqActionGroupInterface;
class pqActiveServer;
class pqActiveViewOptionsManager;
class pqAnimationManager;
class pqAnimationScene;
class pqAnimationViewWidget;
class pqColorScaleToolbar;
class pqDockWindowInterface;
class pqGenericViewModule;
class pqLookmarkManagerModel;
class pqMultiView;
class pqObjectInspectorDriver;
class pqObjectInspectorWidget;
class pqOptionsContainer;
class pqPipelineBrowser;
class pqPipelineMenu;
class pqPipelineSource;
class pqProxy;
class pqProxyMenuManager;
class pqProxyTabWidget;
class pqRenderView;
class pqRubberBandHelper;
class pqSelectionManager;
class pqServer;
class pqServerManagerModelItem;
class pqToolsMenu;
class pqUndoStack;
class pqVCRController;
class pqView;
class pqViewContextMenuManager;
class pqViewManager;
class pqViewMenu;
class vtkUnstructuredGrid;

class QAction;
class QDockWidget;
class QIcon;
class QImage;
class QMainWindow;
class QMenu;
class QPoint;
class QSize;
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

  //This constructor is meant to be paired with the setParent method
  //as an alternate to the original constructor. The purpose is to 
  //let applications use pqClientMainWindow, yet still subclass to 
  //augment pqMainWindowCore
  pqMainWindowCore(); 
  void setParent(QWidget* parent);


  /// Returns a multi-view widget which can be embedded in the UI  
  pqViewManager& multiViewManager();
  /// Returns the selection manager, which handles interactive selection
  pqSelectionManager& selectionManager();
  /// Returns the VCR controller, which can control animation playback
  pqVCRController& VCRController();

  /// Returns the selection helper used for 3D views.
  pqRubberBandHelper* renderViewSelectionHelper() const;
  
  /// Assigns a menu to be populated with sources
  void setSourceMenu(QMenu* menu);
  /// Assigns a menu to be populated with filters
  void setFilterMenu(QMenu* menu);

  /// Assigns a menu to be populated with plugin dock windows
  void setToolbarMenu(pqViewMenu *menu);
  /// Assigns a menu to be populated with plugin toolbars
  void setDockWindowMenu(pqViewMenu *menu);
  
  pqPipelineMenu& pipelineMenu();
  pqPipelineBrowser* pipelineBrowser();
  
  /// Setup a pipeline browser, attaching it to the given dock
  void setupPipelineBrowser(QDockWidget* parent);
  /// Setup a proxy tab widget, attaching it to the given dock
  virtual pqProxyTabWidget* setupProxyTabWidget(QDockWidget* parent);
  /// Setup an object inspector, attaching it to the given dock
  pqObjectInspectorWidget* setupObjectInspector(QDockWidget* parent);
  /// Setup a statistics view, attaching it to the given dock
  void setupStatisticsView(QDockWidget* parent);
  /// Setup a selection inspector, attaching it to the given dock
  void setupSelectionInspector(QDockWidget* parent);
  /// Setup lookmark browser, attaching it to the given dock
  void setupLookmarkBrowser(QDockWidget* parent);
  /// Setup lookmark inspector, attaching it to the given dock
  void setupLookmarkInspector(QDockWidget* parent);

  /// Setup the animation view, attaching it to the given dock.
  pqAnimationViewWidget* setupAnimationView(QDockWidget* parent);
  
  /// Setup a variable-selection toolbar
  void setupVariableToolbar(QToolBar* parent);
  /// Setup a lookmark toolbar
  void setupLookmarkToolbar(QToolBar* parent);
  /// Setup a representation-selection toolbar
  void setupRepresentationToolbar(QToolBar* parent);
  /// Setup a common filters toolbar
  void setupCommonFiltersToolbar(QToolBar* parent);
  
  /// Setup a progress bar, attaching it to the given status bar
  void setupProgressBar(QStatusBar* parent);
  
  /// Setup the application settings dialog
  void setupApplicationSettingsDialog();

  /// Add options to the application settings dialog
  void addApplicationSettings(pqOptionsContainer *);

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

  // creates a list of the sources at the head of the pipeline of the given source "src"
  void getRootSources(QList<pqPipelineSource*> *sources, pqPipelineSource *src);

  /// returns the active server.
  pqServer* getActiveServer() const;

  /// Returns the pqActiveServer instace that keeps track of the active server.
  pqActiveServer* getActiveServerTracker() const;

  void removeActiveSource();
  void removeActiveServer();
  
  // This will create a source with the given xmlname on the active server. 
  // On success returns
  // pqPipelineSource for the source proxy. The actual creation is delegated 
  // to pqObjectBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createSourceOnActiveServer(const QString& xmlname);

  // This will create a filter and connect it to the active source.
  // The actual creation is delegated 
  // to pqObjectBuilder instance. Using this method will optionally,
  // create a display for the source in the active render window (if both
  // the active window is indeed on the active server. The created source
  // becomes the active source.
  pqPipelineSource* createFilterForActiveSource( const QString& xmlname);

  // Utility function to create a reader that reads the file(s) on the 
  // active server. 
  pqPipelineSource* createReaderOnActiveServer(const QStringList& filename);
  
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

  // Returns the active view options manager. If the manager is not
  // created, a new one will be created and returned.
  pqActiveViewOptionsManager* getActiveViewOptionsManager();

  // Returns the view context menu manager. If the manager is not
  // created, a new one will be created and returned.
  pqViewContextMenuManager* getViewContextMenuManager();

  void removePluginToolBars();

  /// Returns the undo stack used for the application.
  pqUndoStack* getApplicationUndoStack() const;

  /// Returns the lookmark model.
  pqLookmarkManagerModel* getLookmarkManagerModel();

  /// Gets the color scale editor manager.
  pqColorScaleToolbar* getColorScaleEditorManager();

  /// Lookup the parent mainwindow if one exists.  Return null if not found.
  QMainWindow* findMainWindow();

  /// Asks the user to make a new server connection, if none exists.
  bool makeServerConnectionIfNoneExists();

  /// Asks the user for a new connection (even if a server connection
  /// already exists.
  bool makeServerConnection();

  /// Provides access to the menu manager used for the filters menu.
  pqProxyMenuManager* filtersMenuManager() const;

  /// Provides access to the menu manager used for the sources menu.
  pqProxyMenuManager* sourcesMenuManager() const;
  
  /// Save the settings of "save data" and "save screenshot" with QSettings.
  void saveSettings();

  /// Apply the settings from QSettings to "save data" and "save screenshot".
  void restoreSettings();

signals:
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
  void enableResetCenter(bool);
  void enablePickCenter(bool);
  void enableShowCenterAxis(bool);
  void pickingCenter(bool);
  void refreshFiltersMenu();
  void refreshSourcesMenu();
  void enableExport(bool);
  void enableTooltips(bool);
  void applicationSettingsChanged();
  
  /** \todo Hide these private implementation details */
  void postAccept();

public slots:
  /// Creates a builtin connection, if no connection
  /// currently exists.
  void makeDefaultConnectionIfNoneExists();

  virtual void onFileOpen();
  virtual void onFileOpen(pqServer* Server);
  virtual void onFileOpen(const QStringList& Files);

  void onFileLoadServerState();
  void onFileLoadServerState(pqServer* Server);
  void onFileLoadServerState(const QStringList& Files);

  void onFileSaveServerState();
  void onFileSaveServerState(const QStringList& Files);
  void onFileSaveRecoveryState();

  void onFileSaveData();
  void onFileSaveData(const QStringList& files);

  /// Called to export the current view.
  void onFileExport();

  void onFileSaveScreenshot();

  void onFileSaveAnimation();

  void onSaveGeometry();
  void onSaveGeometry(const QStringList& files);
  
  void onEditCameraUndo();
  void onEditCameraRedo();
  
  void onServerConnect();
  void onServerDisconnect();

  /// Ignore timesteps provided by selected sources.
  void ignoreTimesFromSelectedSources(bool ignore);
  void onToolsCreateCustomFilter();
  void onToolsManageCustomFilters();
  
  // TO DO: Support lookmark of multiple views and different view types (plots, etc).
  // Right now this creates one for a single render view only.
  void onToolsCreateLookmark();
  void onToolsCreateLookmark(QWidget* widget);
  void onToolsCreateLookmark(pqView*);

  // Have the main window handle all lookmark load signals (from the toolbar, inspector, browser)
  // Load a lookmark with the given name on the active server. 
  void onLoadLookmark(const QString &name);

  // Lookmark toolbar slots:

  // Add an action with the given name and icon to the lookmark toolbar
  void onLookmarkAdded(const QString &name, const QImage &image);
  // Remove the action with the given name from the lookmark toolbar 
  void onLookmarkRemoved(const QString &name);
  // Change the action's text from oldname to newname
  void onLookmarkNameChanged(const QString &oldname, const QString &newname);
  // TO DO: have a separate pqLookmarkToolbarContextMenu class handle the toolbar's context menu event
  void showLookmarkToolbarContextMenu(const QPoint &pos);
  void onRemoveToolbarLookmark();
  void onEditToolbarLookmark();
  void onLoadToolbarLookmark(QAction *action);
 
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

  void onHelpEnableTooltips(bool enabled = true);

  // Called to show the settings dialog.
  void onEditSettings();
  void onEditViewSettings();

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

  // This option is used for testing. Sets the maximum size for
  // all render windows. When size.isEmpty() is true,
  // it resets the maximum bounds on the render windows.
  void setMaxRenderWindowSize(const QSize& size);
  void enableTestingRenderWindowSize(bool enable);

  // Resets the center of rotation to the center of the active 
  // source in the active view.
  void resetCenterOfRotationToCenterOfCurrentData();

  // Next mouse press in 3D window sets the center of rotation to 
  // the corresponding world coordinates.
  void pickCenterOfRotation(bool begin);
  void pickCenterOfRotationFinished(double x, double y, double z);

  // Set center axes visibility on active render view.
  void setCenterAxesVisibility(bool visible);

  // Set the enable state for main window excepting some widgets marked as
  // non-blockable. Non-blockable widgets are registered with the
  // pqProgressManager.
  void setSelectiveEnabledState(bool);

  void quickLaunch();
protected slots:
  void onCreateSource(const QString& sourceName);
  void onCreateFilter(const QString& filtername);

  void onSelectionChanged();
  void onPendingDisplayChanged(bool pendingDisplays);

  /// Called when the active view in the pqActiveView singleton changes.
  virtual void onActiveViewChanged(pqView* view);

  void onActiveViewUndoChanged();

  /// Called when the active animation scene changes. We update the menu state
  /// for items such as "Save Animation"/"Save Geometry".
  void onActiveSceneChanged(pqAnimationScene *scene);

  /// Called when a new source/filter/reader is created
  /// by the GUI. This slot is connected with 
  /// Qt::QueuedConnection.
  void onSourceCreationFinished(pqPipelineSource *source);

  /// Called when a new source/filter/reader is created
  /// by the GUI. Unlike  onSourceCreationFinished
  /// this is not connected with Qt::QueuedConnection
  /// hence is called immediately when a source is
  /// created.
  void onSourceCreation(pqPipelineSource* source);

  /// Called when a new reader is created by the GUI.
  /// We add the reader to the recent files menu.
  void onReaderCreated(pqPipelineSource* reader, const QStringList& filenames);

  /// Called when any pqProxy or subclass is created,
  /// We update the undo stack to include an element
  /// which will manage the helper proxies correctly.
  void onProxyCreation(pqProxy*);

  void onServerCreationFinished(pqServer *server);
  void onRemovingServer(pqServer *server);
  virtual void onRemovingSource(pqPipelineSource *source);

  void onServerCreation(pqServer*);

  virtual void onPostAccept();

  void addPluginInterface(QObject* iface);
  void extensionLoaded();
  void addPluginActions(pqActionGroupInterface* iface);
  void addPluginDockWindow(pqDockWindowInterface* iface);

  /// This method is called once after the application event loop
  /// begins. This is where we process certain command line options
  /// such as --data, --server etc.
  virtual void applicationInitialize();

  /// Show the camera dialog for the active view module
  void showCameraDialog(pqView*);

  /// Shows message boxes for server timeout warnings.
  void fiveMinuteTimeoutWarning();
  void finalTimeoutWarning();

  /// Called when a new view is created by the GUI (not undo/redo or python).
  /// If a spreadsheet view has been created, we show the current source in it
  /// by default.
  void onViewCreated(pqView*);

private:
  pqServerManagerModelItem *getActiveObject() const;
  void updatePendingActions(pqServer *server, pqPipelineSource *source,
      int numServers, bool pendingDisplays);
  void updateViewUndoRedo(pqRenderView* renderView);
  class pqImplementation;
  pqImplementation* Implementation;
  void constructorHelper(QWidget *parent);

  QString  ScreenshotExtension;
  QString  DataExtension;
};

#endif // !_pqMainWindowCore_h
