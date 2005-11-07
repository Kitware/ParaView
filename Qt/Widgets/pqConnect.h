/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqConnect_h
#define _pqConnect_h

class QObject;

struct pqConnect
{
  pqConnect(const char* Signal, const QObject* Receiver, const char* Method);
  
  const char* Signal;
  const QObject* Receiver;
  const char* Method;
};

template<typename T>
T* operator<<(T* LHS, const pqConnect& RHS)
{
  LHS->connect(LHS, RHS.Signal, RHS.Receiver, RHS.Method);
  return LHS;
}

#endif // !_pqConnect_h

