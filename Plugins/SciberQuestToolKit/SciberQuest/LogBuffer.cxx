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
#include "LogBuffer.h"

#include <cstring>
#include <cstdlib>
#include "postream.h"
#include "SQMacros.h"

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

//-----------------------------------------------------------------------------
LogBuffer::LogBuffer()
      :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
{}

//-----------------------------------------------------------------------------
LogBuffer::~LogBuffer(){ free(this->Data); }

//-----------------------------------------------------------------------------
LogBuffer::LogBuffer(const LogBuffer &other)
      :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
{
  *this=other;
}

//-----------------------------------------------------------------------------
void LogBuffer::operator=(const LogBuffer &other)
{
  if (this==&other) return;
  this->Clear();
  this->Resize(other.GetSize());
  memcpy(this->Data,other.Data,other.GetSize());
}

//-----------------------------------------------------------------------------
void LogBuffer::ClearForReal()
{
  this->At=0;
  this->Size=0;
  free(this->Data);
  this->Data=0;
}

//-----------------------------------------------------------------------------
LogBuffer &LogBuffer::operator<<(const int v)
{
  const char c='i';
  this->PushBack(&c,1);
  this->PushBack(&v,sizeof(int));
  return *this;
}

//-----------------------------------------------------------------------------
LogBuffer &LogBuffer::operator<<(const long long v)
{
  const char c='l';
  this->PushBack(&c,1);
  this->PushBack(&v,sizeof(long long));
  return *this;
}

//-----------------------------------------------------------------------------
LogBuffer &LogBuffer::operator<<(const double v)
{
  const char c='d';
  this->PushBack(&c,1);
  this->PushBack(&v,sizeof(double));
  return *this;
}

//-----------------------------------------------------------------------------
LogBuffer &LogBuffer::operator<<(const char *v)
{
  const char c='s';
  this->PushBack(&c,1);
  size_t n=strlen(v)+1;
  this->PushBack(v,n);
  return *this;
}

//-----------------------------------------------------------------------------
LogBuffer &LogBuffer::operator>>(std::ostringstream &s)
{
  size_t i=0;
  while (i<this->At)
    {
    char c=this->Data[i];
    ++i;
    switch (c)
      {
      case 'i':
        s << *((int*)(this->Data+i));
        i+=sizeof(int);
        break;

      case 'l':
        s << *((long long*)(this->Data+i));
        i+=sizeof(long long);
        break;

      case 'd':
        s << *((double*)(this->Data+i));
        i+=sizeof(double);
        break;

      case 's':
        {
        s << this->Data+i;
        size_t n=strlen(this->Data+i)+1;
        i+=n;
        }
        break;

      default:
        sqErrorMacro(
          pCerr(),
          "Bad case at " << i-1 << " " << c << ", " << (int)c);
        return *this;
      }
    }
  return *this;
}

//-----------------------------------------------------------------------------
void LogBuffer::Gather(int worldRank, int worldSize, int rootRank)
{
  #ifdef SQTK_WITHOUT_MPI
  (void)worldRank;
  (void)worldSize;
  (void)rootRank;
  #else
  // in serial this is a no-op
  if (worldSize>1)
    {
    int *bufferSizes=0;
    int *disp=0;
    if (worldRank==rootRank)
      {
      bufferSizes=(int *)malloc(worldSize*sizeof(int));
      disp=(int *)malloc(worldSize*sizeof(int));
      }
    int bufferSize=(int)this->GetSize();
    MPI_Gather(
        &bufferSize,
        1,
        MPI_INT,
        bufferSizes,
        1,
        MPI_INT,
        rootRank,
        MPI_COMM_WORLD);
    char *log=0;
    int cumSize=0;
    if (worldRank==rootRank)
      {
      for (int i=0; i<worldSize; ++i)
        {
        disp[i]=cumSize;
        cumSize+=bufferSizes[i];
        }
      log=(char*)malloc(cumSize);
      }
    MPI_Gatherv(
      this->Data,
      bufferSize,
      MPI_CHAR,
      //this->Data,
      log,
      bufferSizes,
      disp,
      MPI_CHAR,
      rootRank,
      MPI_COMM_WORLD);
    if (worldRank==rootRank)
      {
      this->Clear();
      this->PushBack(log,cumSize);
      free(bufferSizes);
      free(disp);
      free(log);
      }
    else
      {
      this->Clear();
      }
    }
  #endif
}

//-----------------------------------------------------------------------------
void LogBuffer::PushBack(const void *data, size_t n)
{
  size_t nextAt=this->At+n;
  this->Resize(nextAt);
  memcpy(this->Data+this->At,data,n);
  this->At=nextAt;
}

//-----------------------------------------------------------------------------
void LogBuffer::Resize(size_t newSize)
{
  //size_t oldSize=this->Size;
  if (newSize<=this->Size) return;
  while(this->Size<newSize) this->Size+=this->GrowBy;
  this->Data=(char*)realloc(this->Data,this->Size);
  //memset(this->Data+oldSize,-1,this->Size-oldSize);
}
