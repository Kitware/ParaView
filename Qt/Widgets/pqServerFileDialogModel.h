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

#endif // !_pqServerFileDialogModel_h

