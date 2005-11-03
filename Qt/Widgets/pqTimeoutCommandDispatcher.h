/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqTimeoutCommandDispatcher_h
#define _pqTimeoutCommandDispatcher_h

#include "pqCommandDispatcher.h"

#include <QTimer>

/// Implements a screen update policy that defers state changes until a short time after the user stops making changes
/** \todo Need to figure-out how this will interoperate with pqSMAdaptor and server-manager undo/redo */
class pqTimeoutCommandDispatcher :
  public pqCommandDispatcher
{
  Q_OBJECT
  
public:
  pqTimeoutCommandDispatcher(unsigned long Interval);
  ~pqTimeoutCommandDispatcher();

  void dispatchCommand(pqCommand*);
  
private:
  QTimer Timer;
  QList<pqCommand*> Commands;
  
private slots:
  void onExecute();
};

#endif // !_pqTimeoutCommandDispatcher_h

