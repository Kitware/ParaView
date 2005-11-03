/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqDoubleSpinBoxEventTranslator_h
#define _pqDoubleSpinBoxEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt spinbox events into high-level ParaQ events that can be recorded as test cases
class pqDoubleSpinBoxEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqDoubleSpinBoxEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event);

private:
  pqDoubleSpinBoxEventTranslator(const pqDoubleSpinBoxEventTranslator&);
  pqDoubleSpinBoxEventTranslator& operator=(const pqDoubleSpinBoxEventTranslator&);

  QObject* currentObject;
  
private slots:
  void onValueChanged(double);  
};

#endif // !_pqDoubleSpinBoxEventTranslator_h

