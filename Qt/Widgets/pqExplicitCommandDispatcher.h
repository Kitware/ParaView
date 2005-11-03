/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqExplicitCommandDispatcher_h
#define _pqExplicitCommandDispatcher_h

#include "pqCommandDispatcher.h"

/// Implements an update policy that defers state changes until the user explicitly OKs them
/** \todo Need to figure-out how this will interoperate with pqSMAdaptor and server-manager undo/redo */
class pqExplicitCommandDispatcher :
  public pqCommandDispatcher
{
  Q_OBJECT
  
public:
  ~pqExplicitCommandDispatcher();

  void dispatchCommand(pqCommand*);

public slots:
  void onExecute();

signals:
  void commandsPending(bool);

private:
  QList<pqCommand*> Commands;
};

#endif // !_pqExplicitCommandDispatcher_h

