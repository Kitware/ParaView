/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
// .NAME vtkSQProcessMonitor - Monitors pvserver heatlth as it runs.
// .SECTION Description
// .SECTION Caveats

#ifndef __vtkSQProcessMonitor_h
#define __vtkSQProcessMonitor_h

#include "vtkPolyDataAlgorithm.h"

#include <signal.h>
#include <string>
using std::string;

//BTX
class SystemInterface;
//ETX

// define the following ot enable debuging io
// #define vtkSQProcessMonitorDEBUG

class VTK_EXPORT vtkSQProcessMonitor : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkSQProcessMonitor,vtkPolyDataAlgorithm);

  // Description:
  // Construct plane perpendicular to z-axis, resolution 1x1, width
  // and height 1.0, and centered at the origin.
  static vtkSQProcessMonitor *New();

  // Description:
  // Get the configuration in a stream.
  // nProcs proc1_hostname proc1_pid ... procN_hostname procN_pid
  vtkGetStringMacro(ConfigStream);
  vtkSetStringMacro(ConfigStream);

  // Description:
  // Get the memory usage in a stream.
  // nProcs proc1_mem_use ... procN_mem_use
  vtkGetStringMacro(MemoryUseStream);
  vtkSetStringMacro(MemoryUseStream);

  // Description:
  // Updated if the object has been modifed.
  vtkSetMacro(InformationMTime,int);
  vtkGetMacro(InformationMTime,int);


  // Description:
  // Enable backtrace trap on signals SEGV, ILL, FPE, and BUS. Disabling
  // will restore the previous handler.
  void SetEnableBacktraceHandler(int on);

  // Description:
  // Explicitly turn on/off floating point exceptions.
  void SetEnableFE_ALL(int on);
  void SetEnableFE_DIVBYZERO(int on);
  void SetEnableFE_INEXACT(int on);
  void SetEnableFE_INVALID(int on);
  void SetEnableFE_OVERFLOW(int on);
  void SetEnableFE_UNDERFLOW(int on);

protected:
  vtkSQProcessMonitor();
  virtual ~vtkSQProcessMonitor();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *,vtkInformationVector **,vtkInformationVector *);

private:
  int WorldRank;
  int WorldSize;
  int Pid;
  string HostName;

  char *ConfigStream;
  char *MemoryUseStream;
  int InformationMTime;

  SystemInterface *ServerSystem;

private:
  vtkSQProcessMonitor(const vtkSQProcessMonitor&);  // Not implemented.
  void operator=(const vtkSQProcessMonitor&);  // Not implemented.
};

#endif
