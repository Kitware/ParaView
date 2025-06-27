// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
  Q_FOREACH (QWidget* topWidget, QApplication::topLevelWidgets())
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
    new pqFileDialog(server, this->mainWindow(), "Open Blot File", QString(), filter, false);
  QObject::connect(fdialog, &QWidget::close, fdialog, &QObject::deleteLater);
  fdialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(dialog, QOverload<const QStringList&>::of(&pqFileDialog::filesSelected), this,
    QOverload<const QStringList&>::of(&PVBlotPluginActions::startPVBlot));
  fdialog->show();
}

//-----------------------------------------------------------------------------
void PVBlotPluginActions::startPVBlot(const QString& filename)
{
  pqServer* server = PVBlotPluginActions::activeServer();

  pqBlotDialog* dialog = new pqBlotDialog(this->mainWindow());
  QObject::connect(dialog, &QWidget::close, dialog, &QObject::deleteLater);
  dialog->setActiveServer(server);
  dialog->show();
  dialog->open(filename);
}

//-----------------------------------------------------------------------------
void PVBlotPluginActions::startPVBlot(const QStringList& filenames)
{
  this->startPVBlot(filenames[0]);
}
