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

#define vtkSQLogDEBUG -1
//#ifdef SQTK_DEBUG
//#define vtkSQLogDEBUG 1
//#endif

#include "vtkSciberQuestModule.h" // for export macro
#include "vtkObject.h"

//BTX
#include "LogBuffer.h" // for LogBuffer

#include <vector> // for vector
#include <string> // for string
#include <sstream> // for sstream

#if vtkSQLogDEBUG > 0
#include <iostream> // for cerr
#endif
//ETX


class vtkPVXMLElement;
class vtkSQLog;

//BTX
/**
A class responsible for delete'ing the global instance of the log.
*/
class VTKSCIBERQUEST_EXPORT vtkSQLogDestructor
{
public:
  vtkSQLogDestructor() : Log(0) {}
  ~vtkSQLogDestructor();

  void SetLog(vtkSQLog *log){ this->Log=log; }

private:
  vtkSQLog *Log;
};

/**
Type used to direct an output stream into the log's header. The header
is a buffer used only by the root rank.
*/
class VTKSCIBERQUEST_EXPORT LogHeaderType
{
public:
  template<typename T> LogHeaderType &operator<<(const T& s);
};

/**
Type used to direct an output stream into the log's body. The body is a
buffer that all ranks write to.
*/
class VTKSCIBERQUEST_EXPORT LogBodyType
{
public:
  template<typename T> LogBodyType &operator<<(const T& s);
};
//ETX

//=============================================================================
class VTKSCIBERQUEST_EXPORT vtkSQLog : public vtkObject
{
public:
  static vtkSQLog *New();
  vtkTypeMacro(vtkSQLog,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // intialize from an xml document.
  int Initialize(vtkPVXMLElement *root);

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
  //BTX
  void SetFileName(const std::string &fileName){ this->SetFileName(fileName.c_str()); }
  //ETX

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

  // Description:
  // stream output to the log's header(root rank only).
  LogHeaderType GetHeader(){ return LogHeaderType(); }

  // Description:
  // stream output to log body(all ranks).
  LogBodyType GetBody(){ return LogBodyType(); }
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
  // The log class implements the singleton patern so that it
  // may be shared accross class boundaries. If the log instance
  // doesn't exist then one is created. It will be automatically
  // destroyed at exit by the signleton destructor. It can be
  // destroyed explicitly by calling DeleteGlobalInstance.
  static vtkSQLog *GetGlobalInstance();

  // Description:
  // Explicitly delete the singleton.
  static void DeleteGlobalInstance();

  // Description:
  // If enabled and used as a singleton the log will write
  // it's contents to disk during program termination.
  vtkSetMacro(WriteOnClose,int);
  vtkGetMacro(WriteOnClose,int);

  // Description:
  // Set/Get the global log level. Applications can set this to the
  // desired level so that all pipeline objects will log data.
  vtkSetMacro(GlobalLevel,int);
  vtkGetMacro(GlobalLevel,int);

protected:
  vtkSQLog();
  virtual ~vtkSQLog();

private:
  vtkSQLog(const vtkSQLog&); // Not implemented
  void operator=(const vtkSQLog&); // Not implemented

private:
  int GlobalLevel;
  int WorldRank;
  int WorldSize;
  int WriterRank;
  char *FileName;
  int WriteOnClose;
  std::vector<double> StartTime;
  #if vtkSQLogDEBUG < 0
  std::vector<std::string> EventId;
  #endif

  LogBuffer *Log;

  static vtkSQLog *GlobalInstance;
  static vtkSQLogDestructor GlobalInstanceDestructor;

  std::ostringstream HeaderBuffer;

  friend class LogHeaderType;
  friend class LogBodyType;
};

//BTX
 //-----------------------------------------------------------------------------
template<typename T>
vtkSQLog &vtkSQLog::operator<<(const T& s)
{
  if (this->WorldRank==this->WriterRank)
    {
    this->HeaderBuffer << s;
    #if vtkSQLogDEBUG > 0
    std::cerr << s;
    #endif
    }
  return *this;
}

//-----------------------------------------------------------------------------
template<typename T>
LogHeaderType &LogHeaderType::operator<<(const T& s)
{
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();

  if (log->WorldRank==log->WriterRank)
    {
    log->HeaderBuffer << s;
    #if vtkSQLogDEBUG > 0
    std::cerr << s;
    #endif
    }

  return *this;
}

//-----------------------------------------------------------------------------
template<typename T>
LogBodyType &LogBodyType::operator<<(const T& s)
{
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();

  *(log->Log) <<  s;
  #if vtkSQLogDEBUG > 0
  std::cerr << s;
  #endif

  return *this;
}
//ETX

#endif
