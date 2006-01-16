/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqServerBrowser_h
#define _pqServerBrowser_h

#include "QtWidgetsExport.h"
#include "ui_pqServerBrowser.h"

class pqServer;

/// Provides a user-interface component for creating server connections
class QTWIDGETS_EXPORT pqServerBrowser :
  public QDialog
{
  typedef QDialog base;

  Q_OBJECT

public:
  pqServerBrowser(QWidget* Parent);

signals:
  /// This signal will be emitted iff a server connection is successfully created
  void serverConnected(pqServer*);
 
private:
  ~pqServerBrowser();
  pqServerBrowser(const pqServerBrowser&);
  pqServerBrowser& operator=(const pqServerBrowser&);
  
  Ui::pqServerBrowser Ui;
  
private slots:
  void accept();
  void onServerTypeActivated(int);
};

#endif // !_pqServerBrowser_h

