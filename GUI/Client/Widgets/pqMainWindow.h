/*=========================================================================

   Program: ParaView
   Module:    pqMainWindow.h

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

#ifndef _pqMainWindow_h
#define _pqMainWindow_h

#include "pqWidgetsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QMainWindow>

class pqMultiViewFrame;
class pqPipelineBuilder;
class pqServerManagerModelItem;
class pqPipelineServer;
class pqPipelineSource;
class pqRenderModule;
class pqServerManagerObserver;
class pqServer;
class pqToolsMenu;
class pqViewMenu;

class vtkCommand;
class vtkObject;
class vtkSMDisplayProxy;
class vtkSMProxy;
class vtkSMRenderModuleProxy;
class vtkSMSourceProxy;
class vtkUnstructuredGrid;

class QIcon;
class QVTKWidget;

/** \brief Provides a standardized main window for ParaView applications -
application authors can derive from pqMainWindow and call its member functions
to use as-much or as-little of the standardized functionality as desired */

class PQWIDGETS_EXPORT pqMainWindow :
  public QMainWindow
{
  Q_OBJECT
  
public:
  pqMainWindow();
  virtual ~pqMainWindow();
  
  // High-level methods used to create a "standard" client
  
  /// Creates the "standard" ParaView file menu
  void createStandardFileMenu();
  /// Creates the "standard" ParaView view menu
  void createStandardEditMenu();
  /// Creates the "standard" ParaView view menu
  void createStandardViewMenu();
  /// Creates the "standard" ParaView server menu
  void createStandardServerMenu();
  /// Creates the "standard" ParaView sources menu
  void createStandardSourcesMenu();
  /// Creates the "standard" ParaView filters menu
  void createStandardFiltersMenu();
  /// Creates the "standard" ParaView pipeline menu
  void createStandardPipelineMenu();
  /// Creates the "standard" ParaView tools menu
  void createStandardToolsMenu();
  /// Creates the "standard" ParaView help menu
  void createStandardHelpMenu();

  /// Creates a "standard" ParaView pipeline browser
  void createStandardPipelineBrowser(bool visible = true);
  /// Creates a "standard" ParaView object inspector
  void createStandardObjectInspector(bool visible = true);
  /// Creates a "standard" ParaView element inspector
  void createStandardElementInspector(bool visible = true);
  /// Create a "standard" ParaView Data Information Widget.
  void createStandardDataInformationWidget(bool visible=true);

  /// Creates the "standard" VCR toolbar
  void createStandardVCRToolBar();
  /// Create the "undo/redo" toolbar.
  void createUndoRedoToolBar();
  /// Create the selection toolbar.
  void createSelectionToolBar();
  /// Creates the "standard" global variable toolbar
  void createStandardVariableToolBar();
  /// Creates the "standard" compound-proxy toolbar
  void createStandardCompoundProxyToolBar();
  /// Creates a status bar.
  void createStandardStatusBar();

  // Lower-level methods that can be used to create a "custom" client

  /// Returns the file menu, creating it if it doesn't already exist.
  QMenu* fileMenu();
  QMenu* recentFilesMenu();
  /// Returns the edit menu, creating it if it doesn't already exist.
  QMenu* editMenu();
  /// Returns the view menu, creating it if it doesn't already exist
  pqViewMenu* viewMenu();
  /// Returns the server menu, creating it if it doesn't already exist
  QMenu* serverMenu();
  /// Returns the sources menu, creating it if it doesn't already exist
  QMenu* sourcesMenu();
  /// Returns the filters menu, creating it if it doesn't already exist
  QMenu* filtersMenu();
  /// Returns the tools menu, creating it if it doesn't already exist
  pqToolsMenu* toolsMenu();
  /// Returns the help menu, creating it if it doesn't already exist
  QMenu* helpMenu();

  /// Adds a dock widget to the window, automatically adding it to the view 
  /// menu, and setting the initial visibility  
  void addStandardDockWidget(Qt::DockWidgetArea area, QDockWidget* dockwidget, 
    const QIcon& icon, bool visible = true);


  /// Compares the contents of the window with the given reference image,
  /// returns true iff they "match" within some tolerance
  bool compareView(const QString& ReferenceImage, double Threshold, 
    ostream& Output, const QString& TempDirectory);

  virtual bool eventFilter(QObject* watched, QEvent* e);

signals:
  /// Signal emitted whenever the current variable changes
  void variableChanged(pqVariableType, const QString&);

  /// Signal emitted when the active server/source changes. The
  /// handler can be a browser that shows the selected element.
  void select(pqServerManagerModelItem*);

public slots:
  void onFileNew();
  void onFileOpen();
  void onFileOpen(pqServer* Server);
  void onFileOpen(const QStringList& Files);
  void onFileOpenServerState();
  void onFileOpenServerState(pqServer* Server);
  void onFileOpenServerState(const QStringList& Files);
  void onFileSaveServerState();
  void onFileSaveServerState(const QStringList& Files);
  void onFileSaveScreenshot();
  void onFileSaveScreenshot(const QStringList& Files);
  void onFileSaveData();
  void onFileSaveData(const QStringList& files);
  void onFileSaveAnimation();
  void onFileSaveAnimation(const QStringList& files);

  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();

  void enableTooltips(bool enabled = true);

  // This method determine the enable state for various menus/toobars etc,
  // and updates it.
  void updateEnableState();

  // Add the file to the recently loaded menu
  void addFileToRecentList(const QString& fileName);

protected slots:
  // This will update the recently loaded files menu
  void updateRecentlyLoadedFilesMenu(bool enabled);

  // Called when the Undo/Redo stack changes.
  void onUndoRedoStackChanged(bool canUndo, QString,
    bool canRedo, QString);

  // Called to undo interaciton on active view.
  void UndoActiveViewInteraction();

  // Called to redo interaciton on active view.
  void RedoActiveViewInteraction();

  // Called when the interaction undo/redo stack changes.
  void updateInteractionUndoRedoState();

private slots:
  // Called when selection on the pqPipelineBrowser changes.
  // Use this slot to communicate the pipeline browser selection to the
  // ApplicationCore. Any work that needs to be done on selection
  // change should actually be done by monitoring selection events
  // from the ApplicationCore.
  void onBrowserSelectionChanged(pqServerManagerModelItem*);

  void onActiveSourceChanged(pqPipelineSource*);
  virtual void onActiveServerChanged(pqServer*);

  void onRecentFileOpen();
 
private slots:
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);

  void onNewSelections(vtkSMProxy* p, vtkUnstructuredGrid* selections);
  void onCompoundProxyAdded(QString proxy);
  void onCompoundProxyRemoved(QString proxy);
  void onCreateCompoundProxy(QAction*);
  
  void onRemoveServer(pqServer *server);
  void onAddWindow(QWidget *window);

  // performs updates that need to be done after accept.
  void postAcceptUpdate();
  // performs updates that need to be done before accept.
  void preAcceptUpdate();

  // Handler when the core indicates change in the active server/source.
  void onCoreActiveChanged();

  // Handler when the core indicates change in the active render module.
  void onActiveRenderModuleChanged(pqRenderModule* rm);

protected:
  void buildSourcesMenu();
  void buildFiltersMenu();

  // enable/disable filters as per the source.
  void updateFiltersMenu(pqPipelineSource* source);

private:
  void setServer(pqServer* Server);

  /// Stores private implementation details
  class pqImplementation;
  /// Stores private implementation details
  pqImplementation* const Implementation;
};

#endif // !_pqMainWindow_h
