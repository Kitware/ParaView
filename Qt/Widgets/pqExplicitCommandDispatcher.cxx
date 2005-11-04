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

void pqExplicitCommandDispatcher::DispatchCommand(pqCommand* Command)
{
  if(Command)
    {
    this->Commands.push_back(Command);
    emit CommandsPending(true);
    }
}

void pqExplicitCommandDispatcher::OnExecute()
{
  for(int i = 0; i != this->Commands.size(); ++i)
    this->Commands[i]->Execute();
    
  emit UpdateWindows();
  
  for(int i = 0; i != this->Commands.size(); ++i)
    delete this->Commands[i];
    
  this->Commands.clear();
  
  emit CommandsPending(false);
}

