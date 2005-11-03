/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqServerFileDialogModel_h
#define _pqServerFileDialogModel_h

#include "pqFileDialogModel.h"

class vtkProcessModule;

/// Implementation of pqFileDialogModel that allows remote browsing of a connected ParaView server's filesystem
class pqServerFileDialogModel :
  public pqFileDialogModel
{
  typedef pqFileDialogModel base;
  
  Q_OBJECT

public:
  pqServerFileDialogModel(vtkProcessModule* ProcessModule, QObject* Parent = 0);
  ~pqServerFileDialogModel();

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
  class Implementation;
  Implementation* const implementation;
};

#endif // !_pqServerFileDialogModel_h

