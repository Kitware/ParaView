// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "pqBlotDialog.h"

#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqSettings.h"

#include <QFile>
#include <QToolBar>
#include <QtDebug>

#include "ui_pqBlotDialog.h"
class pqBlotDialog::UI : public Ui::pqBlotDialog
{
};

//=============================================================================
pqBlotDialog::pqBlotDialog(QWidget* p)
  : QDialog(p)
{
  this->ui = new pqBlotDialog::UI();
  this->ui->setupUi(this);

  // Create the toolbar
  QToolBar* toolbar = new QToolBar(this);
  toolbar->setObjectName("toolbar");
  this->layout()->setMenuBar(toolbar);

  toolbar->addAction(this->ui->actionWireframe);
  toolbar->addAction(this->ui->actionSolid);

  QObject::connect(this->ui->runScript, SIGNAL(clicked()), this, SLOT(runScript()));
  QObject::connect(this->ui->close, SIGNAL(clicked()), this, SLOT(accept()));

  QObject::connect(
    this->ui->shellWidget, SIGNAL(executing(bool)), this->ui->buttons, SLOT(setDisabled(bool)));

  pqBlotDialogExecuteAction::connect(this->ui->actionWireframe, this->ui->shellWidget);
  pqBlotDialogExecuteAction::connect(this->ui->actionSolid, this->ui->shellWidget);

  pqApplicationCore::instance()->settings()->restoreState("PVBlotDialog", *this);
}

pqBlotDialog::~pqBlotDialog()
{
  pqApplicationCore::instance()->settings()->saveState(*this, "PVBlotDialog");
  delete this->ui;
}

//-----------------------------------------------------------------------------
pqServer* pqBlotDialog::activeServer() const
{
  return this->ui->shellWidget->activeServer();
}

void pqBlotDialog::setActiveServer(pqServer* server)
{
  this->ui->shellWidget->setActiveServer(server);
}

//-----------------------------------------------------------------------------
void pqBlotDialog::open() {}

//-----------------------------------------------------------------------------
void pqBlotDialog::open(const QString& filename)
{
  this->ui->shellWidget->initialize(filename);
}

//-----------------------------------------------------------------------------
void pqBlotDialog::open(const QStringList& filenames)
{
  this->open(filenames[0]);
}

//-----------------------------------------------------------------------------
void pqBlotDialog::runScript()
{
  QString filters = QString("%1 (*.blot *.bl);;%2 (*)").arg(tr("BLOT Script")).arg(tr("All files"));
  pqFileDialog* const dialog =
    new pqFileDialog(nullptr, this, tr("Run BLOT Script"), QString(), filters, false);

  dialog->setObjectName("BLOTShellRunScriptDialog");
  dialog->setFileMode(pqFileDialog::ExistingFile);
  QObject::connect(
    dialog, SIGNAL(filesSelected(const QStringList&)), this, SLOT(runScript(const QStringList&)));
  dialog->show();
}

//-----------------------------------------------------------------------------
void pqBlotDialog::runScript(const QStringList& files)
{
  Q_FOREACH (QString filename, files)
  {
    this->ui->shellWidget->executeBlotScript(filename);
  }
}

//=============================================================================
pqBlotDialogExecuteAction::pqBlotDialogExecuteAction(QObject* p, const QString& c)
  : QObject(p)
  , Command(c)
{
}

//-----------------------------------------------------------------------------
pqBlotDialogExecuteAction* pqBlotDialogExecuteAction::connect(QAction* action, pqBlotShell* shell)
{
  pqBlotDialogExecuteAction* convert = new pqBlotDialogExecuteAction(shell, action->text());
  QObject::connect(action, SIGNAL(triggered()), convert, SLOT(trigger()));
  QObject::connect(convert, SIGNAL(triggered(const QString&)), shell,
    SLOT(echoExecuteBlotCommand(const QString&)));

  return convert;
}

//-----------------------------------------------------------------------------
void pqBlotDialogExecuteAction::trigger()
{
  Q_EMIT this->triggered(this->Command);
}
