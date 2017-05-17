/*=========================================================================

   Program: ParaView
   Module:    pqFileDialog.cxx

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

#include "pqFileDialog.h"
#include "pqApplicationCore.h"
#include "pqFileDialogFavoriteModel.h"
#include "pqFileDialogFilter.h"
#include "pqFileDialogModel.h"
#include "pqFileDialogRecentDirsModel.h"
#include "pqServer.h"
#include "pqSettings.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QCompleter>
#include <QDir>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QtDebug>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QShowEvent>

#include <string>
#include <vtksys/SystemTools.hxx>

class pqFileComboBox : public QComboBox
{
public:
  pqFileComboBox(QWidget* p)
    : QComboBox(p)
  {
  }
  void showPopup()
  {
    QWidget* container = this->view()->parentWidget();
    container->setMaximumWidth(this->width());
    QComboBox::showPopup();
  }
};
#include "ui_pqFileDialog.h"

namespace
{

QStringList MakeFilterList(const QString& filter)
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
  if (start != -1 && end != -1)
  {
    f = f.mid(start + 1, end - start - 1);
  }
  else if (start != -1 || end != -1)
  {
    f = QString(); // hmm...  I'm confused
  }

  // separated by spaces or semi-colons
  QStringList fs = f.split(QRegExp("[\\s+;]"), QString::SkipEmptyParts);

  // add a *.ext.* for every *.ext we get to support file groups
  QStringList ret = fs;
  foreach (QString ext, fs)
  {
    ret.append(ext + ".*");
  }
  return ret;
}
}

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog::pqImplementation

class pqFileDialog::pqImplementation : public QObject
{
public:
  pqFileDialogModel* const Model;
  pqFileDialogFavoriteModel* const FavoriteModel;
  pqFileDialogRecentDirsModel* const RecentModel;
  pqFileDialogFilter FileFilter;
  QStringList FileNames; // list of file names in the FileName ui text edit
  QCompleter* Completer;
  pqFileDialog::FileMode Mode;
  Ui::pqFileDialog Ui;
  QList<QStringList> SelectedFiles;
  QStringList Filters;
  bool SuppressOverwriteWarning;
  bool ShowMultipleFileHelp;
  QString FileNamesSeperator;

  // remember the last locations we browsed
  static QMap<QPointer<pqServer>, QString> ServerFilePaths;
  static QString LocalFilePath;

  pqImplementation(pqFileDialog* p, pqServer* server)
    : QObject(p)
    , Model(new pqFileDialogModel(server, NULL))
    , FavoriteModel(new pqFileDialogFavoriteModel(server, NULL))
    , RecentModel(new pqFileDialogRecentDirsModel(Model, server, NULL))
    , FileFilter(this->Model)
    , Completer(new QCompleter(&this->FileFilter, NULL))
    , Mode(ExistingFile)
    , SuppressOverwriteWarning(false)
    , ShowMultipleFileHelp(false)
    , FileNamesSeperator(";")
  {
    QObject::connect(p, SIGNAL(filesSelected(const QList<QStringList>&)), this->RecentModel,
      SLOT(setChosenFiles(const QList<QStringList>&)));
  }

  ~pqImplementation()
  {
    delete this->RecentModel;
    delete this->FavoriteModel;
    delete this->Model;
    delete this->Completer;
  }

  bool eventFilter(QObject* obj, QEvent* anEvent)
  {
    if (obj == this->Ui.Files)
    {
      if (anEvent->type() == QEvent::KeyPress)
      {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(anEvent);
        if (keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete)
        {
          this->Ui.FileName->setFocus(Qt::OtherFocusReason);
          // send out a backspace event to the file name now
          QKeyEvent replicateDelete(keyEvent->type(), keyEvent->key(), keyEvent->modifiers());
          QApplication::sendEvent(this->Ui.FileName, &replicateDelete);
          return true;
        }
      }
      return false;
    }
    return QObject::eventFilter(obj, anEvent);
  }

  QString getStartPath()
  {
    pqServer* s = this->Model->server();
    if (s)
    {
      QMap<QPointer<pqServer>, QString>::iterator iter;
      iter = this->ServerFilePaths.find(this->Model->server());
      if (iter != this->ServerFilePaths.end())
      {
        return *iter;
      }
    }
    else if (!this->LocalFilePath.isEmpty())
    {
      return this->LocalFilePath;
    }
    return this->Model->getCurrentPath();
  }

  void setCurrentPath(const QString& p)
  {
    this->Model->setCurrentPath(p);
    pqServer* s = this->Model->server();
    if (s)
    {
      this->ServerFilePaths[s] = p;
    }
    else
    {
      this->LocalFilePath = p;
    }
    this->Ui.Favorites->clearSelection();
    this->Ui.Recent->clearSelection();
    this->Ui.Files->setFocus(Qt::OtherFocusReason);
  }

  void addHistory(const QString& p)
  {
    this->BackHistory.append(p);
    this->ForwardHistory.clear();
    if (this->BackHistory.size() > 1)
    {
      this->Ui.NavigateBack->setEnabled(true);
    }
    else
    {
      this->Ui.NavigateBack->setEnabled(false);
    }
    this->Ui.NavigateForward->setEnabled(false);
  }
  QString backHistory()
  {
    QString path = this->BackHistory.takeLast();
    this->ForwardHistory.append(this->Model->getCurrentPath());
    this->Ui.NavigateForward->setEnabled(true);
    if (this->BackHistory.size() == 1)
    {
      this->Ui.NavigateBack->setEnabled(false);
    }
    return path;
  }
  QString forwardHistory()
  {
    QString path = this->ForwardHistory.takeLast();
    this->BackHistory.append(this->Model->getCurrentPath());
    this->Ui.NavigateBack->setEnabled(true);
    if (this->ForwardHistory.size() == 0)
    {
      this->Ui.NavigateForward->setEnabled(false);
    }
    return path;
  }

protected:
  QStringList BackHistory;
  QStringList ForwardHistory;
};

QMap<QPointer<pqServer>, QString> pqFileDialog::pqImplementation::ServerFilePaths;
QString pqFileDialog::pqImplementation::LocalFilePath;

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog

pqFileDialog::pqFileDialog(pqServer* server, QWidget* p, const QString& title,
  const QString& startDirectory, const QString& nameFilter)
  : Superclass(p)
  , Implementation(new pqImplementation(this, server))
{
  this->Implementation->Ui.setupUi(this);
  // ensures that the favorites and the browser component are sized
  // proportionately.
  this->Implementation->Ui.mainSplitter->setStretchFactor(0, 1);
  this->Implementation->Ui.mainSplitter->setStretchFactor(1, 4);
  this->setWindowTitle(title);

  this->Implementation->Ui.Files->setEditTriggers(QAbstractItemView::EditKeyPressed);

  // install the event filter
  this->Implementation->Ui.Files->installEventFilter(this->Implementation);

  // install the autocompleter
  this->Implementation->Ui.FileName->setCompleter(this->Implementation->Completer);

  QPixmap back = style()->standardPixmap(QStyle::SP_FileDialogBack);
  this->Implementation->Ui.NavigateBack->setIcon(back);
  this->Implementation->Ui.NavigateBack->setEnabled(false);
  QObject::connect(
    this->Implementation->Ui.NavigateBack, SIGNAL(clicked(bool)), this, SLOT(onNavigateBack()));
  // just flip the back image to make a forward image
  QPixmap forward = QPixmap::fromImage(back.toImage().mirrored(true, false));
  this->Implementation->Ui.NavigateForward->setIcon(forward);
  this->Implementation->Ui.NavigateForward->setDisabled(true);
  QObject::connect(this->Implementation->Ui.NavigateForward, SIGNAL(clicked(bool)), this,
    SLOT(onNavigateForward()));
  this->Implementation->Ui.NavigateUp->setIcon(
    style()->standardPixmap(QStyle::SP_FileDialogToParent));
  this->Implementation->Ui.CreateFolder->setIcon(
    style()->standardPixmap(QStyle::SP_FileDialogNewFolder));
  this->Implementation->Ui.CreateFolder->setDisabled(true);

  this->Implementation->Ui.ShowDetail->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced26.png"));

  this->Implementation->Ui.Files->setModel(&this->Implementation->FileFilter);
  this->Implementation->Ui.Files->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->Implementation->Ui.Files->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(this->Implementation->Ui.Files,
    SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(onContextMenuRequested(const QPoint&)));
  this->Implementation->Ui.CreateFolder->setEnabled(true);

  this->Implementation->Ui.Favorites->setModel(this->Implementation->FavoriteModel);
  this->Implementation->Ui.Favorites->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->Implementation->Ui.Recent->setModel(this->Implementation->RecentModel);
  this->Implementation->Ui.Recent->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->setFileMode(ExistingFile);

  QObject::connect(this->Implementation->Model, SIGNAL(modelReset()), this, SLOT(onModelReset()));

  QObject::connect(
    this->Implementation->Ui.NavigateUp, SIGNAL(clicked()), this, SLOT(onNavigateUp()));

  QObject::connect(
    this->Implementation->Ui.CreateFolder, SIGNAL(clicked()), this, SLOT(onCreateNewFolder()));

  QObject::connect(this->Implementation->Ui.Parents, SIGNAL(activated(const QString&)), this,
    SLOT(onNavigate(const QString&)));

  QObject::connect(this->Implementation->Ui.FileType, SIGNAL(currentIndexChanged(const QString&)),
    this, SLOT(onFilterChange(const QString&)));

  QObject::connect(this->Implementation->Ui.Favorites, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedFavorite(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Recent, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedRecent(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedFile(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(fileSelectionChanged()));

  QObject::connect(this->Implementation->Ui.Favorites, SIGNAL(activated(const QModelIndex&)), this,
    SLOT(onActivateFavorite(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Recent, SIGNAL(activated(const QModelIndex&)), this,
    SLOT(onActivateRecent(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.Files, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onDoubleClickFile(const QModelIndex&)));

  QObject::connect(this->Implementation->Ui.FileName, SIGNAL(textChanged(const QString&)), this,
    SLOT(onTextEdited(const QString&)));

  QStringList filterList = MakeFilterList(nameFilter);
  if (filterList.empty())
  {
    this->Implementation->Ui.FileType->addItem("All Files (*)");
    this->Implementation->Filters << "All Files (*)";
  }
  else
  {
    this->Implementation->Ui.FileType->addItems(filterList);
    this->Implementation->Filters = filterList;
  }
  this->onFilterChange(this->Implementation->Ui.FileType->currentText());

  QString startPath = startDirectory;
  if (startPath.isEmpty())
  {
    startPath = this->Implementation->getStartPath();
  }
  this->Implementation->addHistory(startPath);
  this->Implementation->setCurrentPath(startPath);

  this->Implementation->Ui.Files->resizeColumnToContents(0);
  this->Implementation->Ui.Files->setTextElideMode(Qt::ElideMiddle);
  QHeaderView* header = this->Implementation->Ui.Files->header();

  // This code is similar to QFileDialog code
  // It positions different columns and orders in a standard way
  QFontMetrics fm(this->font());
  header->resizeSection(0, fm.width(QLatin1String("wwwwwwwwwwwwwwwwwwwwwwwwww")));
  header->resizeSection(1, fm.width(QLatin1String("mp3Folder")));
  header->resizeSection(2, fm.width(QLatin1String("128.88 GB")));
  header->resizeSection(3, fm.width(QLatin1String("10/29/81 02:02PM")));
  this->Implementation->Ui.Files->setSortingEnabled(true);
  this->Implementation->Ui.Files->header()->setSortIndicator(0, Qt::AscendingOrder);

  // Use saved state if any
  this->restoreState();

  bool showDetail = this->Implementation->Model->isShowingDetailedInfo();
  this->Implementation->Ui.ShowDetail->setChecked(showDetail);
  this->Implementation->Ui.Files->setColumnHidden(2, !showDetail);
  this->Implementation->Ui.Files->setColumnHidden(3, !showDetail);
}

//-----------------------------------------------------------------------------
pqFileDialog::~pqFileDialog()
{
  this->saveState();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onCreateNewFolder()
{
  // Add a directory entry with a default name to the model
  // This actually creates a directory with the given name,
  //   but this temporary directory will be deleted and a new one created
  //   once the user provides a new name for it.
  //   TODO: I guess we could insert an item into the model without
  //    actually creating a new directory but this way I could reuse code.
  QString dirName = QString("New Folder");
  int i = 0;
  QString fullDir;
  while (this->Implementation->Model->dirExists(dirName, fullDir))
  {
    dirName = QString("New Folder%1").arg(i++);
  }

  if (!this->Implementation->Model->mkdir(dirName))
  {
    QMessageBox message(QMessageBox::Warning, this->windowTitle(),
      QString(tr("Unable to create directory %1.")).arg(dirName), QMessageBox::Ok);
    message.exec();
    return;
  }

  // Get the index of the new directory in the model
  QAbstractProxyModel* m = &this->Implementation->FileFilter;
  int numrows = m->rowCount(QModelIndex());
  bool found = false;
  QModelIndex idx;
  for (i = 0; i < numrows; i++)
  {
    idx = m->index(i, 0, QModelIndex());
    if (dirName == m->data(idx, Qt::DisplayRole))
    {
      found = true;
      break;
    }
  }
  if (!found)
  {
    return;
  }

  this->Implementation->Ui.Files->scrollTo(idx);
  this->Implementation->Ui.Files->selectionModel()->select(
    idx, QItemSelectionModel::Select | QItemSelectionModel::Current);
  this->Implementation->Ui.Files->edit(idx);
  this->Implementation->Ui.FileName->clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onContextMenuRequested(const QPoint& menuPos)
{
  QMenu menu;
  menu.setObjectName("FileDialogContextMenu");

  // Only display new dir option if we're saving, not opening
  if (this->Implementation->Mode == pqFileDialog::AnyFile)
  {
    QAction* actionNewDir = new QAction("Create New Folder", this);
    QObject::connect(actionNewDir, SIGNAL(triggered()), this, SLOT(onCreateNewFolder()));
    menu.addAction(actionNewDir);
  }

  QAction* actionHiddenFiles = new QAction("Show Hidden Files", this);
  actionHiddenFiles->setCheckable(true);
  actionHiddenFiles->setChecked(this->Implementation->FileFilter.getShowHidden());
  QObject::connect(actionHiddenFiles, SIGNAL(triggered(bool)), this, SLOT(onShowHiddenFiles(bool)));
  menu.addAction(actionHiddenFiles);

  menu.exec(this->Implementation->Ui.Files->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
void pqFileDialog::setFileMode(pqFileDialog::FileMode mode)
{
  // this code is only needed for the 3.10 release as
  // after that the user should know that the dialog support multiple file open
  bool setupMutlipleFileHelp = false;
  this->Implementation->Mode = mode;
  QAbstractItemView::SelectionMode selectionMode;
  switch (this->Implementation->Mode)
  {
    case AnyFile:
    case ExistingFile:
    case Directory:
    default:
      selectionMode = QAbstractItemView::SingleSelection;
      break;
    case ExistingFiles:
      setupMutlipleFileHelp = (this->Implementation->ShowMultipleFileHelp != true);
      selectionMode = QAbstractItemView::ExtendedSelection;
      break;
  }
  if (setupMutlipleFileHelp)
  {
    // only set the tooltip and window title the first time through
    this->Implementation->ShowMultipleFileHelp = true;
    this->setWindowTitle(this->windowTitle() + "  (open multiple files with <ctrl> key.)");
    this->setToolTip("open multiple files with <ctrl> key.");
  }
  this->Implementation->Ui.Files->setSelectionMode(selectionMode);
  this->Implementation->Ui.Favorites->setSelectionMode(selectionMode);
}

//-----------------------------------------------------------------------------
void pqFileDialog::setRecentlyUsedExtension(const QString& fileExtension)
{
  if (fileExtension == QString())
  {
    // upon the initial use of any kind (e.g., data or screenshot) of dialog
    // 'fileExtension' is equal /set to an empty string.
    // In this case, no any user preferences are considered
    this->Implementation->Ui.FileType->setCurrentIndex(0);
  }
  else
  {
    int index = this->Implementation->Ui.FileType->findText(fileExtension, Qt::MatchContains);
    // just in case the provided extension is not in the combobox list
    index = (index == -1) ? 0 : index;
    this->Implementation->Ui.FileType->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::addToFilesSelected(const QStringList& files)
{
  // Ensure that we are hidden before broadcasting the selection,
  // so we don't get caught by screen-captures
  this->setVisible(false);
  this->Implementation->SelectedFiles.append(files);
}

//-----------------------------------------------------------------------------
void pqFileDialog::emitFilesSelectionDone()
{
  emit filesSelected(this->Implementation->SelectedFiles);
  if (this->Implementation->Mode != this->ExistingFiles &&
    this->Implementation->SelectedFiles.size() > 0)
  {
    emit filesSelected(this->Implementation->SelectedFiles[0]);
  }
  this->done(QDialog::Accepted);
}

//-----------------------------------------------------------------------------
QList<QStringList> pqFileDialog::getAllSelectedFiles()
{
  return this->Implementation->SelectedFiles;
}

//-----------------------------------------------------------------------------
QStringList pqFileDialog::getSelectedFiles(int index)
{
  if (index < 0 || index >= this->Implementation->SelectedFiles.size())
  {
    return QStringList();
  }
  return this->Implementation->SelectedFiles[index];
}

//-----------------------------------------------------------------------------
void pqFileDialog::accept()
{
  bool loadedFile = false;
  switch (this->Implementation->Mode)
  {
    case AnyFile:
    case Directory:
      loadedFile = this->acceptDefault(false);
      break;
    case ExistingFiles:
    case ExistingFile:
      loadedFile = this->acceptExistingFiles();
      break;
  }
  if (loadedFile)
  {
    emit this->emitFilesSelectionDone();
  }
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptExistingFiles()
{
  bool loadedFiles = false;
  QString filename;
  if (this->Implementation->FileNames.size() == 0)
  {
    // when we have nothing selected in the current selection model, we will
    // attempt to use the default way
    return this->acceptDefault(true);
  }
  foreach (filename, this->Implementation->FileNames)
  {
    filename = filename.trimmed();

    QString fullFilePath = this->Implementation->Model->absoluteFilePath(filename);
    emit this->fileAccepted(fullFilePath);
    loadedFiles = (this->acceptInternal(this->buildFileGroup(filename), false) || loadedFiles);
  }
  return loadedFiles;
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptDefault(const bool& checkForGrouping)
{
  QString filename = this->Implementation->Ui.FileName->text();
  filename = filename.trimmed();

  QString fullFilePath = this->Implementation->Model->absoluteFilePath(filename);
  emit this->fileAccepted(fullFilePath);

  QStringList files;
  if (checkForGrouping)
  {
    files = this->buildFileGroup(filename);
  }
  else
  {
    files = QStringList(fullFilePath);
  }
  return this->acceptInternal(files, false);
}

//-----------------------------------------------------------------------------
QStringList pqFileDialog::buildFileGroup(const QString& filename)
{
  QStringList files;

  // if we find the file passed in is the parent of a group,
  // add the entire group to the return QList
  QAbstractProxyModel* model = &this->Implementation->FileFilter;

  for (int row = 0; row < model->rowCount(); row++)
  {
    QModelIndex rowIndex = model->index(row, 0);

    for (int column = 0; column < model->columnCount(rowIndex); column++)
    {
      QModelIndex index;
      if (column == 0)
      {
        index = rowIndex;
      }
      else
      {
        index = model->index(0, column, rowIndex);
      }

      QString label = model->data(index, Qt::DisplayRole).toString();

      if (filename == label)
      {
        if (column == 0)
        {
          QModelIndex sourceIndex = model->mapToSource(index);
          files += this->Implementation->Model->getFilePaths(sourceIndex);
        }
        else
        {
          // UserRole will return the full file path
          files += model->data(index, Qt::UserRole).toString();
        }
      }
    }
  }

  if (files.empty())
  {
    files.append(this->Implementation->Model->absoluteFilePath(filename));
  }

  return files;
}

//-----------------------------------------------------------------------------
void pqFileDialog::onModelReset()
{
  this->Implementation->Ui.Parents->clear();

  QString currentPath = this->Implementation->Model->getCurrentPath();
  // clean the path to always look like a unix path
  currentPath = QDir::cleanPath(currentPath);

  // the separator is always the unix separator
  QChar separator = '/';

  QStringList parents = currentPath.split(separator, QString::SkipEmptyParts);

  // put our root back in
  if (parents.count())
  {
    int idx = currentPath.indexOf(parents[0]);
    if (idx != 0 && idx != -1)
    {
      parents.prepend(currentPath.left(idx));
    }
  }
  else
  {
    parents.prepend(separator);
  }

  for (int i = 0; i != parents.size(); ++i)
  {
    QString str;
    for (int j = 0; j <= i; j++)
    {
      str += parents[j];
      if (!str.endsWith(separator))
      {
        str += separator;
      }
    }
    this->Implementation->Ui.Parents->addItem(str);
  }
  this->Implementation->Ui.Parents->setCurrentIndex(parents.size() - 1);
  disconnect(this->Implementation->Ui.ShowDetail, SIGNAL(clicked(bool)), NULL, NULL);
  connect(this->Implementation->Ui.ShowDetail, SIGNAL(clicked(bool)), this,
    SLOT(onShowDetailToggled(bool)));
  bool showDetail = this->Implementation->Model->isShowingDetailedInfo();
  this->Implementation->Ui.ShowDetail->setChecked(showDetail);
  this->Implementation->Ui.Files->setColumnHidden(2, !showDetail);
  this->Implementation->Ui.Files->setColumnHidden(3, !showDetail);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigate(const QString& Path)
{
  this->Implementation->addHistory(this->Implementation->Model->getCurrentPath());
  this->Implementation->setCurrentPath(Path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateUp()
{
  this->Implementation->addHistory(this->Implementation->Model->getCurrentPath());
  QFileInfo info(this->Implementation->Model->getCurrentPath());
  this->Implementation->setCurrentPath(info.path());
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateDown(const QModelIndex& idx)
{
  if (!this->Implementation->Model->isDir(idx))
    return;

  const QStringList paths = this->Implementation->Model->getFilePaths(idx);

  if (1 != paths.size())
    return;

  this->Implementation->addHistory(this->Implementation->Model->getCurrentPath());
  this->Implementation->setCurrentPath(paths[0]);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateBack()
{
  QString path = this->Implementation->backHistory();
  this->Implementation->setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateForward()
{
  QString path = this->Implementation->forwardHistory();
  this->Implementation->setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onFilterChange(const QString& filter)
{
  // set filter on proxy
  this->Implementation->FileFilter.setFilter(filter);

  // update view
  this->Implementation->FileFilter.clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFavorite(const QModelIndex&)
{
  this->Implementation->Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedRecent(const QModelIndex&)
{
  this->Implementation->Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFile(const QModelIndex& vtkNotUsed(index))
{
  this->Implementation->Ui.Favorites->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateFavorite(const QModelIndex& index)
{
  if (this->Implementation->FavoriteModel->isDir(index))
  {
    QString file = this->Implementation->FavoriteModel->filePath(index);
    this->onNavigate(file);
    this->Implementation->Ui.FileName->selectAll();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateRecent(const QModelIndex& index)
{
  QString file = this->Implementation->RecentModel->filePath(index);
  this->onNavigate(file);
  this->Implementation->Ui.FileName->selectAll();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onDoubleClickFile(const QModelIndex& index)
{
  if (this->Implementation->Mode == Directory)
  {
    QModelIndex actual_index = index;
    if (actual_index.model() == &this->Implementation->FileFilter)
      actual_index = this->Implementation->FileFilter.mapToSource(actual_index);

    QStringList selected_files;
    QStringList paths;
    QString path;

    paths = this->Implementation->Model->getFilePaths(actual_index);
    foreach (path, paths)
    {
      selected_files << this->Implementation->Model->absoluteFilePath(path);
    }
    this->acceptInternal(selected_files, true);
  }
  else
  {
    this->accept();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowHiddenFiles(const bool& hidden)
{
  this->Implementation->FileFilter.setShowHidden(hidden);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowDetailToggled(bool show)
{
  this->Implementation->Model->setShowDetailedInfo(show);
  this->Implementation->Ui.Files->setColumnHidden(2, !show);
  this->Implementation->Ui.Files->setColumnHidden(3, !show);
}

//-----------------------------------------------------------------------------
void pqFileDialog::setShowHidden(const bool& hidden)
{
  this->onShowHiddenFiles(hidden);
}

//-----------------------------------------------------------------------------
bool pqFileDialog::getShowHidden()
{
  return this->Implementation->FileFilter.getShowHidden();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onTextEdited(const QString& str)
{
  this->Implementation->Ui.Favorites->clearSelection();

  // really important to block signals so that the clearSelection
  // doesn't cause a signal to be fired that calls fileSelectionChanged
  this->Implementation->Ui.Files->blockSignals(true);
  this->Implementation->Ui.Files->clearSelection();
  if (str.size() > 0)
  {
    // convert the typed information to be this->Implementation->FileNames
    this->Implementation->FileNames =
      str.split(this->Implementation->FileNamesSeperator, QString::SkipEmptyParts);
  }
  else
  {
    this->Implementation->FileNames.clear();
  }
  this->Implementation->Ui.Files->blockSignals(false);
}

//-----------------------------------------------------------------------------
QString pqFileDialog::fixFileExtension(const QString& filename, const QString& filter)
{
  // Add missing extension if necessary
  QFileInfo fileInfo(filename);
  QString ext = fileInfo.completeSuffix();
  QString extensionWildcard = GetWildCardsFromFilter(filter).first();
  QString wantedExtension = extensionWildcard.mid(extensionWildcard.indexOf('.') + 1);

  if (!ext.isEmpty())
  {
    // Ensure that the extension the user added is indeed of one the supported
    // types. (BUG #7634).
    QStringList wildCards;
    foreach (QString curfilter, this->Implementation->Filters)
    {
      wildCards += ::GetWildCardsFromFilter(curfilter);
    }
    bool pass = false;
    foreach (QString wildcard, wildCards)
    {
      if (wildcard.indexOf('.') != -1)
      {
        // we only need to validate the extension, not the filename.
        wildcard = QString("*.%1").arg(wildcard.mid(wildcard.indexOf('.') + 1));
        QRegExp regEx = QRegExp(wildcard, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (regEx.exactMatch(fileInfo.fileName()))
        {
          pass = true;
          break;
        }
      }
      else
      {
        // we have a filter which does not specify any rule for the extension.
        // In that case, just assume the extension matched.
        pass = true;
        break;
      }
    }
    if (!pass)
    {
      // force adding of the wantedExtension.
      ext = QString();
    }
  }

  QString fixedFilename = filename;
  if (ext.isEmpty() && !wantedExtension.isEmpty() && wantedExtension != "*")
  {
    if (fixedFilename.at(fixedFilename.size() - 1) != '.')
    {
      fixedFilename += ".";
    }
    fixedFilename += wantedExtension;
  }
  return fixedFilename;
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptInternal(const QStringList& selected_files, const bool& doubleclicked)
{
  if (selected_files.empty())
  {
    return false;
  }

  QString file = selected_files[0];
  // User chose an existing directory
  if (this->Implementation->Model->dirExists(file, file))
  {
    switch (this->Implementation->Mode)
    {
      case Directory:
        if (!doubleclicked)
        {
          this->addToFilesSelected(QStringList(file));
          this->onNavigate(file);
          return true;
        }
      case ExistingFile:
      case ExistingFiles:
      case AnyFile:
        this->onNavigate(file);
        this->Implementation->Ui.FileName->clear();
        break;
    }
    return false;
  }

  // User chose an existing file or a brand new filename.
  if (this->Implementation->Mode == pqFileDialog::AnyFile)
  {
    // If mode is a "save" dialog, we fix the extension first.
    file = this->fixFileExtension(file, this->Implementation->Ui.FileType->currentText());

    // It is very possible that after fixing the extension,
    // the new filename is an already present directory,
    // hence we handle that case:
    if (this->Implementation->Model->dirExists(file, file))
    {
      this->onNavigate(file);
      this->Implementation->Ui.FileName->clear();
      return false;
    }
  }

  // User chose an existing file-or-files
  if (this->Implementation->Model->fileExists(file, file))
  {
    switch (this->Implementation->Mode)
    {
      case Directory:
        // User chose a file in directory mode, do nothing
        this->Implementation->Ui.FileName->clear();
        break;
      case ExistingFile:
      case ExistingFiles:
        this->addToFilesSelected(selected_files);
        break;
      case AnyFile:
        // User chose an existing file, prompt before overwrite
        if (!this->Implementation->SuppressOverwriteWarning)
        {
          if (QMessageBox::No ==
            QMessageBox::warning(this, this->windowTitle(),
                QString(tr("%1 already exists.\nDo you want to replace it?")).arg(file),
                QMessageBox::Yes, QMessageBox::No))
          {
            return false;
          }
        }
        this->addToFilesSelected(QStringList(file));
        break;
    }
    return true;
  }
  else // User choose non-existent file.
  {
    switch (this->Implementation->Mode)
    {
      case Directory:
      case ExistingFile:
      case ExistingFiles:
        this->Implementation->Ui.FileName->selectAll();
        return false;

      case AnyFile:
        this->addToFilesSelected(QStringList(file));
        return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void pqFileDialog::fileSelectionChanged()
{
  // Selection changed, update the FileName entry box
  // to reflect the current selection.
  QString fileString;
  const QModelIndexList indices =
    this->Implementation->Ui.Files->selectionModel()->selectedIndexes();
  const QModelIndexList rows = this->Implementation->Ui.Files->selectionModel()->selectedRows();

  if (indices.isEmpty())
  {
    // do not change the FileName text if no selections
    return;
  }
  QStringList fileNames;

  QString name;
  for (int i = 0; i != indices.size(); ++i)
  {
    QModelIndex index = indices[i];
    if (index.column() != 0)
    {
      continue;
    }
    if (index.model() == &this->Implementation->FileFilter)
    {
      name = this->Implementation->FileFilter.data(index).toString();
      fileString += name;
      if (i != rows.size() - 1)
      {
        fileString += this->Implementation->FileNamesSeperator;
      }
      fileNames.append(name);
    }
  }
  // if we are in directory mode we have to enable / disable the OK button
  // based on if the user has selected a file.
  if (this->Implementation->Mode == pqFileDialog::Directory &&
    indices[0].model() == &this->Implementation->FileFilter)
  {
    QModelIndex idx = this->Implementation->FileFilter.mapToSource(indices[0]);
    bool enabled = this->Implementation->Model->isDir(idx);
    this->Implementation->Ui.OK->setEnabled(enabled);
    if (enabled)
    {
      this->Implementation->Ui.FileName->setText(fileString);
    }
    else
    {
      this->Implementation->Ui.FileName->clear();
    }
    return;
  }

  // user is currently editing a name, don't change the text
  this->Implementation->Ui.FileName->blockSignals(true);
  this->Implementation->Ui.FileName->setText(fileString);
  this->Implementation->Ui.FileName->blockSignals(false);

  this->Implementation->FileNames = fileNames;
}

//-----------------------------------------------------------------------------
bool pqFileDialog::selectFile(const QString& f)
{
  // We don't use QFileInfo here since it messes the paths up if the client and
  // the server are heterogeneous systems.
  std::string unix_path = f.toUtf8().data();
  vtksys::SystemTools::ConvertToUnixSlashes(unix_path);

  std::string filename, dirname;
  std::string::size_type slashPos = unix_path.rfind("/");
  if (slashPos != std::string::npos)
  {
    filename = unix_path.substr(slashPos + 1);
    dirname = unix_path.substr(0, slashPos);
  }
  else
  {
    filename = unix_path;
    dirname = "";
  }

  QPointer<QDialog> diag = this;
  this->Implementation->Model->setCurrentPath(dirname.c_str());
  this->Implementation->Ui.FileName->setText(filename.c_str());
  this->Implementation->SuppressOverwriteWarning = true;
  this->accept();
  if (diag && diag->result() != QDialog::Accepted)
  {
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
void pqFileDialog::showEvent(QShowEvent* _showEvent)
{
  QDialog::showEvent(_showEvent);
  // Qt sets the default keyboard focus to the last item in the tab order
  // which is determined by the creation order. This means that we have
  // to explicitly state that the line edit has the focus on showing no
  // matter the tab order
  this->Implementation->Ui.FileName->setFocus(Qt::OtherFocusReason);
}

//-----------------------------------------------------------------------------
QString pqFileDialog::getSaveFileName(pqServer* server, QWidget* parentWdg, const QString& title,
  const QString& directory, const QString& filter)
{
  pqFileDialog fileDialog(server, parentWdg, title, directory, filter);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    return fileDialog.getSelectedFiles()[0];
  }
  return QString();
}

//-----------------------------------------------------------------------------
void pqFileDialog::saveState()
{
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    pqSettings* settings = core->settings();
    settings->saveState(*this, "FileDialog");
    settings->beginGroup("FileDialog");

    QHeaderView* header = this->Implementation->Ui.Files->header();
    settings->setValue("Header", header->saveState());
    settings->setValue("MainSplitter", this->Implementation->Ui.mainSplitter->saveState());
    settings->setValue("Splitter", this->Implementation->Ui.splitter->saveState());
    settings->endGroup();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::restoreState()
{

  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    pqSettings* settings = core->settings();
    settings->restoreState("FileDialog", *this);
    settings->beginGroup("FileDialog");

    if (settings->contains("Header"))
    {
      this->Implementation->Ui.Files->header()->restoreState(
        settings->value("Header").toByteArray());
    }

    if (settings->contains("MainSplitter"))
    {
      this->Implementation->Ui.mainSplitter->restoreState(
        settings->value("MainSplitter").toByteArray());
    }
    if (settings->contains("Splitter"))
    {
      this->Implementation->Ui.splitter->restoreState(settings->value("Splitter").toByteArray());
    }
    settings->endGroup();
  }
}
