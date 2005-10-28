/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqEventObserverStdout_h
#define _pqEventObserverStdout_h

#include <QObject>

/// Observes high-level ParaQ "events" and writes them to stdout, mainly for debugging purposes
class pqEventObserverStdout :
  public QObject
{
  Q_OBJECT

public slots:
  void onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments);
};

#endif // !_pqEventObserverStdout_h

