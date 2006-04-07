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

#include <pqConnect.h>
#include <pqFileDialog.h>
#include <pqLineChartWidget.h>
#include <pqLocalFileDialogModel.h>
#include <pqSetName.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSpinBox>
#include <QVBoxLayout>

MainWindow::MainWindow() :
  LineChartWidget(0),
  LineChart(0),
  ChooseDataCombo(0)
{
  this->setWindowTitle("Dobran-O-Viz 0.3");

  this->createStandardFileMenu();
  this->createStandardViewMenu();
  this->createStandardServerMenu();
  this->createStandardSourcesMenu();
  this->createStandardFiltersMenu();
  this->createStandardToolsMenu();

  QMenu* const help_menu = this->helpMenu();
  help_menu->addAction(tr("&About Dobran-O-Viz"))
    << pqSetName("About")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onHelpAbout()));
  
  this->createStandardPipelineBrowser(false);
  this->createStandardObjectInspector(false);
  this->createStandardElementInspector(false);
  
  this->createStandardVCRToolBar();
  this->createStandardVariableToolBar();
  this->createStandardCompoundProxyToolBar();

  // Add the line plot dock window.
  QDockWidget* const line_chart_dock = new QDockWidget("Line Chart View", this);
  line_chart_dock->setObjectName("LineChartDock");
  line_chart_dock->setFeatures(QDockWidget::AllDockWidgetFeatures);
  line_chart_dock->setAllowedAreas(
    Qt::BottomDockWidgetArea |
    Qt::LeftDockWidgetArea |
    Qt::RightDockWidgetArea);

  this->LineChartWidget = new pqLineChartWidget(0);
  this->LineChartWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(this->LineChartWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onLineChartContextMenu(const QPoint&)));
  
  this->LineChart = new ::LineChartAdapter(*this->LineChartWidget);

  this->connect(this->LineChart, SIGNAL(experimentalDataChanged(const QStringList&)), this, SLOT(onExperimentalDataChanged(const QStringList&)));
  this->connect(this->LineChart, SIGNAL(visibleDataChanged(const QString&)), this, SLOT(onVisibleDataChanged(const QString&)));

  this->ChooseDataCombo = new QComboBox();
  this->connect(this->ChooseDataCombo, SIGNAL(activated(const QString&)),  this->LineChart, SLOT(setVisibleData(const QString&)));

  QLabel* const sample_size_label = new QLabel(tr("Samples:"));
  sample_size_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QSpinBox* const sample_size_box = new QSpinBox();
  sample_size_box->setMinimum(2);
  sample_size_box->setMaximum(999999);
  sample_size_box->setValue(50);
  this->connect(sample_size_box, SIGNAL(valueChanged(int)), this->LineChart, SLOT(setSamples(int)));

  QLabel* const error_bar_size_label = new QLabel(tr("Error Bar Width:"));
  error_bar_size_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  
  QDoubleSpinBox* const error_bar_size_box = new QDoubleSpinBox();
  error_bar_size_box->setMinimum(0);
  error_bar_size_box->setMaximum(1);
  error_bar_size_box->setSingleStep(0.05);
  error_bar_size_box->setValue(0.5);
  this->connect(error_bar_size_box, SIGNAL(valueChanged(double)), this->LineChart, SLOT(setErrorBarWidth(double)));

  QCheckBox* const show_data_button = new QCheckBox(tr("Show Data"));
  this->connect(show_data_button, SIGNAL(toggled(bool)), this->LineChart, SLOT(showData(bool)));
  show_data_button->setChecked(true);

  QCheckBox* const show_differences_button = new QCheckBox(tr("Show Differences"));
  this->connect(show_differences_button, SIGNAL(toggled(bool)), this->LineChart, SLOT(showDifferences(bool)));
  show_differences_button->setChecked(false);

  QHBoxLayout* const hbox5 = new QHBoxLayout();
  hbox5->setMargin(0);
  hbox5->addWidget(this->ChooseDataCombo);
  hbox5->addWidget(sample_size_label);
  hbox5->addWidget(sample_size_box);
  hbox5->addWidget(error_bar_size_label);
  hbox5->addWidget(error_bar_size_box);
  hbox5->addWidget(show_data_button);
  hbox5->addWidget(show_differences_button);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox5);
  vbox->addWidget(this->LineChartWidget);
  
  QWidget* const widget = new QWidget();
  widget->setContextMenuPolicy(Qt::CustomContextMenu);
  this->connect(widget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onLineChartContextMenu(const QPoint&)));
  
  widget->setLayout(vbox);
  
  line_chart_dock->setWidget(widget);

  connect(this, SIGNAL(serverChanged(pqServer*)), this->LineChart, SLOT(setServer(pqServer*)));
  connect(this, SIGNAL(proxyChanged(vtkSMProxy*)), this->LineChart, SLOT(setExodusProxy(vtkSMProxy*)));
  connect(this, SIGNAL(variableChanged(pqVariableType, const QString&)), this->LineChart, SLOT(setExodusVariable(pqVariableType, const QString&)));

  this->addStandardDockWidget(Qt::BottomDockWidgetArea, line_chart_dock, QIcon(":pqChart/pqLineChart22.png"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::onHelpAbout()
{
  AboutDialog* const dialog = new AboutDialog(this);
  dialog->show();
}

void MainWindow::onLoadSetup()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open Setup:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(loadSetup(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onSavePDF()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save .pdf File:"), this, "fileSavePDFDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(savePDF(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onSavePNG()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Save .png File:"), this, "fileSavePNGDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(saveImage(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentalData()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open Experimental Data:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(loadExperimentalData(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentalUncertainty()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open Experimental Uncertainty Data:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(loadExperimentalUncertainty(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onLoadSimulationUncertainty()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open Simulation Uncertainty Data:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(loadSimulationUncertainty(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onLoadExperimentSimulationMap()
{
  pqFileDialog* file_dialog = new pqFileDialog(new pqLocalFileDialogModel(), tr("Open Experiment / Simulation Map:"), this, "fileOpenDialog")
    << pqConnect(SIGNAL(filesSelected(const QStringList&)), this->LineChart, SLOT(loadExperimentSimulationMap(const QStringList&)));
    
  file_dialog->show();
}

void MainWindow::onExperimentalDataChanged(const QStringList& changed_data)
{
  this->ChooseDataCombo->clear();
  this->ChooseDataCombo->addItems(changed_data);
}

void MainWindow::onVisibleDataChanged(const QString& text)
{
  this->ChooseDataCombo->setCurrentIndex(this->ChooseDataCombo->findText(text));
}

void MainWindow::onLineChartContextMenu(const QPoint&)
{
  QMenu popup_menu;

  popup_menu.addAction("Clear All")
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentalData()))
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentalUncertainty()))
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearSimulationUncertainty()))
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentSimulationMap()));

  popup_menu.addAction("Load Setup")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadSetup()));

  popup_menu.addSeparator();

  this->LineChartWidget->addMenuActions(popup_menu);

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experimental Data")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentalData()));

  popup_menu.addAction("Clear Experimental Data")
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentalData()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experimental Uncertainty")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentalUncertainty()));

  popup_menu.addAction("Clear Experimental Uncertainty")
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentalUncertainty()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Simulation Uncertainty")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadSimulationUncertainty()));

  popup_menu.addAction("Clear Simulation Uncertainty")
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearSimulationUncertainty()));

  popup_menu.addSeparator();
  
  popup_menu.addAction("Load Experiment / Simulation Map")
    << pqConnect(SIGNAL(triggered()), this, SLOT(onLoadExperimentSimulationMap()));

  popup_menu.addAction("Clear Experiment / Simulation Map")
    << pqConnect(SIGNAL(triggered()), this->LineChart, SLOT(clearExperimentSimulationMap()));

  popup_menu.exec(QCursor::pos());
}
