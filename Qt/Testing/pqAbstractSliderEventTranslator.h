/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqAbstractSliderEventTranslator_h
#define _pqAbstractSliderEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt slider events into high-level ParaQ events that can be recorded as test cases
class pqAbstractSliderEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqAbstractSliderEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event, bool& Error);

private:
  pqAbstractSliderEventTranslator(const pqAbstractSliderEventTranslator&);
  pqAbstractSliderEventTranslator& operator=(const pqAbstractSliderEventTranslator&);

  QObject* CurrentObject;
  
private slots:
  void onValueChanged(int);  
};

#endif // !_pqAbstractSliderEventTranslator_h

