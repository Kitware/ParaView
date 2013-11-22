/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#ifndef __LogBuffer_h
#define __LogBuffer_h

#include <sstream> // for ostringstream

/**
A parallel buffer for logging events and other data during an mpi run.
*/
class LogBuffer
{
public:
  LogBuffer();
  ~LogBuffer();

  /// copiers
  LogBuffer(const LogBuffer &other);
  void operator=(const LogBuffer &other);

  /// accessors
  const char *GetData() const { return this->Data; }
  char *GetData(){ return this->Data; }
  size_t GetSize() const { return this->At; }
  size_t GetCapacity() const { return this->Size; }

  /// fast clear
  void Clear(){ this->At=0; }

  /// clear that releases all resources.
  void ClearForReal();

  /// insertion operators
  LogBuffer &operator<<(const int v);
  LogBuffer &operator<<(const long long v);
  LogBuffer &operator<<(const double v);
  LogBuffer &operator<<(const char *v);
  template<size_t N> LogBuffer &operator<<(const char v[N]);

  /// formatted extraction operator.
  LogBuffer &operator>>(std::ostringstream &s);

  /// collect buffer to a root process
  void Gather(int worldRank, int worldSize, int rootRank);

protected:
  /// push n bytes onto the buffer, resizing if necessary
  void PushBack(const void *data, size_t n);

  /// resize to at least newSize bytes
  void Resize(size_t newSize);

private:
  size_t Size;
  size_t At;
  size_t GrowBy;
  char *Data;
};

//-----------------------------------------------------------------------------
template<size_t N>
LogBuffer &LogBuffer::operator<<(const char v[N])
{
  const char c='s';
  this->PushBack(&c,1);
  this->PushBack(&v[0],N);
  return *this;
}

#endif

// VTK-HeaderTest-Exclude: LogBuffer.h
