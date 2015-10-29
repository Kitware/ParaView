/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef LogBuffer_h
#define LogBuffer_h

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
