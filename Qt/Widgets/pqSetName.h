/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqSetName_h
#define _pqSetName_h

#include <QString>

struct pqSetName
{
  pqSetName(const QString& Name);
  const QString Name;
};

template<typename T>
T* operator<<(T* LHS, const pqSetName& RHS)
{
  LHS->setObjectName(RHS.Name);
  return LHS;
}

#endif // !_pqSetName_h

