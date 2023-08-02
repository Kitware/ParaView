// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

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
  bool isSetAsDefault = false;

  pqInternal(pqSelectReaderDialog* self)
  {
    this->setupUi(self);
    self->setObjectName("pqSelectReaderDialog"); // to avoid breaking old tests.
    connect(this->setAsDefault, &QPushButton::clicked, [this, self] {
      this->isSetAsDefault = true;
      self->accept();
    });

    this->okButton->setDefault(true);
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
    tr("A reader for \"%1\" could not be found. Please choose one:").arg(file));

  // populate the list view with readers
  vtkStringList* readers = readerFactory->GetPossibleReaders(file.toUtf8().data(), s->session());
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
    tr("More than one reader for \"%1\" found. Please choose one:").arg(file));

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

//-----------------------------------------------------------------------------
bool pqSelectReaderDialog::isSetAsDefault() const
{
  return this->Internal->isSetAsDefault;
}
