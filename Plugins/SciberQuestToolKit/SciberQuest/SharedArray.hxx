/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __SharedArray_h
#define __SharedArray_h

#include "RefCountedPointer.h"
#include <cstdlib>
#include <exception>

/// Templated ref counted array implementation
/**
A light weight templated implementation of a shared array.
*/
template<typename T>
class SharedArray : public RefCountedPointer
{
public:
  static SharedArray *New(){ return new SharedArray; }

  /**
  Direct accesss to the array
  */
  T &operator[](size_t i){ return this->Data[i]; }
  const T &operator[](size_t i) const { return this->Data[i]; }

  /**
  Direct accesss to the array
  */
  T *GetPointer(){ return this->Data; }
  const T *GetPointer() const { return this->Data; }

  /**
  Return the array's size.
  */
  size_t Size(){ return this->DataSize; }

  /**
  Replace the contents of the array with what's passed in.
  */
  void Assign(T *data, size_t dataSize);
  void Assign(SharedArray<T> *other);

  /**
  Inplace extend/truncate array.
  */
  void Resize(size_t n);
  void Clear(){ this->Resize(0); }

protected:
  SharedArray():Data(0),DataSize(0){}
  virtual ~SharedArray() { free(this->Data); }

private:
  SharedArray(SharedArray &); // not implemented
  void operator=(SharedArray &); // not implemented

  template<typename T1>
  friend std::ostream &operator<<(std::ostream &os, SharedArray<T1> &sa);

private:
  T *Data;
  size_t DataSize;
};

//-----------------------------------------------------------------------------
template<typename T>
void SharedArray<T>::Resize(size_t dataSize)
{
  this->Data=(T*)realloc(this->Data,dataSize*sizeof(T));
  this->DataSize=dataSize;
  if (this->DataSize && (this->Data==0))
    {
    throw std::bad_alloc();
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void SharedArray<T>::Assign(T *data, size_t dataSize)
{
  if (this->Data==data)
    {
    return;
    }
  this->Resize(dataSize);
  for (size_t i=0; i<dataSize; ++i)
    {
    this->Data[i]=data[i];
    }
}

//-----------------------------------------------------------------------------
template<typename T>
void SharedArray<T>::Assign(SharedArray<T> *other)
{
  this->Assign(other->GetPointer(),other->Size());
}

//-----------------------------------------------------------------------------
template<typename T>
std::ostream &operator<<(std::ostream &os, SharedArray<T> &sa)
{
  size_t n=sa.Size();
  if (n)
    {
    os << sa[0];
    for (size_t i=1; i<n; ++i)
      {
      os << ", " << sa[i];
      }
    }
  return os;
}

#endif
