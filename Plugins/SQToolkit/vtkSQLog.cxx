/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/

#include "vtkSQLog.h"

#include "SQMacros.h"
#include "postream.h"

#include "vtkObjectFactory.h"

#include <ctime>
#include <sys/time.h>
#include <unistd.h>

#include <fstream>
using std::ofstream;

#include <mpi.h>

//-----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkSQLog, "$Revision: 0.0 $");

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQLog);

//-----------------------------------------------------------------------------
vtkSQLog::vtkSQLog()
{
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro("MPI has not been initialized.");
    }

  MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
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
}

//-----------------------------------------------------------------------------
void vtkSQLog::StartEvent(const char *event)
{
  double walls=0.0;
  timeval wallt;
  gettimeofday(&wallt,0x0);
  walls=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;

  this->EventId.push_back(event);
  this->StartTime.push_back(walls);
}

//-----------------------------------------------------------------------------
void vtkSQLog::EndEvent(const char *event)
{
  // this argument is included for better readability
  // in caller's code.
  (void*)event;

  double walle=0.0;
  timeval wallt;
  gettimeofday(&wallt,0x0);
  walle=(double)wallt.tv_sec+((double)wallt.tv_usec)/1.0E6;

  double walls=this->StartTime.back();

  this->Log
    << this->WorldRank << " "
    << this->EventId.back() << " "
    << walls << " "
    << walle << " "
    << walle-walls
    << endl;

  this->EventId.pop_back();
  this->StartTime.pop_back();
}

//-----------------------------------------------------------------------------
int vtkSQLog::WriteLog(int writerRank, const char *fileName)
{
  cerr << "writing log" << endl;
  cerr << "WorldRank=" << this->WorldRank << endl;
  cerr << "WorldSize=" << this->WorldSize << endl;
  cerr << "writerRank=" << writerRank << endl;

  int iErr=0;
  int *bufferSizes=0;
  int *disp=0;
  if (this->WorldRank == writerRank)
    {
    bufferSizes=(int *)malloc(this->WorldSize*sizeof(int));
    disp=(int *)malloc(this->WorldSize*sizeof(int));
    }
  int bufferSize=this->Log.str().size();
  cerr << "bufferSize=" << bufferSize << endl;
  cerr << "bufferSizes=" << bufferSizes << endl;
  MPI_Gather(
      &bufferSize,
      1,
      MPI_INT,
      bufferSizes,
      1,
      MPI_INT,
      writerRank,
      MPI_COMM_WORLD);
  cerr << __LINE__ << endl;
  char *log=0;
  if (this->WorldRank == writerRank)
    {
    int cumSize=0;
    for (int i=0; i<this->WorldSize; ++i)
      {
      disp[i] = cumSize;
      cumSize += bufferSizes[i];

      cerr << "disp=" << disp[i] << endl;
      cerr << "bufferSizes[" << i << "]=" << bufferSizes[i] << endl;
      }
    cerr << "cumSize=" << cumSize << endl;
    log=(char*)malloc(cumSize+1);
    log[cumSize]='\0';
    }
  MPI_Gatherv(
    (char*)this->Log.str().c_str(),
    bufferSize,
    MPI_INT,
    log,
    bufferSizes,
    disp,
    MPI_INT,
    writerRank,
    MPI_COMM_WORLD);
  if (this->WorldRank == writerRank)
    {
    ofstream f(fileName);
    if (!f.good())
      {
      sqErrorMacro(
        pCerr(),
        << "Failed to open "
        << fileName
        << " for  writing.");
      iErr=-1;
      }
    else
      {
      time_t t;
      time(&t);
      f << ctime(&t) << endl
        << log << endl;
      f.close();
      }
    free(bufferSizes);
    free(disp);
    free(log);
    }
  return iErr;
}

//-----------------------------------------------------------------------------
void vtkSQLog::PrintSelf(ostream& os, vtkIndent indent)
{
  os
    << indent << "WorldRank=" << this->WorldRank << endl
    << indent << "Log=" << this->Log.str().c_str() << endl;
}

