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
  
  virtual QString getStartPath() = 0;
  virtual void setCurrentPath(const QString&) = 0;
  virtual QString getCurrentPath() = 0;
  virtual bool isDir(const QModelIndex&) = 0;
  virtual QStringList getFilePaths(const QModelIndex&) = 0;
  virtual QString getFilePath(const QString&) = 0;
  virtual QString getParentPath(const QString&) = 0;
  virtual QStringList splitPath(const QString&) = 0;

  virtual QAbstractItemModel* fileModel() = 0;
  virtual QAbstractItemModel* favoriteModel() = 0;

protected:
  pqFileDialogModel(QObject* Parent = 0);
};

#endif // !_pqFileDialogModel_h

