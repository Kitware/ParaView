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

#include "pqWidgetsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QObject>

class pqMultiView;
class pqObjectInspectorWidget;
class pqPipelineMenu;
class pqPipelineSource;
class pqRenderModule;
class pqRenderWindowManager;
class pqSelectionManager;
class pqServer;
class pqServerManagerModelItem;
class pqToolsMenu;
class pqVCRController;
class pqViewMenu;

class vtkSMProxy;
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

class PQWIDGETS_EXPORT pqMainWindowCore :
  public QObject
{
  Q_OBJECT
  
public:
  pqMainWindowCore(QWidget* parent);
  ~pqMainWindowCore();

  /// Returns a multi-view widget which can be embedded in the UI  
  pqMultiView& multiViewManager();
  /// Returns a render-window manager
  pqRenderWindowManager& renderWindowManager();
  /// Returns the selection manager, which handles interactive selection
  pqSelectionManager& selectionManager();
  /// Returns the VCR controller, which can control animation playback
  pqVCRController& VCRController();
  
  /// Assigns a menu to be populated with recent files
  void setRecentFilesMenu(QMenu* menu);
  /// Assigns a menu to be populated with sources
  void setSourceMenu(QMenu* menu);
  /// Assigns a menu to be populated with filters
  void setFilterMenu(QMenu* menu);
  
  pqPipelineMenu& pipelineMenu();
  
  /// Setup a pipeline browser, attaching it to the given dock
  void setupPipelineBrowser(QDockWidget* parent);
  /// Setup an object inspector, attaching it to the given dock
  pqObjectInspectorWidget* setupObjectInspector(QDockWidget* parent);
  /// Setup a statistics view, attaching it to the given dock
  void setupStatisticsView(QDockWidget* parent);
  /// Setup an element inspector, attaching it to the given dock
  void setupElementInspector(QDockWidget* parent);
  
  /// Setup a variable-selection toolbar
  void setupVariableToolbar(QToolBar* parent);
  /// Setup a compound-proxy toolbar
  void setupCustomFilterToolbar(QToolBar* parent);
  
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
  
signals:
  void enableFileOpen(bool);
  void enableFileLoadServerState(bool);
  void enableFileSaveServerState(bool);
  void enableFileSaveData(bool);
  void enableFileSaveScreenshot(bool);
  void enableFileSaveAnimation(bool);
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
  
  void activeSourceChanged(vtkSMProxy*);
  
  /** \todo Hide these private implementation details */
  void postAccept();
  void select(pqServerManagerModelItem*);

public slots:
  void onFileNew();

  void onFileOpen();
  void onFileOpen(pqServer* Server);
  void onFileOpen(const QStringList& Files);

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
  
  void onEditCameraUndo();
  void onEditCameraRedo();
  
  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();

  void onToolsCreateCustomFilter();
  void onToolsManageCustomFilters();

  void onToolsDumpWidgetNames();
  
  void onToolsRecordTest();
  void onToolsRecordTest(const QStringList &fileNames);
  
  void onToolsRecordTestScreenshot();
  void onToolsRecordTestScreenshot(const QStringList &fileNames);
  
  void onToolsPlayTest();
  void onToolsPlayTest(const QStringList &fileNames);
  
  void onToolsPythonShell();
  
  void onHelpEnableTooltips(bool enabled = true);

private slots:
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);

  void onCreateCompoundProxy(QAction*);
  void onCompoundProxyAdded(QString proxy);
  void onCompoundProxyRemoved(QString proxy);
  void onActiveSourceChanged(pqPipelineSource*);
  void onActiveServerChanged(pqServer*);
  void onActiveRenderModuleChanged(pqRenderModule* rm);
  void onCoreActiveChanged();

  void onInitializeStates();
  void onInitializeInteractionStates();

  void onPostAccept();

  // Called when selection on the pqPipelineBrowser changes.
  // Use this slot to communicate the pipeline browser selection to the
  // ApplicationCore. Any work that needs to be done on selection
  // change should actually be done by monitoring selection events
  // from the ApplicationCore.
  void onBrowserSelectionChanged(pqServerManagerModelItem*);

  // enable/disable filters as per the source.
  void updateFiltersMenu(pqPipelineSource* source);

  // Add the file to the recently loaded menu
  void addRecentFile(const QString& fileName);
  // This will update the recently loaded files menu
  void updateRecentFilesMenu(bool enabled);
  void onRecentFileOpen();

/*
  virtual bool eventFilter(QObject* watched, QEvent* e);
*/

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqMainWindowCore_h
