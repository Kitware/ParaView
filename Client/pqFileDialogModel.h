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

#include <QAbstractItemModel>

class pqFileDialogModel :
  public QAbstractItemModel
{
  typedef QAbstractItemModel base;

  Q_OBJECT

public:
  virtual QString getStartDirectory() = 0;
  virtual void setViewDirectory(const QString&) = 0;
  virtual QString getViewDirectory() = 0;
  virtual bool isDir(const QModelIndex&) = 0;
  virtual QString getFilePath(const QModelIndex&) = 0;

public slots:
  virtual void navigateUp() = 0;
  virtual void navigateDown(const QModelIndex&) = 0;

protected:
  pqFileDialogModel(QObject* Parent = 0);
  ~pqFileDialogModel();
};

#endif // !_pqFileDialogModel_h

