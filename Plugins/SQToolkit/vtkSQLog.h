/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
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

#include "vtkObject.h"

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::ostringstream;

class vtkSQLog : public vtkObject
{
public:
  static vtkSQLog *New();
  vtkTypeRevisionMacro(vtkSQLog,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The log works as an event stack. EventStart pushes the
  // event identifier and its start time onto the stack. EventEnd
  // pops the most recent event time and identifier computes the
  // ellapsed time and adds an entry to the log recording the
  // event, it's start and end times, and its ellapsed time.
  void StartEvent(const char *event);
  void EndEvent(const char *event);

  // Description:
  // Add to the log.
  template<typename T>
  vtkSQLog &operator<<(const T& data)
    {
    this->Log << data;
    return *this;
    }

  // Description:
  // Get the log contents.
  const char *GetLog() const { return this->Log.str().c_str(); }
  ostream &GetLogStream(){ return this->Log; }

  // Description:
  // Clear the log.
  void Initialize(){ this->Log.str(""); }

  // Description:
  // Write the log contents to a file. This is a collective
  // call. Each ranks log data is gathered to the writer rank
  // process and combined in process order for transfer to
  // the named file.
  int WriteLog(int writerRank, const char *fileName);

protected:
  vtkSQLog();
  virtual ~vtkSQLog();

private:
  vtkSQLog(const vtkSQLog &); // not implemented
  void operator=(const vtkSQLog &); // not implemented

private:
  int WorldRank;
  int WorldSize;

  ostringstream Log;

  vector<double> StartTime;
  vector<string> EventId;
};

#endif

