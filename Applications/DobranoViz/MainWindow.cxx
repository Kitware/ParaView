/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "AboutDialog.h"
#include "MainWindow.h"
#include "LineChartAdapter.h"

#include "ui_MainWindow.h"

#include <pqApplicationCore.h>
#include <pqChartContextMenu.h>
#include <pqConnect.h>
#include <pqSetName.h>
#include <pqFileDialog.h>
#include <pqLineChartWidget.h>
#include <pqMainWindowCore.h>
#include <pqObjectInspectorWidget.h>
#include <pqPipelineMenu.h>
#include <pqViewManager.h>
#include <pqSelectionManager.h>
#include <pqSetName.h>
#include <pqUndoStack.h>
#include <pqVCRController.h>
#include <pqViewMenu.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSpinBox>
#include <QVBoxLayout>

class MainWindow::pqImplementation
{
public:
  pqImplementation(QWidget* parent) :
    Core(parent),
    ViewMenu(0),
    LineChartWidget(0),
    ChartMenu(0),
    LineChart(0),
    ChooseDataCombo(0)
  {
  }

  ~pqImplementation()
  {
    delete this->ChooseDataCombo;
    delete this->LineChart;
    delete this->LineChartWidget;
    delete this->ViewMenu;
  }
  
  Ui::MainWindow UI;
  pqMainWindowCore Core;
  pqViewMenu* ViewMenu;
  pqLineChartWidget* LineChartWidget;
  pqChartContextMenu *ChartMenu;
  LineChartAdapter* LineChart;
  QComboBox* ChooseDataCombo;
};

MainWindow::MainWindow() :
  Implementation(new pqImplementation(this))
{
  this->Implementation->UI.setupUi(this);
  this->Implementation->ViewMenu = new pqViewMenu(*this->Implementation->UI.menuView);

  // Setup the default chart context menu.
  this->Implementation->ChartMenu = new pqChartContextMenu(this);

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
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(undo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(canUndoChanged(bool)), this->Implementation->UI.actionEditUndo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(undoLabelChanged(const QString&)), this, SLOT(onUndoLabel(const QString&)));
    
  connect(this->Implementation->UI.actionEditRedo,
    SIGNAL(triggered()), pqApplicationCore::instance()->getUndoStack(), SLOT(Redo()));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(canRedoChanged(bool)), this->Implementation->UI.actionEditRedo, SLOT(setEnabled(bool)));
  connect(pqApplicationCore::instance()->getUndoStack(),
    SIGNAL(redoLabelChanged(const QString&)), this, SLOT(onRedoLabel(const QString&)));

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

  connect(this->Implementation->UI.actionHelpAbout,
    SIGNAL(triggered()), this, SLOT(onHelpAbout()));

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

  // Add the line plot dock window.
  QDockWidget* const line_chart_dock = new QDockWidget("Line Chart View", this);
  line_chart_dock->setObjectName("LineChartDock");
  line_chart_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  line_chart_dock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);

  this->Implementation->LineChartWidget = new pqLineChartWidget(0);
  this->Implementation->LineChartWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(this->Implementation->LineChartWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onLineChartContextMenu(const QPoint&)));
  
  this->Implementation->LineChart = new ::LineChartAdapter(*this->Implementation->LineChartWidget);

  this->connect(this->Implementation->LineChart, SIGNAL(experimentalDataChanged(const QStringList&)), this, SLOT(onExperimentalDataChanged(const QStringList&)));
  this->connect(this->Implementation->LineChart, SIGNAL(visibleDataChanged(const QString&)), this, SLOT(onVisibleDataChanged(const QString&)));

  this->Implementation->ChooseDataCombo = new QComboBox();
  this->connect(this->Implementation->ChooseDataCombo, SIGNAL(activated(const QString&)),  this->Implementation->LineChart, SLOT(setVisibleData(const QString&)));

  QLabel* const sample_size_label = new QLabel(tr("Samples:"));
  sample_size_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QSpinBox* const sample_size_box = new QSpinBox();
  sample_size_box->setMinimum(2);
  sample_size_box->setMaximum(999999);
  sample_size_box->setValue(50);
  this->connect(sample_size_box, SIGNAL(valueChanged(int)), this->Implementation->LineChart, SLOT(setSamples(int)));

  QLabel* const error_bar_size_label = new QLabel(tr("Error Bar Width:"));
  error_bar_size_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  
  QDoubleSpinBox* const error_bar_size_box = new QDoubleSpinBox();
  error_bar_size_box->setMinimum(0);
  error_bar_size_box->setMaximum(1);
  error_bar_size_box->setSingleStep(0.05);
  error_bar_size_box->setValue(0.5);
  this->connect(error_bar_size_box, SIGNAL(valueChanged(double)), this->Implementation->LineChart, SLOT(setErrorBarWidth(double)));

  QCheckBox* const show_data_button = new QCheckBox(tr("Show Data"));
  this->connect(show_data_button, SIGNAL(toggled(bool)), this->Implementation->LineChart, SLOT(showData(bool)));
  show_data_button->setChecked(true);

  QCheckBox* const show_differences_button = new QCheckBox(tr("Show Differences"));
  this->connect(show_differences_button, SIGNAL(toggled(bool)), this->Implementation->LineChart, SLOT(showDifferences(bool)));
  show_differences_button->setChecked(false);

  QHBoxLayout* const hbox5 = new QHBoxLayout();
  hbox5->setMargin(0);
  hbox5->addWidget(this->Implementation->ChooseDataCombo);
  hbox5->addWidget(sample_size_label);
  hbox5->addWidget(sample_size_box);
  hbox5->addWidget(error_bar_size_label);
  hbox5->addWidget(error_bar_size_box);
  hbox5->addWidget(show_data_button);
  hbox5->addWidget(show_differences_button);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox5);
  vbox->addWidget(this->Implementation->LineChartWidget);
  
  QWidget* const widget = new QWidget();
  widget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onLineChartContextMenu(const QPoint&)));
  
  widget->setLayout(vbox);
  
  line_chart_dock->setWidget(widget);
  this->addDockWidget(Qt::BottomDockWidgetArea, line_chart_dock);

  connect(
    &this->Implementation->Core,
    SIGNAL(serverChanged(pqServer*)),
    this->Implementation->LineChart,
    SLOT(setServer(pqServer*)));
  connect(
    &this->Implementation->Core,
    SIGNAL(proxyChanged(vtkSMProxy*)),
    this->Implementation->LineChart,
    SLOT(setExodusProxy(vtkSMProxy*)));
  connect(
    &this->Implementation->Core,
    SIGNAL(variableChanged(pqVariableType, const QString&)),
    this->Implementation->LineChart,
    SLOT(setExodusVariable(pqVariableType, const QString&)));

  this->Implementation->ViewMenu->addWidget(
    line_chart_dock,
    line_chart_dock->windowTitle(),
    QIcon(":pqChart/pqLineChart22.png"));

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

void MainWindow::onHelpAbout()
{
  AboutDialog* const dialog = new AboutDialog(this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->show();
}

void MainWindow::onLoadSetup()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Open Setup:"))
    << pqSetName("fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(loadSetup(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
    
  file_dialog->show();
}

void MainWindow::onSavePDF()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Save .pdf File:"))
    << pqSetName("fileSavePDFDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(savePDF(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::AnyFile);
    
  file_dialog->show();
}

void MainWindow::onSavePNG()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Save .png File:"))
    << pqSetName("fileSavePNGDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(saveImage(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::AnyFile);
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentalData()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Open Experimental Data:"))
    << pqSetName("fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(loadExperimentalData(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentalUncertainty()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Open Experimental Uncertainty Data:"))
    << pqSetName("fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(loadExperimentalUncertainty(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
    
  file_dialog->show();
}

void MainWindow::onLoadSimulationUncertainty()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Open Simulation Uncertainty Data:"))
    << pqSetName("fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(loadSimulationUncertainty(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentSimulationMap()
{
  pqFileDialog* file_dialog = new pqFileDialog(NULL,
      this, tr("Open Experiment / Simulation Map:"))
    << pqSetName("fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->Implementation->LineChart, SLOT(loadExperimentSimulationMap(const QStringList&)));
  file_dialog->setAttribute(Qt::WA_DeleteOnClose);
  file_dialog->setFileMode(pqFileDialog::ExistingFiles);
    
  file_dialog->show();
}

void MainWindow::onExperimentalDataChanged(const QStringList& changed_data)
{
  this->Implementation->ChooseDataCombo->clear();
  this->Implementation->ChooseDataCombo->addItems(changed_data);
}

void MainWindow::onVisibleDataChanged(const QString& text)
{
  this->Implementation->ChooseDataCombo->setCurrentIndex(this->Implementation->ChooseDataCombo->findText(text));
}

void MainWindow::onLineChartContextMenu(const QPoint&)
{
  QMenu popup_menu;

  popup_menu.addAction("Clear All")
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentalData()))
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentalUncertainty()))
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearSimulationUncertainty()))
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentSimulationMap()));

  popup_menu.addAction("Load Setup")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadSetup()));

  popup_menu.addSeparator();

  //this->Implementation->LineChartWidget->addMenuActions(popup_menu);
  this->Implementation->ChartMenu->addMenuActions(popup_menu,
      this->Implementation->LineChartWidget);

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experimental Data")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentalData()));

  popup_menu.addAction("Clear Experimental Data")
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentalData()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experimental Uncertainty")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentalUncertainty()));

  popup_menu.addAction("Clear Experimental Uncertainty")
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentalUncertainty()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Simulation Uncertainty")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadSimulationUncertainty()));

  popup_menu.addAction("Clear Simulation Uncertainty")
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearSimulationUncertainty()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experiment / Simulation Map")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentSimulationMap()));

  popup_menu.addAction("Clear Experiment / Simulation Map")
    << pqConnect(SIGNAL(triggered()), this->Implementation->LineChart, SLOT(clearExperimentSimulationMap()));

  popup_menu.exec(QCursor::pos());
}
