/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqCommand_h
#define _pqCommand_h

/// Abstract interface for recording "commands" that will be executed at a later time (makes it possible to implement varying screen update policies)
class pqCommand
{
public:
  virtual ~pqCommand() {}
  
  /// Called to execute the command encapsulated by this object
  virtual void Execute() = 0;
  
protected:
  pqCommand() {}
  pqCommand(const pqCommand&) {}
  pqCommand& operator=(const pqCommand&) { return *this; }
};

#endif // !_pqCommand_h

