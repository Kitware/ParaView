/*=========================================================================

   Program: ParaView
   Module:    pqPythonDialog.cxx

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
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
#include "pqPythonDialog.h"
#include "pqApplicationCore.h"
#include "pqFileDialog.h"
#include "pqSettings.h"
#include "ui_pqPythonDialog.h"

#include <QCloseEvent>
#include <QFile>
#include <QtDebug>

//////////////////////////////////////////////////////////////////////
// pqPythonDialog::pqImplementation

struct pqPythonDialog::pqImplementation
{
  Ui::pqPythonDialog Ui;
};

pqPythonDialog::pqPythonDialog(QWidget* Parent)
  : Superclass(Parent)
  , Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  this->setObjectName("pythonDialog");
  this->setWindowTitle(tr("Python Shell"));

  QObject::connect(this->Implementation->Ui.clear, SIGNAL(clicked()), this, SLOT(clearConsole()));

  QObject::connect(this->Implementation->Ui.close, SIGNAL(clicked()), this, SLOT(close()));

  QObject::connect(this->Implementation->Ui.runScript, SIGNAL(clicked()), this, SLOT(runScript()));

  QObject::connect(this->Implementation->Ui.reset, SIGNAL(clicked()),
    this->Implementation->Ui.shellWidget, SLOT(reset()));

  QObject::connect(this->Implementation->Ui.shellWidget, SIGNAL(executing(bool)),
    this->Implementation->Ui.runScript, SLOT(setDisabled(bool)));

  QObject::connect(this->Implementation->Ui.shellWidget, SIGNAL(executing(bool)),
    this->Implementation->Ui.clear, SLOT(setDisabled(bool)));

  QObject::connect(this->Implementation->Ui.shellWidget, SIGNAL(executing(bool)),
    this->Implementation->Ui.close, SLOT(setDisabled(bool)));

  pqApplicationCore::instance()->settings()->restoreState("PythonDialog", *this);
}

pqPythonDialog::~pqPythonDialog()
{
  // Only save geometry state if visible.  If not visible, the dialog reports
  // its position to be [0,0].  If not visible, then we have already saved
  // the correct geometry state in closeEvent().
  if (this->isVisible())
  {
    pqApplicationCore::instance()->settings()->saveState(*this, "PythonDialog");
  }
  delete Implementation;
}

void pqPythonDialog::closeEvent(QCloseEvent* e)
{
  pqApplicationCore::instance()->settings()->saveState(*this, "PythonDialog");
  e->accept();
}

pqPythonShell* pqPythonDialog::shell()
{
  return this->Implementation->Ui.shellWidget;
}

void pqPythonDialog::runScript()
{
  pqFileDialog dialog(
    NULL, this, tr("Run Script"), QString(), QString(tr("Python Script (*.py);;All files (*)")));
  dialog.setObjectName("PythonShellRunScriptDialog");
  dialog.setFileMode(pqFileDialog::ExistingFile);
  if (dialog.exec() == QDialog::Accepted)
  {
    this->runScript(dialog.getSelectedFiles());
  }
}

void pqPythonDialog::runScript(const QStringList& files)
{
  for (int i = 0; i != files.size(); ++i)
  {
    QFile file(files[i]);
    if (file.open(QIODevice::ReadOnly))
    {
      QByteArray code = file.readAll();
      this->Implementation->Ui.shellWidget->executeScript(code.data());
    }
    else
    {
      qCritical() << "Error opening " << files[i];
    }
  }
}

void pqPythonDialog::print(const QString& msg)
{
  this->Implementation->Ui.shellWidget->printMessage(msg);
}

void pqPythonDialog::runString(const QString& str)
{
  this->Implementation->Ui.shellWidget->executeScript(str);
}

void pqPythonDialog::clearConsole()
{
  this->Implementation->Ui.shellWidget->clear();
}
