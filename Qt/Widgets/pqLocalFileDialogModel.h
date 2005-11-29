/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqLocalFileDialogModel_h
#define _pqLocalFileDialogModel_h

#include "QtWidgetsExport.h"
#include "pqFileDialogModel.h"

/// Implementation of pqFileDialogModel that provides browsing capabilities for the local filesystem
class QTWIDGETS_EXPORT pqLocalFileDialogModel :
  public pqFileDialogModel
{
  Q_OBJECT

public:
  pqLocalFileDialogModel(QObject* Parent = 0);
  ~pqLocalFileDialogModel();

  QString getStartPath();
  void setCurrentPath(const QString&);
  QString getCurrentPath();
  bool isDir(const QModelIndex&);
  QStringList getFilePaths(const QModelIndex&);
  QString getFilePath(const QString&);
  QString getParentPath(const QString&);
  QStringList splitPath(const QString&);
  QAbstractItemModel* fileModel();
  QAbstractItemModel* favoriteModel();
  
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqLocalFileDialogModel_h

