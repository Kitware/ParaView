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

#include "pqServerBrowser.ui.h"

class pqServerBrowser :
  public QDialog
{
  Q_OBJECT

public:
  pqServerBrowser(QWidget* Parent = 0, const char* const Name = 0);
 
  Ui::pqServerBrowser ui;
};

#endif // !_pqServerBrowser_h

