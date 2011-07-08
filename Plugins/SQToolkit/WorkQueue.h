/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __WorkQueue_h
#define __WorkQueue_h

#include "IdBlock.h"
#include "minmax.h"


/// Partitions a contiguous set of indices on demand.
class WorkQueue
{
public:
  WorkQueue(int size)
      :
    m_at(0),
    m_end(size)
     { }

  int GetBlock(IdBlock &b, int size)
    {
    if (m_at==m_end)
      {
      b.first()=b.size()=0;
      return 0;
      }
    int
    next_at=min(m_at+size,m_end);
    size=next_at-m_at;
    b.first()=m_at;
    b.size()=size;
    m_at=next_at;
    return size;
    }

private:
  int m_at;
  int m_end;

private:
  WorkQueue();
};

#endif
