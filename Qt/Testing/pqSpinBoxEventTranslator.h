/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqSpinBoxEventTranslator_h
#define _pqSpinBoxEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt spinbox events into high-level ParaQ events that can be recorded as test cases
class pqSpinBoxEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqSpinBoxEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event, bool& Error);

private:
  pqSpinBoxEventTranslator(const pqSpinBoxEventTranslator&);
  pqSpinBoxEventTranslator& operator=(const pqSpinBoxEventTranslator&);

  QObject* CurrentObject;
  
private slots:
  void onValueChanged(int);  
};

#endif // !_pqSpinBoxEventTranslator_h

