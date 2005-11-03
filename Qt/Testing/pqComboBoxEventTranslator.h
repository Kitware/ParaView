/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqComboBoxEventTranslator_h
#define _pqComboBoxEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt checkbox events into high-level ParaQ events that can be recorded as test cases
class pqComboBoxEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqComboBoxEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event);

private:
  pqComboBoxEventTranslator(const pqComboBoxEventTranslator&);
  pqComboBoxEventTranslator& operator=(const pqComboBoxEventTranslator&);

  QObject* currentObject;
  
private slots:
  void onDestroyed(QObject*);
  void onStateChanged(const QString&);  
};

#endif // !_pqComboBoxEventTranslator_h

