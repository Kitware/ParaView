/*=========================================================================

  Program:   Visualization Toolkit
  Module:    pvTestDriver.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME pvTestDriver - A program to run paraview for testing mpi and server modes.
// .SECTION Description
// 


#ifndef __pvTestDriver_h
#define __pvTestDriver_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include <kwsys/Process.h>
#include <kwsys/stl/string>
#include <kwsys/stl/vector>

class pvTestDriver 
{
public:
  int Main(int argc, char* argv[]);
  pvTestDriver();
  ~pvTestDriver();
protected:
  void SeparateArguments(const char* str, 
                         vtkstd::vector<vtkstd::string>& flags);
  
  void ReportCommand(const char* const* command, const char* name);
  int ReportStatus(kwsysProcess* process, const char* name);
  int ProcessCommandLine(int argc, char* argv[]);
  void CollectConfiguredOptions();
  void CreateCommandLine(kwsys_stl::vector<const char*>& commandLine,
                         const char* paraView,
                         const char* paraviewFlags, 
                         const char* numProc,
                         int argStart=0,
                         int argCount=0,
                         char* argv[]=0);
  
  int StartServer(kwsysProcess* server, const char* name,
                  vtkstd::vector<char>& out, vtkstd::vector<char>& err);
  int OutputStringHasError(const char* pname, vtkstd::string& output);

  int WaitForLine(kwsysProcess* process, vtkstd::string& line, double timeout,
                  vtkstd::vector<char>& out, vtkstd::vector<char>& err);
  void PrintLine(const char* pname, const char* line);
  int WaitForAndPrintLine(const char* pname, kwsysProcess* process,
                          vtkstd::string& line, double timeout,
                          vtkstd::vector<char>& out, vtkstd::vector<char>& err,
                          int* foundWaiting);
private:
  vtkstd::string ParaView;
  vtkstd::string ParaViewServer;
  vtkstd::string ParaViewRenderServer;
  vtkstd::string MPIRun;
  vtkstd::vector<vtkstd::string> MPIFlags;
  vtkstd::vector<vtkstd::string> MPIPostFlags;
  vtkstd::string MPINumProcessFlag;
  vtkstd::string MPIClientNumProcessFlag;
  vtkstd::string MPIServerNumProcessFlag;
  vtkstd::string MPIRenderServerNumProcessFlag;
  vtkstd::string CurrentPrintLineName;
  int RenderServerNumProcesses;
  double TimeOut;
  int TestRenderServer;
  int TestServer;
  int ArgStart;
  int AllowErrorInOutput;
  int ReverseConnection;
};

#endif

