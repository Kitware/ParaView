/*=========================================================================

   Program:   ParaQ
   Module:    pqFileDialog.cxx

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
#include "pqFileDialogFilter.h"

#include <QDir>
#include <QTimer>
#include <QtDebug>

#include <vtkstd/set>

namespace {

  QStringList MakeFilterList(const QString &filter)
  {
    QString f(filter);
    
    if (f.isEmpty())
      {
      return QStringList();
      }
    
    QString sep(";;");
    int i = f.indexOf(sep, 0);
    if (i == -1) 
      {
      if (f.indexOf("\n", 0) != -1) 
        {
        sep = "\n";
        i = f.indexOf(sep, 0);
        }
      }
    return f.split(sep, QString::SkipEmptyParts);
  }

}

pqFileDialog::pqFileDialog(pqFileDialogModel* model, QWidget* p, 
                           const QString& title, 
                           const QString& startDirectory, 
                           const QString& nameFilter) :
  QDialog(p),
  Model(model),
  Ui(new Ui::pqFileDialog())
{
  this->setAttribute(Qt::WA_DeleteOnClose);

  this->Ui->setupUi(this);
  this->Ui->NavigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));

  this->Filter = new pqFileDialogFilter(this->Model, this);
  this->Ui->Files->setModel(this->Filter);

  this->Ui->Favorites->setModel(this->Model->favoriteModel());

  this->Ui->Files->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui->Files->setSelectionMode(QAbstractItemView::ExtendedSelection);
  
  this->Ui->Favorites->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Ui->Favorites->setSelectionMode(QAbstractItemView::ExtendedSelection);

  this->setWindowTitle(title);

  QObject::connect(this->Model->fileModel(), 
                   SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), 
                   this, 
                   SLOT(onDataChanged(const QModelIndex&, const QModelIndex&)));

  QObject::connect(this->Ui->NavigateUp, 
                   SIGNAL(clicked()), 
                   this, 
                   SLOT(onNavigateUp()));

  QObject::connect(this->Ui->Files, 
                   SIGNAL(activated(const QModelIndex&)), 
                   this, 
                   SLOT(onActivated(const QModelIndex&)));

  QObject::connect(this->Ui->Files, 
                   SIGNAL(clicked(const QModelIndex&)), 
                   this, 
                   SLOT(onClicked(const QModelIndex&)));

  QObject::connect(this->Ui->Favorites, 
                   SIGNAL(activated(const QModelIndex&)), 
                   this, 
                   SLOT(onActivated(const QModelIndex&)));

  QObject::connect(this->Ui->Favorites, 
                   SIGNAL(clicked(const QModelIndex&)), 
                   this, 
                   SLOT(onClicked(const QModelIndex&)));

  QObject::connect(this->Ui->Parents, 
                   SIGNAL(activated(const QString&)), 
                   this, 
                   SLOT(onNavigate(const QString&)));

  QObject::connect(this->Ui->FileName, 
                   SIGNAL(textEdited(const QString&)), 
                   this, 
                   SLOT(onManualEntry(const QString&)));
  
  QObject::connect(this->Ui->FileType, 
                   SIGNAL(activated(const QString&)), 
                   this, 
                   SLOT(onFilterChange(const QString&)));
  
  QStringList filterList = MakeFilterList(nameFilter);
  if(filterList.empty())
    {
    this->Ui->FileType->addItem("All Files (*)");
    }
  else
    {
    this->Ui->FileType->addItems(filterList);
    }
  this->onFilterChange(this->Ui->FileType->currentText());
  
  QString startPath = startDirectory;
  if(startPath.isNull())
    {
    startPath = this->Model->getStartPath();
    }
  this->Model->setCurrentPath(startPath);
}

pqFileDialog::~pqFileDialog()
{
  delete this->Ui;
  delete this->Model;
}

void pqFileDialog::emitFilesSelected(const QStringList& files)
{
  // Ensure that we are hidden before broadcasting the selection, so we don't get caught by screen-captures
  this->setVisible(false);
  emit filesSelected(files);
  this->done(QDialog::Accepted);
}

void pqFileDialog::accept()
{
  QStringList selected_files;
  
  // Collect files that were selected from the list ...
  QModelIndexList indexes = this->Ui->Files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indexes.size(); ++i)
    {
    QModelIndex idx = indexes[i];

    if(idx.column() != 0)
      continue;
      
    if(idx.model() == this->Filter)
      {
      idx = this->Filter->mapToSource(idx);
      }

    QStringList files = this->Model->getFilePaths(idx);
    for(int j = 0; j != files.size(); ++j)
      selected_files.push_back(files[j]);
    }

  // Collect manually entered filenames (if any) ...
  const QString manual_filename = this->Ui->FileName->text();
  if(!manual_filename.isEmpty())
    {
    const QString manual_path = this->Model->getFilePath(manual_filename);
    if(!selected_files.contains(manual_path))
      {
      selected_files.push_back(manual_path);
      }
    }
  
  // Ensure that we are hidden before broadcasting the selection, so we don't get caught by screen-captures
  this->hide();
  
  if(selected_files.size())
    emit filesSelected(selected_files);
  
  base::accept();
}

void pqFileDialog::reject()
{
  // this->setObjectName("");
  base::reject();
}

void pqFileDialog::onDataChanged(const QModelIndex&, const QModelIndex&)
{
  this->Ui->Parents->clear();
  
  const QStringList split_path = this->Model->splitPath(this->Model->getCurrentPath());
  for(int i = 0; i != split_path.size(); ++i)
    {
    QString path;
    for(int j = 0; j <= i; ++j)
      {
      if(j)
        {
        path += QDir::separator();
        }
        
      path += split_path[j];
      }
    this->Ui->Parents->addItem(path);
    }
    
  this->Ui->Parents->setCurrentIndex(split_path.size() - 1);
}

void pqFileDialog::onActivated(const QModelIndex& Index)
{
  QModelIndex i = Index;
  if(i.model() == this->Filter)
    {
    i = this->Filter->mapToSource(i);
    }

  if(this->Model->isDir(i))
    {
    this->Temp = i;
    QTimer::singleShot(0, this, SLOT(onNavigateDown()));
    }
  else
    {
    emitFilesSelected(this->Model->getFilePaths(i));
    }
}

void pqFileDialog::onClicked(const QModelIndex& Index)
{
  QModelIndex i = Index;
  if(i.model() == this->Filter)
    {
    i = this->Filter->mapToSource(i);
    }

  if(this->Model->isDir(i))
    return;

  QStringList files = this->Model->getFilePaths(i);
  if(files.empty())
    return;
  
  const QStringList components = this->Model->splitPath(files[0]);
  if(components.empty())
    return;
  
  this->Ui->FileName->setText(components.back());
}

void pqFileDialog::onManualEntry(const QString&)
{
  this->Ui->Files->clearSelection();
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
  if(!this->Model->isDir(this->Temp))
    return;
    
  const QStringList paths =
    this->Model->getFilePaths(this->Temp);
  if(1 != paths.size())
    return;
    
  this->Model->setCurrentPath(paths[0]);
}

void pqFileDialog::onFilterChange(const QString& filter)
{
  if(!this->Filter)
    {
    return;
    }

  QString f = filter;

  // if we have (...) in our filter, strip everything out but the contents of ()
  int start, end;
  start = filter.indexOf('(');
  end = filter.lastIndexOf(')');
  if(start != -1 && end != -1)
    {
    f = f.mid(start+1, end-start-1);
    }
  else if(start != -1 || end != -1)
    {
    f = QString();  // hmm...  I'm confused
    }

  // separated by spaces or semi-colons
  QStringList fs = f.split(QRegExp("[\\s+;]"));

  // set filter on proxy
  this->Filter->setFilter(fs);
  
  // update view
  this->Filter->clear();
}


QString pqFileDialog::currentFilter()
{
  return this->Ui->FileType->currentText();
}

