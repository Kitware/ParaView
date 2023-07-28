// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#ifndef pqFileDialogModel_h
#define pqFileDialogModel_h

#include "pqCoreModule.h"

#include "vtkPVFileInformation.h"
#include "vtkParaViewDeprecation.h" // for deprecation

#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QObject>

class vtkProcessModule;
class pqServer;
class QModelIndex;

/**
pqFileDialogModel allows remote browsing of a connected ParaView server's
filesystem, as well as browsing of the local file system.

To use, pass a new instance of pqServerFileDialogModel to pqFileDialog object.

\sa pqFileDialog
*/
class PQCORE_EXPORT pqFileDialogModel : public QAbstractItemModel
{
  typedef QAbstractItemModel base;

  Q_OBJECT

public:
  /**
   * server is the server for which we need the listing.
   * if the server is nullptr, we get file listings locally
   */
  pqFileDialogModel(pqServer* server, QObject* Parent = nullptr);
  ~pqFileDialogModel() override;

  /**
   * set the flags for items of type file
   */
  void setFileItemFlags(const Qt::ItemFlags& flags);

  /**
   * set the flags for items of type directory
   */
  void setDirectoryItemFlags(const Qt::ItemFlags& flags);

  ///@{
  /**
   * Get/Sets whether the dialog shows additional information about the files
   * like modification time and file size.  This information can be
   * time consuming to display if the server has many files in one directory
   * so it is not displayed by default.
   */
  void setShowDetailedInfo(bool show);
  bool isShowingDetailedInfo();
  ///@}

  ///@{
  /**
   * Get/Sets whether the dialog should group numbered files together
   * into a single file when the sequencs numbers are replaced by `..`.
   */
  void setGroupFiles(bool group);
  bool isGroupingFiles();
  ///@}

  /**
   * Sets groupFiles to the provided value then set the path that the file dialog will display.
   */
  PARAVIEW_DEPRECATED_IN_5_12_0("Use setGroupFiles(bool) and setCurrentPath(path) instead.")
  void setCurrentPath(const QString& path, bool groupFiles);

  /**
   * Set the path that the file dialog will display.
   */
  void setCurrentPath(const QString& path);

  /**
   * Returns the path the the file dialog will display
   */
  QString getCurrentPath();

  /**
   * Return true if the file at the index is hidden
   */
  bool isHidden(const QModelIndex&);

  /**
   * Return true if the given row is a directory
   */
  bool isDir(const QModelIndex&) const;

  // Creates a directory. "dirName" can be relative or absolute path
  bool mkdir(const QString& dirname);

  // Removes a directory. "dirName" can be relative or absolute path
  bool rmdir(const QString& dirname);

  // Renames a directory or file.
  bool rename(const QString& oldname, const QString& newname);

  /**
   * Returns whether the file exists
   * also returns the full path, which could be a resolved shortcut
   */
  bool fileExists(const QString& file, QString& fullpath);

  /**
   * Returns whether a directory exists
   * also returns the full path, which could be a resolved shortcut
   */
  bool dirExists(const QString& dir, QString& fullpath);

  /**
   * Returns true if a directory exists and is empty
   * also returns the full path, which could be a resolved shortcut
   */
  bool dirIsEmpty(const QString& dir, QString& fullpath);

  /**
   * returns the path delimiter, could be \ or / depending on the platform
   * this model is browsing
   */
  QChar separator() const;

  /**
   * return the absolute path for this file
   */
  QString absoluteFilePath(const QString&);

  /**
   * return the file type of a file
   */
  int fileType(const QString&);

  /**
   * Returns the set of file paths associated with the given row
   * (a row may represent one-to-many paths if grouping is implemented)
   * this also resolved symlinks if necessary
   */
  QStringList getFilePaths(const QModelIndex&);

  /**
   * Returns the server that this model is browsing
   */
  pqServer* server() const;

  /**
   * sets data (used by the view when editing names of folders)
   */
  bool setData(const QModelIndex& idx, const QVariant& value, int role) override;

  // overloads for QAbstractItemModel

  /**
   * return the number of columns in the model
   */
  int columnCount(const QModelIndex&) const override;
  /**
   * return the data for an item
   */
  QVariant data(const QModelIndex& idx, int role) const override;
  /**
   * return an index from another index
   */
  QModelIndex index(int row, int column, const QModelIndex&) const override;
  /**
   * return the parent index of an index
   */
  QModelIndex parent(const QModelIndex&) const override;
  /**
   * return the number of rows under a given index
   */
  int rowCount(const QModelIndex&) const override;
  /**
   * return whether a given index has children
   */
  bool hasChildren(const QModelIndex& p) const override;
  /**
   * returns header data
   */
  QVariant headerData(int section, Qt::Orientation, int role) const override;
  /**
   * returns flags for item
   */
  Qt::ItemFlags flags(const QModelIndex& idx) const override;

private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

class pqFileDialogModelIconProvider : protected QFileIconProvider
{
public:
  enum IconType
  {
    Invalid,
    Computer,
    Drive,
    Folder,
    File,
    FolderLink,
    FileLink,
    NetworkRoot,
    NetworkDomain,
    NetworkFolder
  };
  pqFileDialogModelIconProvider();
  QIcon icon(IconType t) const;
  QIcon icon(vtkPVFileInformation::FileTypes f) const;

protected:
  QIcon icon(const QFileInfo& info) const override;
  QIcon icon(QFileIconProvider::IconType ico) const override;

  QIcon InvalidIcon;
  QIcon FolderLinkIcon;
  QIcon FileLinkIcon;
  QIcon DomainIcon;
  QIcon NetworkIcon;
};

#endif // !pqFileDialogModel_h
