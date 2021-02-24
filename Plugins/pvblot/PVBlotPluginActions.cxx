// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PVBlotPluginActions.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "PVBlotPluginActions.h"

#include "pqApplicationCore.h"
#include "pqBlotDialog.h"
#include "pqFileDialog.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QApplication>
#include <QMainWindow>

//=============================================================================
PVBlotPluginActions::PVBlotPluginActions(QObject* p)
  : QActionGroup(p)
{
  QAction* action = new QAction("PVBlot", this);
  QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(startPVBlot()));
  this->addAction(action);
}

//-----------------------------------------------------------------------------
pqServer* PVBlotPluginActions::activeServer()
{
  pqApplicationCore* app = pqApplicationCore::instance();
  pqServerManagerModel* smModel = app->getServerManagerModel();
  pqServer* server = smModel->getItemAtIndex<pqServer*>(0);
  return server;
}

//-----------------------------------------------------------------------------
QWidget* PVBlotPluginActions::mainWindow()
{
  foreach (QWidget* topWidget, QApplication::topLevelWidgets())
  {
    if (qobject_cast<QMainWindow*>(topWidget))
      return topWidget;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void PVBlotPluginActions::startPVBlot()
{
  pqServer* server = PVBlotPluginActions::activeServer();

  // Allow the user to pick a file, and then send that to create a new
  // blot window.
  QString filter =
    "Exodus Files (*.g *.e *.ex2 *.ex2v2 *.exo *.gen *.exoII *.0 *.00 *.000 *.0000 *.exii);;"
    "SpyPlot CTH Files (*.spcth *.0);;"
    "All Files (*)";

  pqFileDialog* fdialog =
    new pqFileDialog(server, this->mainWindow(), "Open Blot File", QString(), filter);
  fdialog->setAttribute(Qt::WA_DeleteOnClose);
  fdialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(fdialog, SIGNAL(filesSelected(const QStringList&)), this,
    SLOT(startPVBlot(const QStringList&)));
  fdialog->show();
}

//-----------------------------------------------------------------------------
void PVBlotPluginActions::startPVBlot(const QString& filename)
{
  pqServer* server = PVBlotPluginActions::activeServer();

  pqBlotDialog* dialog = new pqBlotDialog(this->mainWindow());
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setActiveServer(server);
  dialog->show();
  dialog->open(filename);
}

//-----------------------------------------------------------------------------
void PVBlotPluginActions::startPVBlot(const QStringList& filenames)
{
  this->startPVBlot(filenames[0]);
}
