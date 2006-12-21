/*=========================================================================

   Program: ParaView
   Module:    pqFileChooserWidget.cxx

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


#include "pqFileChooserWidget.h"

// Qt includes
#include <QHBoxLayout>
#include <QLineEdit>
#include <QToolButton>

// ParaView
#include "pqFileDialog.h"

pqFileChooserWidget::pqFileChooserWidget(QWidget* p)
  : QWidget(p), Server(NULL)
{
  QHBoxLayout* l = new QHBoxLayout(this);
  l->setMargin(0);
  l->setSpacing(2);
  this->LineEdit = new QLineEdit(this);
  this->LineEdit->setObjectName("FileLineEdit");
  
  QToolButton* tb = new QToolButton(this);
  tb->setObjectName("FileButton");
  tb->setText("...");
  QObject::connect(tb, SIGNAL(clicked(bool)),
                   this, SLOT(chooseFile()));
  
  l->addWidget(this->LineEdit);
  l->addWidget(tb);

  QObject::connect(this->LineEdit,
                   SIGNAL(textChanged(const QString&)),
                   this,
                   SIGNAL(filenameChanged(const QString&)));

}

pqFileChooserWidget::~pqFileChooserWidget()
{
}


QString pqFileChooserWidget::filename()
{
  return this->LineEdit->text();
}

void pqFileChooserWidget::setFilename(const QString& file)
{
  this->LineEdit->setText(file);
}

QString pqFileChooserWidget::extension()
{
  return this->Extension;
}

void pqFileChooserWidget::setExtension(const QString& ext)
{
  this->Extension = ext;
}

pqServer* pqFileChooserWidget::server()
{
  return this->Server;
}

void pqFileChooserWidget::setServer(pqServer* s)
{
  this->Server = s;
}

void pqFileChooserWidget::chooseFile()
{
  QString filters = this->Extension;
  filters += ";;All files (*)";

  pqFileDialog* dialog = new pqFileDialog(this->Server,
    this, tr("Open File:"), QString(), filters);
  dialog->setFileMode(pqFileDialog::ExistingFile);
  if(QDialog::Accepted == dialog->exec())
    {
    QStringList files = dialog->getSelectedFiles();
    if(files.size())
      {
      this->LineEdit->setText(files[0]);
      }
    }
}



