/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqCheckBoxEventTranslator_h
#define _pqCheckBoxEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt checkbox events into high-level ParaQ events that can be recorded as test cases
class pqCheckBoxEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqCheckBoxEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event);

private:
  pqCheckBoxEventTranslator(const pqCheckBoxEventTranslator&);
  pqCheckBoxEventTranslator& operator=(const pqCheckBoxEventTranslator&);

  QObject* CurrentObject;
  
private slots:
  void onStateChanged(int);  
};

#endif // !_pqCheckBoxEventTranslator_h

