// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "pqFileDialog.h"
#include "pqApplicationCore.h"
#include "pqFileDialogFavoriteModel.h"
#include "pqFileDialogFilter.h"
#include "pqFileDialogLocationModel.h"
#include "pqFileDialogModel.h"
#include "pqFileDialogRecentDirsModel.h"
#include "pqQtDeprecated.h"
#include "pqServer.h"
#include "pqSettings.h"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QCompleter>
#include <QDesktopServices>
#include <QDir>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPoint>
#include <QRegularExpression>
#include <QScopedValueRollback>
#include <QShortcut>
#include <QShowEvent>
#include <QTabWidget>
#include <QtDebug>

#include <string>

#include "vtkPVSession.h"
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

#include <QtGlobal>

namespace
{

QStringList MakeFilterList(const QString& filter)
{
  if (filter.contains(";;"))
  {
    return filter.split(";;", PV_QT_SKIP_EMPTY_PARTS);
  }

  // check if '\n' is being used as separator.
  // (not sure why, but the old code was doing it, and if some applications
  // are relying on it, I don't want to break them right now).
  return filter.split('\n', PV_QT_SKIP_EMPTY_PARTS);
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
  QStringList fs = f.split(QRegularExpression("[\\s+;]"), PV_QT_SKIP_EMPTY_PARTS);

  // add a *.ext.* for every *.ext we get to support file groups
  QStringList ret = fs;
  Q_FOREACH (QString ext, fs)
  {
    ret.append(ext + ".*");
  }
  return ret;
}
}

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog::pqImplementation

class pqFileDialog::pqImplementation : public QWidget
{
public:
  pqFileDialogModel* const Model;
  pqFileDialogFavoriteModel* const FavoriteModel;
  pqFileDialogLocationModel* const LocationModel;
  pqFileDialogRecentDirsModel* const RecentModel;
  QSortFilterProxyModel* proxyFavoriteModel;
  pqFileDialogFilter FileFilter;
  QStringList FileNames; // list of file names in the FileName ui text edit
  QCompleter* Completer;
  pqFileDialog::FileMode Mode;
  Ui::pqFileDialog Ui;
  QList<QStringList> SelectedFiles;
  int SelectedFilterIndex;
  QStringList Filters;
  bool SuppressOverwriteWarning;
  bool ShowMultipleFileHelp;
  bool SupportsGroupFiles = true;
  QString FileNamesSeperator;
  bool InDoubleClickHandler; //< used to determine if we're "accept"ing as a result of
                             //  double-clicking as that elicits a different
                             //  response.

  // remember the last locations we browsed
  static QMap<QPointer<pqServer>, QString> FilePaths;

  pqImplementation(pqFileDialog* p, pqServer* server)
    : QWidget(p)
    , Model(new pqFileDialogModel(server, nullptr))
    , FavoriteModel(new pqFileDialogFavoriteModel(Model, server, nullptr))
    , LocationModel(new pqFileDialogLocationModel(Model, server, nullptr))
    , RecentModel(new pqFileDialogRecentDirsModel(Model, server, nullptr))
    , FileFilter(this->Model)
    , Completer(new QCompleter(&this->FileFilter, nullptr))
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
    delete this->LocationModel;
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
          this->Ui.EntityName->setFocus(Qt::OtherFocusReason);
          // send out a backspace event to the file name now
          QKeyEvent replicateDelete(keyEvent->type(), keyEvent->key(), keyEvent->modifiers());
          QApplication::sendEvent(this->Ui.EntityName, &replicateDelete);
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
    auto iter = this->FilePaths.find(s);
    if (iter != this->FilePaths.end() && !iter->isEmpty())
    {
      return *iter;
    }
    return this->Model->getCurrentPath();
  }

  void setCurrentPath(const QString& p)
  {
    this->Model->setCurrentPath(p);
    pqServer* s = this->Model->server();
    this->FilePaths[s] = p;
    this->Ui.Favorites->clearSelection();
    this->Ui.Locations->clearSelection();
    this->Ui.Recent->clearSelection();
    this->Ui.Files->setFocus(Qt::OtherFocusReason);
  }

  void addHistory(const QString& p)
  {
    this->BackHistory.append(p);
    this->ForwardHistory.clear();
    this->Ui.NavigateBack->setEnabled(this->BackHistory.size() > 1);
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
    if (this->ForwardHistory.empty())
    {
      this->Ui.NavigateForward->setEnabled(false);
    }
    return path;
  }

protected:
  QStringList BackHistory;
  QStringList ForwardHistory;
};

QMap<QPointer<pqServer>, QString> pqFileDialog::pqImplementation::FilePaths;

/////////////////////////////////////////////////////////////////////////////
// pqFileDialog

pqFileDialog::pqFileDialog(pqServer* server, QWidget* p, const QString& title,
  const QString& startDirectory, const QString& nameFilter, bool supportsGroupFiles,
  bool onlyBrowseRemotely)
  : Superclass(p)
{
  // remove do-nothing "?" title bar button on Windows.
  this->setWindowFlags(this->windowFlags().setFlag(Qt::WindowContextHelpButtonHint, false));
  this->setWindowTitle(title);

  // create a tab widget for the vtkPVSession::CLIENT and vtkPVSession::DATA_SERVER locations
  QPointer<QTabWidget> tabWidget = new QTabWidget(this);
  tabWidget->setObjectName("tabWidget");

  // check if local file system can only be used
  const bool canOnlyUseLocalFileSystem = !server || !server->isRemote();
  // check if local file system can be used
  const bool enableLocalImplementation = !onlyBrowseRemotely ? true : canOnlyUseLocalFileSystem;
  // check if remote file system can be used
  const bool enableRemoteImplementation = !canOnlyUseLocalFileSystem;
  // compute default location
  const vtkTypeUInt32 defaultLocation = !onlyBrowseRemotely || canOnlyUseLocalFileSystem
    ? vtkPVSession::CLIENT
    : vtkPVSession::DATA_SERVER;

  std::vector<std::pair<vtkTypeUInt32, QString>> locationsNames;
  if (enableLocalImplementation)
  {
    locationsNames.emplace_back(vtkPVSession::CLIENT, tr("Local File System"));
    this->Implementations[vtkPVSession::CLIENT] = new pqImplementation(this, /*server=*/nullptr);
  }
  if (enableRemoteImplementation)
  {
    locationsNames.emplace_back(vtkPVSession::DATA_SERVER, tr("Remote File System"));
    this->Implementations[vtkPVSession::DATA_SERVER] = new pqImplementation(this, server);
  }
  for (const auto& locationName : locationsNames)
  {
    const auto location = locationName.first;
    // the selected location is temporarily set here,
    // so that some slots called by signals can be executed properly.
    this->SelectedLocation = location;
    auto& impl = *this->Implementations[location];

    // set up ui for the file system
    this->Implementations[location]->Ui.setupUi(this->Implementations[location]);

    // set up ok and cancel signals/slots
    QObject::connect(impl.Ui.OK, &QPushButton::clicked, this, &pqFileDialog::accept);
    QObject::connect(impl.Ui.Cancel, &QPushButton::clicked, this, &pqFileDialog::reject);

    // ensures that the favorites and the browser component are sized proportionately.
    impl.Ui.mainSplitter->setStretchFactor(0, 1);
    impl.Ui.mainSplitter->setStretchFactor(1, 4);

    impl.Ui.Files->setEditTriggers(QAbstractItemView::EditKeyPressed);

    // install the event filter
    impl.Ui.Files->installEventFilter(this->Implementations[location]);

    // install the autocompleter
    impl.Ui.EntityName->setCompleter(impl.Completer);

    // this is the Navigate button, which is only shown when needed
    impl.Ui.Navigate->hide();

    QPixmap back = style()->standardPixmap(QStyle::SP_FileDialogBack);
    impl.Ui.NavigateBack->setIcon(back);
    impl.Ui.NavigateBack->setEnabled(false);
    impl.Ui.NavigateBack->setShortcut(QKeySequence::Back);
    impl.Ui.NavigateBack->setToolTip(
      tr("Navigate Back (%1)").arg(impl.Ui.NavigateBack->shortcut().toString()));

    QObject::connect(impl.Ui.NavigateBack, SIGNAL(clicked(bool)), this, SLOT(onNavigateBack()));
    // just flip the back image to make a forward image
    QPixmap forward = QPixmap::fromImage(back.toImage().mirrored(true, false));
    impl.Ui.NavigateForward->setIcon(forward);
    impl.Ui.NavigateForward->setDisabled(true);
    impl.Ui.NavigateForward->setShortcut(QKeySequence::Forward);
    impl.Ui.NavigateForward->setToolTip(
      tr("Navigate Forward (%1)").arg(impl.Ui.NavigateForward->shortcut().toString()));
    QObject::connect(
      impl.Ui.NavigateForward, SIGNAL(clicked(bool)), this, SLOT(onNavigateForward()));
    impl.Ui.NavigateUp->setIcon(style()->standardPixmap(QStyle::SP_FileDialogToParent));
    impl.Ui.NavigateUp->setShortcut(Qt::ALT + Qt::Key_Up);
    impl.Ui.NavigateUp->setToolTip(
      tr("Navigate Up (%1)").arg(impl.Ui.NavigateUp->shortcut().toString()));
    impl.Ui.CreateFolder->setIcon(style()->standardPixmap(QStyle::SP_FileDialogNewFolder));
    impl.Ui.CreateFolder->setShortcut(QKeySequence::New);
    impl.Ui.CreateFolder->setToolTip(
      tr("Create New Folder (%1)").arg(impl.Ui.CreateFolder->shortcut().toString()));

    impl.Ui.ShowDetail->setIcon(QIcon(":/pqWidgets/Icons/pqAdvanced.svg"));

    impl.Ui.Files->setModel(&impl.FileFilter);
    impl.Ui.Files->setSelectionBehavior(QAbstractItemView::SelectRows);

    impl.Ui.Files->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(impl.Ui.Files, SIGNAL(customContextMenuRequested(const QPoint&)), this,
      SLOT(onContextMenuRequested(const QPoint&)));

    impl.Ui.Favorites->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(impl.Ui.Favorites, SIGNAL(customContextMenuRequested(const QPoint&)), this,
      SLOT(onFavoritesContextMenuRequested(const QPoint&)));

    impl.Ui.Favorites->setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);

    auto shortcutDel = new QShortcut(QKeySequence::Delete, this);
    QObject::connect(shortcutDel, &QShortcut::activated, this,
      &pqFileDialog::onRemoveSelectedDirectoriesFromFavorites);

    impl.Ui.AddCurrentDirectoryToFavorites->setIcon(QIcon(":/QtWidgets/Icons/pqPlus.svg"));
    QObject::connect(impl.Ui.AddCurrentDirectoryToFavorites, SIGNAL(clicked()), this,
      SLOT(onAddCurrentDirectoryToFavorites()));
    impl.Ui.ResetFavortiesToSystemDefault->setIcon(QIcon(":/pqWidgets/Icons/pqReset.svg"));
    QObject::connect(impl.Ui.ResetFavortiesToSystemDefault, SIGNAL(clicked()), this,
      SLOT(onResetFavoritesToSystemDefault()));

    impl.proxyFavoriteModel = new QSortFilterProxyModel(impl.FavoriteModel);
    impl.proxyFavoriteModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    impl.proxyFavoriteModel->setSourceModel(impl.FavoriteModel);

    impl.Ui.Favorites->setModel(impl.proxyFavoriteModel);
    impl.Ui.Favorites->setSelectionMode(QAbstractItemView::ExtendedSelection);

    impl.Ui.Locations->setModel(impl.LocationModel);
    impl.Ui.Recent->setModel(impl.RecentModel);

    this->setFileMode(ExistingFile, location);

    QObject::connect(impl.Model, SIGNAL(modelReset()), this, SLOT(onModelReset()));

    QObject::connect(impl.Ui.NavigateUp, SIGNAL(clicked()), this, SLOT(onNavigateUp()));

    QObject::connect(impl.Ui.CreateFolder, SIGNAL(clicked()), this, SLOT(onCreateNewFolder()));

    QObject::connect(
      impl.Ui.Parents, SIGNAL(activated(const QString&)), this, SLOT(onNavigate(const QString&)));

    QObject::connect(impl.Ui.EntityType, SIGNAL(currentIndexChanged(const QString&)), this,
      SLOT(onFilterChange(const QString&)));

    QObject::connect(impl.Ui.Favorites, SIGNAL(clicked(const QModelIndex&)), this,
      SLOT(onClickedFavorite(const QModelIndex&)));

    QObject::connect(impl.Ui.favoritesSearchBar, &QLineEdit::textChanged, this,
      &pqFileDialog::FilterDirectoryFromFavorites);

    QObject::connect(impl.Ui.Recent, SIGNAL(clicked(const QModelIndex&)), this,
      SLOT(onClickedRecent(const QModelIndex&)));
    QObject::connect(impl.Ui.Locations, SIGNAL(clicked(const QModelIndex&)), this,
      SLOT(onClickedRecent(const QModelIndex&)));

    QObject::connect(impl.Ui.Files, SIGNAL(clicked(const QModelIndex&)), this,
      SLOT(onClickedFile(const QModelIndex&)));

    QObject::connect(impl.Ui.Files->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
      SLOT(fileSelectionChanged()));

    QObject::connect(impl.Ui.Favorites, SIGNAL(activated(const QModelIndex&)), this,
      SLOT(onActivateFavorite(const QModelIndex&)));

    QObject::connect(impl.Ui.Locations, SIGNAL(activated(const QModelIndex&)), this,
      SLOT(onActivateLocation(const QModelIndex&)));
    QObject::connect(impl.Ui.Recent, SIGNAL(activated(const QModelIndex&)), this,
      SLOT(onActivateRecent(const QModelIndex&)));

    QObject::connect(impl.Ui.Files, SIGNAL(doubleClicked(const QModelIndex&)), this,
      SLOT(onDoubleClickFile(const QModelIndex&)));

    QObject::connect(impl.Ui.EntityName, SIGNAL(textChanged(const QString&)), this,
      SLOT(onTextEdited(const QString&)));

    impl.Completer->setCaseSensitivity(Qt::CaseInsensitive);

    QStringList filterList = MakeFilterList(nameFilter);
    if (filterList.empty())
    {
      impl.Ui.EntityType->addItem(tr("All Files") + " (*)");
      impl.Filters << tr("All Files") + " (*)";
    }
    else
    {
      impl.Ui.EntityType->addItems(filterList);
      impl.Filters = filterList;
    }
    this->onFilterChange(impl.Ui.EntityType->currentText());

    QString startPath = startDirectory;
    if (startPath.isEmpty() || (!startPath.isEmpty() && location != defaultLocation))
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
    header->resizeSection(0, fm.horizontalAdvance(QLatin1String("wwwwwwwwwwwwwwwwwwwwwwwwww")));
    header->resizeSection(1, fm.horizontalAdvance(QLatin1String("mp3Folder")));
    header->resizeSection(2, fm.horizontalAdvance(QLatin1String("128.88 GB")));
    header->resizeSection(3, fm.horizontalAdvance(QLatin1String("10/29/81 02:02PM")));
    impl.Ui.Files->setSortingEnabled(true);
    impl.Ui.Files->header()->setSortIndicator(0, Qt::AscendingOrder);

    bool showDetail = impl.Model->isShowingDetailedInfo();
    impl.Ui.ShowDetail->setChecked(showDetail);
    impl.Ui.Files->setColumnHidden(2, !showDetail);
    impl.Ui.Files->setColumnHidden(3, !showDetail);

    // Group files handling
    impl.SupportsGroupFiles = supportsGroupFiles;
    impl.Ui.GroupFiles->setEnabled(impl.SupportsGroupFiles);
    impl.Ui.GroupFiles->setVisible(impl.SupportsGroupFiles);
    this->connect(impl.Ui.GroupFiles, SIGNAL(clicked(bool)), this, SLOT(onGroupFilesToggled(bool)));

    // let's manage the default button.
    impl.Ui.OK->setDefault(true);
    impl.Ui.Navigate->setDefault(false);

    this->connect(impl.Ui.Navigate, SIGNAL(clicked()), SLOT(onNavigate()));

    // Use saved state if any
    this->restoreState(location);

    // add widget as different tabs
    tabWidget->addTab(this->Implementations[location], locationName.second);
  }

  // set the QTabWidget as the central widget of the dialog
  QPointer<QVBoxLayout> layout = new QVBoxLayout(this);
  layout->addWidget(tabWidget);
  this->setLayout(layout);

  // set default location
  this->SelectedLocation = defaultLocation;

  QObject::connect(tabWidget, &QTabWidget::currentChanged, this, &pqFileDialog::onLocationChanged);
}

//-----------------------------------------------------------------------------
pqFileDialog::~pqFileDialog()
{
  this->saveState();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onCreateNewFolder()
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  // Add a directory entry with a default name to the model
  // This actually creates a directory with the given name,
  //   but this temporary directory will be deleted and a new one created
  //   once the user provides a new name for it.
  //   TODO: I guess we could insert an item into the model without
  //    actually creating a new directory but this way I could reuse code.
  QString dirName = QString("New Folder");
  int i = 0;
  QString fullpath;
  while (impl.Model->dirExists(dirName, fullpath))
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
  impl.Ui.EntityName->clear();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onContextMenuRequested(const QPoint& menuPos)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  QMenu menu(this); // Make sure to set the parent to `this` to solve #20981
  menu.setObjectName("FileDialogContextMenu");

  QModelIndex proxyItemIndex =
    this->Implementations[this->SelectedLocation]->Ui.Files->indexAt(menuPos).siblingAtColumn(0);
  QModelIndex sourceItemIndex =
    this->Implementations[this->SelectedLocation]->FileFilter.mapToSource(proxyItemIndex);

  bool isCurrentIndexADirectory =
    this->Implementations[this->SelectedLocation]->Model->isDir(sourceItemIndex);

  // Add to favorites action
  if (isCurrentIndexADirectory)
  {
    auto addToFavoritesAction = new QAction(tr("Add to favorites"), this);

    QStringList filePaths = impl.Model->getFilePaths(sourceItemIndex);
    if (filePaths.size() == 1)
    {
      QString const dirPath = filePaths.front();
      QObject::connect(
        addToFavoritesAction, &QAction::triggered, [=] { this->AddDirectoryToFavorites(dirPath); });
      menu.addAction(addToFavoritesAction);
    }
  }

  // Rename action
  if (proxyItemIndex.flags() & Qt::ItemFlag::ItemIsEditable)
  {
    auto renameAction = new QAction(tr("Rename"), this);
    QObject::connect(renameAction, &QAction::triggered,
      [=]() { this->Implementations[this->SelectedLocation]->Ui.Files->edit(proxyItemIndex); });
    menu.addAction(renameAction);
  }

  // Open in file explorer
  auto openInFileExplorerAction = new QAction(tr("Open in file explorer"), this);
  QString dirToOpen;
  if (isCurrentIndexADirectory)
  {
    dirToOpen = impl.Model->data(sourceItemIndex, Qt::UserRole).toString();
  }
  else
  {
    dirToOpen = this->Implementations[this->SelectedLocation]->Model->getCurrentPath();
  }

  QObject::connect(openInFileExplorerAction, &QAction::triggered,
    [=] { QDesktopServices::openUrl(QUrl::fromLocalFile(dirToOpen)); });
  menu.addAction(openInFileExplorerAction);

  // Only display new dir option if we're saving, not opening
  if (impl.Mode == pqFileDialog::AnyFile)
  {
    QAction* createNewDirAction = new QAction(tr("Create New Folder"), this);
    QObject::connect(createNewDirAction, SIGNAL(triggered()), this, SLOT(onCreateNewFolder()));
    menu.addAction(createNewDirAction);
  }

  // Delete directory action
  if (isCurrentIndexADirectory)
  {
    QString temp;
    QString dir = impl.Model->data(sourceItemIndex, Qt::UserRole).toString();
    if (this->Implementations[this->SelectedLocation]->Model->dirIsEmpty(dir, temp))
    {
      auto deleteDirectoryAction = new QAction(tr("Delete empty directory"), this);
      QObject::connect(deleteDirectoryAction, &QAction::triggered,
        [=]() { this->Implementations[this->SelectedLocation]->Model->rmdir(dir); });
      menu.addAction(deleteDirectoryAction);
    }
  }

  // Show hidden files action
  QAction* showHiddenFilesAction = new QAction(tr("Show Hidden Files"), this);
  showHiddenFilesAction->setCheckable(true);
  showHiddenFilesAction->setChecked(impl.FileFilter.getShowHidden());
  QObject::connect(
    showHiddenFilesAction, SIGNAL(triggered(bool)), this, SLOT(onShowHiddenFiles(bool)));
  menu.addAction(showHiddenFilesAction);

  menu.exec(impl.Ui.Files->viewport()->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
void pqFileDialog::AddDirectoryToFavorites(QString const& directory)
{
  this->Implementations[this->SelectedLocation]->FavoriteModel->addToFavorites(directory);
}

//-----------------------------------------------------------------------------
void pqFileDialog::RemoveDirectoryFromFavorites(QString const& directory)
{
  this->Implementations[this->SelectedLocation]->FavoriteModel->removeFromFavorites(directory);
}

//-----------------------------------------------------------------------------
void pqFileDialog::FilterDirectoryFromFavorites(const QString& filter)
{
  this->Implementations[this->SelectedLocation]->proxyFavoriteModel->setFilterRegularExpression(
    filter);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onAddCurrentDirectoryToFavorites()
{
  QString const currentPath =
    this->Implementations[this->SelectedLocation]->Model->getCurrentPath();
  this->AddDirectoryToFavorites(currentPath);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onRemoveSelectedDirectoriesFromFavorites()
{
  QStringList selectedDirs;
  for (const QModelIndex& index : this->Implementations[this->SelectedLocation]
                                    ->Ui.Favorites->selectionModel()
                                    ->selectedIndexes())
  {
    QString dirPath = this->Implementations[this->SelectedLocation]->FavoriteModel->filePath(index);
    selectedDirs.push_back(dirPath);
  }

  for (const QString& dir : selectedDirs)
  {
    this->Implementations[this->SelectedLocation]->FavoriteModel->removeFromFavorites(dir);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onResetFavoritesToSystemDefault()
{
  int const ret = QMessageBox::warning(this, tr("Clear favorites"),
    tr("This will clear the favorites list.\nAre you sure you want to continue ?"),
    QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);

  if (ret == QMessageBox::StandardButton::Yes)
  {
    this->Implementations[this->SelectedLocation]->FavoriteModel->resetFavoritesToDefault();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onFavoritesContextMenuRequested(const QPoint& menuPos)
{
  QMenu menu(this); // Make sure to set the parent to `this` to solve #21680
  menu.setObjectName("FileDialogFavoritesContextMenu");

  QModelIndex index = this->Implementations[this->SelectedLocation]->Ui.Favorites->indexAt(menuPos);
  if (index.isValid())
  {
    QString dirPath = this->Implementations[this->SelectedLocation]->FavoriteModel->filePath(index);
    if (!dirPath.isEmpty())
    {
      auto removeFromFavorites = new QAction(tr("Remove from favorites"), this);
      QObject::connect(removeFromFavorites, &QAction::triggered,
        [=] { this->onRemoveSelectedDirectoriesFromFavorites(); });
      menu.addAction(removeFromFavorites);

      auto renameLabel = new QAction(tr("Rename label"), this);
      QObject::connect(renameLabel, &QAction::triggered,
        [=] { this->Implementations[this->SelectedLocation]->Ui.Favorites->edit(index); });
      menu.addAction(renameLabel);
    }
  }

  menu.exec(
    this->Implementations[this->SelectedLocation]->Ui.Favorites->viewport()->mapToGlobal(menuPos));
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void pqFileDialog::setFileMode(FileMode mode, vtkTypeUInt32 location)
{
  auto& impl = *this->Implementations[location];
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

  impl.Model->setDirectoryItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  impl.Model->setFileItemFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
  switch (mode)
  {
    case Directory:
      // final selectable entities will be limited to directories.
      impl.Ui.EntityNameLabel->setText(tr("Directory name:"));
      impl.Ui.EntityTypeLabel->setVisible(false);
      impl.Ui.EntityType->setVisible(false);
      impl.Model->setFileItemFlags(Qt::NoItemFlags);
      break;
    case ExistingFilesAndDirectories:
      // final selectable entities can be files or directories.
      impl.Ui.EntityNameLabel->setText(tr("Name:"));
      impl.Ui.EntityTypeLabel->setVisible(true);
      impl.Ui.EntityType->setVisible(true);
      break;
    default:
      // final selectable entities will be limited to files.
      impl.Ui.EntityNameLabel->setText(tr("File name:"));
      impl.Ui.EntityTypeLabel->setVisible(true);
      impl.Ui.EntityType->setVisible(true);
      break;
  }

  if (setupMultipleFileHelp)
  {
    // only set the tooltip and window title the first time through
    impl.ShowMultipleFileHelp = true;
    this->setWindowTitle(this->windowTitle() + "  " + tr("open multiple files with <ctrl> key.)"));
    this->setToolTip(tr("open multiple files with <ctrl> key."));
  }
  impl.Ui.Files->setSelectionMode(selectionMode);

  impl.Ui.Navigate->show();
  impl.Ui.Navigate->setEnabled(false);
  impl.Ui.CreateFolder->setVisible(
    impl.Mode == pqFileDialog::AnyFile || impl.Mode == pqFileDialog::Directory);
  this->updateButtonStates(location);
}

//-----------------------------------------------------------------------------
void pqFileDialog::setFileMode(FileMode mode)
{
  if (this->Implementations.find(vtkPVSession::CLIENT) != this->Implementations.end())
  {
    this->setFileMode(mode, vtkPVSession::CLIENT);
  }
  if (this->Implementations.find(vtkPVSession::DATA_SERVER) != this->Implementations.end())
  {
    this->setFileMode(mode, vtkPVSession::DATA_SERVER);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::setRecentlyUsedExtension(const QString& fileExtension, vtkTypeUInt32 location)
{
  auto& impl = *this->Implementations[location];

  if (fileExtension == QString())
  {
    // upon the initial use of any kind (e.g., data or screenshot) of dialog
    // 'fileExtension' is equal /set to an empty string.
    // In this case, no any user preferences are considered
    impl.Ui.EntityType->setCurrentIndex(0);
  }
  else
  {
    int index = impl.Ui.EntityType->findText(fileExtension, Qt::MatchContains);
    // just in case the provided extension is not in the combobox list
    index = (index == -1) ? 0 : index;
    impl.Ui.EntityType->setCurrentIndex(index);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::setRecentlyUsedExtension(const QString& fileExtension)
{
  const auto currentLocation = this->SelectedLocation;
  // the selected location is temporarily set here,
  // so that onFilterChange can be executed properly for both locations
  if (this->Implementations.find(vtkPVSession::CLIENT) != this->Implementations.end())
  {
    this->SelectedLocation = vtkPVSession::CLIENT;
    this->setRecentlyUsedExtension(fileExtension, vtkPVSession::CLIENT);
  }
  if (this->Implementations.find(vtkPVSession::DATA_SERVER) != this->Implementations.end())
  {
    this->SelectedLocation = vtkPVSession::DATA_SERVER;
    this->setRecentlyUsedExtension(fileExtension, vtkPVSession::DATA_SERVER);
  }
  this->SelectedLocation = currentLocation;
}

//-----------------------------------------------------------------------------
void pqFileDialog::addToFilesSelected(const QStringList& files)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  // Ensure that we are hidden before broadcasting the selection,
  // so we don't get caught by screen-captures
  this->setVisible(false);
  impl.SelectedFiles.append(files);
}

//-----------------------------------------------------------------------------
void pqFileDialog::emitFilesSelectionDone()
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  Q_EMIT filesSelected(impl.SelectedFiles);
  if (impl.Mode != this->ExistingFiles && !impl.SelectedFiles.empty())
  {
    Q_EMIT filesSelected(impl.SelectedFiles[0]);
  }
  this->done(QDialog::Accepted);
}

//-----------------------------------------------------------------------------
QList<QStringList> pqFileDialog::getAllSelectedFiles()
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  return impl.SelectedFiles;
}

//-----------------------------------------------------------------------------
QStringList pqFileDialog::getSelectedFiles(int index)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  if (index < 0 || index >= impl.SelectedFiles.size())
  {
    return QStringList();
  }
  return impl.SelectedFiles[index];
}

//-----------------------------------------------------------------------------
int pqFileDialog::getSelectedFilterIndex()
{
  return this->Implementations[this->SelectedLocation]->SelectedFilterIndex;
}

//-----------------------------------------------------------------------------
void pqFileDialog::accept()
{
  auto& impl = *this->Implementations[this->SelectedLocation];

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
    Q_EMIT this->emitFilesSelectionDone();
  }
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptExistingFiles()
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  bool loadedFiles = false;
  QString filename;
  if (impl.FileNames.empty())
  {
    // when we have nothing selected in the current selection model, we will
    // attempt to use the default way
    return this->acceptDefault(true);
  }
  Q_FOREACH (filename, impl.FileNames)
  {
    filename = filename.trimmed();

    QString fullFilePath = impl.Model->absoluteFilePath(filename);
    Q_EMIT this->fileAccepted(fullFilePath);
    loadedFiles = (this->acceptInternal(this->buildFileGroup(filename)) || loadedFiles);
  }
  return loadedFiles;
}

//-----------------------------------------------------------------------------
bool pqFileDialog::acceptDefault(const bool& checkForGrouping)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  QString filename = impl.Ui.EntityName->text();
  filename = filename.trimmed();

  QString fullFilePath = impl.Model->absoluteFilePath(filename);
  Q_EMIT this->fileAccepted(fullFilePath);

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
  auto& impl = *this->Implementations[this->SelectedLocation];

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
void pqFileDialog::onLocationChanged(int location)
{
  if (this->Implementations.size() > 1)
  {
    this->SelectedLocation = location == 0 ? vtkPVSession::CLIENT : vtkPVSession::DATA_SERVER;
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onModelReset()
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  impl.Ui.Parents->clear();

  QString currentPath = impl.Model->getCurrentPath();
  // clean the path to always look like a unix path
  currentPath = QDir::cleanPath(currentPath);

  // the separator is always the unix separator
  QChar separator = '/';

  QStringList parents = currentPath.split(separator, PV_QT_SKIP_EMPTY_PARTS);

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
  disconnect(impl.Ui.ShowDetail, SIGNAL(clicked(bool)), nullptr, nullptr);
  connect(impl.Ui.ShowDetail, SIGNAL(clicked(bool)), this, SLOT(onShowDetailToggled(bool)));
  bool showDetail = impl.Model->isShowingDetailedInfo();
  impl.Ui.ShowDetail->setChecked(showDetail);
  impl.Ui.Files->setColumnHidden(2, !showDetail);
  impl.Ui.Files->setColumnHidden(3, !showDetail);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigate(const QString& newpath)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  QString path_to_navigate(newpath);
  if (newpath.isEmpty() && !impl.FileNames.empty())
  {
    path_to_navigate = impl.FileNames.front();
    path_to_navigate = impl.Model->absoluteFilePath(path_to_navigate);
  }

  impl.addHistory(impl.Model->getCurrentPath());
  impl.setCurrentPath(path_to_navigate);
  this->updateButtonStates(this->SelectedLocation);
  impl.Ui.EntityName->clear();
  impl.Ui.EntityName->setFocus(Qt::OtherFocusReason);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateUp()
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  impl.addHistory(impl.Model->getCurrentPath());
  QFileInfo info(impl.Model->getCurrentPath());
  impl.setCurrentPath(info.path());
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateDown(const QModelIndex& idx)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

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
  auto& impl = *this->Implementations[this->SelectedLocation];
  QString path = impl.backHistory();
  impl.setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onNavigateForward()
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  QString path = impl.forwardHistory();
  impl.setCurrentPath(path);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onFilterChange(const QString& filter)
{
  auto& impl = *this->Implementations[this->SelectedLocation];

  // set filter on proxy
  impl.FileFilter.setFilter(filter);

  // update view
  impl.FileFilter.invalidate();
  impl.Ui.EntityType->setToolTip(impl.Ui.EntityType->currentText());

  impl.SelectedFilterIndex = impl.Ui.EntityType->currentIndex();

  this->updateButtonStates(this->SelectedLocation);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFavorite(const QModelIndex&)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedRecent(const QModelIndex&)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Ui.Files->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onClickedFile(const QModelIndex& vtkNotUsed(index))
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Ui.Favorites->clearSelection();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateFavorite(const QModelIndex& index)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  if (impl.FavoriteModel->isDirectory(index))
  {
    QString file = impl.FavoriteModel->filePath(index);
    this->onNavigate(file);
    impl.Ui.EntityName->selectAll();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateLocation(const QModelIndex& index)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  if (impl.LocationModel->isDirectory(index))
  {
    QString file = impl.LocationModel->filePath(index);
    this->onNavigate(file);
    impl.Ui.EntityName->selectAll();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::onActivateRecent(const QModelIndex& index)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  QString file = impl.RecentModel->filePath(index);
  this->onNavigate(file);
  impl.Ui.EntityName->selectAll();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onDoubleClickFile(const QModelIndex&)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  QScopedValueRollback<bool> setter(impl.InDoubleClickHandler, true);
  this->accept();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowHiddenFiles(const bool& hidden)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.FileFilter.setShowHidden(hidden);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onShowDetailToggled(bool show)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Model->setShowDetailedInfo(show);
  impl.Ui.Files->setColumnHidden(2, !show);
  impl.Ui.Files->setColumnHidden(3, !show);
}

//-----------------------------------------------------------------------------
void pqFileDialog::onGroupFilesToggled(bool group)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Model->setGroupFiles(group);
}

//-----------------------------------------------------------------------------
void pqFileDialog::setShowHidden(const bool& hidden)
{
  this->onShowHiddenFiles(hidden);
}

//-----------------------------------------------------------------------------
bool pqFileDialog::getShowHidden()
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  return impl.FileFilter.getShowHidden();
}

//-----------------------------------------------------------------------------
void pqFileDialog::onTextEdited(const QString& str)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  impl.Ui.Favorites->clearSelection();

  // really important to block signals so that the clearSelection
  // doesn't cause a signal to be fired that calls fileSelectionChanged
  impl.Ui.Files->blockSignals(true);
  impl.Ui.Files->clearSelection();
  if (str.size() > 0)
  {
    // convert the typed information to be impl.FileNames
    impl.FileNames = str.split(impl.FileNamesSeperator, PV_QT_SKIP_EMPTY_PARTS);
  }
  else
  {
    impl.FileNames.clear();
  }
  impl.Ui.Files->blockSignals(false);
  this->updateButtonStates(this->SelectedLocation);
}

//-----------------------------------------------------------------------------
QString pqFileDialog::fixFileExtension(const QString& filename, const QString& filter)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
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
    Q_FOREACH (QString curfilter, impl.Filters)
    {
      wildCards += ::GetWildCardsFromFilter(curfilter);
    }
    bool pass = false;
    Q_FOREACH (QString wildcard, wildCards)
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

  auto& impl = *this->Implementations[this->SelectedLocation];
  QString file = selected_files[0];

  if (file.isEmpty() && (impl.Mode == Directory || impl.Mode == ExistingFilesAndDirectories))
  {
    this->addToFilesSelected(QStringList(impl.Model->getCurrentPath()));
    return true;
  }

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
        impl.Ui.EntityName->clear();
        break;
    }
    return false;
  }

  // User chose an existing file or a brand new filename.
  if (impl.Mode == pqFileDialog::AnyFile)
  {
    // If mode is a "save" dialog, we fix the extension first.
    file = this->fixFileExtension(file, impl.Ui.EntityType->currentText());

    // It is very possible that after fixing the extension,
    // the new filename is an already present directory,
    // hence we handle that case:
    if (impl.Model->dirExists(file, file))
    {
      this->onNavigate(file);
      impl.Ui.EntityName->clear();
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
        impl.Ui.EntityName->clear();
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
        impl.Ui.EntityName->selectAll();
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
  auto& impl = *this->Implementations[this->SelectedLocation];
  // Selection changed, update the EntityName entry box
  // to reflect the current selection.
  QString fileString;
  const QModelIndexList indices = impl.Ui.Files->selectionModel()->selectedIndexes();
  const QModelIndexList rows = impl.Ui.Files->selectionModel()->selectedRows();

  if (indices.isEmpty())
  {
    // do not change the EntityName text if no selections
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
  impl.Ui.EntityName->blockSignals(true);
  impl.Ui.EntityName->setText(fileString);
  impl.Ui.EntityName->blockSignals(false);

  impl.FileNames = fileNames;
  this->updateButtonStates(this->SelectedLocation);
}

//-----------------------------------------------------------------------------
bool pqFileDialog::selectFile(const QString& f)
{
  auto& impl = *this->Implementations[this->SelectedLocation];
  // We don't use QFileInfo here since it messes the paths up if the client and
  // the server are heterogeneous systems.
  std::string unix_path = f.toUtf8().data();
  vtksys::SystemTools::ConvertToUnixSlashes(unix_path);

  std::string filename, dirname;
  std::string::size_type slashPos = unix_path.rfind('/');
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
  impl.Ui.EntityName->setText(filename.c_str());
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
  auto& impl = *this->Implementations[this->SelectedLocation];
  QDialog::showEvent(_showEvent);
  // Qt sets the default keyboard focus to the last item in the tab order
  // which is determined by the creation order. This means that we have
  // to explicitly state that the line edit has the focus on showing no
  // matter the tab order
  impl.Ui.EntityName->setFocus(Qt::OtherFocusReason);
}

//-----------------------------------------------------------------------------
QPair<QString, vtkTypeUInt32> pqFileDialog::getSaveFileNameAndLocation(pqServer* server,
  QWidget* parentWdg, const QString& title, const QString& directory, const QString& filter,
  bool supportGroupFiles, bool onlyBrowseRemotely)
{
  pqFileDialog fileDialog(
    server, parentWdg, title, directory, filter, supportGroupFiles, onlyBrowseRemotely);
  fileDialog.setObjectName("FileOpenDialog");
  fileDialog.setFileMode(pqFileDialog::AnyFile);
  if (fileDialog.exec() == QDialog::Accepted)
  {
    return { fileDialog.getSelectedFiles()[0], fileDialog.getSelectedLocation() };
  }
  return { QString(), fileDialog.getSelectedLocation() };
}

//-----------------------------------------------------------------------------
void pqFileDialog::saveState(vtkTypeUInt32 location)
{
  auto& impl = *this->Implementations[location];
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

    if (impl.SupportsGroupFiles)
    {
      settings->setValue("GroupFiles", impl.Ui.GroupFiles->isChecked() ? 1 : 0);
    }

    settings->endGroup();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::saveState()
{
  if (this->Implementations.find(vtkPVSession::CLIENT) != this->Implementations.end())
  {
    this->saveState(vtkPVSession::CLIENT);
  }
  if (this->Implementations.find(vtkPVSession::DATA_SERVER) != this->Implementations.end())
  {
    this->saveState(vtkPVSession::DATA_SERVER);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::restoreState(vtkTypeUInt32 location)
{
  auto& impl = *this->Implementations[location];
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

    if (impl.SupportsGroupFiles)
    {
      bool groupFiles = settings->value("GroupFiles", true).toBool();
      impl.Ui.GroupFiles->setChecked(groupFiles);
      this->onGroupFilesToggled(groupFiles);
    }
    else
    {
      this->onGroupFilesToggled(false);
    }
    settings->endGroup();
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::restoreState()
{
  if (this->Implementations.find(vtkPVSession::CLIENT) != this->Implementations.end())
  {
    this->restoreState(vtkPVSession::CLIENT);
  }
  if (this->Implementations.find(vtkPVSession::DATA_SERVER) != this->Implementations.end())
  {
    this->restoreState(vtkPVSession::DATA_SERVER);
  }
}

//-----------------------------------------------------------------------------
void pqFileDialog::updateButtonStates(vtkTypeUInt32 location)
{
  auto& impl = *this->Implementations[location];

  if (impl.FileNames.empty())
  {
    QString const currentDirName = QFileInfo(impl.Model->getCurrentPath()).fileName();
    // Enables "Ok" only if the current directory can be opened
    impl.Ui.OK->setEnabled((impl.Mode == Directory || impl.Mode == ExistingFilesAndDirectories) &&
      impl.FileFilter.getWildcards().exactMatch(currentDirName));

    impl.Ui.Navigate->setEnabled(false);
    return;
  }

  bool is_dir = false;
  if (impl.FileNames.size() == 1)
  {
    QString tmp;
    is_dir = impl.Model->dirExists(impl.FileNames.front(), tmp);
  }

  switch (impl.Mode)
  {
    case Directory:
      // if mode is Directory, update OK button state.
      impl.Ui.OK->setEnabled(is_dir);
      break;
    case AnyFile:
      impl.Ui.OK->setEnabled(true);
      break;
    default:
      // Check that the files match the selected filter (without existence checks)
      const bool filesMatching = std::all_of(
        impl.FileNames.begin(), impl.FileNames.end(), [&](QString const& fileOrGroupName) {
          // Get all the files of the group if fileName is a group, else just get {fileName}
          const QStringList fileNames = buildFileGroup(fileOrGroupName);
          return std::all_of(fileNames.begin(), fileNames.end(), [&](QString const& fileName) {
            const QString fileNameWithoutPath =
              vtksys::SystemTools::GetFilenameName(fileName.toStdString()).c_str();
            return impl.FileFilter.getWildcards().exactMatch(fileNameWithoutPath);
          });
        });
      impl.Ui.OK->setEnabled(filesMatching);
  }

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
