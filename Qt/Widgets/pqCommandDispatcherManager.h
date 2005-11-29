/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqCommandDispatcherManager_h
#define _pqCommandDispatcherManager_h

#include "QtWidgetsExport.h"
#include <QObject>

class pqCommandDispatcher;

/// Singleton object that manages a "current" command dispatcher that implements the user's preferred update policy - immediate updates, explicit updates, etc.
/** \todo Need to figure-out how this will interoperate with pqSMAdaptor and server-manager undo/redo */
class QTWIDGETS_EXPORT pqCommandDispatcherManager :
  public QObject
{
  Q_OBJECT
  
public:
  static pqCommandDispatcherManager& Instance();

  /// Returns the current command dispatcher
  pqCommandDispatcher& getDispatcher();
  /// Sets a new current command dispatcher (this object assumes ownership of the supplied dispatcher)
  void setDispatcher(pqCommandDispatcher*);

signals:
  /// Signal emitted whenever a new command dispatcher becomes current
  void dispatcherChanged();
  /// Signal emitted whenever the current command dispatcher has requested screen updates
  void updateWindows();
  
private:
  pqCommandDispatcherManager();
  ~pqCommandDispatcherManager();

  /// Stores the current command dispatcher
  pqCommandDispatcher* Dispatcher;
};



#endif // !_pqCommandDispatcherManager_h

