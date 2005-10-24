/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommandDispatcherManager.h"
#include "pqImmediateCommandDispatcher.h"

pqCommandDispatcherManager& pqCommandDispatcherManager::instance()
{
  static pqCommandDispatcherManager* g_instance = 0;
  if(!g_instance)
    g_instance = new pqCommandDispatcherManager();
  
  return *g_instance;
}

pqCommandDispatcherManager::pqCommandDispatcherManager() :
  dispatcher(new pqImmediateCommandDispatcher())
{
}

pqCommandDispatcherManager::~pqCommandDispatcherManager()
{
  delete dispatcher;
}

pqCommandDispatcher& pqCommandDispatcherManager::getDispatcher()
{
  if(!dispatcher)
    dispatcher = new pqImmediateCommandDispatcher();
    
  return *dispatcher;
}

void pqCommandDispatcherManager::setDispatcher(pqCommandDispatcher* Dispatcher)
{
  delete dispatcher;
  dispatcher = Dispatcher;
  
  emit dispatcherChanged();
}

