/*=========================================================================

   Program: ParaView
   Module:    pqFileDialogModel.h

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

#ifndef _pqFileDialogModel_h
#define _pqFileDialogModel_h

#include "pqCoreModule.h"
#include <QAbstractItemModel>
#include <QFileIconProvider>
#include <QObject>

#include "vtkPVFileInformation.h"
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

  //@{
  /**
   * Get/Sets whether the dialog shows additional information about the files
   * like modification time and file size.  This information can be
   * time consuming to display if the server has many files in one directory
   * so it is not displayed by default.
   */
  void setShowDetailedInfo(bool show);
  bool isShowingDetailedInfo();
  //@}

  /**
  * Sets the path that the file dialog will display
  */
  void setCurrentPath(const QString&);

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
  bool isDir(const QModelIndex&);

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
  * returns the path delimiter, could be \ or / depending on the platform
  * this model is browsing
  */
  QChar separator() const;

  /**
  * return the absolute path for this file
  */
  QString absoluteFilePath(const QString&);

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

  QIcon FolderLinkIcon;
  QIcon FileLinkIcon;
  QIcon DomainIcon;
  QIcon NetworkIcon;
};

#endif // !_pqFileDialogModel_h
