/*=========================================================================

   Program: ParaView
   Module:    pqPythonDialog.cxx

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.1. 

   See License_v1.1.txt for the full ParaView license.
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
#include "ui_pqPythonDialog.h"

#include <pqFileDialog.h>
#include <pqLocalFileDialogModel.h>

#include <QFile>
#include <QtDebug>

//////////////////////////////////////////////////////////////////////
// pqPythonDialog::pqImplementation

struct pqPythonDialog::pqImplementation
{
  Ui::pqPythonDialog Ui;
};

pqPythonDialog::pqPythonDialog(QWidget* Parent, int argc, char** argv) :
  QDialog(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  this->setObjectName("pythonDialog");
  this->setWindowTitle(tr("Python Shell"));
  
  QObject::connect(
    this->Implementation->Ui.clear,
    SIGNAL(clicked()),
    this,
    SLOT(clearConsole()));
    
  QObject::connect(
    this->Implementation->Ui.runScript,
    SIGNAL(clicked()),
    this,
    SLOT(runScript()));

  this->Implementation->Ui.shellWidget->InitializeInterpretor(argc, argv);
}

pqPythonDialog::~pqPythonDialog()
{
  delete Implementation;
}

void pqPythonDialog::runScript()
{
  pqFileDialog* const dialog = new pqFileDialog(
    new pqLocalFileDialogModel(),
    this,
    tr("Run Script"),
    QString(),
    QString(tr("Python Script (*.py);;All files (*)")));
    
  dialog->setObjectName("PythonShellRunScriptDialog");
  dialog->setFileMode(pqFileDialog::ExistingFiles);
  QObject::connect(
    dialog,
    SIGNAL(filesSelected(const QStringList&)), 
    this,
    SLOT(runScript(const QStringList&)));
  dialog->show(); 
}

void pqPythonDialog::runScript(const QStringList& files)
{
  for(int i = 0; i != files.size(); ++i)
    {
    QFile file(files[i]);
    if(file.open(QIODevice::ReadOnly))
      {
      this->Implementation->Ui.shellWidget->executeScript(
        file.readAll().data());
      }
    else
      {
      qCritical() << "Error opening " << files[i];
      }
    }
}

void pqPythonDialog::clearConsole()
{
  this->Implementation->Ui.shellWidget->clear();
}

void pqPythonDialog::accept()
{
  QDialog::accept();
  delete this;
}

void pqPythonDialog::reject()
{
  QDialog::reject();
  delete this;
}

