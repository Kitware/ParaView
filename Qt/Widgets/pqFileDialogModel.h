/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqFileDialogModel_h
#define _pqFileDialogModel_h

#include <QObject>

class QAbstractItemModel;
class QModelIndex;
class QString;

/// Abstract interface to a file-browsing "back-end" that can be used by the pqFileDialog "front-end"
class pqFileDialogModel :
  public QObject
{
  Q_OBJECT

public:
  ~pqFileDialogModel();
  
  /// Returns the path that will be automatically displayed when the file dialog is opened
  virtual QString getStartPath() = 0;
  /// Sets the path that the file dialog will display
  virtual void setCurrentPath(const QString&) = 0;
  /// Returns the path the the file dialog will display
  virtual QString getCurrentPath() = 0;
  /// Return true iff the given row is a directory
  virtual bool isDir(const QModelIndex&) = 0;
  /// Returns the set of file paths associated with the given row (a row may represent one-to-many paths if grouping is implemented)
  virtual QStringList getFilePaths(const QModelIndex&) = 0;
  /// Converts a file into an absolute path
  virtual QString getFilePath(const QString&) = 0;
  /// Returns the parent path of the given file path (this is handled by the back-end so it can deal with issues of delimiters, symlinks, network resources, etc)
  virtual QString getParentPath(const QString&) = 0;
  /// Splits a path into its components (this is handled by the back-end so it can deal with issues of delimiters, symlinks, multi-root filesystems, etc)
  virtual QStringList splitPath(const QString&) = 0;

  /// Returns a Qt model that will contain the contents of the right-hand pane of the file dialog
  virtual QAbstractItemModel* fileModel() = 0;
  /// Returns a Qt model that will contain the contents of the left-hand pane of the file dialog
  virtual QAbstractItemModel* favoriteModel() = 0;

protected:
  pqFileDialogModel(QObject* Parent = 0);
};

#endif // !_pqFileDialogModel_h

