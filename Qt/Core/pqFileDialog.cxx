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
#include <QScopedValueRollback>
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
  void showPopup() override
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
  if (filter.contains(";;"))
  {
    return filter.split(";;", QString::SkipEmptyParts);
  }

  // check if '\n' is being used as separator.
  // (not sure why, but the old code was doing it, and if some applications
  // are relying on it, I don't want to break them right now).
  return filter.split('\n', QString::SkipEmptyParts);
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
  bool InDoubleClickHandler; //< used to determine if we're "accept"ing as a result of
                             //  double-clicking as that elicits a different
                             //  response.

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
    , InDoubleClickHandler(false)
  {
    QObject::connect(p, SIGNAL(filesSelected(const QList<QStringList>&)), this->RecentModel,
      SLOT(setChosenFiles(const QList<QStringList>&)));
  }

  ~pqImplementation() override
  {
    delete this->RecentModel;
    delete this->FavoriteModel;
    delete this->Model;
    delete this->Completer;
  }

  bool eventFilter(QObject* obj, QEvent* anEvent) override
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
  auto& impl = *this->Implementation;

  impl.Ui.setupUi(this);
  // ensures that the favorites and the browser component are sized
  // proportionately.
  impl.Ui.mainSplitter->setStretchFactor(0, 1);
  impl.Ui.mainSplitter->setStretchFactor(1, 4);
  this->setWindowTitle(title);

  impl.Ui.Files->setEditTriggers(QAbstractItemView::EditKeyPressed);

  // install the event filter
  impl.Ui.Files->installEventFilter(this->Implementation);

  // install the autocompleter
  impl.Ui.FileName->setCompleter(impl.Completer);

  // this is the Navigate button, which is only shown when needed
  // and that too in ExistingFilesAndDirectories and Directory mode alone.
  impl.Ui.Navigate->hide();

  QPixmap back = style()->standardPixmap(QStyle::SP_FileDialogBack);
  impl.Ui.NavigateBack->setIcon(back);
  impl.Ui.NavigateBack->setEnabled(false);
  QObject::connect(impl.Ui.NavigateBack, SIGNAL(clicked(bool)), this, SLOT(onNavigateBack()));
  // just flip the back image to make a forward image
  QPixmap forward = QPixmap::fromImage(back.toImage().mirrored(true, false));
  impl.Ui.NavigateForward->setIcon(forward);
  impl.Ui.NavigateForward->setDisabled(true);
  QObject::connect(impl.Ui.NavigateForward, SIGNAL(clicked(bool)), this, SLOT(onNavigateForward()));
  impl.Ui.NavigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
  impl.Ui.CreateFolder->setIcon(style()->standardPixmap(QStyle::SP_FileDialogNewFolder));
  impl.Ui.CreateFolder->setDisabled(true);

  impl.Ui.ShowDetail->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced26.png"));

  impl.Ui.Files->setModel(&impl.FileFilter);
  impl.Ui.Files->setSelectionBehavior(QAbstractItemView::SelectRows);

  impl.Ui.Files->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(impl.Ui.Files, SIGNAL(customContextMenuRequested(const QPoint&)), this,
    SLOT(onContextMenuRequested(const QPoint&)));
  impl.Ui.CreateFolder->setEnabled(true);

  impl.Ui.Favorites->setModel(impl.FavoriteModel);
  impl.Ui.Recent->setModel(impl.RecentModel);

  this->setFileMode(ExistingFile);

  QObject::connect(impl.Model, SIGNAL(modelReset()), this, SLOT(onModelReset()));

  QObject::connect(impl.Ui.NavigateUp, SIGNAL(clicked()), this, SLOT(onNavigateUp()));

  QObject::connect(impl.Ui.CreateFolder, SIGNAL(clicked()), this, SLOT(onCreateNewFolder()));

  QObject::connect(
    impl.Ui.Parents, SIGNAL(activated(const QString&)), this, SLOT(onNavigate(const QString&)));

  QObject::connect(impl.Ui.FileType, SIGNAL(currentIndexChanged(const QString&)), this,
    SLOT(onFilterChange(const QString&)));

  QObject::connect(impl.Ui.Favorites, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedFavorite(const QModelIndex&)));

  QObject::connect(impl.Ui.Recent, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedRecent(const QModelIndex&)));

  QObject::connect(impl.Ui.Files, SIGNAL(clicked(const QModelIndex&)), this,
    SLOT(onClickedFile(const QModelIndex&)));

  QObject::connect(impl.Ui.Files->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(fileSelectionChanged()));

  QObject::connect(impl.Ui.Favorites, SIGNAL(activated(const QModelIndex&)), this,
    SLOT(onActivateFavorite(const QModelIndex&)));

  QObject::connect(impl.Ui.Recent, SIGNAL(activated(const QModelIndex&)), this,
    SLOT(onActivateRecent(const QModelIndex&)));

  QObject::connect(impl.Ui.Files, SIGNAL(doubleClicked(const QModelIndex&)), this,
    SLOT(onDoubleClickFile(const QModelIndex&)));

  QObject::connect(impl.Ui.FileName, SIGNAL(textChanged(const QString&)), this,
    SLOT(onTextEdited(const QString&)));

  QStringList filterList = MakeFilterList(nameFilter);
  if (filterList.empty())
  {
    impl.Ui.FileType->addItem("All Files (*)");
    impl.Filters << "All Files (*)";
  }
  else
  {
    impl.Ui.FileType->addItems(filterList);
    impl.Filters = filterList;
  }
  this->onFilterChange(impl.Ui.FileType->currentText());

  QString startPath = startDirectory;
  if (startPath.isEmpty())
  {
    startPath = impl.getStartPath();
  }
  impl.addHistory(startPath);
  impl.setCurrentPath(startPath);

  impl.Ui.Files->resizeColumnToContents(0);
  impl.Ui.Files->setTextElideMode(Qt::ElideMiddle);
  QHeaderView* header = impl.Ui.Files->header();

  // This code is similar to QFileDialog code
  // It positions different columns and orders in a standard way
  QFontMetrics fm(this->font());
  header->resizeSection(0, fm.width(QLatin1String("wwwwwwwwwwwwwwwwwwwwwwwwww")));
  header->resizeSection(1, fm.width(QLatin1String("mp3Folder")));
  header->resizeSection(2, fm.width(QLatin1String("128.88 GB")));
  header->resizeSection(3, fm.width(QLatin1String("10/29/81 02:02PM")));
  impl.Ui.Files->setSortingEnabled(true);
  impl.Ui.Files->header()->setSortIndicator(0, Qt::AscendingOrder);

  // Use saved state if any
  this->restoreState();

  bool showDetail = impl.Model->isShowingDetailedInfo();
  impl.Ui.ShowDetail->setChecked(showDetail);
  impl.Ui.Files->setColumnHidden(2, !showDetail);
  impl.Ui.Files->setColumnHidden(3, !showDetail);

  // let's manage the default button.
  impl.Ui.OK->setDefault(true);
  impl.Ui.Navigate->setDefault(false);

  this->connect(impl.Ui.Navigate, SIGNAL(clicked()), SLOT(onNavigate()));
}

//-----------------------------------------------------------------------------
pqFileDialog::~pqFileDialog()
{
  this->saveState();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onCreateNewFolder()
{
  auto& impl = *this->Implementation;

  // Add a directory entry with a default name to the model
  // This actually creates a directory with the given name,
  //   but this temporary directory will be deleted and a new one created
  //   once the user provides a new name for it.
  //   TODO: I guess we could insert an item into the model without
  //    actually creating a new directory but this way I could reuse code.
  QString dirName = QString("New Folder");
  int i = 0;
  QString fullDir;
  while (impl.Model->dirExists(dirName, fullDir))
  {
    dirName = QString("New Folder%1").arg(i++);
  }

  if (!impl.Model->mkdir(dirName))
  {
    QMessageBox message(QMessageBox::Warning, this->windowTitle(),
      QString(tr("Unable to create directory %1.")).arg(dirName), QMessageBox::Ok);
    message.exec();
    return;
  }

  // Get the index of the new directory in the model
  QAbstractProxyModel* m = &impl.FileFilter;
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

  impl.Ui.Files->scrollTo(idx);
  impl.Ui.Files->selectionModel()->select(
    idx, QItemSelectionModel::Select | QItemSelectionModel::Current);
  impl.Ui.Files->edit(idx);
  impl.Ui.FileName->clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onContextMenuRequested(const QPoint& menuPos)
{
  auto& impl = *this->Implementation;

  QMenu menu;
  menu.setObjectName("FileDialogContextMenu");

  // Only display new dir option if we're saving, not opening
  if (impl.Mode == pqFileDialog::AnyFile)
  {
    QAction* actionNewDir = new QAction("Create New Folder", this);
    QObject::connect(actionNewDir, SIGNAL(triggered()), this, SLOT(onCreateNewFolder()));
    menu.addAction(actionNewDir);
  }

  QAction* actionHiddenFiles = new QAction("Show Hidden Files", this);
  actionHiddenFiles->setCheckable(true);
  actionHiddenFiles->setChecked(impl.FileFilter.getShowHidden());
  QObject::connect(actionHiddenFiles, SIGNAL(triggered(bool)), this, SLOT(onShowHiddenFiles(bool)));
  menu.addAction(actionHiddenFiles);

  menu.exec(impl.Ui.Files->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
void pqFileDialog::setFileMode(pqFileDialog::FileMode mode)
{
  auto& impl = *this->Implementation;

  // this code is only needed for the 3.10 release as
  // after that the user should know that the dialog support multiple file open
  bool setupMultipleFileHelp = false;
  impl.Mode = mode;
  QAbstractItemView::SelectionMode selectionMode;
  switch (impl.Mode)
  {
    case AnyFile:
    case ExistingFile:
    case Directory:
    default:
      selectionMode = QAbstractItemView::SingleSelection;
      break;
    case ExistingFiles:
    case ExistingFilesAndDirectories:
      setupMultipleFileHelp = (impl.ShowMultipleFileHelp != true);
      selectionMode = QAbstractItemView::ExtendedSelection;
      break;
  }
  if (setupMultipleFileHelp)
  {
    // only set the tooltip and window title the first time through
    impl.ShowMultipleFileHelp = true;
    this->setWindowTitle(this->windowTitle() + "  (open multiple files with <ctrl> key.)");
    this->setToolTip("open multiple files with <ctrl> key.");
  }
  impl.Ui.Files->setSelectionMode(selectionMode);

  if (mode == Directory || mode == ExistingFilesAndDirectories)
  {
    impl.Ui.Navigate->show();
    impl.Ui.Navigate->setEnabled(false);
  }
  else
  {
    impl.Ui.Navigate->hide();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::setRecentlyUsedExtension(const QString& fileExtension)
{
  auto& impl = *this->Implementation;

  if (fileExtension == QString())
  {
    // upon the initial use of any kind (e.g., data or screenshot) of dialog
    // 'fileExtension' is equal /set to an empty string.
    // In this case, no any user preferences are considered
    impl.Ui.FileType->setCurrentIndex(0);
  }
  else
  {
    int index = impl.Ui.FileType->findText(fileExtension, Qt::MatchContains);
    // just in case the provided extension is not in the combobox list
    index = (index == -1) ? 0 : index;
    impl.Ui.FileType->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::addToFilesSelected(const QStringList& files)
{
  auto& impl = *this->Implementation;

  // Ensure that we are hidden before broadcasting the selection,
  // so we don't get caught by screen-captures
  this->setVisible(false);
  impl.SelectedFiles.append(files);
}

//-----------------------------------------------------------------------------
void pqFileDialog::emitFilesSelectionDone()
{
  auto& impl = *this->Implementation;
  emit filesSelected(impl.SelectedFiles);
  if (impl.Mode != this->ExistingFiles && impl.SelectedFiles.size() > 0)
  {
    emit filesSelected(impl.SelectedFiles[0]);
  }
  this->done(QDialog::Accepted);
}

//-----------------------------------------------------------------------------
QList<QStringList> pqFileDialog::getAllSelectedFiles()
{
  auto& impl = *this->Implementation;
  return impl.SelectedFiles;
}

//-----------------------------------------------------------------------------
QStringList pqFileDialog::getSelectedFiles(int index)
{
  auto& impl = *this->Implementation;
  if (index < 0 || index >= impl.SelectedFiles.size())
  {
    return QStringList();
  }
  return impl.SelectedFiles[index];
}

//-----------------------------------------------------------------------------
void pqFileDialog::accept()
{
  auto& impl = *this->Implementation;

  bool loadedFile = false;
  switch (impl.Mode)
  {
    case AnyFile:
    case Directory:
      loadedFile = this->acceptDefault(false);
      break;
    case ExistingFiles:
    case ExistingFile:
    case ExistingFilesAndDirectories:
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
  auto& impl = *this->Implementation;

  bool loadedFiles = false;
  QString filename;
  if (impl.FileNames.size() == 0)
  {
    // when we have nothing selected in the current selection model, we will
    // attempt to use the default way
    return this->acceptDefault(true);
  }
  foreach (filename, impl.FileNames)
  {
    filename = filename.trimmed();

    QString fullFilePath = impl.Model->absoluteFilePath(filename);
    emit this->fileAccepted(fullFilePath);
    loadedFiles = (this->acceptInternal(this->buildFileGroup(filename)) || loadedFiles);
  }
  return loadedFiles;
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptDefault(const bool& checkForGrouping)
{
  auto& impl = *this->Implementation;

  QString filename = impl.Ui.FileName->text();
  filename = filename.trimmed();

  QString fullFilePath = impl.Model->absoluteFilePath(filename);
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
  return this->acceptInternal(files);
}

//-----------------------------------------------------------------------------
QStringList pqFileDialog::buildFileGroup(const QString& filename)
{
  auto& impl = *this->Implementation;

  QStringList files;

  // if we find the file passed in is the parent of a group,
  // add the entire group to the return QList
  QAbstractProxyModel* model = &impl.FileFilter;

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
          files += impl.Model->getFilePaths(sourceIndex);
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
    files.append(impl.Model->absoluteFilePath(filename));
  }

  return files;
}

//-----------------------------------------------------------------------------
void pqFileDialog::onModelReset()
{
  auto& impl = *this->Implementation;

  impl.Ui.Parents->clear();

  QString currentPath = impl.Model->getCurrentPath();
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
    impl.Ui.Parents->addItem(str);
  }
  impl.Ui.Parents->setCurrentIndex(parents.size() - 1);
  disconnect(impl.Ui.ShowDetail, SIGNAL(clicked(bool)), NULL, NULL);
  connect(impl.Ui.ShowDetail, SIGNAL(clicked(bool)), this, SLOT(onShowDetailToggled(bool)));
  bool showDetail = impl.Model->isShowingDetailedInfo();
  impl.Ui.ShowDetail->setChecked(showDetail);
  impl.Ui.Files->setColumnHidden(2, !showDetail);
  impl.Ui.Files->setColumnHidden(3, !showDetail);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigate(const QString& newpath)
{
  auto& impl = *this->Implementation;

  QString path_to_navigate(newpath);
  if (newpath.isEmpty() && impl.FileNames.size() > 0)
  {
    path_to_navigate = impl.FileNames.front();
    path_to_navigate = impl.Model->absoluteFilePath(path_to_navigate);
  }

  impl.addHistory(impl.Model->getCurrentPath());
  impl.setCurrentPath(path_to_navigate);
  this->updateButtonStates();
  impl.Ui.FileName->clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateUp()
{
  auto& impl = *this->Implementation;

  impl.addHistory(impl.Model->getCurrentPath());
  QFileInfo info(impl.Model->getCurrentPath());
  impl.setCurrentPath(info.path());
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateDown(const QModelIndex& idx)
{
  auto& impl = *this->Implementation;

  if (!impl.Model->isDir(idx))
    return;

  const QStringList paths = impl.Model->getFilePaths(idx);

  if (1 != paths.size())
    return;

  impl.addHistory(impl.Model->getCurrentPath());
  impl.setCurrentPath(paths[0]);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateBack()
{
  auto& impl = *this->Implementation;
  QString path = impl.backHistory();
  impl.setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateForward()
{
  auto& impl = *this->Implementation;
  QString path = impl.forwardHistory();
  impl.setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onFilterChange(const QString& filter)
{
  auto& impl = *this->Implementation;

  // set filter on proxy
  impl.FileFilter.setFilter(filter);

  // update view
  impl.FileFilter.clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFavorite(const QModelIndex&)
{
  auto& impl = *this->Implementation;
  impl.Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedRecent(const QModelIndex&)
{
  auto& impl = *this->Implementation;
  impl.Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFile(const QModelIndex& vtkNotUsed(index))
{
  auto& impl = *this->Implementation;
  impl.Ui.Favorites->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateFavorite(const QModelIndex& index)
{
  auto& impl = *this->Implementation;
  if (impl.FavoriteModel->isDir(index))
  {
    QString file = impl.FavoriteModel->filePath(index);
    this->onNavigate(file);
    impl.Ui.FileName->selectAll();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateRecent(const QModelIndex& index)
{
  auto& impl = *this->Implementation;
  QString file = impl.RecentModel->filePath(index);
  this->onNavigate(file);
  impl.Ui.FileName->selectAll();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onDoubleClickFile(const QModelIndex&)
{
  auto& impl = *this->Implementation;
  QScopedValueRollback<bool> setter(impl.InDoubleClickHandler, true);
  this->accept();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowHiddenFiles(const bool& hidden)
{
  auto& impl = *this->Implementation;
  impl.FileFilter.setShowHidden(hidden);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowDetailToggled(bool show)
{
  auto& impl = *this->Implementation;
  impl.Model->setShowDetailedInfo(show);
  impl.Ui.Files->setColumnHidden(2, !show);
  impl.Ui.Files->setColumnHidden(3, !show);
}

//-----------------------------------------------------------------------------
void pqFileDialog::setShowHidden(const bool& hidden)
{
  this->onShowHiddenFiles(hidden);
}

//-----------------------------------------------------------------------------
bool pqFileDialog::getShowHidden()
{
  auto& impl = *this->Implementation;
  return impl.FileFilter.getShowHidden();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onTextEdited(const QString& str)
{
  auto& impl = *this->Implementation;
  impl.Ui.Favorites->clearSelection();

  // really important to block signals so that the clearSelection
  // doesn't cause a signal to be fired that calls fileSelectionChanged
  impl.Ui.Files->blockSignals(true);
  impl.Ui.Files->clearSelection();
  if (str.size() > 0)
  {
    // convert the typed information to be impl.FileNames
    impl.FileNames = str.split(impl.FileNamesSeperator, QString::SkipEmptyParts);
  }
  else
  {
    impl.FileNames.clear();
  }
  impl.Ui.Files->blockSignals(false);
  this->updateButtonStates();
}

//-----------------------------------------------------------------------------
QString pqFileDialog::fixFileExtension(const QString& filename, const QString& filter)
{
  auto& impl = *this->Implementation;
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
    foreach (QString curfilter, impl.Filters)
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
bool pqFileDialog::acceptInternal(const QStringList& selected_files)
{
  if (selected_files.empty())
  {
    return false;
  }

  auto& impl = *this->Implementation;
  QString file = selected_files[0];
  // User chose an existing directory
  if (impl.Model->dirExists(file, file))
  {
    switch (impl.Mode)
    {
      case ExistingFilesAndDirectories:
        if (!impl.InDoubleClickHandler)
        {
          this->addToFilesSelected(selected_files);
          return true;
        }
        VTK_FALLTHROUGH;
      case Directory:
        if (!impl.InDoubleClickHandler)
        {
          this->addToFilesSelected(QStringList(file));
          this->onNavigate(file);
          return true;
        }
        VTK_FALLTHROUGH;
      case ExistingFile:
      case ExistingFiles:
      case AnyFile:
        this->onNavigate(file);
        impl.Ui.FileName->clear();
        break;
    }
    return false;
  }

  // User chose an existing file or a brand new filename.
  if (impl.Mode == pqFileDialog::AnyFile)
  {
    // If mode is a "save" dialog, we fix the extension first.
    file = this->fixFileExtension(file, impl.Ui.FileType->currentText());

    // It is very possible that after fixing the extension,
    // the new filename is an already present directory,
    // hence we handle that case:
    if (impl.Model->dirExists(file, file))
    {
      this->onNavigate(file);
      impl.Ui.FileName->clear();
      return false;
    }
  }

  // User chose an existing file-or-files
  if (impl.Model->fileExists(file, file))
  {
    switch (impl.Mode)
    {
      case Directory:
        // User chose a file in directory mode, do nothing
        impl.Ui.FileName->clear();
        break;
      case ExistingFile:
      case ExistingFiles:
      case ExistingFilesAndDirectories:
        this->addToFilesSelected(selected_files);
        break;
      case AnyFile:
        // User chose an existing file, prompt before overwrite
        if (!impl.SuppressOverwriteWarning)
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
    switch (impl.Mode)
    {
      case Directory:
      case ExistingFile:
      case ExistingFiles:
      case ExistingFilesAndDirectories:
        impl.Ui.FileName->selectAll();
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
  auto& impl = *this->Implementation;
  // Selection changed, update the FileName entry box
  // to reflect the current selection.
  QString fileString;
  const QModelIndexList indices = impl.Ui.Files->selectionModel()->selectedIndexes();
  const QModelIndexList rows = impl.Ui.Files->selectionModel()->selectedRows();

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
    if (index.model() == &impl.FileFilter)
    {
      name = impl.FileFilter.data(index).toString();
      fileString += name;
      if (i != rows.size() - 1)
      {
        fileString += impl.FileNamesSeperator;
      }
      fileNames.append(name);
    }
  }

  // user is currently editing a name, don't change the text
  impl.Ui.FileName->blockSignals(true);
  impl.Ui.FileName->setText(fileString);
  impl.Ui.FileName->blockSignals(false);

  impl.FileNames = fileNames;
  this->updateButtonStates();
}

//-----------------------------------------------------------------------------
bool pqFileDialog::selectFile(const QString& f)
{
  auto& impl = *this->Implementation;
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
  impl.Model->setCurrentPath(dirname.c_str());
  impl.Ui.FileName->setText(filename.c_str());
  impl.SuppressOverwriteWarning = true;
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
  auto& impl = *this->Implementation;
  QDialog::showEvent(_showEvent);
  // Qt sets the default keyboard focus to the last item in the tab order
  // which is determined by the creation order. This means that we have
  // to explicitly state that the line edit has the focus on showing no
  // matter the tab order
  impl.Ui.FileName->setFocus(Qt::OtherFocusReason);
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
  auto& impl = *this->Implementation;
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    pqSettings* settings = core->settings();
    settings->saveState(*this, "FileDialog");
    settings->beginGroup("FileDialog");

    QHeaderView* header = impl.Ui.Files->header();
    settings->setValue("Header", header->saveState());
    settings->setValue("MainSplitter", impl.Ui.mainSplitter->saveState());
    settings->setValue("Splitter", impl.Ui.splitter->saveState());
    settings->endGroup();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::restoreState()
{
  auto& impl = *this->Implementation;
  pqApplicationCore* core = pqApplicationCore::instance();
  if (core)
  {
    pqSettings* settings = core->settings();
    settings->restoreState("FileDialog", *this);
    settings->beginGroup("FileDialog");

    if (settings->contains("Header"))
    {
      impl.Ui.Files->header()->restoreState(settings->value("Header").toByteArray());
    }

    if (settings->contains("MainSplitter"))
    {
      impl.Ui.mainSplitter->restoreState(settings->value("MainSplitter").toByteArray());
    }
    if (settings->contains("Splitter"))
    {
      impl.Ui.splitter->restoreState(settings->value("Splitter").toByteArray());
    }
    settings->endGroup();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::updateButtonStates()
{
  auto& impl = *this->Implementation;

  bool is_dir = false;
  if (impl.FileNames.size() == 1)
  {
    QString tmp;
    is_dir = impl.Model->dirExists(impl.FileNames.front(), tmp);
  }

  // if mode is Directory, update OK button state.
  if (impl.Mode == Directory)
  {
    impl.Ui.OK->setEnabled(is_dir);
  }
  else
  {
    impl.Ui.OK->setEnabled(true);
  }

  if (impl.Mode == Directory || impl.Mode == ExistingFilesAndDirectories)
  {
    // show the Navigate button.
    impl.Ui.Navigate->setVisible(true);

    // let's see if the Navigate button should be enabled. If the Navigate
    // button is enabled, it is also made the "default" button i.e. the button
    // that's triggered when user this the "Enter" key. If Navigate is not
    // enabled, then the OK button is the "default" button.
    if (is_dir)
    {
      impl.Ui.OK->setDefault(false);
      impl.Ui.Navigate->setEnabled(true);
      impl.Ui.Navigate->setDefault(true);
    }
    else
    {
      impl.Ui.Navigate->setEnabled(false);
      impl.Ui.Navigate->setDefault(false);
      impl.Ui.OK->setDefault(true);
    }
  }
  else
  {
    impl.Ui.Navigate->setVisible(false);
    impl.Ui.Navigate->setDefault(false);
    impl.Ui.OK->setDefault(true);
  }
}
