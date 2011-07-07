/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2008 SciberQuest Inc.

*/
#ifndef __Tuple_h
#define __Tuple_h

#include <vector>
using std::vector;

#include <iostream>
using std::ostream;
using std::endl;

template <typename T> class Tuple;
template<typename T>
ostream &operator<<(ostream &os, const Tuple<T> &t);

/// Class to handle printing of Tuples commmonly used in VTK.
template<typename T>
class Tuple
{
public:
  Tuple(const Tuple &other);
  Tuple(T t1, T t2, T t3);
  Tuple(T t1 ,T t2, T t3, T t4, T t5, T t6);
  Tuple(const T *t, int n);
  Tuple(vector<T> &v);
  ~Tuple();

  const Tuple &operator=(const Tuple &rhs);

private:
  Tuple(); // not implemented
  void Initialize(const T *t, int n);

  friend ostream &operator<< <> (ostream &os, const Tuple<T> &t);

private:
  int Size;
  T *Data;
};

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::Tuple(const Tuple &other)
{
  *this=other;
}

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::Tuple(const T *data, int size)
      :
  Size(0),
  Data(0)
{
  this->Initialize(data,size);
}

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::Tuple(T t1, T t2, T t3)
      :
  Size(0),
  Data(0)
{
  T data[3]={t1,t2,t3};
  this->Initialize(data,3);
}

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::Tuple(T t1 ,T t2, T t3, T t4, T t5, T t6)
      :
  Size(0),
  Data(0)
{
  T data[6]={t1,t2,t3,t4,t5,t6};
  this->Initialize(data,6);
}

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::Tuple(vector<T> &v)
      :
  Size(0),
  Data(0)
{
  T *p=&v[0];
  this->Initialize(p,v.size());
}

//-----------------------------------------------------------------------------
template<typename T>
Tuple<T>::~Tuple()
{
  this->Initialize(NULL,0);
}

//-----------------------------------------------------------------------------
template<typename T>
void Tuple<T>::Initialize(const T *data, int size)
{
  if (this->Data)
    {
    delete [] this->Data;
    this->Data=NULL;
    this->Size=0;
    }
  if (size && data)
    {
    this->Data=new T [size];
    this->Size=size;
    for (int i=0; i<size; ++i)
      {
      this->Data[i]=data[i];
      }
    }
}

//-----------------------------------------------------------------------------
template<typename T>
const Tuple<T> & Tuple<T>::operator=(const Tuple<T> &rhs)
{
  if (this!=&rhs)
    {
    this->Initialize(rhs.Data,rhs.Size);
    }
  return *this;
}

//-----------------------------------------------------------------------------
template<typename T>
ostream &operator<<(ostream &os, const Tuple<T> &t)
{
  os << "(";
  if (t.Size)
    {
    os << t.Data[0];
    for (int i=1; i<t.Size; ++i)
      {
      os << ", " << t.Data[i];
      }
    }
    os << ")";
  return os;
}

#endif
