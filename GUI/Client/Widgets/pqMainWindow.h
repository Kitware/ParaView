/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

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

#ifndef _pqMainWindow_h
#define _pqMainWindow_h

#include "pqWidgetsExport.h"
#include "pqVariableType.h"

#include <vtkIOStream.h>

#include <QMainWindow>

class pqMultiViewFrame;
class pqPipelineServer;
class pqPipelineWindow;
class pqServer;

class vtkSMProxy;
class vtkUnstructuredGrid;

class QIcon;
class QVTKWidget;

/** \brief Provides a standardized main window for ParaQ applications -
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
  
  /// Creates the "standard" ParaQ file menu
  void createStandardFileMenu();
  /// Creates the "standard" ParaQ view menu
  void createStandardViewMenu();
  /// Creates the "standard" ParaQ server menu
  void createStandardServerMenu();
  /// Creates the "standard" ParaQ sources menu
  void createStandardSourcesMenu();
  /// Creates the "standard" ParaQ filters menu
  void createStandardFiltersMenu();
  /// Creates the "standard" ParaQ tools menu
  void createStandardToolsMenu();

  /// Creates a "standard" ParaQ pipeline browser
  void createStandardPipelineBrowser(bool visible = true);
  /// Creates a "standard" ParaQ object inspector
  void createStandardObjectInspector(bool visible = true);
  /// Creates a "standard" ParaQ element inspector
  void createStandardElementInspector(bool visible = true);

  /// Creates the "standard" VCR toolbar
  void createStandardVCRToolBar();
  /// Creates the "standard" global variable toolbar
  void createStandardVariableToolBar();
  /// Creates the "standard" compound-proxy toolbar
  void createStandardCompoundProxyToolBar();

  // Lower-level methods that can be used to create a "custom" client

  /// Returns the file menu, creating it if it doesn't already exist
  QMenu* fileMenu();
  /// Returns the view menu, creating it if it doesn't already exist
  QMenu* viewMenu();
  /// Returns the server menu, creating it if it doesn't already exist
  QMenu* serverMenu();
  /// Returns the sources menu, creating it if it doesn't already exist
  QMenu* sourcesMenu();
  /// Returns the filters menu, creating it if it doesn't already exist
  QMenu* filtersMenu();
  /// Returns the tools menu, creating it if it doesn't already exist
  QMenu* toolsMenu();
  /// Returns the help menu, creating it if it doesn't already exist
  QMenu* helpMenu();

  /// Adds a dock widget to the window, automatically adding it to the view menu, and setting the initial visibility  
  void addStandardDockWidget(Qt::DockWidgetArea area, QDockWidget* dockwidget, const QIcon& icon, bool visible = true);

  /// Compares the contents of the window with the given reference image, returns true iff they "match" within some tolerance
  bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);
  
  virtual bool eventFilter(QObject* watched, QEvent* e);

signals:
  /// Signal emitted whenever the server changes
  /**\todo Revisit this API once we truly support simultaneous connections */
  void serverChanged(pqServer*);
  /// Signal emitted whenever a different proxy is selected in the pipeline browser
  void proxyChanged(vtkSMProxy*);
  /// Signal emitted whenever the current variable changes
  void variableChanged(pqVariableType, const QString&);

public slots:
  void onFileNew();
  void onFileNew(pqServer* Server);
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

  void onServerConnect();
  void onServerConnect(pqServer* Server);
  void onServerDisconnect();

  void onRecordTest();
  void onRecordTest(const QStringList& Files);
  void onPlayTest();
  void onPlayTest(const QStringList& Files);
  
  void onPythonShell();

  void onFirstTimeStep();
  void onPreviousTimeStep();
  void onNextTimeStep();
  void onLastTimeStep();

private slots:
  void onNewQVTKWidget(pqMultiViewFrame* parent);
  void onDeleteQVTKWidget(pqMultiViewFrame* parent);
  void onFrameActive(QWidget*);

  void onUpdateSourcesFiltersMenu(pqServer*);
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);
  void onOpenLinkEditor();
  void onOpenCompoundFilterWizard();

  void onNewSelections(vtkSMProxy* p, vtkUnstructuredGrid* selections);
  void onCompoundProxyAdded(const QString&, const QString&);
  void onCreateCompoundProxy(QAction*);
  
  void onProxySelected(vtkSMProxy*);
  void onVariableChanged(pqVariableType, const QString&);
  
  void onAddServer(pqPipelineServer *server);
  void onRemoveServer(pqPipelineServer *server);
  void onAddWindow(pqPipelineWindow *window);

private:
  void setServer(pqServer* Server);
  void cleanUpWindow(QVTKWidget *window);

  /// Stores private implementation details
  class pqImplementation;
  /// Stores private implementation details
  pqImplementation* const Implementation;
};

#endif // !_pqMainWindow_h
