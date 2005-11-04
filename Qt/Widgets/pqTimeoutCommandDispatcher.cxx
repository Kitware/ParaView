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
  this->Timer.setInterval(Interval);
  QObject::connect(&this->Timer, SIGNAL(timeout()), this, SLOT(onExecute()));
}

pqTimeoutCommandDispatcher::~pqTimeoutCommandDispatcher()
{
  for(int i = 0; i != this->Commands.size(); ++i)
    delete this->Commands[i];
}

void pqTimeoutCommandDispatcher::dispatchCommand(pqCommand* Command)
{
  if(Command)
    {
    this->Commands.push_back(Command);
    this->Timer.stop();
    this->Timer.start();
    }
}

void pqTimeoutCommandDispatcher::onExecute()
{
  this->Timer.stop();
  
  for(int i = 0; i != this->Commands.size(); ++i)
    this->Commands[i]->execute();
    
  emit updateWindows();
  
  for(int i = 0; i != this->Commands.size(); ++i)
    delete this->Commands[i];
    
  this->Commands.clear();
}

