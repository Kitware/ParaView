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

class pqLocalFileDialogModel :
  public pqFileDialogModel
{
  typedef pqFileDialogModel base;
  
  Q_OBJECT

public:
  pqLocalFileDialogModel(QObject* Parent = 0);
  ~pqLocalFileDialogModel();

  QString getStartDirectory();
  void setViewDirectory(const QString& Path);
  QString getViewDirectory();
  bool isDir(const QModelIndex&);
  QString getFilePath(const QModelIndex&);
  void navigateUp();
  void navigateDown(const QModelIndex&);
  
private:
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex parent(const QModelIndex& index) const;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  
  class Implementation;
  Implementation* const implementation;
};

#endif // !_pqLocalFileDialogModel_h

