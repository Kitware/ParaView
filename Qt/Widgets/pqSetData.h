/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#ifndef _pqSetData_h
#define _pqSetData_h

#include <QVariant>

struct pqSetData
{
  pqSetData(const QVariant& Data);
  const QVariant Data;
};

template<typename T>
T* operator<<(T* LHS, const pqSetData& RHS)
{
  LHS->setData(RHS.Data);
  return LHS;
}

#endif // !_pqSetData_h

