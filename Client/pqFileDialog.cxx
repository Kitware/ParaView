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
  QDialog(Parent),
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
  this->setObjectName(Name);

  QObject::connect(this->model->fileModel(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->ui.navigateUp, SIGNAL(clicked()), this, SLOT(onNavigateUp()));
  QObject::connect(this->ui.files, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));
  QObject::connect(this->ui.favorites, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));
  QObject::connect(this->ui.parents, SIGNAL(activated(const QString&)), this, SLOT(onNavigate(const QString&)));
  QObject::connect(this->ui.fileName, SIGNAL(textChanged(const QString&)), this, SLOT(onManualEntry(const QString&)));

  this->model->setCurrentPath(this->model->getStartPath());
}

pqFileDialog::~pqFileDialog()
{
  delete this->model;
}

void pqFileDialog::accept()
{
  QStringList selected_files;
  
  // Collect files that were selected from the list ...
  QModelIndexList indexes = this->ui.files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indexes.size(); ++i)
    {
    if(indexes[i].column() != 0)
      continue;
      
    QStringList files = this->model->getFilePaths(indexes[i]);
    for(int j = 0; j != files.size(); ++j)
      selected_files.push_back(files[j]);
    }

  // Collect manually entered filenames (if any) ...
  const QString manual_filename = this->ui.fileName->text();
  if(!manual_filename.isEmpty())
    selected_files.push_back(this->model->getFilePath(manual_filename));
  
  if(selected_files.size())
    emit filesSelected(selected_files);
  
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
  const QStringList split_path = this->model->splitPath(this->model->getCurrentPath());
  ui.parents->clear();
  for(int i = 0; i != split_path.size(); ++i)
    ui.parents->addItem(split_path[i]);
  ui.parents->setCurrentIndex(split_path.size() - 1);
}

void pqFileDialog::onActivated(const QModelIndex& Index)
{
  if(this->model->isDir(Index))
    {
    this->temp = &Index;
    QTimer::singleShot(0, this, SLOT(onNavigateDown()));
    }
  else
    {
    emit filesSelected(this->model->getFilePaths(Index));
    QTimer::singleShot(0, this, SLOT(onAutoDelete()));
    }
}

void pqFileDialog::onManualEntry(const QString&)
{
  this->ui.files->clearSelection();
}

void pqFileDialog::onNavigate(const QString& Path)
{
  this->model->setCurrentPath(Path);
}

void pqFileDialog::onNavigateUp()
{
  this->model->setCurrentPath(this->model->getParentPath(this->model->getCurrentPath()));
}

void pqFileDialog::onNavigateDown()
{
  if(!this->model->isDir(*this->temp))
    return;
    
  const QStringList paths = this->model->getFilePaths(*this->temp);
  if(1 != paths.size())
    return;
    
  this->model->setCurrentPath(paths[0]);
}

void pqFileDialog::onAutoDelete()
{
  delete this;
}

