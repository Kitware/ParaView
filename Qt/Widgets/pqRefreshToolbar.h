/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqRefreshToolbar_h
#define _pqRefreshToolbar_h

#include <QToolBar>

class QComboBox;
class QPushButton;

/// Provides a user-interface component that allows the user to choose their preferred screen-update policy
class pqRefreshToolbar :
  public QToolBar
{
  Q_OBJECT
  
public:
  pqRefreshToolbar(QWidget* Parent);

private:
  QPushButton* RefreshButton;
  
private slots:
  void onRefreshType(int);
};

#endif // !_pqRefreshToolbar_h

