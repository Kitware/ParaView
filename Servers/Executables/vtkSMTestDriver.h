/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTestDriver.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMTestDriver - A program to run paraview-based clients for testing
// mpi and server modes.
// .SECTION Description
// 


#ifndef __vtkSMTestDriver_h
#define __vtkSMTestDriver_h

#include <vtkstd/string>
#include <vtkstd/vector>
#include <vtksys/Process.h>
#include <vtksys/stl/string>
#include <vtksys/stl/vector>

class vtkSMTestDriver 
{
public:
  int Main(int argc, char* argv[]);
  vtkSMTestDriver();
  ~vtkSMTestDriver();

protected:
  enum ProcessType
    {
    CLIENT,
    SERVER,
    DATA_SERVER,
    RENDER_SERVER
    };
  void SeparateArguments(const char* str, 
                         vtkstd::vector<vtkstd::string>& flags);
  
  void ReportCommand(const char* const* command, const char* name);
  int ReportStatus(vtksysProcess* process, const char* name);
  int ProcessCommandLine(int argc, char* argv[]);
  void CollectConfiguredOptions();
  void CreateCommandLine(vtksys_stl::vector<const char*>& commandLine,
                         const char* paraView,
                         ProcessType type, 
                         const char* numProc,
                         int argStart=0,
                         int argCount=0,
                         char* argv[]=0);
  
  int StartServer(vtksysProcess* server, const char* name,
                  vtkstd::vector<char>& out, vtkstd::vector<char>& err);
  int StartClient(vtksysProcess* client, const char* name);
  void Stop(vtksysProcess* p, const char* name);
  int OutputStringHasError(const char* pname, vtkstd::string& output);

  int WaitForLine(vtksysProcess* process, vtkstd::string& line, double timeout,
                  vtkstd::vector<char>& out, vtkstd::vector<char>& err);
  void PrintLine(const char* pname, const char* line);
  int WaitForAndPrintLine(const char* pname, vtksysProcess* process,
                          vtkstd::string& line, double timeout,
                          vtkstd::vector<char>& out, vtkstd::vector<char>& err,
                          int* foundWaiting);

  vtkstd::string GetDirectory(vtkstd::string location);

private:
  vtkstd::string ClientExecutable;  // fullpath to paraview executable
  vtkstd::string ServerExecutable;  // fullpath to paraview server executable
  vtkstd::string RenderServerExecutable;  // fullpath to paraview renderserver executable
  vtkstd::string DataServerExecutable;  // fullpath to paraview dataserver executable
  vtkstd::string MPIRun;  // fullpath to mpirun executable


  // This specify the preflags and post flags that can be set using:
  // VTK_MPI_PRENUMPROC_FLAGS VTK_MPI_PREFLAGS / VTK_MPI_POSTFLAGS at config time
  vtkstd::vector<vtkstd::string> MPIPreNumProcFlags;
  vtkstd::vector<vtkstd::string> MPIPreFlags;
  vtkstd::vector<vtkstd::string> MPIPostFlags;
 
  // ClientPostFlags are additional arguments sent to client.
  vtkstd::vector<vtkstd::string> ClientPostFlags;

  // MPIServerFlags allows you to specify flags specific for 
  // the client or the server
  vtkstd::vector<vtkstd::string> MPIServerPreFlags;
  vtkstd::vector<vtkstd::string> MPIServerPostFlags;

  // TDClientFlags / TDServerFlags allows you to specify flags specific for 
  // the client or the server when running in tiled display mode
  vtkstd::vector<vtkstd::string> TDClientPreFlags;
  vtkstd::vector<vtkstd::string> TDServerPreFlags;
  vtkstd::vector<vtkstd::string> TDClientPostFlags;
  vtkstd::vector<vtkstd::string> TDServerPostFlags;

  // PVSSHFlags allows user to pass ssh command to access distant machine
  // to do remote testing
  vtkstd::vector<vtkstd::string> PVSSHFlags;
  vtkstd::string                 PVSetupScript;
  
  // Specify the number of process flag, this can be set using: VTK_MPI_NUMPROC_FLAG. 
  // This is then split into : 
  // MPIServerNumProcessFlag & MPIRenderServerNumProcessFlag
  vtkstd::string MPINumProcessFlag;
  vtkstd::string MPIServerNumProcessFlag;
  vtkstd::string MPIRenderServerNumProcessFlag;

  vtkstd::string CurrentPrintLineName;

  int RenderServerNumProcesses;
  double TimeOut;
  double ServerExitTimeOut; // time to wait for servers to finish.
  int TestRenderServer;
  int TestServer;
  int TestTiledDisplay;
  int ArgStart;
  int AllowErrorInOutput;
  int TestRemoteRendering;
  
  // Specify if the -rc flag was passed or not
  int ReverseConnection;
};

#endif

