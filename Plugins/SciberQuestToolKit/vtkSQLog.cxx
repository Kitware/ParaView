/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQLog.h"

#include "XMLUtils.h"
#include "LogBuffer.h"
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
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

/*
For singleton pattern
**/
vtkSQLog *vtkSQLog::GlobalInstance=0;
vtkSQLogDestructor vtkSQLog::GlobalInstanceDestructor;

//-----------------------------------------------------------------------------
vtkSQLogDestructor::~vtkSQLogDestructor()
{
  if (this->Log)
    {
    Log->Delete();
    }
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQLog);

//-----------------------------------------------------------------------------
vtkSQLog::vtkSQLog()
        :
    GlobalLevel(0),
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
    size_t nIds=this->EventId.size();
    sqErrorMacro(
      pCerr(),
      << "Event id stack has "
      << nIds << " remaining.");
    for (size_t i=0; i<nIds; ++i)
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
  this->HeaderBuffer.str("");
}

//-----------------------------------------------------------------------------
int vtkSQLog::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQLogDEBUG
  //pCerr() << "=====vtkSQLog::Initialize" << endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQLog");
  if (elem==0)
    {
    return -1;
    }

  int global_level=0;
  GetOptionalAttribute<int,1>(elem,"global_level",&global_level);
  this->SetGlobalLevel(global_level);

  string file_name;
  GetOptionalAttribute<string,1>(elem,"file_name",&file_name);
  if (file_name.size()>0)
    {
    this->SetFileName(file_name.c_str());
    }

  if (this->GlobalLevel>0)
    {
    this->GetHeader()
      << "# ::vtkSQLogSource" << "\n"
      << "#   global_level=" << this->GlobalLevel << "\n"
      << "#   file_name=" << this->FileName << "\n";
    }

  return 0;
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
  #ifdef SQTK_WITHOUT_MPI
  (void)rank;
  #else
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
    f << "# " << ctime(&t) << this->HeaderBuffer.str() << oss.str();
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
