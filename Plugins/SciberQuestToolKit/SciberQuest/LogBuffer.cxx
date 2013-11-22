/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
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
