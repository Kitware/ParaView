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

#include <string>
#include <vector>
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
                         std::vector<std::string>& flags);
  
  void ReportCommand(const char* const* command, const char* name);
  int ReportStatus(vtksysProcess* process, const char* name);
  int ProcessCommandLine(int argc, char* argv[]);
  void CollectConfiguredOptions();
  void CreateCommandLine(vtksys_stl::vector<const char*>& commandLine,
                         const char* paraView,
                         ProcessType type, 
                         const char* numProc,
                         int argStart=0,
                         int argEnd=0,
                         char* argv[]=0);
  
  int StartProcessAndWait(vtksysProcess* server, const char* name,
                  std::vector<char>& out, std::vector<char>& err,
                  const char* string_to_wait_for,
                  std::string& matched_line);
  int StartProcess(vtksysProcess* client, const char* name);
  void Stop(vtksysProcess* p, const char* name);
  int OutputStringHasError(const char* pname, std::string& output);

  int WaitForLine(vtksysProcess* process, std::string& line, double timeout,
                  std::vector<char>& out, std::vector<char>& err);
  void PrintLine(const char* pname, const char* line);
  int WaitForAndPrintLine(const char* pname, vtksysProcess* process,
                          std::string& line, double timeout,
                          std::vector<char>& out, std::vector<char>& err,
                          const char* string_to_wait_for=NULL,
                          int* foundWaiting=NULL,
                          std::string* matched_line=NULL);

  std::string GetDirectory(std::string location);

private:
  struct ClientExecutableInfo
    {
    std::string ClientExecutable; //< fullpath to paraview executable
    int ArgStart;
    int ArgEnd;
    };

  // Description:
  // These method setup the \c process appropriately i.e. set the command,
  // timeout etc.
  bool SetupRenderServer(vtksysProcess* process);
  bool SetupServer(vtksysProcess* process);
  bool SetupClient(vtksysProcess* process, const ClientExecutableInfo& info,
    char* argv[]);

  std::vector<ClientExecutableInfo> ClientExecutables;
  std::string ServerExecutable;  // fullpath to paraview server executable
  std::string RenderServerExecutable;  // fullpath to paraview renderserver executable
  std::string DataServerExecutable;  // fullpath to paraview dataserver executable
  std::string MPIRun;  // fullpath to mpirun executable


  // This specify the preflags and post flags that can be set using:
  // VTK_MPI_PRENUMPROC_FLAGS VTK_MPI_PREFLAGS / VTK_MPI_POSTFLAGS at config time
  std::vector<std::string> MPIPreNumProcFlags;
  std::vector<std::string> MPIPreFlags;
  std::vector<std::string> MPIPostFlags;
 
  // ClientPostFlags are additional arguments sent to client.
  std::vector<std::string> ClientPostFlags;

  // MPIServerFlags allows you to specify flags specific for 
  // the client or the server
  std::vector<std::string> MPIServerPreFlags;
  std::vector<std::string> MPIServerPostFlags;

  // TDClientFlags / TDServerFlags allows you to specify flags specific for 
  // the client or the server when running in tiled display mode
  std::vector<std::string> TDClientPreFlags;
  std::vector<std::string> TDServerPreFlags;
  std::vector<std::string> TDClientPostFlags;
  std::vector<std::string> TDServerPostFlags;

  // PVSSHFlags allows user to pass ssh command to access distant machine
  // to do remote testing
  std::vector<std::string> PVSSHFlags;
  std::string                 PVSetupScript;
  
  // Specify the number of process flag, this can be set using: VTK_MPI_NUMPROC_FLAG. 
  // This is then split into : 
  // MPIServerNumProcessFlag & MPIRenderServerNumProcessFlag
  std::string MPINumProcessFlag;
  std::string MPIServerNumProcessFlag;
  std::string MPIRenderServerNumProcessFlag;

  std::string CurrentPrintLineName;

  // identifies how to connect to the server.
  std::string ServerURL;

  double TimeOut;
  double ServerExitTimeOut; // time to wait for servers to finish.
  int TestRenderServer;
  int TestServer;
  int TestTiledDisplay;
  std::string TestTiledDisplayTDX;
  std::string TestTiledDisplayTDY;
  int AllowErrorInOutput;
  int TestRemoteRendering;
  int TestMultiClient;
  int NumberOfServers;
  
  // Specify if the -rc flag was passed or not
  int ReverseConnection;
};

#endif

