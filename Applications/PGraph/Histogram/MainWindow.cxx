/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "MainWindow.h"
#include "ChartAdapter.h"
#include "ui_MainWindow.h"

#include <pqApplicationCore.h>
#include <pqHistogramWidget.h>
#include <pqMainWindowCore.h>
#include <pqObjectInspectorWidget.h>
#include <pqPipelineMenu.h>
#include <pqPipelineSource.h>
#include <pqRenderWindowManager.h>
#include <pqSelectionManager.h>
#include <pqUndoStack.h>
#include <pqVCRController.h>
#include <pqViewMenu.h>

#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

class MainWindow::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    Core(parent),
    ViewMenu(0),
    Chart(0),
    Adapter(0)
  {
  }
  
  ~pqImplementation()
  {
    delete this->Chart;
    delete this->Adapter;
  }
  
  Ui::MainWindow UI;
  pqMainWindowCore Core;
  pqViewMenu* ViewMenu;
  pqHistogramWidget* Chart;
  ChartAdapter* Adapter;
};

MainWindow::MainWindow() :
  Implementation(new pqImplementation(this))
{
  this->Implementation->UI.setupUi(this);
  this->Implementation->ViewMenu = new pqViewMenu(*this->Implementation->UI.menuView);

  // Setup menus and toolbars ...
  connect(this->Implementation->UI.actionFileNew,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileNew()));

  connect(this->Implementation->UI.actionFileOpen,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileOpen()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileOpen(bool)),
    this->Implementation->UI.actionFileOpen,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileLoadServerState,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileLoadServerState()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileLoadServerState(bool)),
    this->Implementation->UI.actionFileLoadServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveServerState,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveServerState()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveServerState(bool)),
    this->Implementation->UI.actionFileSaveServerState,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveData,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveData()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveData(bool)),
    this->Implementation->UI.actionFileSaveData,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveScreenshot,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveScreenshot()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveScreenshot(bool)),
    this->Implementation->UI.actionFileSaveScreenshot,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileSaveAnimation,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onFileSaveAnimation()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFileSaveAnimation(bool)),
    this->Implementation->UI.actionFileSaveAnimation,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionFileExit,
    SIGNAL(triggered()), QApplication::instance(), SLOT(quit()));  

  connect(this->Implementation->UI.actionEditUndo,
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(Undo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(CanUndoChanged(bool)), this->Implementation->UI.actionEditUndo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(UndoLabelChanged(const QString&)), this, SLOT(onUndoLabel(const QString&)));
    
  connect(this->Implementation->UI.actionEditRedo,
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(Redo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(CanRedoChanged(bool)), this->Implementation->UI.actionEditRedo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(RedoLabelChanged(const QString&)), this, SLOT(onRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionEditCameraUndo,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onEditCameraUndo()));
  connect(&this->Implementation->Core,
    SIGNAL(enableCameraUndo(bool)),
    this->Implementation->UI.actionEditCameraUndo,
    SLOT(setEnabled(bool)));
  connect(&this->Implementation->Core,
    SIGNAL(cameraUndoLabel(const QString&)),
    this,
    SLOT(onCameraUndoLabel(const QString&)));
    
  connect(this->Implementation->UI.actionEditCameraRedo,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onEditCameraRedo()));
  connect(&this->Implementation->Core,
    SIGNAL(enableCameraRedo(bool)),
    this->Implementation->UI.actionEditCameraRedo,
    SLOT(setEnabled(bool)));
  connect(&this->Implementation->Core,
    SIGNAL(cameraRedoLabel(const QString&)),
    this,
    SLOT(onCameraRedoLabel(const QString&)));

  connect(this->Implementation->UI.actionServerConnect,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onServerConnect()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableServerConnect(bool)),
    this->Implementation->UI.actionServerConnect,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionServerDisconnect,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onServerDisconnect()));
  connect(
    &this->Implementation->Core,
    SIGNAL(enableServerDisconnect(bool)),
    this->Implementation->UI.actionServerDisconnect,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setSourceMenu(
    this->Implementation->UI.menuSources);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableSourceCreate(bool)),
    this->Implementation->UI.menuSources,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setFilterMenu(
    this->Implementation->UI.menuFilters);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableFilterCreate(bool)),
    this->Implementation->UI.menuFilters,
    SLOT(setEnabled(bool)));

  connect(this->Implementation->UI.actionToolsCreateCustomFilter,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsCreateCustomFilter()));

  connect(this->Implementation->UI.actionToolsManageCustomFilters,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsManageCustomFilters()));

  connect(this->Implementation->UI.actionToolsDumpWidgetNames,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsDumpWidgetNames()));

  connect(this->Implementation->UI.actionToolsRecordTest,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsRecordTest()));

  connect(this->Implementation->UI.actionToolsRecordTestScreenshot,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsRecordTestScreenshot()));

  connect(this->Implementation->UI.actionToolsPlayTest,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsPlayTest()));

  connect(this->Implementation->UI.actionToolsPythonShell,
    SIGNAL(triggered()), &this->Implementation->Core, SLOT(onToolsPythonShell()));

  this->Implementation->Core.pipelineMenu().addActionsToMenu(
    this->Implementation->UI.menuPipeline);

  connect(this->Implementation->UI.actionHelpEnableTooltips,
    SIGNAL(triggered(bool)), &this->Implementation->Core, SLOT(onHelpEnableTooltips(bool)));
  this->Implementation->Core.onHelpEnableTooltips(
    this->Implementation->UI.actionHelpEnableTooltips->isChecked());

  connect(this->Implementation->UI.actionVCRFirstFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onFirstFrame()));
    
  connect(this->Implementation->UI.actionVCRPreviousFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onPreviousFrame()));
    
  connect(this->Implementation->UI.actionVCRNextFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onNextFrame()));
    
  connect(this->Implementation->UI.actionVCRLastFrame,
    SIGNAL(triggered()), &this->Implementation->Core.VCRController(), SLOT(onLastFrame()));

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.VCRToolbar,
    this->Implementation->UI.VCRToolbar->windowTitle());

  connect(this->Implementation->UI.actionMoveMode, 
    SIGNAL(triggered()), &this->Implementation->Core.selectionManager(), SLOT(switchToInteraction()));
    
  connect(this->Implementation->UI.actionSelectionMode, 
    SIGNAL(triggered()), &this->Implementation->Core.selectionManager(), SLOT(switchToSelection()));

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.selectionToolbar,
    this->Implementation->UI.selectionToolbar->windowTitle());

  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.undoRedoToolbar,
    this->Implementation->UI.undoRedoToolbar->windowTitle());

  this->Implementation->Core.setupVariableToolbar(
    this->Implementation->UI.variableToolbar);
  connect(
    &this->Implementation->Core,
    SIGNAL(enableVariableToolbar(bool)),
    this->Implementation->UI.variableToolbar,
    SLOT(setEnabled(bool)));
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.variableToolbar,
    this->Implementation->UI.variableToolbar->windowTitle());

  connect(
    &this->Implementation->Core,
    SIGNAL(enableSelectionToolbar(bool)),
    this->Implementation->UI.selectionToolbar,
    SLOT(setEnabled(bool)));

  this->Implementation->Core.setupCustomFilterToolbar(
    this->Implementation->UI.customFilterToolbar);
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.customFilterToolbar,
    this->Implementation->UI.customFilterToolbar->windowTitle());

  this->Implementation->ViewMenu->addSeparator();

  // Setup dockable windows ...
  this->Implementation->Core.setupPipelineBrowser(
    this->Implementation->UI.pipelineBrowserDock);
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.pipelineBrowserDock,
    this->Implementation->UI.pipelineBrowserDock->windowTitle());

  pqObjectInspectorWidget* const object_inspector =
    this->Implementation->Core.setupObjectInspector(
      this->Implementation->UI.objectInspectorDock);
      
  QObject::connect(
    object_inspector,
    SIGNAL(preaccept()), 
    this,
    SLOT(onPreAccept()));

  QObject::connect(
    object_inspector,
    SIGNAL(postaccept()), 
    this,
    SLOT(onPostAccept()));
      
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.objectInspectorDock,
    this->Implementation->UI.objectInspectorDock->windowTitle());
    
  this->Implementation->Core.setupStatisticsView(
    this->Implementation->UI.statisticsViewDock);
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.statisticsViewDock,
    this->Implementation->UI.statisticsViewDock->windowTitle());
    
  this->Implementation->Core.setupElementInspector(
    this->Implementation->UI.elementInspectorDock);
  this->Implementation->ViewMenu->addWidget(this->Implementation->UI.elementInspectorDock,
    this->Implementation->UI.elementInspectorDock->windowTitle());

  // Add the chart dock window.
  QDockWidget* const chart_dock = new QDockWidget("Chart View", this);
  chart_dock->setObjectName("ChartDock");
  chart_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  chart_dock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);

  QLabel* const proxy_label = new QLabel("Chart Source: ");
  proxy_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  
  QComboBox* const proxy_selector = new QComboBox();

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->setMargin(0);
  hbox->addWidget(proxy_label);
  hbox->addWidget(proxy_selector);

  this->Implementation->Chart = new pqHistogramWidget(0); 
  this->Implementation->Adapter = new ::ChartAdapter(*this->Implementation->Chart);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(this->Implementation->Chart);
  
  QWidget* const widget = new QWidget();
  widget->setLayout(vbox);
  
  chart_dock->setWidget(widget);
  this->addDockWidget(Qt::BottomDockWidgetArea, chart_dock);

  connect(
    pqApplicationCore::instance(),
    SIGNAL(activeSourceChanged(pqPipelineSource*)),
    this,
    SLOT(onActiveSourceChanged(pqPipelineSource*)));

  this->Implementation->ViewMenu->addWidget(chart_dock,
    chart_dock->windowTitle());

  // Setup the multiview render window ...
  this->setCentralWidget(&this->Implementation->Core.multiViewManager());

  // Setup the statusbar ...
  this->Implementation->Core.setupProgressBar(this->statusBar());

  // Set up the dock window corners to give the vertical docks more room.
  this->setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
  this->setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

  // Setup the default dock configuration ...
  this->Implementation->UI.elementInspectorDock->hide();
  this->Implementation->UI.statisticsViewDock->hide();

  // Now that we're ready, initialize everything ...
  this->Implementation->Core.initializeStates();
}

MainWindow::~MainWindow()
{
  delete this->Implementation;
}

void MainWindow::onUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditUndo->setText(
    label.isEmpty() ? tr("Can't Undo") : QString(tr("&Undo %1")).arg(label));
}

void MainWindow::onRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditRedo->setText(
    label.isEmpty() ? tr("Can't Redo") : QString(tr("&Redo %1")).arg(label));
}

void MainWindow::onCameraUndoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraUndo->setText(
    label.isEmpty() ? tr("Can't Undo Camera") : QString(tr("U&ndo %1")).arg(label));
}

void MainWindow::onCameraRedoLabel(const QString& label)
{
  this->Implementation->UI.actionEditCameraRedo->setText(
    label.isEmpty() ? tr("Can't Redo Camera") : QString(tr("R&edo %1")).arg(label));
}

void MainWindow::onPreAccept()
{
  this->setEnabled(false);
  this->statusBar()->showMessage(tr("Updating..."));
}

void MainWindow::onPostAccept()
{
  this->setEnabled(true);
  this->statusBar()->showMessage(tr("Ready"), 2000);
}

void MainWindow::onActiveSourceChanged(pqPipelineSource* source)
{
  this->Implementation->Adapter->setSource(source ? source->getProxy() : 0);
}
