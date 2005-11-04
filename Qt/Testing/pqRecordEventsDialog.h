/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqRecordEventsDialog_h
#define _pqRecordEventsDialog_h

#include "ui_pqRecordEventsDialog.h"

class pqRecordEventsDialog :
  public QDialog
{
  Q_OBJECT
  
public:
  pqRecordEventsDialog(const QString& Path, QWidget* Parent);

private:
  pqRecordEventsDialog(const pqRecordEventsDialog&);
  pqRecordEventsDialog& operator=(const pqRecordEventsDialog&);
  ~pqRecordEventsDialog();

  class pqImplementation;
  pqImplementation* const Implementation;

  Ui::pqRecordEventsDialog Ui;

private slots:
  void accept();
  void reject();
  void onAutoDelete();
};

#endif // !_pqRecordEventsDialog_h

