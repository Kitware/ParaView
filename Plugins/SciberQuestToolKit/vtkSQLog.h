/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/

// .NAME vtkSQLog -- Distributed log.
// .SECTION Description
//
//  Provides ditributed log functionality. When the file is
//  written each process data is collected by rank 0 who
//  writes the data to the disk in rank order.
//

#ifndef __vtkSQLog_h
#define __vtkSQLog_h

#define vtkSQLogDEBUG

#include "vtkObject.h"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::ostringstream;

class vtkSQLog;
class LogBuffer;

//BTX
//=============================================================================
class vtkSQLogDestructor
{
public:
  vtkSQLogDestructor() : Log(0) {}
  ~vtkSQLogDestructor();

  void SetLog(vtkSQLog *log){ this->Log=log; }

private:
  vtkSQLog *Log;
};
//ETX

//=============================================================================
class vtkSQLog : public vtkObject
{
public:
  static vtkSQLog *New();
  vtkTypeMacro(vtkSQLog,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the rank who writes.
  vtkSetMacro(WriterRank,int);
  vtkGetMacro(WriterRank,int);

  // Description:
  // Set the filename that is used during write when the object
  // is used as a singleton. If nothing is set the default is
  // ROOT_RANKS_PID.log
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // The log works as an event stack. EventStart pushes the
  // event identifier and its start time onto the stack. EventEnd
  // pops the most recent event time and identifier computes the
  // ellapsed time and adds an entry to the log recording the
  // event, it's start and end times, and its ellapsed time.
  // EndEventSynch includes a barrier before the measurement.
  void StartEvent(const char *event);
  void StartEvent(int rank, const char *event);
  void EndEvent(const char *event);
  void EndEvent(int rank, const char *event);
  void EndEventSynch(const char *event);
  void EndEventSynch(int rank, const char *event);

  //BTX
  // Description:
  // Insert text into the log header on the writer rank.
  template<typename T>
  vtkSQLog &operator<<(const T& s);
  //ETX

  // Description:
  // Clear the log.
  void Clear();

  // Description:
  // When an object is finished writing data to the log
  // object it must call Update to send the data to the writer
  // rank.
  // This ensures that all data is transfered to the root before
  // MPI_Finalize is called while allowing the write to occur
  // after Mpi_finalize. Note: This is a collective call.
  void Update();

  // Description:
  // Write the log contents to a file.
  int Write();

  // Description:
  // The log class implements the singleton patern. It's optional.
  // When used this way when objects are finished with the log they
  // are required to call Update to push the local changes to the
  // root. When the root is destroyed the data is written to disk.
  static vtkSQLog *GetGlobalInstance();

  // Description:
  // If enabled(default) and used as a singleton the log will write
  // it's contents to disk during program termination.
  vtkSetMacro(WriteOnClose,int);
  vtkGetMacro(WriteOnClose,int);

protected:
  vtkSQLog();
  virtual ~vtkSQLog();

private:
  vtkSQLog(const vtkSQLog &); // not implemented
  void operator=(const vtkSQLog &); // not implemented

private:
  int WorldRank;
  int WorldSize;
  int WriterRank;
  char *FileName;
  int WriteOnClose;
  vector<double> StartTime;
  #if defined vtkSQLogDEBUG
  vector<string> EventId;
  #endif

  LogBuffer *Log;

  static vtkSQLog *GlobalInstance;
  static vtkSQLogDestructor GlobalInstanceDestructor;

  ostringstream Header;
};

//BTX
//-----------------------------------------------------------------------------
template<typename T>
vtkSQLog &vtkSQLog::operator<<(const T& s)
{
  if (this->WorldRank==this->WriterRank)
    {
    this->Header << s;
    }
    return *this;
}
//ETX

#endif
