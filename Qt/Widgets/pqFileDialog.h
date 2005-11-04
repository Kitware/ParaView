/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqFileDialog_h
#define _pqFileDialog_h

#include "pqFileDialog.ui.h"

class pqFileDialogModel;

/// Provides a standard file dialog "front-end" for the pqFileDialogModel "back-end", i.e. it can be used for both local and remote file browsing
class pqFileDialog :
  public QDialog
{
  typedef QDialog base;
  
  Q_OBJECT
  
public:
  pqFileDialog(pqFileDialogModel* Model, const QString& Title, QWidget* Parent, const char* const Name);

signals:
  /// Signal emitted when the user has chosen a set of files and accepted the dialog
  void FilesSelected(const QStringList&);

private:
  ~pqFileDialog();
  pqFileDialog(const pqFileDialog&);
  pqFileDialog& operator=(const pqFileDialog&);

  void accept();
  void reject();
  
  pqFileDialogModel* const Model;
  Ui::pqFileDialog Ui;
  const QModelIndex* Temp;
  
private slots:
  void OnDataChanged(const QModelIndex&, const QModelIndex&);
  void OnActivated(const QModelIndex&);
  void OnManualEntry(const QString&);
  void OnNavigate(const QString&);
  void OnNavigateUp();
  void OnNavigateDown();
  void OnAutoDelete();
};

#endif // !_pqFileDialog_h
