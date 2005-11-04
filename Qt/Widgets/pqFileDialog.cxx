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

pqFileDialog::pqFileDialog(pqFileDialogModel* model, const QString& title, QWidget* parent, const char* const name) :
  QDialog(parent),
  Model(model)
{
  this->Ui.setupUi(this);
  this->Ui.navigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
  this->Ui.files->setModel(this->Model->FileModel());
  this->Ui.favorites->setModel(this->Model->FavoriteModel());

  this->Ui.files->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui.files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->Ui.favorites->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui.favorites->setSelectionMode(QAbstractItemView::ExtendedSelection);

  this->setWindowTitle(title);
  this->setObjectName(name);

  QObject::connect(this->Model->FileModel(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(OnDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Ui.navigateUp, SIGNAL(clicked()), this, SLOT(OnNavigateUp()));
  QObject::connect(this->Ui.files, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnActivated(const QModelIndex&)));
  QObject::connect(this->Ui.favorites, SIGNAL(activated(const QModelIndex&)), this, SLOT(OnActivated(const QModelIndex&)));
  QObject::connect(this->Ui.parents, SIGNAL(activated(const QString&)), this, SLOT(OnNavigate(const QString&)));
  QObject::connect(this->Ui.fileName, SIGNAL(textChanged(const QString&)), this, SLOT(OnManualEntry(const QString&)));

  this->Model->SetCurrentPath(this->Model->GetStartPath());
}

pqFileDialog::~pqFileDialog()
{
  delete this->Model;
}

void pqFileDialog::accept()
{
  QStringList selected_files;
  
  // Collect files that were selected from the list ...
  QModelIndexList indexes = this->Ui.files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indexes.size(); ++i)
    {
    if(indexes[i].column() != 0)
      continue;
      
    QStringList files = this->Model->GetFilePaths(indexes[i]);
    for(int j = 0; j != files.size(); ++j)
      selected_files.push_back(files[j]);
    }

  // Collect manually entered filenames (if any) ...
  const QString manual_filename = this->Ui.fileName->text();
  if(!manual_filename.isEmpty())
    selected_files.push_back(this->Model->GetFilePath(manual_filename));
  
  if(selected_files.size())
    emit FilesSelected(selected_files);
  
  base::accept();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqFileDialog::reject()
{
  base::reject();
  QTimer::singleShot(0, this, SLOT(onAutoDelete()));
}

void pqFileDialog::OnDataChanged(const QModelIndex&, const QModelIndex&)
{
  const QStringList split_path = this->Model->SplitPath(this->Model->GetCurrentPath());
  this->Ui.parents->clear();
  for(int i = 0; i != split_path.size(); ++i)
    this->Ui.parents->addItem(split_path[i]);
  this->Ui.parents->setCurrentIndex(split_path.size() - 1);
}

void pqFileDialog::OnActivated(const QModelIndex& Index)
{
  if(this->Model->IsDir(Index))
    {
    this->Temp = &Index;
    QTimer::singleShot(0, this, SLOT(OnNavigateDown()));
    }
  else
    {
    emit FilesSelected(this->Model->GetFilePaths(Index));
    QTimer::singleShot(0, this, SLOT(OnAutoDelete()));
    }
}

void pqFileDialog::OnManualEntry(const QString&)
{
  this->Ui.files->clearSelection();
}

void pqFileDialog::OnNavigate(const QString& Path)
{
  this->Model->SetCurrentPath(Path);
}

void pqFileDialog::OnNavigateUp()
{
  this->Model->SetCurrentPath(this->Model->GetParentPath(this->Model->GetCurrentPath()));
}

void pqFileDialog::OnNavigateDown()
{
  if(!this->Model->IsDir(*this->Temp))
    return;
    
  const QStringList paths = this->Model->GetFilePaths(*this->Temp);
  if(1 != paths.size())
    return;
    
  this->Model->SetCurrentPath(paths[0]);
}

void pqFileDialog::OnAutoDelete()
{
  delete this;
}

