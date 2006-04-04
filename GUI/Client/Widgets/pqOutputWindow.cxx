/*=========================================================================

   Program:   ParaQ
   Module:    $RCS $

   Copyright (c) 2005,2006 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaQ is a free software; you can redistribute it and/or modify it
   under the terms of the ParaQ license version 1.1. 

   See License_v1.1.txt for the full ParaQ license.
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

#include "pqOutputWindow.h"
#include "ui_pqOutputWindow.h"

//#include <QTextCharFormat>

//////////////////////////////////////////////////////////////////////
// pqOutputWindow::pqImplementation

struct pqOutputWindow::pqImplementation
{
  Ui::pqOutputWindow Ui;
};

//////////////////////////////////////////////////////////////////////
// pqOutputWindow

pqOutputWindow::pqOutputWindow(QWidget* Parent) :
  QDialog(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  this->setObjectName("outputDialog");
  this->setWindowTitle(tr("Output Messages"));
}

pqOutputWindow::~pqOutputWindow()
{
  delete Implementation;
}

void pqOutputWindow::onDisplayText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkGreen);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");

  this->show();
}

void pqOutputWindow::onDisplayWarningText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");

  this->show();
}

void pqOutputWindow::onDisplayGenericWarningText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");

  this->show();
}

void pqOutputWindow::onDisplayErrorText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::white);
  format.setBackground(Qt::red);
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");

  this->show();
}

void pqOutputWindow::accept()
{
  this->hide();
  QDialog::accept();
}

void pqOutputWindow::reject()
{
  this->hide();
  QDialog::reject();
}
