// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialog_h
#define pqFileDialog_h

#include "pqCoreModule.h"

#include "vtkType.h" // needed for vtkTypeUInt32

#include <QDialog>
#include <QMap>
#include <QStringList>

class QModelIndex;
class QPoint;
class pqServer;
class QShowEvent;

/**
  Provides a standard file dialog "front-end" for the pqFileDialogModel
  "back-end", i.e. it can be used for both local and remote file browsing.

  pqFileDialog can be used in both "modal" and "non-modal" operations.
  For "non-modal" operation, create an instance of pqFileDialog on the heap,
  set the Qt::WA_DeleteOnClose flag, connect to the fileSelected() signal,
  and show the dialog.  The dialog will be automatically destroyed when the
  user completes their file selection, and your slot will be called with
  the files the user selected:

  /code
  pqFileDialog* dialog = new pqFileDialog(nullptr, this);
  dialog->setAttribute(Qt::WA_DeleteOnClose);

  QObject::connect(
    dialog,
    SIGNAL(filesSelected(const QList<QStringList>&)),
    this,
    SLOT(onOpenSessionFile(const QList<QStringList>&)));

  dialog->show();
  /endcode

  For "modal" operation, create an instance of pqFileDialog on the stack,
  call its exec() method, and retrieve the user's file selection with the
  getSelectedFiles() method:

  /code
  pqFileDialog dialog(nullptr, this);
  if(Qt::Accepted == dialog.exec())
    {
    //each string list holds a list of files that represent a file-series
    QList<QStringList> files = dialog.getAllSelectedFiles();
    }
  /endcode

  \sa pqFileDialogModel
*/

class PQCORE_EXPORT pqFileDialog : public QDialog
{
  typedef QDialog Superclass;
  Q_OBJECT
public:
  /**
   * choose mode for selecting file/folder.
   * \li \c AnyFile:
   *         The name of a file, whether it exists or not. Typically used by "Save As..."
   * \li \c ExistingFile:
   *         The name of a single existing file. Typically used by "Open..."
   *         This mode allows the user to select a single file, or a group of files.
   * \li \c ExistingFiles:
   *         The names of zero or more existing files (or groups of
   *         files). Typically used by "Open..." when you want multiple file selection.
   *         This mode allows the user to select multiples files, and multiple time series groups at
   * the
   *         same time.
   * \li \c Directory:
   *         The name of a directory.
   * \li \c ExistingFilesAndDirectories:
   *         This mode is combination of `ExistingFiles` and `Directory` where
   *         either a collection of files or directories can be selected.
   */
  enum FileMode
  {
    AnyFile,
    ExistingFile,
    ExistingFiles,
    Directory,
    ExistingFilesAndDirectories
  };

  /**
   * Creates a file dialog with the specified server
   * if the server is nullptr, files are browsed locally. else remotely and optionally locally.
   * the title, and start directory may be specified
   * the filter is a string of semi-colon separated filters
   * if supportGroupFiles is true, then file sequences will support being grouped into a file name
   * where the sequence numbers are replaced by `..`
   *
   * if onlyBrowseRemotely is false,
   *    and server == nullptr, then you can only browse locally, and defaults to local.
   *    and server != nullptr, then you can browse locally and remotely, and defaults to local.
   * if onlyBrowseRemotely is true,
   *    and server == nullptr, then you can only browse locally, and defaults to local.
   *    and server != nullptr, then you can browse locally and remotely, and defaults to remote.
   */
  pqFileDialog(pqServer* server, QWidget* parent, const QString& title = QString(),
    const QString& directory = QString(), const QString& filter = QString(),
    bool supportGroupFiles = true, bool onlyBrowseRemotely = true);
  ~pqFileDialog() override;

  ///@{
  /**
   * set the file mode
   */
  void setFileMode(FileMode, vtkTypeUInt32);
  void setFileMode(FileMode);
  ///@}

  ///@{
  /**
   * set the most recently used file extension
   */
  void setRecentlyUsedExtension(const QString& fileExtension, vtkTypeUInt32 location);
  void setRecentlyUsedExtension(const QString& fileExtension);
  ///@}

  /**
   * Returns the group of files for the given index
   */
  QStringList getSelectedFiles(int index = 0);

  /**
   * Returns all the file groups
   */
  QList<QStringList> getAllSelectedFiles();

  /**
   * Return the used filter index
   */
  int getSelectedFilterIndex();

  /**
   * accept this dialog
   */
  void accept() override;

  /**
   * set a file current to support test playback
   */
  bool selectFile(const QString&);

  /**
   * set if we show hidden files and holders
   */
  void setShowHidden(const bool& hidden);

  /**
   *returns the state of the show hidden flag
   */
  bool getShowHidden();

  /**
   * Get the location that the selected files/directories belong to.
   * The only return values (as of now) are vtkPVSession::CLIENT, vtkPVSession::DATA_SERVER.
   */
  vtkTypeUInt32 getSelectedLocation() const { return this->SelectedLocation; }

  ///@{
  /**
   * static method similar to QFileDialog::getSaveFileName(...) to make it
   * easier to get a file name to save a file as.
   */
  static QString getSaveFileName(pqServer* server, QWidget* parentWdg,
    const QString& title = QString(), const QString& directory = QString(),
    const QString& filter = QString())
  {
    const QPair<QString, vtkTypeUInt32> result =
      pqFileDialog::getSaveFileNameAndLocation(server, parentWdg, title, directory, filter);
    return result.first;
  }
  static QPair<QString, vtkTypeUInt32> getSaveFileNameAndLocation(pqServer* server,
    QWidget* parentWdg, const QString& title = QString(), const QString& directory = QString(),
    const QString& filter = QString(), bool supportGroupFiles = false,
    bool onlyBrowseRemotely = true);
  ///@}
Q_SIGNALS:
  /**
   * Signal emitted when the user has chosen a set of files
   */
  void filesSelected(const QList<QStringList>&);

  /**
   * Signal emitted when the user has chosen a set of files
   * NOTE:
   * The mode has to be not ExistingFiles for this signal to be emitted!
   * This signal is deprecated and should not be used anymore. Instead
   * use the fileSelected(const QList<QStringList> &)
   */
  void filesSelected(const QStringList&);

  /**
   * signal emitted when user has chosen a set of files and accepted the
   * dialog.  This signal includes only the path and file string as is
   * This is to support test recording
   */
  void fileAccepted(const QString&);

protected:
  bool acceptExistingFiles();
  bool acceptDefault(const bool& checkForGrouping);

  QStringList buildFileGroup(const QString& filename);

  void showEvent(QShowEvent* showEvent) override;

private Q_SLOTS:
  void onLocationChanged(int fs);
  void onModelReset();
  void onNavigate(const QString& = QString());
  void onNavigateUp();
  void onNavigateBack();
  void onNavigateForward();
  void onNavigateDown(const QModelIndex&);
  void onFilterChange(const QString&);

  void onClickedRecent(const QModelIndex&);
  void onClickedFavorite(const QModelIndex&);
  void onClickedFile(const QModelIndex&);

  void onActivateFavorite(const QModelIndex&);
  void onActivateLocation(const QModelIndex&);
  void onActivateRecent(const QModelIndex&);
  void onDoubleClickFile(const QModelIndex&);

  void onTextEdited(const QString&);

  void onShowHiddenFiles(const bool& hide);

  void onShowDetailToggled(bool show);

  void onGroupFilesToggled(bool group);

  // Called when the user changes the file selection.
  void fileSelectionChanged();

  // Called when the user right-clicks in the file qtreeview
  void onContextMenuRequested(const QPoint& pos);

  // Called when the user right-clicks in the favorites qlistview
  void onFavoritesContextMenuRequested(const QPoint& pos);

  void AddDirectoryToFavorites(QString const&);
  void RemoveDirectoryFromFavorites(QString const&);
  void FilterDirectoryFromFavorites(const QString& filter);

  void onAddCurrentDirectoryToFavorites();
  void onRemoveSelectedDirectoriesFromFavorites();
  void onResetFavoritesToSystemDefault();

  // Called when the user requests to create a new directory in the cwd
  void onCreateNewFolder();

  /**
   * Adds this grouping of files to the files selected list
   */
  void addToFilesSelected(const QStringList&);

  /**
   * Emits the filesSelected() signal and closes the dialog,
   */
  void emitFilesSelectionDone();

  /**
   * This is called every time the set of chosen files may have changed. Here,
   * we update the visibility and enabled state of the `OK` and `Navigate`
   * buttons.
   */
  void updateButtonStates(vtkTypeUInt32 fileSystem);

private: // NOLINT(readability-redundant-access-specifiers)
  pqFileDialog(const pqFileDialog&);
  pqFileDialog& operator=(const pqFileDialog&);

  class pqImplementation;
  QMap<vtkTypeUInt32, pqImplementation*> Implementations;
  vtkTypeUInt32 SelectedLocation;

  // returns if true if files are loaded
  bool acceptInternal(const QStringList& selected_files);
  QString fixFileExtension(const QString& filename, const QString& filter);

  ///@{
  /**
   * save current state of dialog(size, position, splitters and position of files header)
   */
  void saveState(vtkTypeUInt32 fileSystem);
  void saveState();
  ///@}

  ///@{
  /**
   * restore state of dialog
   */
  void restoreState(vtkTypeUInt32 fileSystem);
  void restoreState();
  ///@}
};

#endif // pqFileDialog_h
