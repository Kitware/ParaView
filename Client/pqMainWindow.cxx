/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqMainWindow.h"
#include "pqServerFileBrowser.h"

#include <vtkProcessModule.h>

#include <QApplication>
#include <QMenu>
#include <QMenuBar>

pqMainWindow::pqMainWindow(QApplication& Application)
{
  setWindowTitle("ParaQ Client");

  QAction* const fileOpenAction = new QAction(tr("Open..."), this);
  connect(fileOpenAction, SIGNAL(activated()), this, SLOT(onFileOpen()));

  QAction* const fileQuitAction = new QAction(tr("Quit"), this);
  connect(fileQuitAction, SIGNAL(activated()), &Application, SLOT(quit()));

  QAction* const test1Action = new QAction(tr("Test 1"), this);
  connect(test1Action, SIGNAL(activated()), this, SLOT(onTest1()));

  QAction* const test2Action = new QAction(tr("Test 2"), this);
  connect(test2Action, SIGNAL(activated()), this, SLOT(onTest2()));

  QMenu* const fileMenu = menuBar()->addMenu(tr("File"));
  fileMenu->addAction(fileOpenAction);
  fileMenu->addAction(fileQuitAction);
  
  QMenu* const testMenu = menuBar()->addMenu(tr("Test"));
  testMenu->addAction(test1Action);
  testMenu->addAction(test2Action);
}

void pqMainWindow::onFileOpen()
{
//  pqServerFileBrowserList* const file_browser = new pqServerFileBrowserList();
//  file_browser->setProcessModule(vtkProcessModule::GetProcessModule());
//  Ui::ServerFileBrowser* const file_browser = new Ui::ServerFileBrowser();
//  file_browser->setupUi();
  pqServerFileBrowser* const file_browser = new pqServerFileBrowser();
  file_browser->show();
}

void pqMainWindow::onTest1()
{
}

void pqMainWindow::onTest2()
{
}

