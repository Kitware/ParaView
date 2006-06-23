/*=========================================================================

   Program: ParaView
   Module:    pqFileDialog.cxx

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

#include "pqFileDialog.h"
#include "pqFileDialogModel.h"
#include "ui_pqFileDialog.h"
#include "pqFileDialogFilter.h"

#include <QDir>
#include <QMessageBox>
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


  QStringList GetWildCardsFromFilter(const QString& filter)
    {
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
    return fs;
    }
}

pqFileDialog::pqFileDialog(pqFileDialogModel* model, QWidget* p, 
                           const QString& title, 
                           const QString& startDirectory, 
                           const QString& nameFilter) :
  QDialog(p),
  Model(model),
  Ui(new Ui::pqFileDialog()),
  Mode(ExistingFile)
{
  this->setAttribute(Qt::WA_DeleteOnClose);

  this->Ui->setupUi(this);
  this->Ui->NavigateBack->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogBack));
  this->Ui->NavigateBack->setDisabled( true );
  this->Ui->NavigateForward->setIcon( 
      QIcon(":/pqWidgets/pqNavigateForward16.png"));
  this->Ui->NavigateForward->setDisabled( true );
  this->Ui->NavigateUp->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogToParent));
  this->Ui->CreateFolder->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogNewFolder));
  this->Ui->CreateFolder->setDisabled( true );

  this->Filter = new pqFileDialogFilter(this->Model, this);
  this->Ui->Files->setModel(this->Filter);

  this->Ui->Favorites->setModel(this->Model->favoriteModel());

  this->Ui->Files->setSelectionBehavior(QAbstractItemView::SelectRows);
  
  this->Ui->Favorites->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->setFileMode(ExistingFile);

  this->setWindowTitle(title);

  // TODO:  This connection seems weird.
  //        What we're really after is whether the view's rootIndex/currentIndex changed.
  //        The model's dataChanged signal isn't an equivalent of that.
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
  
pqFileDialog::FileMode pqFileDialog::fileMode()
{
  return this->Mode;
}

void pqFileDialog::setFileMode(pqFileDialog::FileMode mode)
{
  this->Mode = mode;
  switch(this->Mode)
    {
    case AnyFile:
    case ExistingFile:
    case Directory:
        {
        this->Ui->Files->setSelectionMode(
             QAbstractItemView::SingleSelection);
        this->Ui->Favorites->setSelectionMode(
             QAbstractItemView::SingleSelection);
        }
      break;
    case ExistingFiles:
        {
        this->Ui->Files->setSelectionMode(
             QAbstractItemView::ExtendedSelection);
        this->Ui->Favorites->setSelectionMode(
             QAbstractItemView::ExtendedSelection);
        }
    }
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

  // gotta have something
  if(selected_files.empty())
    return;
  
  
  switch(this->Mode)
    {
    case Directory:
      {
      QString fn = selected_files.first();
      if(this->Model->dirExists(fn))
        {
        this->hide();
        emit this->filesSelected(selected_files);
        base::accept();
        }
      // not found, issue error and return
      QString message = tr("\nDirectory not found.\nPlease verify the "
                           "correct file name was given");
      QMessageBox::warning(this, this->windowTitle(), fn + message);
      return;
      }
    break;
    case ExistingFile:
    case ExistingFiles:
      {
      int numFiles = selected_files.size();
      for(int j=0; j<numFiles; j++)
        {
        QString fn = selected_files.at(j);
        if(this->Model->dirExists(fn))
          {
          onNavigate(fn);
          this->Ui->FileName->selectAll();
          return;
          }
        else if(!this->Model->fileExists(fn))
          {
          QString message = tr("\nFile not found.\nPlease verify the "
                               "correct file name was given.");
          QMessageBox::warning(this, this->windowTitle(), fn + message);
          return;
          }
        }
      this->hide();
      emit this->filesSelected(selected_files);
      base::accept();
      }
    break;
    case AnyFile:
      {
      QString fn = selected_files.first();
      if(this->Model->dirExists(fn))
        {
        //navigate to that dir
        this->onNavigate(fn);
        this->Ui->FileName->selectAll();
        return;
        }
      else
        {
        // add missing extension if necessary
        QFileInfo fi(fn);
        QString ext = fi.completeSuffix();
        QString extensionWildcard = GetWildCardsFromFilter(
             this->Ui->FileType->currentText()).first();
        QString wantedExtension =
          extensionWildcard.mid(extensionWildcard.indexOf('.')+1);
        if(ext.isEmpty() && !wantedExtension.isEmpty())
          {
          if(fn.at(fn.size() - 1) != '.')
            {
            fn += ".";
            }
          fn += wantedExtension;
          }

        if(this->Model->fileExists(fn))
          {
          // found, ask to overwrite
          QString message = tr(" already exists.\nDo you want to replace it?");
          if(QMessageBox::warning(this, this->windowTitle(), fn + message,
                                  QMessageBox::Yes, QMessageBox::No)
             == QMessageBox::No)
            {
            return;
            }
          }
        this->hide();
        QStringList files;
        files.append(fn);
        emit this->filesSelected(files);
        base::accept();
        }
      }
    break;
    }
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

void pqFileDialog::onActivated(const QModelIndex&)
{
  this->accept();
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

void pqFileDialog::onNavigateDown(const QModelIndex& idx)
{
  if(!this->Model->isDir(idx))
    return;
    
  const QStringList paths = this->Model->getFilePaths(idx);

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

  QStringList fs = GetWildCardsFromFilter(filter);

  // set filter on proxy
  this->Filter->setFilter(fs);
  
  // update view
  this->Filter->clear();
}


QString pqFileDialog::currentFilter()
{
  return this->Ui->FileType->currentText();
}

