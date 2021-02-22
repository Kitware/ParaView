/*=========================================================================

   Program: ParaView
   Module:    pqSelectReaderDialog.cxx

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

#include "pqSelectReaderDialog.h"
#include "ui_pqSelectReaderDialog.h"

#include <QListWidgetItem>

#include "pqServer.h"
#include "vtkSMReaderFactory.h"
#include "vtkStringList.h"

//-----------------------------------------------------------------------------
class pqSelectReaderDialog::pqInternal : public Ui::SelectReaderDialog
{
public:
  pqInternal(pqSelectReaderDialog* self)
  {
    this->setupUi(self);
    self->setObjectName("pqSelectReaderDialog"); // to avoid breaking old tests.
  }

  void setHeader(const QString& header) { this->FileInfo->setText(header); }

  void updateList(vtkStringList* readers)
  {
    QListWidget* lw = this->listWidget;
    lw->clear();
    for (int cc = 0; (cc + 2) < readers->GetNumberOfStrings(); cc += 3)
    {
      QString desc = readers->GetString(cc + 2);
      // We want the first character to always be uppercase so:
      // 1 the list looks nicer
      // 2 the list case sensitive sort works "better"
      desc.replace(0, 1, desc.at(0).toUpper());
      QListWidgetItem* item = new QListWidgetItem(desc, lw);
      item->setData(Qt::UserRole, readers->GetString(cc + 0));
      item->setData(Qt::UserRole + 1, readers->GetString(cc + 1));
      if (cc == 0)
      {
        // ensure that the first item is selected.
        lw->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
      }
    }
  }
};

//-----------------------------------------------------------------------------
pqSelectReaderDialog::pqSelectReaderDialog(
  const QString& file, pqServer* s, vtkSMReaderFactory* readerFactory, QWidget* p)
  : QDialog(p)
  , Internal(new pqSelectReaderDialog::pqInternal(this))
{
  // set the helper/information string
  this->Internal->setHeader(
    QString("A reader for \"%1\" could not be found. Please choose one:").arg(file));

  // populate the list view with readers
  vtkStringList* readers =
    readerFactory->GetPossibleReaders(file.toLocal8Bit().data(), s->session());
  this->Internal->updateList(readers);
};

//-----------------------------------------------------------------------------
pqSelectReaderDialog::pqSelectReaderDialog(
  const QString& file, pqServer* vtkNotUsed(server), vtkStringList* list, QWidget* p)
  : QDialog(p)
  , Internal(new pqSelectReaderDialog::pqInternal(this))
{
  // set the helper/information string
  this->Internal->setHeader(
    QString("More than one reader for \"%1\" found. Please choose one:").arg(file));

  // populate the list view with readers
  this->Internal->updateList(list);
};

//-----------------------------------------------------------------------------
pqSelectReaderDialog::~pqSelectReaderDialog() = default;

//-----------------------------------------------------------------------------
QString pqSelectReaderDialog::getGroup() const
{
  QListWidget* lw = this->Internal->listWidget;
  QList<QListWidgetItem*> selection = lw->selectedItems();
  if (selection.empty())
  {
    return QString();
  }

  // should have only one with single selection mode
  QListWidgetItem* item = selection[0];
  return item->data(Qt::UserRole + 0).toString();
}

//-----------------------------------------------------------------------------
QString pqSelectReaderDialog::getReader() const
{
  QListWidget* lw = this->Internal->listWidget;
  QList<QListWidgetItem*> selection = lw->selectedItems();
  if (selection.empty())
  {
    return QString();
  }

  // should have only one with single selection mode
  QListWidgetItem* item = selection[0];
  return item->data(Qt::UserRole + 1).toString();
}
