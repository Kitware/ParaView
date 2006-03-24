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

#include <pqVariableType.h>
#include <QMainWindow>
#include <vtkIOStream.h>

class pqObjectInspectorWidget;
class pqPipelineData;
class pqPipelineListWidget;
class pqPipelineServer;
class pqPipelineWindow;
class pqServer;
class pqSMAdaptor;
class pqSourceProxyInfo;
class pqMultiViewManager;
class pqMultiViewFrame;

class QVTKWidget;
class vtkSMRenderModuleProxy;
class vtkEventQtSlotConnect;
class vtkUnstructuredGrid;
class vtkSMProxy;

class QAction;
class QDockWidget;
class QToolBar;
class QTreeView;
class QTabWidget;

/// Provides the main window for the ParaQ application
class pqMainWindow :
        public QMainWindow
{
  Q_OBJECT

public:
  pqMainWindow();
  virtual ~pqMainWindow();

  virtual bool eventFilter(QObject* watched, QEvent* e);

  /// Compares the contents of the window with the given reference image, returns true iff they "match" within some tolerance
  bool compareView(const QString& ReferenceImage, double Threshold, ostream& Output, const QString& TempDirectory);

signals:
  void serverChanged(pqServer*);
  
private:
  void setServer(pqServer* Server);

  pqServer* CurrentServer;
  QToolBar* PropertyToolbar;
  pqMultiViewManager* MultiViewManager;
  QAction* ServerDisconnectAction;
  pqSMAdaptor *Adaptor;
  pqPipelineData *Pipeline;
  QMenu* SourcesMenu;
  QMenu* FiltersMenu;
  QMenu* ToolsMenu;
  pqObjectInspectorWidget *Inspector;
  QDockWidget *InspectorDock;
  QAction *InspectorDockAction;
  pqPipelineListWidget *PipelineList;
  QDockWidget *PipelineDock;
  QAction *PipelineDockAction;
  QDockWidget *HistogramDock;
  QDockWidget *LineChartDock;
  QAction *HistogramDockAction;
  QAction *LineChartDockAction;
  pqMultiViewFrame* ActiveView;
  QDockWidget *ElementInspectorDock;
  QAction *ElementDockAction;
  QToolBar* CompoundProxyToolBar;
  QToolBar* VariableSelectorToolBar;
  QToolBar* VCRControlsToolBar;

  pqSourceProxyInfo* ProxyInfo;
  vtkEventQtSlotConnect* VTKConnector;
  vtkSMProxy* CurrentProxy;

private slots:
  
  void onNewQVTKWidget(pqMultiViewFrame* parent);
  void onDeleteQVTKWidget(pqMultiViewFrame* parent);
  void onFrameActive(QWidget*);

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
  void onUpdateSourcesFiltersMenu(pqServer*);
  void onHelpAbout();
  void onUpdateWindows();
  void onCreateSource(QAction*);
  void onCreateFilter(QAction*);
  void onOpenLinkEditor();
  void onOpenCompoundFilterWizard();

  void onNewSelections(vtkSMProxy* p, vtkUnstructuredGrid* selections);
  void onCompoundProxyAdded(const QString&, const QString&);
  void onCreateCompoundProxy(QAction*);
  
  void onProxySelected(vtkSMProxy*);
  void onVariableChanged(pqVariableType, const QString&);
  
  void onRecordTest();
  void onRecordTest(const QStringList& Files);
  void onPlayTest();
  void onPlayTest(const QStringList& Files);
  
  void onPythonShell();

  void onAddServer(pqPipelineServer *server);
  void onRemoveServer(pqPipelineServer *server);
  void onAddWindow(pqPipelineWindow *window);

  void onFirstTimeStep();
  void onPreviousTimeStep();
  void onNextTimeStep();
  void onLastTimeStep();

private:
  void cleanUpWindow(QVTKWidget *window);
};

#endif // !_pqMainWindow_h

