/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommand.h"
#include "pqTimeoutCommandDispatcher.h"

pqTimeoutCommandDispatcher::pqTimeoutCommandDispatcher(unsigned long Interval)
{
  timer.setInterval(Interval);
  QObject::connect(&timer, SIGNAL(timeout()), this, SLOT(onExecute()));
}

pqTimeoutCommandDispatcher::~pqTimeoutCommandDispatcher()
{
  for(int i = 0; i != commands.size(); ++i)
    delete commands[i];
}

void pqTimeoutCommandDispatcher::dispatchCommand(pqCommand* Command)
{
  if(Command)
    {
    commands.push_back(Command);
    timer.stop();
    timer.start();
    }
}

void pqTimeoutCommandDispatcher::onExecute()
{
  timer.stop();
  
  for(int i = 0; i != commands.size(); ++i)
    commands[i]->redoCommand();
    
  emit updateWindow();
  
  for(int i = 0; i != commands.size(); ++i)
    delete commands[i];
    
  commands.clear();
}

