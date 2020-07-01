
/*=========================================================================

  Program:   ParaView
  Module:    vtkSMTestDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSystemIncludes.h"

#include "vtkSMTestDriver.h"
#include "vtkSMTestDriverConfig.h"

#include <vtksys/RegularExpression.hxx>
#include <vtksys/String.hxx>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <sstream>
#include <thread>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace
{
bool GetHostAndPort(const std::string& output, std::string& hostname, int& port)
{
  vtksys::RegularExpression regEx("Accepting connection\\(s\\): ([^:]+):([0-9]+)");
  if (regEx.find(output))
  {
    hostname = regEx.match(1);
    port = atoi(regEx.match(2).c_str());
    return true;
  }
  return false;
}

int FindLastExecutableArg(int startArg, int maxArgs, char* argv[])
{
  for (int i = startArg; i < maxArgs; ++i)
  {
    if (!strcmp(argv[i], "--data-server") || !strcmp(argv[i], "--render-server") ||
      !strcmp(argv[i], "--client") || !strcmp(argv[i], "--server") || !strcmp(argv[i], "--script"))
    {
      return i;
    }
  }
  return maxArgs;
}
}

// The main function as this class should only be used by this program
int main(int argc, char* argv[])
{
  vtkSMTestDriver d;
  return d.Main(argc, argv);
}

vtkSMTestDriver::vtkSMTestDriver()
{
  this->AllowErrorInOutput = 0;
  this->ScriptIgnoreOutputErrors = 0;
  this->TimeOut = -1; // no timeout
  this->ServerExitTimeOut = 60;
  this->ScriptExitTimeOut = 0;
  this->TestRenderServer = 0;
  this->TestServer = 0;
  this->TestScript = 0;
  this->ReverseConnection = 0;
  this->TestRemoteRendering = 0;
  this->TestMultiClient = 0;
  this->NumberOfServers = 1;
  this->ClientUseMPI = false;

  // Setup process types
  this->ServerExecutable.Type = SERVER;
  this->ServerExecutable.TypeName = "server";
  this->DataServerExecutable.Type = DATA_SERVER;
  this->DataServerExecutable.TypeName = "server";
  this->RenderServerExecutable.Type = RENDER_SERVER;
  this->RenderServerExecutable.TypeName = "renderserver";
  this->ScriptExecutable.Type = SCRIPT;
  this->ScriptExecutable.TypeName = "script";
}

vtkSMTestDriver::~vtkSMTestDriver()
{
}

// now implement the vtkSMTestDriver class

void vtkSMTestDriver::SeparateArguments(const char* str, std::vector<std::string>& flags)
{
  std::string arg = str;
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = arg.find_first_of(" ;");
  if (pos2 == arg.npos)
  {
    flags.push_back(str);
    return;
  }
  while (pos2 != arg.npos)
  {
    flags.push_back(arg.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = arg.find_first_of(" ;", pos1 + 1);
  }
  flags.push_back(arg.substr(pos1, pos2 - pos1));
}

void vtkSMTestDriver::CollectConfiguredOptions()
{
// try to make sure that this timesout before dart so it can kill all the processes

// now find all the mpi information if mpi run is set
#ifdef PARAVIEW_USE_MPI
#ifdef PARAVIEW_MPIEXEC_EXECUTABLE
  this->MPIRun = PARAVIEW_MPIEXEC_EXECUTABLE;
#else
#error "Error: PARAVIEW_MPIEXEC_EXECUTABLE must be set when PARAVIEW_USE_MPI is on."
#endif
  int serverNumProc = 1;
  int renderNumProc = 1;
#ifdef PARAVIEW_MPI_MAX_NUMPROCS
  serverNumProc = PARAVIEW_MPI_MAX_NUMPROCS;
#endif
  if (vtksys::SystemTools::HasEnv("SMTESTDRIVER_MPI_NUMPROCS"))
  {
    serverNumProc = std::atoi(vtksys::SystemTools::GetEnv("SMTESTDRIVER_MPI_NUMPROCS"));
  }
  serverNumProc = std::max(1, serverNumProc);
  renderNumProc = std::max(1, serverNumProc - 1);

#ifdef PARAVIEW_MPI_NUMPROC_FLAG
  this->MPINumProcessFlag = PARAVIEW_MPI_NUMPROC_FLAG;
#else
#error "Error PARAVIEW_MPI_NUMPROC_FLAG must be defined to run test if MPI is on."
#endif
#ifdef PARAVIEW_MPI_PREFLAGS
  this->SeparateArguments(PARAVIEW_MPI_PREFLAGS, this->MPIPreFlags);
#endif
#ifdef PARAVIEW_MPI_POSTFLAGS
  this->SeparateArguments(PARAVIEW_MPI_POSTFLAGS, this->MPIPostFlags);
#endif
  char buf[1024];
  sprintf(buf, "%d", serverNumProc);
  this->MPIServerNumProcessFlag = buf;
  this->MPIScriptNumProcessFlag = buf;
  sprintf(buf, "%d", renderNumProc);
  this->MPIRenderServerNumProcessFlag = buf;

#endif

// For remote testing (via ssh)
#ifdef PV_SSH_FLAGS
  this->SeparateArguments(PV_SSH_FLAGS, this->PVSSHFlags);
#endif // PV_SSH_FLAGS

#ifdef PV_SETUP_SCRIPT
  this->PVSetupScript = PV_SETUP_SCRIPT;
#endif // PV_SETUP_SCRIPT
}

/// This adds the debug/build configuration crap for the executable on windows.
static std::string FixExecutablePath(const std::string& path)
{
  // we don't need to FixExecutablePath() anymore. Developers are expected to
  // add tests using
  // add_test(NAME ..
  //          COMMAND ... $<TARGET_FILE:paraview> ...)
  // for to ensure that the executable is pointed to directly.
  return path;
}

int vtkSMTestDriver::ProcessCommandLine(int argc, char* argv[])
{
  int i;
  for (i = 1; i < argc - 1; ++i)
  {
    if (strcmp(argv[i], "--client") == 0 || strcmp(argv[i], "--client-mpi") == 0)
    {
      this->ClientUseMPI = (strcmp(argv[i], "--client-mpi") == 0);
      ExecutableInfo info;
      info.Executable = ::FixExecutablePath(argv[i + 1]);
      info.ArgStart = i + 2;
      info.ArgEnd = argc;
      this->ClientExecutables.push_back(info);
      if (this->ClientExecutables.size() > 1)
      {
        // if more than one client executable was specified, we truncate the
        // previous client's arguments processing at the "--client".
        ExecutableInfo& prevInfo = this->ClientExecutables[this->ClientExecutables.size() - 2];
        prevInfo.ArgEnd = i;
      }
    }
    if (strcmp(argv[i], "--test-remote-rendering") == 0)
    {
      this->TestRemoteRendering = 1;
      fprintf(stderr, "Test Render Server.\n");
    }
    if (strcmp(argv[i], "--render-server") == 0)
    {
      this->TestRenderServer = 1;
      this->RenderServerExecutable.Executable = ::FixExecutablePath(argv[i + 1]);
      this->RenderServerExecutable.ArgStart = i + 2;
      this->RenderServerExecutable.ArgEnd = FindLastExecutableArg(i + 2, argc, argv);
      fprintf(stderr, "Test Render Server.\n");
    }
    if (strcmp(argv[i], "--data-server") == 0)
    {
      this->TestServer = 1;
      this->DataServerExecutable.Executable = ::FixExecutablePath(argv[i + 1]);
      this->DataServerExecutable.ArgStart = i + 2;
      this->DataServerExecutable.ArgEnd = FindLastExecutableArg(i + 2, argc, argv);
      fprintf(stderr, "Test Render Server.\n");
    }
    if (strcmp(argv[i], "--server") == 0)
    {
      this->TestServer = 1;
      this->ServerExecutable.Executable = ::FixExecutablePath(argv[i + 1]);
      this->ServerExecutable.ArgStart = i + 2;
      this->ServerExecutable.ArgEnd = FindLastExecutableArg(i + 2, argc, argv);
      fprintf(stderr, "Test Server.\n");
    }
    if (strcmp(argv[i], "--script") == 0)
    {
      this->TestScript = 1;
      this->ScriptExecutable.Executable = ::FixExecutablePath(argv[i + 1]);
      this->ScriptExecutable.ArgStart = i + 2;
      this->ScriptExecutable.ArgEnd = FindLastExecutableArg(i + 2, argc, argv);
      fprintf(stderr, "Test Script.\n");
    }
    if (strcmp(argv[i], "--test-multi-clients") == 0)
    {
      this->TestMultiClient = 1;
      fprintf(stderr, "Test collaboration.\n");
    }
    if (strcmp(argv[i], "--test-multi-servers") == 0)
    {
      this->NumberOfServers = atoi(argv[i + 1]);
      fprintf(stderr, "Test multi-servers with %d servers.\n", this->NumberOfServers);
    }
    if (strcmp(argv[i], "--script-np") == 0)
    {
      this->MPIScriptNumProcessFlag = argv[i + 1];
      fprintf(stderr, "Test script with %s servers.\n", argv[i + 1]);
    }
    if (strcmp(argv[i], "--one-mpi-np") == 0)
    {
      this->MPIServerNumProcessFlag = this->MPIRenderServerNumProcessFlag = "1";
      fprintf(stderr, "Test with one mpi process.\n");
    }
    if (strcmp(argv[i], "--test-rc") == 0)
    {
      this->ReverseConnection = 1;
      fprintf(stderr, "Test reverse connection.\n");
    }
    if (strncmp(argv[i], "--server-exit-timeout", strlen("--server-exit-timeout")) == 0)
    {
      this->ServerExitTimeOut = atoi(argv[i + 1]);
      fprintf(stderr, "The server exit timeout was set to %f.\n", this->ServerExitTimeOut);
    }
    if (strncmp(argv[i], "--server-preflags", 17) == 0)
    {
      fprintf(stderr, "Server preflags are no longer supported.\n");
    }
    if (strncmp(argv[i], "--allow-errors", strlen("--allow-errors")) == 0)
    {
      this->AllowErrorInOutput = 1;
      fprintf(stderr, "The allow errors in output flag was set to %d.\n", this->AllowErrorInOutput);
    }
    if (strcmp(argv[i], "--script-ignore-output-errors") == 0)
    {
      this->ScriptIgnoreOutputErrors = 1;
      fprintf(stderr, "The ScriptIgnoreOutputErrors flag was set to %d.\n",
        this->ScriptIgnoreOutputErrors);
    }
  }

  if (this->TestMultiClient && (!this->TestServer || this->TestRenderServer))
  {
    fprintf(stderr, "Multi-client tests require --server.\n");
    abort();
  }

  return 1;
}
//-----------------------------------------------------------------------------
void vtkSMTestDriver::CreateCommandLine(std::vector<const char*>& commandLine, const char* paraView,
  vtkSMTestDriver::ProcessType type, const char* numProc, int argStart, int argEnd, char* argv[])
{
  if (this->MPIRun.size() && (type != CLIENT || this->ClientUseMPI))
  {
    commandLine.push_back(this->MPIRun.c_str());
    commandLine.push_back(this->MPINumProcessFlag.c_str());
    commandLine.push_back(numProc);
    for (unsigned int i = 0; i < this->MPIPreFlags.size(); ++i)
    {
      commandLine.push_back(this->MPIPreFlags[i].c_str());
    }
  }

  if (this->PVSSHFlags.size() && (type == SERVER || type == DATA_SERVER))
  {
    {
      // First add the ssh command:
      for (unsigned int i = 0; i < this->PVSSHFlags.size(); ++i)
      {
        commandLine.push_back(this->PVSSHFlags[i].c_str());
      }
      // then the paraview initialization:
      if (this->PVSetupScript.size())
      {
        commandLine.push_back(this->PVSetupScript.c_str());
      }
    }
  }
  else
  {
    commandLine.push_back(paraView);

    if (this->ReverseConnection && type != CLIENT)
    {
      commandLine.push_back("-rc");
#ifdef PV_TEST_CLIENT
      commandLine.push_back("-ch=" PV_TEST_CLIENT);
#endif
    }

    for (unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
    {
      commandLine.push_back(MPIPostFlags[i].c_str());
    }

    // remaining flags for the test
    for (int ii = argStart; ii < argEnd; ++ii)
    {
      commandLine.push_back(argv[ii]);
    }
  }

#if !defined(__APPLE__)
  if (this->TestRemoteRendering && (type == SERVER || type == RENDER_SERVER))
  {
    commandLine.push_back("--use-offscreen-rendering");
  }
#endif

  if (type == SERVER && this->TestMultiClient)
  {
    commandLine.push_back("--multi-clients");
  }

  if (type == CLIENT && this->NumberOfServers > 1)
  {
    std::vector<const char*>::iterator iter = commandLine.begin();
    iter++;
    commandLine.insert(iter, "--multi-servers");
  }

#ifdef PV_TEST_USE_RANDOM_PORTS
  if (!this->ReverseConnection)
  {
    // tell the server to pick a random port number.
    if (type == DATA_SERVER)
    {
      commandLine.push_back("-dsp=0");
    }
    else if (type == SERVER)
    {
      commandLine.push_back("-sp=0");
    }
    else if (type == RENDER_SERVER)
    {
      commandLine.push_back("-rsp=0");
    }
  }
#endif

  commandLine.push_back(0);
}

int vtkSMTestDriver::StartProcessAndWait(vtksysProcess* server, const char* name,
  std::vector<char>& out, std::vector<char>& err, const char* string_to_wait_for,
  std::string& matched_line)
{
  if (!server)
  {
    return 1;
  }
  cerr << "vtkSMTestDriver: starting process " << name << "\n";
  vtksysProcess_SetTimeout(server, this->TimeOut);
  vtksysProcess_Execute(server);
  int foundWaiting = 0;
  std::string output;
  while (!foundWaiting)
  {
    int pipe = this->WaitForAndPrintLine(
      name, server, output, 100.0, out, err, string_to_wait_for, &foundWaiting, &matched_line);
    if (pipe == vtksysProcess_Pipe_None || pipe == vtksysProcess_Pipe_Timeout)
    {
      break;
    }
  }
  if (foundWaiting)
  {
    cerr << "vtkSMTestDriver: " << name << " successfully started.\n";
    return 1;
  }
  else
  {
    cerr << "vtkSMTestDriver: " << name << " never started.\n";
    vtksysProcess_Kill(server);
    return 0;
  }
}

int vtkSMTestDriver::StartProcess(vtksysProcess* client, const char* name)
{
  if (!client)
  {
    return 1;
  }
  cerr << "vtkSMTestDriver: starting process " << name << "\n";
  vtksysProcess_SetTimeout(client, this->TimeOut);
  vtksysProcess_Execute(client);
  if (vtksysProcess_GetState(client) == vtksysProcess_State_Executing)
  {
    cerr << "vtkSMTestDriver: " << name << " successfully started.\n";
    return 1;
  }
  else
  {
    this->ReportStatus(client, name);
    vtksysProcess_Kill(client);
    return 0;
  }
}

void vtkSMTestDriver::Stop(vtksysProcess* p, const char* name)
{
  if (p)
  {
    cerr << "vtkSMTestDriver: killing process " << name << "\n";
    vtksysProcess_Kill(p);
    vtksysProcess_WaitForExit(p, 0);
  }
}

int vtkSMTestDriver::OutputStringHasError(const char* pname, std::string& output)
{
  const char* possibleMPIErrors[] = { "error", "Missing:", "core dumped",
    "process in local group is dead", "Segmentation fault", "erroneous", "ERROR:", "Error:",
    "mpirun can *only* be used with MPI programs", "due to signal", "failure",
    "bnormal termination", "failed", "FAILED", "Failed", 0 };

  const char* nonErrors[] = {
    "Memcheck, a memory error detector",                  // valgrind
    "error in locking authority file",                    // IceT
    "WARNING: Far depth failed sanity check, resetting.", // IceT

#if defined(__APPLE__)
    // reported as https://bugreports.qt.io/browse/QTBUG-58699
    "Layout still needs update after calling -[QNSPanelContentsWrapper layout]",
#endif
    0
  };

  if (this->AllowErrorInOutput)
  {
    return 0;
  }

  std::vector<std::string> lines;
  std::vector<std::string>::iterator it;
  vtksys::SystemTools::Split(output.c_str(), lines);

  int i, j;

  for (it = lines.begin(); it != lines.end(); ++it)
  {
    for (i = 0; possibleMPIErrors[i]; ++i)
    {
      if (it->find(possibleMPIErrors[i]) != it->npos)
      {
        int found = 0;
        for (j = 0; nonErrors[j]; ++j)
        {
          if (it->find(nonErrors[j]) != it->npos)
          {
            found = 1;
          }
        }
        if (!found)
        {
          cerr << "vtkSMTestDriver: ***** Test will fail, because the string: \""
               << possibleMPIErrors[i]
               << "\"\nvtkSMTestDriver: ***** was found in the following output from the " << pname
               << ":\n\"" << it->c_str() << "\"\n";
          return 1;
        }
      }
    }
  }
  return 0;
}

#define VTK_CLEAN_PROCESSES                                                                        \
  for (size_t __kk = 0; __kk < clients.size(); __kk++)                                             \
  {                                                                                                \
    vtksysProcess_Delete(clients[__kk]);                                                           \
  }                                                                                                \
  for (int __kk = 0; __kk < this->NumberOfServers; __kk++)                                         \
  {                                                                                                \
    if (!renderServers.empty())                                                                    \
      vtksysProcess_Delete(renderServers[__kk]);                                                   \
    if (!servers.empty())                                                                          \
      vtksysProcess_Delete(servers[__kk]);                                                         \
  }

//----------------------------------------------------------------------------
int vtkSMTestDriver::Main(int argc, char* argv[])
{
  vtksys::SystemTools::PutEnv("DASHBOARD_TEST_FROM_CTEST=1");
  // we add this so that vtksys::SystemTools::EnableMSVCDebugHook() works. At
  // somepoint vtksys needs to be updated to use the newer variable.
  vtksys::SystemTools::PutEnv("DART_TEST_FROM_DART=1");
  vtksys::SystemTools::PutEnv("PARAVIEW_SMTESTDRIVER=1");
  vtksys::SystemTools::EnableMSVCDebugHook();

#ifdef PV_TEST_INIT_COMMAND
  // run user-specified commands before initialization.
  // For example: "killall -9 rsh paraview;"
  if (strlen(PV_TEST_INIT_COMMAND) > 0)
  {
    std::vector<vtksys::String> commands =
      vtksys::SystemTools::SplitString(PV_TEST_INIT_COMMAND, ';');
    for (unsigned int cc = 0; cc < commands.size(); cc++)
    {
      std::string command = commands[cc];
      if (command.size() > 0)
      {
        system(command.c_str());
      }
    }
  }
#endif

  // temporary hack
  // when pvserver supports -rc option then this can be removed
  // until then, ReverseConnection must be set before CollectConfiguredOptions
  // is called.
  for (int k = 0; k < argc; k++)
  {
    if (strcmp(argv[k], "--test-rc") == 0)
    {
      this->ReverseConnection = 1;
    }
  }
  this->CollectConfiguredOptions();
  if (!this->ProcessCommandLine(argc, argv))
  {
    return 1;
  }

  // mpi code
  // Allocate process managers.
  std::vector<vtksysProcess*> renderServers;
  std::vector<vtksysProcess*> servers;
  std::vector<vtksysProcess*> clients;
  vtksysProcess* script = 0;
  for (int i = 0; this->TestRenderServer && i < this->NumberOfServers; i++)
  {
    vtksysProcess* renderServer = vtksysProcess_New();
    if (!renderServer)
    {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the render server.\n";
      return 1;
    }
    renderServers.push_back(renderServer);
  }
  for (int i = 0; this->TestServer && i < this->NumberOfServers; i++)
  {
    vtksysProcess* server = vtksysProcess_New();
    if (!server)
    {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the server (or dataserver).\n";
      return 1;
    }
    servers.push_back(server);
  }
  for (size_t cc = 0; cc < this->ClientExecutables.size(); cc++)
  {
    vtksysProcess* client = vtksysProcess_New();
    if (!client)
    {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the client.\n";
      return 1;
    }
    clients.push_back(client);
  }
  if (this->TestScript)
  {
    script = vtksysProcess_New();
    if (!script)
    {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the script.\n";
      return 1;
    }
  }

  std::vector<char> ClientStdOut;
  std::vector<char> ClientStdErr;
  std::vector<char> ServerStdOut;
  std::vector<char> ServerStdErr;
  std::vector<char> RenderServerStdOut;
  std::vector<char> RenderServerStdErr;
  std::vector<char> ScriptStdOut;
  std::vector<char> ScriptStdErr;

  if (this->ReverseConnection)
  {
    // FIXME: NEED TO FIX REVERSE CONNECTION TESTS
    // reverse connection is not supported with multiple clients.
    assert(clients.size() == 1);

    std::string connection_info;
    this->SetupClient(clients[0], this->ClientExecutables[0], argv);
    if (!this->StartProcessAndWait(
          clients[0], "client", ClientStdOut, ClientStdErr, "Waiting", connection_info))
    {
      cerr << "vtkSMTestDriver: Reverse connection client never started.\n";
      VTK_CLEAN_PROCESSES;
      return -1;
    }

    // Now run the server
    if (!servers.empty())
    {
      this->SetupServer(servers[0],
        this->TestRenderServer ? this->DataServerExecutable : this->ServerExecutable, argv);
      if (!this->StartProcess(servers[0], "server"))
      {
        this->Stop(clients[0], "client");
        VTK_CLEAN_PROCESSES;
        return -1;
      }
    }

    // Now run the render server
    if (!renderServers.empty())
    {
      this->SetupServer(renderServers[0], this->RenderServerExecutable, argv);
      if (!this->StartProcess(renderServers[0], "renderserver"))
      {
        this->Stop(clients[0], "client");
        this->Stop(servers[0], "server");
        VTK_CLEAN_PROCESSES;
        return -1;
      }
    }
  }
  else
  {
    // Start the render server if there is one.
    std::vector<std::string> rs_connection_infos;
    for (std::vector<vtksysProcess*>::iterator iter = renderServers.begin();
         iter != renderServers.end(); iter++)
    {
      vtksysProcess* renderServer = *iter;
      std::string rs_connection_info;
      this->SetupServer(renderServer, this->RenderServerExecutable, argv);

      std::ostringstream processName;
      processName << "renderserver " << rs_connection_infos.size();

      if (!this->StartProcessAndWait(renderServer, processName.str().c_str(), RenderServerStdOut,
            RenderServerStdErr, "Accepting connection(s):", rs_connection_info))
      {
        cerr << "vtkSMTestDriver: Render server never started.\n";
        VTK_CLEAN_PROCESSES;
        return -1;
      }
      rs_connection_infos.push_back(rs_connection_info);
    }

    // Start the data server if there is one.
    std::vector<std::string> s_connection_infos;
    for (std::vector<vtksysProcess*>::iterator iter = servers.begin(); iter != servers.end();
         iter++)
    {
      vtksysProcess* server = *iter;
      std::string s_connection_info;
      this->SetupServer(
        server, this->TestRenderServer ? this->DataServerExecutable : this->ServerExecutable, argv);

      std::ostringstream processName;
      processName << (renderServers.empty() ? "server " : "dataserver ")
                  << s_connection_infos.size();

      if (!this->StartProcessAndWait(server, processName.str().c_str(), ServerStdOut, ServerStdErr,
            "Accepting connection(s):", s_connection_info))
      {
        for (std::vector<vtksysProcess*>::iterator stopIter = renderServers.begin();
             stopIter != renderServers.end(); stopIter++)
        {
          this->Stop(*stopIter, "renderserver");
        }

        cerr << "vtkSMTestDriver: Server never started.\n";
        VTK_CLEAN_PROCESSES;
        return -1;
      }
      s_connection_infos.push_back(s_connection_info);
    }

    // before launching the clients, we need to identify the host and port for
    // the server processes so we can tell the client.
    if ((!renderServers.empty()) && (!servers.empty()))
    {
      std::string rs_host("localhost"), ds_host("localhost");
      int rs_port(22221), ds_port(11111);
      std::ostringstream stream;
      stream << "-url=";
      for (int i = 0; i < this->NumberOfServers; i++)
      {
        GetHostAndPort(rs_connection_infos[i], rs_host, rs_port);
        GetHostAndPort(s_connection_infos[i], ds_host, ds_port);
        stream << "cdsrs://" << ds_host.c_str() << ":" << ds_port << "/" << rs_host.c_str() << ":"
               << rs_port;
        if ((i + 1) < this->NumberOfServers)
        {
          stream << "|";
        }
      }
      this->ServerURL = stream.str();
    }
    else if (!servers.empty())
    {
      std::string host("localhost");
      int port(11111);
      std::ostringstream stream;
      stream << "-url=";
      for (int i = 0; i < this->NumberOfServers; i++)
      {
        GetHostAndPort(s_connection_infos[i], host, port);
        stream << "cs://" << host.c_str() << ":" << port;
        if ((i + 1) < this->NumberOfServers)
        {
          stream << "|";
        }
      }
      this->ServerURL = stream.str();
    }

    // Now run the client(s).
    for (size_t cc = 0; cc < clients.size(); cc++)
    {
      std::ostringstream client_name;
      client_name << "client";
      if (clients.size() > 1)
      {
        client_name << cc;
        // wait a few seconds before launching the additional clients for
        // collaboration mode.
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }

      std::string output_to_ignore;
      this->SetupClient(clients[cc], this->ClientExecutables[cc], argv);
      if (!this->StartProcessAndWait(clients[cc], client_name.str().c_str(), ClientStdOut,
            ClientStdErr, "started", output_to_ignore))
      {
        for (std::vector<vtksysProcess*>::iterator stopIter = servers.begin();
             stopIter != servers.end(); stopIter++)
        {
          this->Stop(*stopIter, "server");
        }
        for (std::vector<vtksysProcess*>::iterator stopIter = renderServers.begin();
             stopIter != renderServers.end(); stopIter++)
        {
          this->Stop(*stopIter, "renderserver");
        }
        VTK_CLEAN_PROCESSES;
        return -1;
      }
    }
  }
  // Start the script if there is one
  if (script)
  {
    std::string output_to_ignore;
    this->SetupServer(script, this->ScriptExecutable, argv);
    std::ostringstream processName;
    processName << "script";
    if (!this->StartProcessAndWait(
          script, processName.str().c_str(), ScriptStdOut, ScriptStdErr, "", output_to_ignore))
    {
      cerr << "vtkSMTestDriver: Script never started.\n";
      VTK_CLEAN_PROCESSES;
      return -1;
    }
  }

  // Report the output of the processes.
  int clientPipe = 1;
  std::string output;
  int mpiError = 0;
  std::vector<int> client_pipe_status;
  client_pipe_status.resize(clients.size(), 1);
  while (clientPipe)
  {
    clientPipe = 0;
    for (size_t cc = 0; cc < clients.size(); cc++)
    {
      // we don't stop listening from clients until all clients have closed
      // their pipes.
      if (client_pipe_status[cc])
      {
        client_pipe_status[cc] =
          this->WaitForAndPrintLine("client", clients[cc], output, 0.1, ClientStdOut, ClientStdErr);
      }
      clientPipe |= client_pipe_status[cc];
    }
    if (!mpiError && this->OutputStringHasError("client", output))
    {
      mpiError = 1;
    }
    // If client has died, we wait for output from the server processes
    // for this->ServerExitTimeOut, then we'll kill the servers, if needed.
    double timeout = (clientPipe) ? 0.1 : this->ServerExitTimeOut;

    for (std::vector<vtksysProcess*>::iterator waitIter = servers.begin();
         waitIter != servers.end(); waitIter++)
    {
      output = "";
      this->WaitForAndPrintLine(
        "server", *waitIter, output, timeout, ServerStdOut, ServerStdErr, 0);
      if (!mpiError && this->OutputStringHasError("server", output))
      {
        mpiError = 1;
      }
    }

    for (std::vector<vtksysProcess*>::iterator waitIter = renderServers.begin();
         waitIter != renderServers.end(); waitIter++)
    {
      output = "";
      this->WaitForAndPrintLine(
        "renderserver", *waitIter, output, timeout, RenderServerStdOut, RenderServerStdErr, 0);
      if (!mpiError && this->OutputStringHasError("renderserver", output))
      {
        mpiError = 1;
      }
    }

    if (script)
    {
      output = "";
      this->WaitForAndPrintLine("script", script, output, timeout, ScriptStdOut, ScriptStdErr, 0);
      if (!mpiError &&
        (!this->ScriptIgnoreOutputErrors && this->OutputStringHasError("script", output)))
      {
        mpiError = 1;
      }
    }

    output = "";
  }

  // Wait for the client and server to exit.
  for (size_t cc = 0; cc < clients.size(); cc++)
  {
    vtksysProcess_WaitForExit(clients[cc], 0);
  }

  // Once client is finished, the servers
  // must finish quickly. If not, is usually is a sign that
  // the client crashed/exited before it attempted to connect to
  // the server.
  for (std::vector<vtksysProcess*>::iterator exitIter = servers.begin(); exitIter != servers.end();
       exitIter++)
  {
    vtksysProcess_WaitForExit(*exitIter, &this->ServerExitTimeOut);
  }
  for (std::vector<vtksysProcess*>::iterator exitIter = renderServers.begin();
       exitIter != renderServers.end(); exitIter++)
  {
    vtksysProcess_WaitForExit(*exitIter, &this->ServerExitTimeOut);
  }
  // We wait for the script to finish
  vtksysProcess_WaitForExit(script, &this->ScriptExitTimeOut);

#ifdef PV_TEST_CLEAN_COMMAND
  // If any executable did not exit properly, run a user-specified
  // command to cleanup leftover processes.  This is needed for tests
  // that fail when running in MPI mode.
  //
  // For example: "killall -9 rsh paraview"
  //
  if (strlen(PV_TEST_CLEAN_COMMAND) > 0)
  {
    system(PV_TEST_CLEAN_COMMAND);
  }
#endif
  // Get the results (0 == success).
  int clientResult = 0;
  for (size_t cc = 0; cc < clients.size(); cc++)
  {
    std::ostringstream client_name;
    client_name << "client" << cc;
    int cur_result = this->ReportStatus(clients[cc], client_name.str().c_str());
    // 0 == success
    clientResult |= cur_result;
  }
  int serverResult = 0;
  for (std::vector<vtksysProcess*>::iterator killIter = servers.begin(); killIter != servers.end();
       killIter++)
  {
    serverResult += this->ReportStatus(*killIter, "server");
    vtksysProcess_Kill(*killIter);
  }
  int renderServerResult = 0;
  for (std::vector<vtksysProcess*>::iterator killIter = renderServers.begin();
       killIter != renderServers.end(); killIter++)
  {
    renderServerResult += this->ReportStatus(*killIter, "renderserver");
    vtksysProcess_Kill(*killIter);
  }
  int scriptResult = 0;
  int scriptState = 0;
  if (script)
  {
    scriptResult += this->ReportStatus(script, "script");
    scriptState = vtksysProcess_GetState(script);
    vtksysProcess_Kill(script);
  }

  // Free process managers.
  VTK_CLEAN_PROCESSES;

  // Report the server return code if it is nonzero.  Otherwise report
  // the client return code.
  if (serverResult)
  {
    return serverResult;
  }
  // if renderServer return code is nonzero then return it
  if (renderServerResult)
  {
    return renderServerResult;
  }
  // ignore script executing error
  if (scriptResult && scriptState != vtksysProcess_State_Executing)
  {
    return scriptResult;
  }
  if (mpiError)
  {
    cerr << "vtkSMTestDriver: Error string found in output, vtkSMTestDriver returning " << mpiError
         << "\n";
    return mpiError;
  }
  // if both servers are fine return the client result
  return clientResult;
}

//----------------------------------------------------------------------------
void vtkSMTestDriver::ReportCommand(const char* const* command, const char* name)
{
  cerr << "vtkSMTestDriver: " << name << " command is:\n";
  for (const char* const* c = command; *c; ++c)
  {
    cerr << " \"" << *c << "\"";
  }
  cerr << "\n";
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::ReportStatus(vtksysProcess* process, const char* name)
{
  int result = 1;
  switch (vtksysProcess_GetState(process))
  {
    case vtksysProcess_State_Starting:
    {
      cerr << "vtkSMTestDriver: Never started " << name << " process.\n";
    }
    break;
    case vtksysProcess_State_Error:
    {
      cerr << "vtkSMTestDriver: Error executing " << name
           << " process: " << vtksysProcess_GetErrorString(process) << "\n";
    }
    break;
    case vtksysProcess_State_Exception:
    {
      cerr << "vtkSMTestDriver: " << name << " process exited with an exception: ";
      switch (vtksysProcess_GetExitException(process))
      {
        case vtksysProcess_Exception_None:
        {
          cerr << "None";
        }
        break;
        case vtksysProcess_Exception_Fault:
        {
          cerr << "Segmentation fault";
        }
        break;
        case vtksysProcess_Exception_Illegal:
        {
          cerr << "Illegal instruction";
        }
        break;
        case vtksysProcess_Exception_Interrupt:
        {
          cerr << "Interrupted by user";
        }
        break;
        case vtksysProcess_Exception_Numerical:
        {
          cerr << "Numerical exception";
        }
        break;
        case vtksysProcess_Exception_Other:
        {
          cerr << "Unknown";
        }
        break;
      }
      cerr << "\n";
    }
    break;
    case vtksysProcess_State_Executing:
    {
      cerr << "vtkSMTestDriver: Never terminated " << name << " process.\n";
    }
    break;
    case vtksysProcess_State_Exited:
    {
      result = vtksysProcess_GetExitValue(process);
      cerr << "vtkSMTestDriver: " << name << " process exited with code " << result << "\n";
    }
    break;
    case vtksysProcess_State_Expired:
    {
      cerr << "vtkSMTestDriver: killed " << name << " process due to timeout.\n";
    }
    break;
    case vtksysProcess_State_Killed:
    {
      cerr << "vtkSMTestDriver: killed " << name << " process.\n";
    }
    break;
  }
  return result;
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::WaitForLine(vtksysProcess* process, std::string& line, double timeout,
  std::vector<char>& out, std::vector<char>& err)
{
  line = "";
  std::vector<char>::iterator outiter = out.begin();
  std::vector<char>::iterator erriter = err.begin();
  while (1)
  {
    // Check for a newline in stdout.
    for (; outiter != out.end(); ++outiter)
    {
      if ((*outiter == '\r') && ((outiter + 1) == out.end()))
      {
        break;
      }
      else if (*outiter == '\n' || *outiter == '\0')
      {
        int length = outiter - out.begin();
        if (length > 1 && *(outiter - 1) == '\r')
        {
          --length;
        }
        if (length > 0)
        {
          line.append(&out[0], length);
        }
        out.erase(out.begin(), outiter + 1);
        return vtksysProcess_Pipe_STDOUT;
      }
    }

    // Check for a newline in stderr.
    for (; erriter != err.end(); ++erriter)
    {
      if ((*erriter == '\r') && ((erriter + 1) == err.end()))
      {
        break;
      }
      else if (*erriter == '\n' || *erriter == '\0')
      {
        int length = erriter - err.begin();
        if (length > 1 && *(erriter - 1) == '\r')
        {
          --length;
        }
        if (length > 0)
        {
          line.append(&err[0], length);
        }
        err.erase(err.begin(), erriter + 1);
        return vtksysProcess_Pipe_STDERR;
      }
    }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = vtksysProcess_WaitForData(process, &data, &length, &timeout);
    if (pipe == vtksysProcess_Pipe_Timeout)
    {
      // Timeout has been exceeded.
      return pipe;
    }
    else if (pipe == vtksysProcess_Pipe_STDOUT)
    {
      // Append to the stdout buffer.
      std::vector<char>::size_type size = out.size();
      out.insert(out.end(), data, data + length);
      outiter = out.begin() + size;
    }
    else if (pipe == vtksysProcess_Pipe_STDERR)
    {
      // Append to the stderr buffer.
      std::vector<char>::size_type size = err.size();
      err.insert(err.end(), data, data + length);
      erriter = err.begin() + size;
    }
    else if (pipe == vtksysProcess_Pipe_None)
    {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      if (!out.empty())
      {
        line.append(&out[0], outiter - out.begin());
        out.erase(out.begin(), out.end());
        return vtksysProcess_Pipe_STDOUT;
      }
      else if (!err.empty())
      {
        line.append(&err[0], erriter - err.begin());
        err.erase(err.begin(), err.end());
        return vtksysProcess_Pipe_STDERR;
      }
      else
      {
        return vtksysProcess_Pipe_None;
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkSMTestDriver::PrintLine(const char* pname, const char* line)
{
  // if the name changed then the line is output from a different process
  if (this->CurrentPrintLineName != pname)
  {
    cerr << "-------------- " << pname << " output --------------\n";
    // save the current pname
    this->CurrentPrintLineName = pname;
  }
  cerr << line << "\n";
  cerr.flush();
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::WaitForAndPrintLine(const char* pname, vtksysProcess* process,
  std::string& line, double timeout, std::vector<char>& out, std::vector<char>& err,
  const char* string_to_wait_for, int* foundWaiting, std::string* matched_line /*=NULL*/)
{
  int pipe = this->WaitForLine(process, line, timeout, out, err);
  if (pipe == vtksysProcess_Pipe_STDOUT || pipe == vtksysProcess_Pipe_STDERR)
  {
    this->PrintLine(pname, line.c_str());
    if (foundWaiting && string_to_wait_for && (line.find(string_to_wait_for) != line.npos))
    {
      *foundWaiting = 1;
      if (matched_line)
      {
        *matched_line = line;
      }
    }
  }
  return pipe;
}

//----------------------------------------------------------------------------
bool vtkSMTestDriver::SetupServer(vtksysProcess* server, const ExecutableInfo& info, char* argv[])
{
  if (server)
  {
    std::vector<const char*> serverCommand;
    this->CreateCommandLine(serverCommand, info.Executable.c_str(), info.Type,
      info.Type == SCRIPT ? this->MPIScriptNumProcessFlag.c_str()
                          : this->MPIServerNumProcessFlag.c_str(),
      info.ArgStart, info.ArgEnd, argv);
    this->ReportCommand(&serverCommand[0], info.TypeName.c_str());
    vtksysProcess_SetCommand(server, &serverCommand[0]);
    vtksysProcess_SetWorkingDirectory(server, this->GetDirectory(info.Executable).c_str());
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkSMTestDriver::SetupClient(vtksysProcess* process, const ExecutableInfo& info, char* argv[])
{
  if (process)
  {
    std::vector<const char*> clientCommand;
    this->CreateCommandLine(clientCommand, info.Executable.c_str(), CLIENT,
      this->MPIServerNumProcessFlag.c_str(), info.ArgStart, info.ArgEnd, argv);
    if (!this->ReverseConnection && !this->ServerURL.empty())
    {
      // push-back server url, if present.
      std::vector<const char*>::iterator iter = clientCommand.begin();
      iter++;
      clientCommand.insert(iter, this->ServerURL.c_str());
      clientCommand.push_back(NULL);
    }
    this->ReportCommand(&clientCommand[0], "client");
    vtksysProcess_SetCommand(process, &clientCommand[0]);
    vtksysProcess_SetWorkingDirectory(process, this->GetDirectory(info.Executable.c_str()).c_str());
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
std::string vtkSMTestDriver::GetDirectory(std::string location)
{
  return vtksys::SystemTools::GetParentDirectory(location.c_str());
}
