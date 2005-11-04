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

#include "pqFileDialogModel.h"

/// Implementation of pqFileDialogModel that provides browsing capabilities for the local filesystem
class pqLocalFileDialogModel :
  public pqFileDialogModel
{
  Q_OBJECT

public:
  pqLocalFileDialogModel(QObject* Parent = 0);
  ~pqLocalFileDialogModel();

  QString GetStartPath();
  void SetCurrentPath(const QString&);
  QString GetCurrentPath();
  bool IsDir(const QModelIndex&);
  QStringList GetFilePaths(const QModelIndex&);
  QString GetFilePath(const QString&);
  QString GetParentPath(const QString&);
  QStringList SplitPath(const QString&);
  QAbstractItemModel* FileModel();
  QAbstractItemModel* FavoriteModel();
  
private:
  class pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqLocalFileDialogModel_h

