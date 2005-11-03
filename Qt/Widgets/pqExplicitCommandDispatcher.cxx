/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "pqCommand.h"
#include "pqExplicitCommandDispatcher.h"

pqExplicitCommandDispatcher::~pqExplicitCommandDispatcher()
{
  for(int i = 0; i != this->Commands.size(); ++i)
    delete this->Commands[i];
}

void pqExplicitCommandDispatcher::dispatchCommand(pqCommand* Command)
{
  if(Command)
    {
    this->Commands.push_back(Command);
    emit commandsPending(true);
    }
}

void pqExplicitCommandDispatcher::onExecute()
{
  for(int i = 0; i != this->Commands.size(); ++i)
    this->Commands[i]->redoCommand();
    
  emit updateWindow();
  
  for(int i = 0; i != this->Commands.size(); ++i)
    delete this->Commands[i];
    
  this->Commands.clear();
  
  emit commandsPending(false);
}

