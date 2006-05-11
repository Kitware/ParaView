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

#include "pqHistogramWidget.h"

#include <QComboBox>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow() :
  Chart(0),
  Adapter(0)
{
  this->setWindowTitle("PGraphHistogram");

  this->createStandardFileMenu();
  this->createStandardViewMenu();
  this->createStandardServerMenu();
  this->createStandardSourcesMenu();
  this->createStandardFiltersMenu();
  this->createStandardToolsMenu();

  this->createStandardPipelineBrowser(true);
  this->createStandardObjectInspector(true);
  this->createStandardElementInspector(false);
  
  this->createStandardVCRToolBar();
  this->createStandardVariableToolBar();
  this->createStandardCompoundProxyToolBar();

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

  this->Chart = new pqHistogramWidget(0); 
  this->Adapter = new ::ChartAdapter(*this->Chart);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->setMargin(0);
  vbox->addLayout(hbox);
  vbox->addWidget(this->Chart);
  
  QWidget* const widget = new QWidget();
  widget->setLayout(vbox);
  
  chart_dock->setWidget(widget);

//  connect(this, SIGNAL(serverChanged(pqServer*)), this->Adapter, SLOT(setServer(pqServer*)));
  connect(this, SIGNAL(proxyChanged(vtkSMProxy*)), this->Adapter, SLOT(setSource(vtkSMProxy*)));

  this->addStandardDockWidget(Qt::BottomDockWidgetArea, chart_dock, QIcon());
}

MainWindow::~MainWindow()
{
  delete this->Adapter;
  delete this->Chart;
}
