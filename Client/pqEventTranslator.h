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
#include <QVector>

class pqWidgetEventTranslator;

class pqEventTranslator :
  public QObject
{
  Q_OBJECT

public:
  pqEventTranslator();
  ~pqEventTranslator();

  void addWidgetEventTranslator(pqWidgetEventTranslator*);

signals:
  void abstractEvent(const QString& Widget, const QString& Command, const QString& Arguments);

private:
  pqEventTranslator(const pqEventTranslator&);
  pqEventTranslator& operator=(const pqEventTranslator&);

  bool eventFilter(QObject* Object, QEvent* Event);
  
  QVector<pqWidgetEventTranslator*> translators;
};

#endif // !_pqEventTranslator_h

