/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqEventTranslator_h
#define _pqEventTranslator_h

#include <QObject>

class pqWidgetEventTranslator;

/// Manages translation of low-level Qt events to high-level ParaQ events that can be serialized as test-cases, demos, tutorials, etc.
class pqEventTranslator :
  public QObject
{
  Q_OBJECT

public:
  pqEventTranslator();
  ~pqEventTranslator();

  /// Adds the default set of widget translators to the working set.  Translators are executed in order, so you may call addWidgetEventTranslator() before this function to "override" the default translators
  void addDefaultWidgetEventTranslators();
  /// Adds a new translator to the current working set of widget translators.  pqEventTranslator assumes control of the lifetime of the supplied object.
  void addWidgetEventTranslator(pqWidgetEventTranslator*);

  /// Adds an object to a list of objects that should be ignored when translating events (useful to prevent recording UI events from being captured as part of the recording)
  void ignoreObject(QObject* Object);

signals:
  /// This signal will be emitted every time a translator generates a high-level ParaQ event.  Observers should connect to this signal to serialize high-level events.
  void recordEvent(const QString& Object, const QString& Command, const QString& Arguments);

private slots:
  void onRecordEvent(QObject* Object, const QString& Command, const QString& Arguments);
  
private:
  pqEventTranslator(const pqEventTranslator&);
  pqEventTranslator& operator=(const pqEventTranslator&);

  bool eventFilter(QObject* Object, QEvent* Event);

  struct pqImplementation;
  pqImplementation* const Implementation;
};

#endif // !_pqEventTranslator_h

