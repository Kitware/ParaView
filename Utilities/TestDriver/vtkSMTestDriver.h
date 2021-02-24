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

#ifndef vtkSMTestDriver_h
#define vtkSMTestDriver_h

#include <string>
#include <vector>
#include <vtksys/Process.h>

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
    RENDER_SERVER,
    SCRIPT
  };
  void SeparateArguments(const char* str, std::vector<std::string>& flags);

  void ReportCommand(const char* const* command, const char* name);
  int ReportStatus(vtksysProcess* process, const char* name);
  int ProcessCommandLine(int argc, char* argv[]);
  void CollectConfiguredOptions();
  void CreateCommandLine(std::vector<const char*>& commandLine, const char* paraView,
    ProcessType type, const char* numProc, int argStart = 0, int argEnd = 0, char* argv[] = 0);

  int StartProcessAndWait(vtksysProcess* server, const char* name, std::vector<char>& out,
    std::vector<char>& err, const char* string_to_wait_for, std::string& matched_line);
  int StartProcess(vtksysProcess* client, const char* name);
  void Stop(vtksysProcess* p, const char* name);
  int OutputStringHasError(const char* pname, std::string& output);

  int WaitForLine(vtksysProcess* process, std::string& line, double timeout, std::vector<char>& out,
    std::vector<char>& err);
  void PrintLine(const char* pname, const char* line);
  int WaitForAndPrintLine(const char* pname, vtksysProcess* process, std::string& line,
    double timeout, std::vector<char>& out, std::vector<char>& err,
    const char* string_to_wait_for = nullptr, int* foundWaiting = nullptr,
    std::string* matched_line = nullptr);

  std::string GetDirectory(std::string location);

private:
  struct ExecutableInfo
  {
    std::string Executable; //< fullpath to paraview executable
    ProcessType Type;
    std::string TypeName;
    int ArgStart;
    int ArgEnd;
  };

  // Description:
  // These method setup the \c process appropriately i.e. set the command,
  // timeout etc.
  bool SetupServer(vtksysProcess* process, const ExecutableInfo& info, char* argv[]);
  bool SetupClient(vtksysProcess* process, const ExecutableInfo& info, char* argv[]);

  std::vector<ExecutableInfo> ClientExecutables;
  ExecutableInfo ServerExecutable;       // fullpath to paraview server executable + args
  ExecutableInfo RenderServerExecutable; // fullpath to paraview renderserver executable + args
  ExecutableInfo DataServerExecutable;   // fullpath to paraview dataserver executable + args
  ExecutableInfo ScriptExecutable;       // fullpath to an additional executable + args
  std::string MPIRun;                    // fullpath to mpirun executable

  // This specify the preflags and post flags that can be set using:
  // VTK_MPI_PRENUMPROC_FLAGS VTK_MPI_PREFLAGS / VTK_MPI_POSTFLAGS at config time
  std::vector<std::string> MPIPreNumProcFlags;
  std::vector<std::string> MPIPreFlags;
  std::vector<std::string> MPIPostFlags;

  // TDClientFlags / TDServerFlags allows you to specify flags specific for
  // the client or the server when running in tiled display mode
  std::vector<std::string> TDClientPreFlags;
  std::vector<std::string> TDServerPreFlags;
  std::vector<std::string> TDClientPostFlags;
  std::vector<std::string> TDServerPostFlags;

  // PVSSHFlags allows user to pass ssh command to access distant machine
  // to do remote testing
  std::vector<std::string> PVSSHFlags;
  std::string PVSetupScript;

  // Specify the number of process flag, this can be set using: VTK_MPI_NUMPROC_FLAG.
  // This is then split into :
  // MPIServerNumProcessFlag & MPIRenderServerNumProcessFlag
  std::string MPINumProcessFlag;
  std::string MPIServerNumProcessFlag;
  std::string MPIRenderServerNumProcessFlag;
  std::string MPIScriptNumProcessFlag;

  std::string CurrentPrintLineName;

  // identifies how to connect to the server.
  std::string ServerURL;

  double TimeOut;
  double ServerExitTimeOut; // time to wait for servers to finish.
  double ScriptExitTimeOut; // time to wait for the script to finish.
  int TestRenderServer;
  int TestServer;
  int TestScript; // additional process to run
  int AllowErrorInOutput;
  int ScriptIgnoreOutputErrors;
  int TestRemoteRendering;
  int TestMultiClient;
  int NumberOfServers;
  // Specify if the -rc flag was passed or not
  int ReverseConnection;
  bool ClientUseMPI;
};

#endif
