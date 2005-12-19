/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqWidgetEventTranslator_h
#define _pqWidgetEventTranslator_h

#include <QObject>

/// Abstract interface for an object that can translate low-level Qt events into high-level, serializable ParaQ events, for test-cases, demos, tutorials, etc.
class pqWidgetEventTranslator :
  public QObject
{
  Q_OBJECT
  
public:
  virtual ~pqWidgetEventTranslator() {}
  
  /// Derivatives should implement this and translate events into commands, returning "true" if they handled the event, and setting Error to "true" if there were any problems
  virtual bool translateEvent(QObject* Object, QEvent* Event, bool& Error) = 0;

signals:
  /// Derivatives should emit this signal whenever they wish to record a high-level event
  void recordEvent(QObject* Object, const QString& Command, const QString& Arguments);

protected:
  pqWidgetEventTranslator() {}
  
private:
  pqWidgetEventTranslator(const pqWidgetEventTranslator&);
  pqWidgetEventTranslator& operator=(const pqWidgetEventTranslator&);
};

#endif // !_pqWidgetEventTranslator_h

