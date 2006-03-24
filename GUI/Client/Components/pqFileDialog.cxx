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

#include "pqFileDialog.h"
#include "pqFileDialogModel.h"
#include "ui_pqFileDialog.h"

#include <QTimer>

#include <vtkstd/set>

pqFileDialog::pqFileDialog(pqFileDialogModel* model, const QString& title, QWidget* p, const char* const name) :
  QDialog(p),
  Model(model),
  Ui(new Ui::pqFileDialog())
{
  this->Ui->setupUi(this);
  this->Ui->navigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
  this->Ui->files->setModel(this->Model->fileModel());
  this->Ui->favorites->setModel(this->Model->favoriteModel());

  this->Ui->files->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui->files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->Ui->favorites->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui->favorites->setSelectionMode(QAbstractItemView::ExtendedSelection);

  this->setWindowTitle(title);
  this->setObjectName(name);

  QObject::connect(this->Model->fileModel(), SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));
  QObject::connect(this->Ui->navigateUp, SIGNAL(clicked()), this, SLOT(onNavigateUp()));
  QObject::connect(this->Ui->files, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));
  QObject::connect(this->Ui->favorites, SIGNAL(activated(const QModelIndex&)), this, SLOT(onActivated(const QModelIndex&)));
  QObject::connect(this->Ui->parents, SIGNAL(activated(const QString&)), this, SLOT(onNavigate(const QString&)));
  QObject::connect(this->Ui->fileName, SIGNAL(textChanged(const QString&)), this, SLOT(onManualEntry(const QString&)));

  this->Model->setCurrentPath(this->Model->getStartPath());
}

pqFileDialog::~pqFileDialog()
{
  delete this->Ui;
  delete this->Model;
}

void pqFileDialog::accept()
{
  QStringList selected_files;
  
  // Collect files that were selected from the list ...
  QModelIndexList indexes = this->Ui->files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indexes.size(); ++i)
    {
    if(indexes[i].column() != 0)
      continue;
      
    QStringList files = this->Model->getFilePaths(indexes[i]);
    for(int j = 0; j != files.size(); ++j)
      selected_files.push_back(files[j]);
    }

  // Collect manually entered filenames (if any) ...
  const QString manual_filename = this->Ui->fileName->text();
  if(!manual_filename.isEmpty())
    selected_files.push_back(this->Model->getFilePath(manual_filename));
  
  // Ensure that we are hidden before broadcasting the selection, so we don't get caught by screen-captures
  this->hide();
  
  if(selected_files.size())
    emit filesSelected(selected_files);
  
  base::accept();
}

void pqFileDialog::onDataChanged(const QModelIndex&, const QModelIndex&)
{
  const QStringList split_path = this->Model->splitPath(this->Model->getCurrentPath());
  this->Ui->parents->clear();
  for(int i = 0; i != split_path.size(); ++i)
    this->Ui->parents->addItem(split_path[i]);
  this->Ui->parents->setCurrentIndex(split_path.size() - 1);
}

void pqFileDialog::onActivated(const QModelIndex& Index)
{
  if(this->Model->isDir(Index))
    {
    this->Temp = &Index;
    QTimer::singleShot(0, this, SLOT(onNavigateDown()));
    }
  else
    {
    // Ensure that we are hidden before broadcasting the selection, so we don't get caught by screen-captures
    this->hide();
  
    emit filesSelected(this->Model->getFilePaths(Index));
    base::accept();
    }
}

void pqFileDialog::onManualEntry(const QString&)
{
  this->Ui->files->clearSelection();
}

void pqFileDialog::onNavigate(const QString& Path)
{
  this->Model->setCurrentPath(Path);
}

void pqFileDialog::onNavigateUp()
{
  this->Model->setCurrentPath(this->Model->getParentPath(this->Model->getCurrentPath()));
}

void pqFileDialog::onNavigateDown()
{
  if(!this->Model->isDir(*this->Temp))
    return;
    
  const QStringList paths = this->Model->getFilePaths(*this->Temp);
  if(1 != paths.size())
    return;
    
  this->Model->setCurrentPath(paths[0]);
}

