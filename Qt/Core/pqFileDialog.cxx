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
#include "pqFileDialogFavoriteModel.h"
#include "pqFileDialogFilter.h"

#include "ui_pqFileDialog.h"

#include <QDir>
#include <QMessageBox>
#include <QtDebug>

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

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog::pqImplementation

class pqFileDialog::pqImplementation
{
public:
  pqFileDialogModel* const Model;
  pqFileDialogFavoriteModel* const FavoriteModel;
  pqFileDialogFilter FileFilter;
  FileMode Mode;
  Ui::pqFileDialog Ui;
  QStringList SelectedFiles;


  pqImplementation(pqServer* server) :
    Model(new pqFileDialogModel(server, NULL)),
    FavoriteModel(new pqFileDialogFavoriteModel(server, NULL)),
    FileFilter(this->Model),
    Mode(ExistingFile)
  {
  }
  
  ~pqImplementation()
  {
    delete this->Model;
    delete this->FavoriteModel;
  }

};

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog

pqFileDialog::pqFileDialog(
    pqServer* server,
    QWidget* p, 
    const QString& title, 
    const QString& startDirectory, 
    const QString& nameFilter) :
  Superclass(p),
  Implementation(new pqImplementation(server))
{
  this->Implementation->Ui.setupUi(this);

  this->setWindowTitle(title);
  
  this->Implementation->Ui.NavigateBack->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogBack));
  this->Implementation->Ui.NavigateBack->setDisabled( true );
  this->Implementation->Ui.NavigateForward->setIcon( 
      QIcon(":/pqWidgets/pqNavigateForward16.png"));
  this->Implementation->Ui.NavigateForward->setDisabled( true );
  this->Implementation->Ui.NavigateUp->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogToParent));
  this->Implementation->Ui.CreateFolder->setIcon(style()->
      standardPixmap(QStyle::SP_FileDialogNewFolder));
  this->Implementation->Ui.CreateFolder->setDisabled( true );

  this->Implementation->Ui.Files->setModel(&this->Implementation->FileFilter);
  this->Implementation->Ui.Files->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->Implementation->Ui.Favorites->setModel(this->Implementation->FavoriteModel);
  this->Implementation->Ui.Favorites->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->setFileMode(ExistingFile);

  QObject::connect(this->Implementation->Model,
                   SIGNAL(modelReset()), 
                   this, 
                   SLOT(onModelReset()));

  QObject::connect(this->Implementation->Ui.NavigateUp, 
                   SIGNAL(clicked()), 
                   this, 
                   SLOT(onNavigateUp()));

  QObject::connect(this->Implementation->Ui.Parents, 
                   SIGNAL(activated(const QString&)), 
                   this, 
                   SLOT(onNavigate(const QString&)));

  QObject::connect(this->Implementation->Ui.FileType, 
                   SIGNAL(activated(const QString&)), 
                   this, 
                   SLOT(onFilterChange(const QString&)));
  
  QObject::connect(this->Implementation->Ui.Favorites, 
                   SIGNAL(clicked(const QModelIndex&)), 
                   this, 
                   SLOT(onClickedFavorite(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files, 
                   SIGNAL(clicked(const QModelIndex&)), 
                   this, 
                   SLOT(onClickedFile(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files->selectionModel(),
                  SIGNAL(
                    selectionChanged(const QItemSelection&, const QItemSelection&)),
                  this, 
                  SLOT(fileSelectionChanged()));

  QObject::connect(this->Implementation->Ui.Favorites, 
                   SIGNAL(activated(const QModelIndex&)), 
                   this, 
                   SLOT(onActivateFavorite(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files, 
                   SIGNAL(activated(const QModelIndex&)), 
                   this, 
                   SLOT(onActivateFile(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.FileName, 
                   SIGNAL(textEdited(const QString&)), 
                   this, 
                   SLOT(onTextEdited(const QString&)));

  QStringList filterList = MakeFilterList(nameFilter);
  if(filterList.empty())
    {
    this->Implementation->Ui.FileType->addItem("All Files (*)");
    }
  else
    {
    this->Implementation->Ui.FileType->addItems(filterList);
    }
  this->onFilterChange(this->Implementation->Ui.FileType->currentText());
  
  QString startPath = startDirectory;
  if(startPath.isEmpty())
    {
    startPath = this->Implementation->Model->getStartPath();
    }
  this->Implementation->Model->setCurrentPath(startPath);
}

//-----------------------------------------------------------------------------
pqFileDialog::~pqFileDialog()
{
  delete this->Implementation;
}

//-----------------------------------------------------------------------------
void pqFileDialog::setFileMode(pqFileDialog::FileMode mode)
{
  this->Implementation->Mode = mode;
  
  switch(this->Implementation->Mode)
    {
    case AnyFile:
    case ExistingFile:
    case Directory:
        {
        this->Implementation->Ui.Files->setSelectionMode(
             QAbstractItemView::SingleSelection);
        this->Implementation->Ui.Favorites->setSelectionMode(
             QAbstractItemView::SingleSelection);
        }
      break;
    case ExistingFiles:
        {
        this->Implementation->Ui.Files->setSelectionMode(
             QAbstractItemView::ExtendedSelection);
        this->Implementation->Ui.Favorites->setSelectionMode(
             QAbstractItemView::ExtendedSelection);
        }
    }
}

//-----------------------------------------------------------------------------
void pqFileDialog::emitFileSelected(const QString& file)
{
  QStringList files;
  files << file;
  this->emitFilesSelected(files);
}

//-----------------------------------------------------------------------------
void pqFileDialog::emitFilesSelected(const QStringList& files)
{
  // Ensure that we are hidden before broadcasting the selection,
  // so we don't get caught by screen-captures
  this->setVisible(false);
  this->Implementation->SelectedFiles = files;
  emit filesSelected(this->Implementation->SelectedFiles);
  this->done(QDialog::Accepted);
}
  
//-----------------------------------------------------------------------------
QStringList pqFileDialog::getSelectedFiles()
{
  return this->Implementation->SelectedFiles;
}

//-----------------------------------------------------------------------------
void pqFileDialog::accept()
{
  QStringList activeSet = this->Implementation->Ui.FileName->text().split(' ');
  QStringList fullPaths;
  
  foreach (QString name, activeSet)
    {
    fullPaths.append(this->Implementation->Model->absoluteFilePath(name));
    }
  this->acceptInternal(fullPaths);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onModelReset()
{
  this->Implementation->Ui.Parents->clear();
  
  QString currentPath = this->Implementation->Model->getCurrentPath();
  QChar separator = this->Implementation->Model->separator();
  QStringList parents = currentPath.split(separator);

  for(int i = 0; i != parents.size(); ++i)
    {
    QString str;
    for(int j=0; j<=i; j++)
      {
      str += parents[j] + separator;
      }
    this->Implementation->Ui.Parents->addItem(str);
    }
    
  this->Implementation->Ui.Parents->setCurrentIndex(parents.size() - 1);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigate(const QString& Path)
{
  this->Implementation->Model->setCurrentPath(Path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateUp()
{
  this->Implementation->Model->setParentPath();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateDown(const QModelIndex& idx)
{
  if(!this->Implementation->Model->isDir(idx))
    return;
    
  const QStringList paths = this->Implementation->Model->getFilePaths(idx);

  if(1 != paths.size())
    return;
    
  this->Implementation->Model->setCurrentPath(paths[0]);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onFilterChange(const QString& filter)
{
  QStringList fs = GetWildCardsFromFilter(filter);

  // set filter on proxy
  this->Implementation->FileFilter.setFilter(fs);
  
  // update view
  this->Implementation->FileFilter.clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFavorite(const QModelIndex&)
{
  this->Implementation->Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFile(const QModelIndex&)
{
  this->Implementation->Ui.Favorites->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateFavorite(const QModelIndex& index)
{
  if(this->Implementation->FavoriteModel->isDir(index))
    {
    QString file = this->Implementation->FavoriteModel->filePath(index);
    this->onNavigate(file);
    this->Implementation->Ui.FileName->selectAll();
    }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateFile(const QModelIndex& index)
{
  QModelIndex actual_index = index;
  if(actual_index.model() == &this->Implementation->FileFilter)
    actual_index = this->Implementation->FileFilter.mapToSource(actual_index);
    
  QStringList selected_files;
  selected_files << this->Implementation->Model->getFilePaths(actual_index);
  if(selected_files.empty())
    return;
    
  QString file = selected_files.first();
  if(this->Implementation->Model->dirExists(file))
    {
    this->onNavigate(file);
    this->Implementation->Ui.FileName->selectAll();
    }
  else if(this->Implementation->Model->fileExists(file))
    {
    if(this->Implementation->Mode == AnyFile)
      {
      if(QMessageBox::No == QMessageBox::warning(
        this,
        this->windowTitle(),
        QString(tr("%1 already exists.\nDo you want to replace it?")).arg(file),
        QMessageBox::Yes,
        QMessageBox::No))
        {
        return;
        }
      }
      
    this->emitFilesSelected(selected_files);
    }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onTextEdited(const QString&)
{
  this->Implementation->Ui.Favorites->clearSelection();
  this->Implementation->Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
QString pqFileDialog::fixFileExtension(
  const QString& filename, const QString& filter)
{
  // Add missing extension if necessary
  QFileInfo fileInfo(filename);
  QString ext = fileInfo.completeSuffix();
  QString extensionWildcard = ::GetWildCardsFromFilter(filter).first();
  QString wantedExtension =
    extensionWildcard.mid(extensionWildcard.indexOf('.')+1);

  QString fixedFilename = filename;
  if(ext.isEmpty() && !wantedExtension.isEmpty() &&
    wantedExtension != "*")
    {
    if(fixedFilename.at(fixedFilename.size() - 1) != '.')
      {
      fixedFilename += ".";
      }
    fixedFilename += wantedExtension;
    }
  return fixedFilename;
}

//-----------------------------------------------------------------------------
void pqFileDialog::acceptInternal(QStringList& selected_files)
{
  if(selected_files.empty())
    {
    return;
    }
    
  QString file = selected_files[0];
  // User chose an existing directory
  if(this->Implementation->Model->dirExists(file))
    {
    switch(this->Implementation->Mode)
      {
      case Directory:
        this->emitFileSelected(file);
        break;

      case ExistingFile:
      case ExistingFiles:
      case AnyFile:
        this->onNavigate(file);
        this->Implementation->Ui.FileName->clear();
        break;
      }
    return;
    }

  // User choose and existing file or a brand new filename.
  if (this->Implementation->Mode == pqFileDialog::AnyFile)
    {
    // If mode is a "save" dialog, we fix the extension first.
    file = this->fixFileExtension(file,
      this->Implementation->Ui.FileType->currentText());

    // It is very possible that after fixing the extension,
    // the new filename is an already present directory,
    // hence we handle that case:
    if (this->Implementation->Model->dirExists(file))
      {
      this->onNavigate(file);
      this->Implementation->Ui.FileName->clear();
      return;
      }
    }

  // User chose an existing file-or-files
  if (this->Implementation->Model->fileExists(file))
    {
    switch(this->Implementation->Mode)
      {
      case Directory:
        // User chose a file in directory mode, do nothing
        this->Implementation->Ui.FileName->clear();
        return;
      case ExistingFile:
      case ExistingFiles:
        // TODO: we need to verify that all selected files are indeed
        // "existing".
        // User chose an existing file-or-files, we're done
        this->emitFilesSelected(selected_files);
        return;
      case AnyFile:
        // User chose an existing file, prompt before overwrite
        if(QMessageBox::No == QMessageBox::warning(
          this,
          this->windowTitle(),
          QString(tr("%1 already exists.\nDo you want to replace it?")).arg(file),
          QMessageBox::Yes,
          QMessageBox::No))
          {
          return;
          }
        this->emitFileSelected(file);
        return;
      }
    }
  else // User choose non-existent file.
    {
    switch (this->Implementation->Mode)
      {
    case Directory:
    case ExistingFile:
    case ExistingFiles:
      this->Implementation->Ui.FileName->selectAll();
      return;

    case AnyFile:
      this->emitFileSelected(file);
      return;
      }
    }
}

//-----------------------------------------------------------------------------
void pqFileDialog::fileSelectionChanged()
{
  // Selection changed, update the FileName entry box
  // to reflect the current selection.
  QString fileString;

  const QModelIndexList indices =
    this->Implementation->Ui.Files->selectionModel()->selectedIndexes();
  for(int i = 0; i != indices.size(); ++i)
    {
    QModelIndex index = indices[i];
    if(index.column() != 0)
      {
      continue;
      }
      
    if(index.model() == &this->Implementation->FileFilter)
      {
      fileString += this->Implementation->FileFilter.data(index).toString() +
                    " ";
      }
    }


  this->Implementation->Ui.FileName->setText(fileString);
}

