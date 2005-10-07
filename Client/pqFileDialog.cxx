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

#include <vtkstd/set>

pqFileDialog::pqFileDialog(pqFileDialogModel* Model, const QString& Title, QWidget* Parent, const char* const Name) :
  QDialog(Parent, Name),
  model(Model)
{
  this->ui.setupUi(this);
  this->ui.navigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
  this->ui.files->setModel(model->fileModel());
  this->ui.favorites->setModel(model->favoriteModel());

  this->ui.files->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->ui.files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->ui.favorites->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->ui.favorites->setSelectionMode(QAbstractItemView::ExtendedSelection);

  this->setWindowTitle(Title);
  this->setName(Name);

  QObject::connect(model->fileModel(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(ui.navigateUp, SIGNAL(clicked()), Model, SLOT(navigateUp()));
  QObject::connect(ui.files, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));
  QObject::connect(ui.favorites, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));

  model->setViewDirectory(model->getStartDirectory());
}

pqFileDialog::~pqFileDialog()
{
  delete model;
}

void pqFileDialog::accept()
{
  /** \todo For some reason, we get multiple copies of the same file back from selectedIndexes() */
  vtkstd::set<QString> temp;
  QModelIndexList indexes = this->ui.files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indexes.size(); ++i)
    temp.insert(model->getFilePath(indexes[i]));

  QStringList files;
  vtkstd::copy(temp.begin(), temp.end(), vtkstd::back_inserter(files));
  emit filesSelected(files);
  
  base::accept();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqFileDialog::reject()
{
  base::reject();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqFileDialog::onDataChanged(const QModelIndex&, const QModelIndex&)
{
//  ui.viewDirectory->setText(this->model()->getViewDirectory());
}

void pqFileDialog::onActivated(const QModelIndex& Index)
{
  if(model->isDir(Index))
    {
    this->temp = &Index;
    QTimer::singleShot(0, this, SLOT(onNavigateDown()));
    }
  else
    {
    QStringList files;
    files.append(this->model->getFilePath(Index));
    emit filesSelected(files);
    
    QTimer::singleShot(0, this, SLOT(onAutoDelete()));
    }
}

void pqFileDialog::onAutoDelete()
{
  delete this;
}

void pqFileDialog::onNavigateDown()
{
  this->model->navigateDown(*this->temp);
}
