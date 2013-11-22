/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "CellIdIterator.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"

#include <climits>

//-----------------------------------------------------------------------------
CellIdIterator::CellIdIterator()
      :
  StartId(1),
  EndId(0),
  Id(1)
{}

//-----------------------------------------------------------------------------
CellIdIterator::CellIdIterator(size_t startId, size_t endId)
      :
  StartId(startId),
  EndId(endId),
  Id(startId)
{}

//-----------------------------------------------------------------------------
CellIdIterator::CellIdIterator(
      const CellIdIterator &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
CellIdIterator &CellIdIterator::operator=(
      const CellIdIterator &other)
{
  if (this == &other) return *this;

  this->StartId=other.StartId;
  this->EndId=other.EndId;
  this->Id=other.Id;

  return *this;
}
