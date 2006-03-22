/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqEventObserverXML_h
#define _pqEventObserverXML_h

#include <QObject>
#include <vtkIOStream.h>

/// Observes high-level ParaQ events, and serializes them as XML for possible playback (as a test-case, demo, tutorial, etc)
class pqEventObserverXML :
  public QObject
{
  Q_OBJECT
  
public:
  pqEventObserverXML(ostream& Stream);
  ~pqEventObserverXML();

public slots:
  void onRecordEvent(const QString& Widget, const QString& Command, const QString& Arguments);

private:
  /// Stores a stream that will be used to store the XML output
  ostream& Stream;
};

#endif // !_pqEventObserverXML_h

