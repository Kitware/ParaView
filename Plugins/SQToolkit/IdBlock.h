/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#ifndef __IdBlock_h
#define __IdBlock_h

#include <iostream>
using std::ostream;

/// A block of adjecent indexes uint64_to a collection (typically of cells or pouint64_ts).
class IdBlock
{
public:
  IdBlock(){ this->clear(); }
  IdBlock(unsigned long long frst){ m_data[0]=frst; m_data[1]=1; }
  IdBlock(unsigned long long frst, unsigned long long n){ m_data[0]=frst; m_data[1]=n; }
  void clear(){ m_data[0]=m_data[1]=0; }
  unsigned long long &first(){ return m_data[0]; }
  unsigned long long &size(){ return m_data[1]; }
  unsigned long long last(){ return m_data[0]+m_data[1]; }
  unsigned long long *data(){ return m_data; }
  unsigned long long dataSize(){ return 2; }
  bool contains(unsigned long long id){ return ((id>=first()) && (id<last())); }
  bool empty(){ return m_data[1]==0; }
private:
  unsigned long long m_data[2]; // id, size
private:
  friend ostream &operator<<(ostream &os, const IdBlock &b);
};

#endif
