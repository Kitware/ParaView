/*=========================================================================

   Program: ParaView
   Module:    pqOutputWindow.cxx

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

#include "pqApplicationCore.h"
#include "pqOutputWindow.h"
#include "pqSettings.h"

#include "ui_pqOutputWindow.h"

#include <vtkObjectFactory.h>

//////////////////////////////////////////////////////////////////////
// pqOutputWindow::pqImplementation

struct pqOutputWindow::pqImplementation
{
  Ui::pqOutputWindow Ui;
};

//////////////////////////////////////////////////////////////////////
// pqOutputWindow

pqOutputWindow::pqOutputWindow(QWidget* Parent) :
  Superclass(Parent),
  Implementation(new pqImplementation())
{
  this->Implementation->Ui.setupUi(this);
  this->setObjectName("outputDialog");
  this->setWindowTitle(tr("Output Messages"));
  this->ShowOutput = true;

  QObject::connect(this->Implementation->Ui.clearButton, 
    SIGNAL(clicked(bool)), this, SLOT(clear()));
}

pqOutputWindow::~pqOutputWindow()
{
  delete Implementation;
}

void pqOutputWindow::onDisplayTextInWindow(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkGreen);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  this->Implementation->Ui.consoleWidget->printString(text);
  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::onDisplayErrorTextInWindow(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  this->Implementation->Ui.consoleWidget->printString(text);
  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::onDisplayText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkGreen);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");
  cerr << text.toLatin1().data() << endl;
  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::onDisplayWarningText(const QString& text)
{
  if (
    text.contains("QEventDispatcherUNIX::unregisterTimer", Qt::CaseSensitive) ||
    text.contains("looking for 'HistogramView") ||
    text.contains("(looking for 'XYPlot") ||
    text.contains("Unrecognised OpenGL version")
    )
    {
    return;
    }
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::black);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");
  cerr << text.toLatin1().data() << endl;

  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::onDisplayGenericWarningText(const QString& text)
{
  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::black);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");
  cerr << text.toLatin1().data() << endl;

  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::onDisplayErrorText(const QString& text)
{
  if (
    text.contains("Unrecognised OpenGL version") ||
/* Skip DBusMenuExporterPrivate errors. These, I suspect, are due to
     * repeated menu actions in the menus. */
    text.contains("DBusMenuExporterPrivate") ||
    text.contains("DBusMenuExporterDBus")  )
    {
    return;
    }

  QTextCharFormat format = this->Implementation->Ui.consoleWidget->getFormat();
  format.setForeground(Qt::darkRed);
  format.clearBackground();
  this->Implementation->Ui.consoleWidget->setFormat(format);
  
  this->Implementation->Ui.consoleWidget->printString(text + "\n");
  cerr << text.toLatin1().data() << endl;

  if (this->ShowOutput && !text.trimmed().isEmpty())
    {
    this->show();
    }
}

void pqOutputWindow::accept()
{
  this->hide();
  Superclass::accept();
}

void pqOutputWindow::reject()
{
  this->hide();
  Superclass::reject();
}

void pqOutputWindow::clear()
{
  this->Implementation->Ui.consoleWidget->clear();
}

void pqOutputWindow::showEvent(QShowEvent* e)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
    {
    core->settings()->restoreState("OutputWindow", *this);
    }
  Superclass::showEvent(e);
}

void pqOutputWindow::hideEvent(QHideEvent* e)
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
    {
    core->settings()->saveState(*this, "OutputWindow");
    }
  Superclass::hideEvent(e);
}
