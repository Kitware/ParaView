/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQLog.h"

#include "SQMacros.h"
#include "postream.h"

#include "vtkObjectFactory.h"

#include <ctime>
#if !defined(WIN32)
#include <sys/time.h>
#include <unistd.h>
#else
#include <process.h>
#include <Winsock2.h>
#include <time.h>
int gettimeofday(struct timeval *tv, void *)
{
  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  __int64 tmpres = 0;
  tmpres = ft.dwHighDateTime;
  tmpres <<= 32;
  tmpres |= ft.dwLowDateTime;

  /*converting file time to unix epoch*/
  const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;
  tmpres /= 10;  /*convert into microseconds*/
  tmpres -= DELTA_EPOCH_IN_MICROSECS; 
  tv->tv_sec = (__int32)(tmpres*0.000001);
  tv->tv_usec =(tmpres%1000000);

  return 0;
}
#endif

#include <fstream>
using std::ofstream;
using std::ios_base;

#ifndef SQTK_WITHOUT_MPI
#include <mpi.h>
#endif

// set this ini the header since there are conditional compile
// there as well
//#define vtkSQLogDEBUG

//=============================================================================
class LogBuffer
{
public:
  LogBuffer()
        :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
  {}

  ~LogBuffer(){ free(this->Data); }

  /// copiers
  LogBuffer(const LogBuffer &other)
        :
    Size(0),
    At(0),
    GrowBy(4096),
    Data(0)
  {
      *this=other;
  }

  void operator=(const LogBuffer &other)
  {
    if (this==&other) return;
    this->Clear();
    this->Resize(other.GetSize());
    memcpy(this->Data,other.Data,other.GetSize());
  }

  /// accessors
  const char *GetData() const { return this->Data; }
  char *GetData(){ return this->Data; }
  size_t GetSize() const { return this->At; }
  size_t GetCapacity() const { return this->Size; }
  void Clear(){ this->At=0; }
  void ClearForReal()
  {
    this->At=0;
    this->Size=0;
    free(this->Data);
    this->Data=0;
  }

  /// insertion operators
  LogBuffer &operator<<(const int v)
  {
    const char c='i';
    this->PushBack(&c,1);
    this->PushBack(&v,sizeof(int));
    return *this;
  }

  LogBuffer &operator<<(const double v)
  {
    const char c='d';
    this->PushBack(&c,1);
    this->PushBack(&v,sizeof(double));
    return *this;
  }

  LogBuffer &operator<<(const char *v)
  {
    const char c='s';
    this->PushBack(&c,1);
    size_t n=strlen(v)+1;
    this->PushBack(v,n);
    return *this;
  }

  template<size_t N>
  LogBuffer &operator<<(const char v[N])
  {
    const char c='s';
    this->PushBack(&c,1);
    this->PushBack(&v[0],N);
    return *this;
  }

  /// formatted extraction operator.
  LogBuffer &operator>>(ostringstream &s)
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

  /// collect buffer to a root process
  void Gather(int worldRank, int worldSize, int rootRank)
  {
    // in serial this is a no-op
    if (worldSize>1)
      {
      #ifndef SQTK_WITHOUT_MPI
      int *bufferSizes=0;
      int *disp=0;
      if (worldRank==rootRank)
        {
        bufferSizes=(int *)malloc(worldSize*sizeof(int));
        disp=(int *)malloc(worldSize*sizeof(int));
        }
      int bufferSize=this->GetSize();
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
        //this->Resize(cumSize); // can't do inplace since mpi uses memcpy
        //this->At=cumSize;
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
      #endif
      }
  }

protected:
  /// push n bytes onto the buffer, resizing if necessary
  void PushBack(const void *data, size_t n)
  {
  size_t nextAt=this->At+n;
  this->Resize(nextAt);
  memcpy(this->Data+this->At,data,n);
  this->At=nextAt;
  }

  /// resize to at least newSize bytes
  void Resize(size_t newSize)
  {
    //size_t oldSize=this->Size;
    if (newSize<=this->Size) return;
    while(this->Size<newSize) this->Size+=this->GrowBy;
    this->Data=(char*)realloc(this->Data,this->Size);
    //memset(this->Data+oldSize,-1,this->Size-oldSize);
  }

private:
  size_t Size;
  size_t At;
  size_t GrowBy;
  char *Data;
};


//-----------------------------------------------------------------------------
vtkSQLogDestructor::~vtkSQLogDestructor()
{
  if (this->Log)
    {
    Log->Delete();
    }
}


/*
For singleton pattern
**/
vtkSQLog *vtkSQLog::GlobalInstance=0;
vtkSQLogDestructor vtkSQLog::GlobalInstanceDestructor;


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQLog);

//-----------------------------------------------------------------------------
vtkSQLog::vtkSQLog()
        :
    WorldRank(0),
    WorldSize(1),
    WriterRank(0),
    FileName(0),
    WriteOnClose(0),
    Log(0)
{
  #ifndef SQTK_WITHOUT_MPI
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    }
  #endif

  this->StartTime.reserve(256);

  this->Log=new LogBuffer;
}

//-----------------------------------------------------------------------------
vtkSQLog::~vtkSQLog()
{
  // Alert the user that he left events on the stack,
  // this is usually a sign of trouble.
  if (this->StartTime.size()>0)
    {
    sqErrorMacro(
      pCerr(),
      << "Start time stack has "
      << this->StartTime.size()
      << " remaining.");
    }

  #if defined(vtkSQLogDEBUG)
  if (this->EventId.size()>0)
    {
    int nIds=this->EventId.size();
    sqErrorMacro(
      pCerr(),
      << "Event id stack has "
      << nIds << " remaining.");
    for (int i=0; i<nIds; ++i)
      {
      pCerr() << "EventId[" << i << "]=" << this->EventId[i] << endl;
      }
    }
  #endif

  this->SetFileName(0);

  delete this->Log;
}

//-----------------------------------------------------------------------------
vtkSQLog *vtkSQLog::GetGlobalInstance()
{
  if (vtkSQLog::GlobalInstance==0)
    {
    vtkSQLog *log=vtkSQLog::New();
    ostringstream oss;
    oss << getpid() << ".log";
    log->SetFileName(oss.str().c_str());

    vtkSQLog::GlobalInstance=log;
    vtkSQLog::GlobalInstanceDestructor.SetLog(log);
    }
  return vtkSQLog::GlobalInstance;
}

//-----------------------------------------------------------------------------
void vtkSQLog::Clear()
{
  this->Log->Clear();
  this->Header.str("");
}

//-----------------------------------------------------------------------------
void vtkSQLog::StartEvent(int rank, const char *event)
{
  if (this->WorldRank!=rank) return;
  this->StartEvent(event);
}

//-----------------------------------------------------------------------------
void vtkSQLog::StartEvent(const char *event)
{
  double walls=0.0;
  timeval wallt;
  gettimeofday(&wallt,0x0);
  walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;

  #if defined(vtkSQLogDEBUG)
  this->EventId.push_back(event);
  #endif

  this->StartTime.push_back(walls);
}

//-----------------------------------------------------------------------------
void vtkSQLog::EndEvent(int rank, const char *event)
{
  if (this->WorldRank!=rank) return;
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkSQLog::EndEvent(const char *event)
{
  double walle=0.0;
  timeval wallt;
  gettimeofday(&wallt,0x0);
  walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;

  #if defined(vtkSQLogDEBUG)
  if (this->StartTime.size()==0)
    {
    sqErrorMacro(pCerr(),"No event to end! " << event);
    return;
    }
  #endif

  double walls=this->StartTime.back();
  this->StartTime.pop_back();

  *this->Log
    << this->WorldRank << " "
    << event << " "
    << walls << " "
    << walle << " "
    << walle-walls
    << "\n";

  #if defined(vtkSQLogDEBUG)
  const string &sEventId=this->EventId.back();
  const string eEventId=event;
  if (sEventId!=eEventId)
    {
    sqErrorMacro(
      pCerr(),
      "Event mismatch " << sEventId << " != " << eEventId);
    }
  this->EventId.pop_back();
  #endif

}

//-----------------------------------------------------------------------------
void vtkSQLog::EndEventSynch(int rank, const char *event)
{
  #ifndef SQTK_WITHOUT_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  if (this->WorldRank!=rank) return;
  #endif
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkSQLog::EndEventSynch(const char *event)
{
  #ifndef SQTK_WITHOUT_MPI
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
  this->EndEvent(event);
}

//-----------------------------------------------------------------------------
void vtkSQLog::Update()
{
  this->Log->Gather(
      this->WorldRank,
      this->WorldSize,
      this->WriterRank);
}

//-----------------------------------------------------------------------------
int vtkSQLog::Write()
{
  if (this->WorldRank==this->WriterRank)
    {
    ostringstream oss;
    *this->Log >> oss;
    ofstream f(this->FileName, ios_base::out|ios_base::app);
    if (!f.good())
      {
      sqErrorMacro(
        pCerr(),
        << "Failed to open "
        << this->FileName
        << " for  writing.");
      return -1;
      }
    time_t t;
    time(&t);
    f << "# " << ctime(&t) << this->Header.str() << oss.str();
    f.close();
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQLog::PrintSelf(ostream& os, vtkIndent indent)
{
  ostringstream oss;
  *this->Log >> oss;
  os
    << indent << "WorldRank=" << this->WorldRank << endl
    << indent << "WorldSize=" << this->WorldSize << endl
    << indent << "WriterRank=" << this->WriterRank << endl
    << indent << "Log=" << oss.str() << endl;
}
