/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqFileDialog.h"
#include "pqFileDialogModel.h"

#include <QTimer>

pqFileDialog::pqFileDialog(pqFileDialogModel* Model, const QString& Title, QWidget* Parent, const char* const Name) :
  QDialog(Parent, Name)
{
  this->ui.setupUi(this);
  this->ui.toolButton->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
  this->ui.treeView->setModel(Model);

  this->setWindowTitle(Title);
  this->setName(Name);

  QObject::connect(Model, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(ui.toolButton, SIGNAL(clicked()), Model, SLOT(navigateUp()));
  QObject::connect(ui.treeView, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));

  Model->setViewDirectory(Model->getStartDirectory());
}

pqFileDialog::~pqFileDialog()
{
}

/*
QStringList pqFileDialog::selectedFiles()
{
  QModelIndexList indexes = this->selections->selectedIndexes();
  QStringList files;
  foreach(QModelIndex index, indexes)
    {
    files.append(model->filePath(index));
    }
  return files;
}
*/

void pqFileDialog::accept()
{
/*
  QStringList files = this->selectedFiles();
  emit fileSelected(files.at(0));
*/
  
  base::accept();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqFileDialog::reject()
{
  base::reject();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

pqFileDialogModel* pqFileDialog::model()
{
  return reinterpret_cast<pqFileDialogModel*>(this->ui.treeView->model());
}

void pqFileDialog::onDataChanged(const QModelIndex&, const QModelIndex&)
{
  ui.viewDirectory->setText(this->model()->getViewDirectory());
}

void pqFileDialog::onActivated(const QModelIndex& Index)
{
  if(model()->isDir(Index))
    {
    this->temp = &Index;
    QTimer::singleShot(0, this, SLOT(onNavigateDown()));
    }
  else
    {
    emit fileSelected(this->model()->getFilePath(Index));
    QTimer::singleShot(0, this, SLOT(onAutoDelete()));
    }
}

void pqFileDialog::onAutoDelete()
{
  delete this;
}

void pqFileDialog::onNavigateDown()
{
  this->model()->navigateDown(*this->temp);
}
