/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqMenuEventTranslator_h
#define _pqMenuEventTranslator_h

#include "pqWidgetEventTranslator.h"

class pqMenuEventTranslatorAdaptor;

/// Translates low-level Qt action events into high-level ParaQ events that can be recorded as test cases
class pqMenuEventTranslator :
  public pqWidgetEventTranslator
{
  Q_OBJECT
  
public:
  pqMenuEventTranslator();
  ~pqMenuEventTranslator();
  
  virtual bool translateEvent(QObject* Object, QEvent* Event);

private:
  pqMenuEventTranslator(const pqMenuEventTranslator&);
  pqMenuEventTranslator& operator=(const pqMenuEventTranslator&);
  
  void clearActions();
  
  QList<pqMenuEventTranslatorAdaptor*> actions;
  
private slots:
  void onRecordEvent(QObject*, const QString&, const QString&);
};

#endif // !_pqMenuEventTranslator_h

