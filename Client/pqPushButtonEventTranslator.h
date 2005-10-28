/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqPushButtonEventTranslator_h
#define _pqPushButtonEventTranslator_h

#include "pqWidgetEventTranslator.h"

/// Translates low-level Qt push button events into high-level ParaQ events that can be recorded as test cases
class pqPushButtonEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqPushButtonEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event);

private:
  pqPushButtonEventTranslator(const pqPushButtonEventTranslator&);
  pqPushButtonEventTranslator& operator=(const pqPushButtonEventTranslator&);

  QObject* currentObject;
  
private slots:
  void onClicked(bool);  
};

#endif // !_pqPushButtonEventTranslator_h

