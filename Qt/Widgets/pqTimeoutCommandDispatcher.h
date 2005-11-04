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
  /// Constructs the dispatcher, ready to execute commands whenever the given number of milliseconds have elapsed since the most recent command was dispatched
  pqTimeoutCommandDispatcher(unsigned long Interval);
  ~pqTimeoutCommandDispatcher();

  void DispatchCommand(pqCommand*);
  
private:
  QTimer Timer;
  /// Stores the list of queued commands
  QList<pqCommand*> Commands;
  
private slots:
  /// Called to execute commands after some amount of time has elapsed
  void OnExecute();
};

#endif // !_pqTimeoutCommandDispatcher_h

