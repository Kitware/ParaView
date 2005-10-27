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

class pqWidgetEventTranslator :
  public QObject
{
  Q_OBJECT
  
public:
  ~pqWidgetEventTranslator() {}
  virtual bool translateEvent(QObject* Watcher, QEvent* Event) = 0;

signals:
  void abstractEvent(const QString& Widget, const QString& Command, const QString& Arguments);

protected:
  pqWidgetEventTranslator() {}
  pqWidgetEventTranslator(const pqWidgetEventTranslator&) {}
  pqWidgetEventTranslator& operator=(const pqWidgetEventTranslator&) { return *this; }
};

#endif // !_pqWidgetEventTranslator_h

