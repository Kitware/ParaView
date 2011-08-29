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

#include <vtksys/SystemTools.hxx>
#include <vtksys/String.hxx>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h>
# include <sys/wait.h>
#endif

// The main function as this class should only be used by this program
int main(int argc, char* argv[])
{
  vtkSMTestDriver d;
  return d.Main(argc, argv);
}

vtkSMTestDriver::vtkSMTestDriver()
{
  this->AllowErrorInOutput = 0;
  this->RenderServerNumProcesses = 0;
  this->TimeOut = 300;
  this->ServerExitTimeOut = 60;
  this->TestRenderServer = 0;
  this->TestServer = 0;
  this->TestTiledDisplay = 0;
  this->ReverseConnection = 0;
  this->TestRemoteRendering = 0;
}

vtkSMTestDriver::~vtkSMTestDriver()
{
}

// now implement the vtkSMTestDriver class

void vtkSMTestDriver::SeparateArguments(const char* str,
                                     vtkstd::vector<vtkstd::string>& flags)
{
  vtkstd::string arg = str;
  vtkstd::string::size_type pos1 = 0;
  vtkstd::string::size_type pos2 = arg.find_first_of(" ;");
  if(pos2 == arg.npos)
    {
    flags.push_back(str);
    return;
    }
  while(pos2 != arg.npos)
    {
    flags.push_back(arg.substr(pos1, pos2-pos1));
    pos1 = pos2+1;
    pos2 = arg.find_first_of(" ;", pos1+1);
    }
  flags.push_back(arg.substr(pos1, pos2-pos1));
}


void vtkSMTestDriver::CollectConfiguredOptions()
{
  // try to make sure that this timesout before dart so it can kill all the processes
  this->TimeOut = DART_TESTING_TIMEOUT - 10.0;
  if(this->TimeOut < 0)
    {
    this->TimeOut = 1500;
    }

  // now find all the mpi information if mpi run is set
#ifdef VTK_USE_MPI
#ifdef VTK_MPIRUN_EXE
  this->MPIRun = VTK_MPIRUN_EXE;
#else
  cerr << "Error: VTK_MPIRUN_EXE must be set when VTK_USE_MPI is on.\n";
  return;
#endif
  int serverNumProc = 1;
  int renderNumProc = 1;

# ifdef VTK_MPI_MAX_NUMPROCS
  serverNumProc = VTK_MPI_MAX_NUMPROCS;
  renderNumProc = serverNumProc-1;
  if ( renderNumProc <= 0 )
    {
    renderNumProc = 1;
    }
# endif
# ifdef VTK_MPI_NUMPROC_FLAG
  this->MPINumProcessFlag = VTK_MPI_NUMPROC_FLAG;
# else
  cerr << "Error VTK_MPI_NUMPROC_FLAG must be defined to run test if MPI is on.\n";
  return;
# endif
#ifdef VTK_MPI_PRENUMPROC_FLAGS
  this->SeparateArguments(VTK_MPI_PRENUMPROC_FLAGS, this->MPIPreNumProcFlags);
#endif
# ifdef VTK_MPI_PREFLAGS
  this->SeparateArguments(VTK_MPI_PREFLAGS, this->MPIPreFlags);
# endif
# ifdef VTK_MPI_POSTFLAGS
  this->SeparateArguments(VTK_MPI_POSTFLAGS, this->MPIPostFlags);
# endif
  char buf[1024];
  sprintf(buf, "%d", serverNumProc);
  this->MPIServerNumProcessFlag = buf;
  sprintf(buf, "%d", renderNumProc);
  this->MPIRenderServerNumProcessFlag = buf;

#endif // VTK_USE_MPI

# ifdef VTK_MPI_SERVER_PREFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_PREFLAGS, this->MPIServerPreFlags);
# endif
# ifdef VTK_MPI_SERVER_POSTFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_POSTFLAGS, this->MPIServerPostFlags);
# endif
# ifdef VTK_MPI_SERVER_TD_PREFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_TD_PREFLAGS, this->TDServerPreFlags);
# endif
# ifdef VTK_MPI_SERVER_TD_POSTFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_TD_POSTFLAGS, this->TDServerPostFlags);
# endif

// For remote testing (via ssh)
# ifdef PV_SSH_FLAGS
  this->SeparateArguments(PV_SSH_FLAGS, this->PVSSHFlags);
# endif //PV_SSH_FLAGS

# ifdef PV_SETUP_SCRIPT
  this->PVSetupScript = PV_SETUP_SCRIPT;
# endif //PV_SETUP_SCRIPT
}

/// This adds the debug/build configuration crap for the executable on windows.
static vtkstd::string FixExecutablePath(const vtkstd::string& path)
{
#ifdef  CMAKE_INTDIR
  vtkstd::string parent_dir =
    vtksys::SystemTools::GetFilenamePath(path.c_str());

  vtkstd::string filename =
    vtksys::SystemTools::GetFilenameName(path);
  parent_dir += "/" CMAKE_INTDIR "/";
  return parent_dir + filename;
#endif

  return path;
}

int vtkSMTestDriver::ProcessCommandLine(int argc, char* argv[])
{
  this->ArgStart = 1;
  int i;
  for(i =1; i < argc - 1; ++i)
    {
    if(strcmp(argv[i], "--client") == 0)
      {
      this->ArgStart = i+2;
      this->ClientExecutable = ::FixExecutablePath(argv[i+1]);
      }
    if(strcmp(argv[i], "--test-remote-rendering") == 0)
      {
      this->ArgStart = i+1;
      this->TestRemoteRendering = 1;
      fprintf(stderr, "Test Render Server.\n");
      }
    if(strcmp(argv[i], "--render-server") == 0)
      {
      this->ArgStart = i+2;
      this->TestRenderServer = 1;
      this->RenderServerExecutable = ::FixExecutablePath(argv[i+1]);
      fprintf(stderr, "Test Render Server.\n");
      }
    if (strcmp(argv[i], "--data-server") == 0)
      {
      this->ArgStart = i+2;
      this->TestServer = 1;
      this->DataServerExecutable = ::FixExecutablePath(argv[i+1]);
      fprintf(stderr, "Test Render Server.\n");
      }
    if(strcmp(argv[i], "--server") == 0)
      {
      this->ArgStart = i+2;
      this->TestServer = 1;
      this->ServerExecutable = ::FixExecutablePath(argv[i+1]);
      fprintf(stderr, "Test Server.\n");
      }
    if(strcmp(argv[i], "--test-tiled") == 0)
      {
      this->ArgStart = i+1;
      this->TestServer = 1;
      this->TestTiledDisplay = 1;
      fprintf(stderr, "Test Tiled Display.\n");
      }
    if(strcmp(argv[i], "--one-mpi-np") == 0)
      {
      this->MPIServerNumProcessFlag = 
        this->MPIRenderServerNumProcessFlag = "1";
      this->ArgStart = i+1;
      fprintf(stderr, "Test with one mpi process.\n");
      }
    if(strcmp(argv[i], "--test-rc") == 0)
      {
      this->ArgStart = i+1;
      this->ReverseConnection = 1;
      fprintf(stderr, "Test reverse connection.\n");
      }
    if(strcmp(argv[i], "--timeout") == 0)
      {
      this->ArgStart = i+2;
      this->TimeOut = atoi(argv[i+1]);
      fprintf(stderr, "The timeout was set to %f.\n", this->TimeOut);
      }
    if (strncmp(argv[i], "--server-exit-timeout",
        strlen("--server-exit-timeout")) == 0)
      {
      this->ArgStart = i+2;
      this->ServerExitTimeOut = atoi(argv[i+1]);
      fprintf(stderr, "The server exit timeout was set to %f.\n", 
        this->ServerExitTimeOut);
      }
    if(strncmp(argv[i], "--server-preflags",17) == 0)
      {
      this->SeparateArguments(argv[i+1], this->MPIServerPreFlags);
      this->ArgStart = i+2;
      fprintf(stderr, "Extras server preflags were specified: %s\n", argv[i+1]);
      }
    if(strncmp(argv[i], "--server-postflags",18 ) == 0)
      {
      this->SeparateArguments(argv[i+1], this->MPIServerPostFlags);
      this->ArgStart = i+2;
      fprintf(stderr, "Extras server postflags were specified: %s\n", argv[i+1]);
      }
    if (strncmp(argv[i], "--allow-errors", strlen("--allow-errors"))==0)
      {
      this->ArgStart =i+1;
      this->AllowErrorInOutput = 1;
      fprintf(stderr, "The allow erros in output flag was set to %d.\n", 
        this->AllowErrorInOutput);
      }
    }

  return 1;
}

void
vtkSMTestDriver::CreateCommandLine(vtksys_stl::vector<const char*>& commandLine,
                                const char* paraView,
                                vtkSMTestDriver::ProcessType type,
                                const char* numProc,
                                int argStart,
                                int argCount,
                                char* argv[])
{
  if(this->MPIRun.size() && type != CLIENT)
    {
    commandLine.push_back(this->MPIRun.c_str());
    if (!this->TestTiledDisplay)
      {
      for (unsigned int i = 0; i < this->MPIPreNumProcFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIPreNumProcFlags[i].c_str());
        }
      }
    commandLine.push_back(this->MPINumProcessFlag.c_str());
    commandLine.push_back(numProc);
    if (!this->TestTiledDisplay)
      {
      for(unsigned int i = 0; i < this->MPIPreFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIPreFlags[i].c_str());
        }
      // If there is specific flags for the server to pass to mpirun, add them
      if( type == SERVER || type == DATA_SERVER )
        {
        for(unsigned int i = 0; i < this->MPIServerPreFlags.size(); ++i)
          {
          commandLine.push_back(this->MPIServerPreFlags[i].c_str());
          }
        }
      }
    else ///  When tile display is enabled.
      {
      // If there is specific flags for the server to pass to mpirun, add them
      if( type == SERVER || type == DATA_SERVER)
        {
        for(unsigned int i = 0; i < this->TDServerPreFlags.size(); ++i)
          {
          commandLine.push_back(this->TDServerPreFlags[i].c_str());
          }
        }
      }
    }

  if(this->PVSSHFlags.size() && (type == SERVER || type == DATA_SERVER))
    {
      {
      // First add the ssh command:
      for(unsigned int i = 0; i < this->PVSSHFlags.size(); ++i)
        {
        commandLine.push_back(this->PVSSHFlags[i].c_str());
        }
      // then the paraview intialization:
      if( this->PVSetupScript.size() )
        {
        commandLine.push_back(this->PVSetupScript.c_str());
        }
      }
    }
  else
    {
    commandLine.push_back(paraView);
    if (type == CLIENT)
      {
      for (unsigned int i=0; i < this->ClientPostFlags.size(); ++i)
        {
        commandLine.push_back(this->ClientPostFlags[i].c_str());
        }
      }

    if(this->ReverseConnection && type != CLIENT)
      {
      commandLine.push_back("-rc");
#ifdef PV_TEST_CLIENT
      commandLine.push_back("-ch=" PV_TEST_CLIENT);
#endif
      }

    if (!this->TestTiledDisplay)
      {
      for(unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
        {
        commandLine.push_back(MPIPostFlags[i].c_str());
        }
      // If there is specific flags for the server to pass to mpirun, add them
      if( type == SERVER || type == DATA_SERVER )
        {
        for(unsigned int i = 0; i < this->MPIServerPostFlags.size(); ++i)
          {
          commandLine.push_back(this->MPIServerPostFlags[i].c_str());
          }
        }
      }
    else
      {
      // If there is specific flags for the server to pass to mpirun, add them
      if( type == SERVER || type == DATA_SERVER )
        {
        for(unsigned int i = 0; i < this->TDServerPostFlags.size(); ++i)
          {
          commandLine.push_back(this->TDServerPostFlags[i].c_str());
          }
        }
      }

    // remaining flags for the test
    for(int ii = argStart; ii < argCount; ++ii)
      {
      commandLine.push_back(argv[ii]);
      }
    }

#if !defined(__APPLE__)
  if ( this->TestRemoteRendering && (type == SERVER || type == RENDER_SERVER) )
    {
    commandLine.push_back("--use-offscreen-rendering");
    }
#endif

  commandLine.push_back(0);
}

int vtkSMTestDriver::StartServer(vtksysProcess* server, const char* name,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err)
{
  if(!server)
    {
    return 1;
    }
  cerr << "vtkSMTestDriver: starting process " << name << "\n";
  vtksysProcess_SetTimeout(server, this->TimeOut);
  vtksysProcess_Execute(server);
  int foundWaiting = 0;
  vtkstd::string output;
  while(!foundWaiting)
    {
    int pipe = this->WaitForAndPrintLine(name, server, output, 100.0, out, err,
                                         &foundWaiting);
    if(pipe == vtksysProcess_Pipe_None ||
       pipe == vtksysProcess_Pipe_Timeout)
      {
      break;
      }
    }
  if(foundWaiting)
    {
    cerr << "vtkSMTestDriver: " << name << " sucessfully started.\n";
    return 1;
    }
  else
    {
    cerr << "vtkSMTestDriver: " << name << " never started.\n";
    vtksysProcess_Kill(server);
    return 0;
    }
}

int vtkSMTestDriver::StartClient(vtksysProcess* client, const char* name)
{
  if(!client)
    {
    return 1;
    }
  cerr << "vtkSMTestDriver: starting process " << name << "\n";
  vtksysProcess_SetTimeout(client, this->TimeOut);
  vtksysProcess_Execute(client);
  if(vtksysProcess_GetState(client) == vtksysProcess_State_Executing)
    {
    cerr << "vtkSMTestDriver: " << name << " sucessfully started.\n";
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
  if(p)
    {
    cerr << "vtkSMTestDriver: killing process " << name << "\n";
    vtksysProcess_Kill(p);
    vtksysProcess_WaitForExit(p, 0);
    }
}

int vtkSMTestDriver::OutputStringHasError(const char* pname, vtkstd::string& output)
{
  const char* possibleMPIErrors[] = {
    "error",
    "Missing:",
    "core dumped",
    "process in local group is dead",
    "Segmentation fault",
    "erroneous",
    "ERROR:",
    "Error:",
    "mpirun can *only* be used with MPI programs",
    "due to signal",
    "failure",
    "bnormal termination",
    "failed",
    "FAILED",
    "Failed",
    0
  };

  const char* nonErrors[] = {
    "Memcheck, a memory error detector",  //valgrind
    "error in locking authority file",  //IceT
    "WARNING: Far depth failed sanity check, resetting.", //IceT
    0
  };

  if(this->AllowErrorInOutput)
    {
    return 0;
    }

  vtkstd::vector<vtkstd::string> lines;
  vtkstd::vector<vtkstd::string>::iterator it;
  vtksys::SystemTools::Split(output.c_str(), lines);

  int i, j;

  for ( it = lines.begin(); it != lines.end(); ++ it )
    {
    for(i =0; possibleMPIErrors[i]; ++i)
      {
      if(it->find(possibleMPIErrors[i]) != it->npos)
        {
        int found = 0;
        for (j = 0; nonErrors[j]; ++ j)
          {
          if ( it->find(nonErrors[j]) != it->npos )
            {
            found = 1;
            }
          }
        if ( !found )
          {
          cerr << "vtkSMTestDriver: ***** Test will fail, because the string: \""
            << possibleMPIErrors[i]
            << "\"\nvtkSMTestDriver: ***** was found in the following output from the "
            << pname << ":\n\""
            << it->c_str() << "\"\n";
          return 1;
          }
        }
      }
    }
  return 0;
}

#define VTK_CLEAN_PROCESSES \
  vtksysProcess_Delete(client); \
  vtksysProcess_Delete(renderServer); \
  vtksysProcess_Delete(server); 

//----------------------------------------------------------------------------
int vtkSMTestDriver::Main(int argc, char* argv[])
{
  vtksys::SystemTools::PutEnv("DASHBOARD_TEST_FROM_CTEST=1");

#ifdef PV_TEST_INIT_COMMAND
  // run user-specified commands before initialization.
  // For example: "killall -9 rsh paraview;"
  if(strlen(PV_TEST_INIT_COMMAND) > 0)
    {
    vtkstd::vector<vtksys::String> commands = vtksys::SystemTools::SplitString(
      PV_TEST_INIT_COMMAND, ';');
    for (unsigned int cc=0; cc < commands.size(); cc++)
      {
      vtkstd::string command = commands[cc];
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
  for(int k =0; k < argc; k++)
    {
    if(strcmp(argv[k],"--test-rc") == 0)
      {
      this->ReverseConnection = 1;
      }
    }
  this->CollectConfiguredOptions();
  if(!this->ProcessCommandLine(argc, argv))
    {
    return 1;
    }

  // mpi code
  // Allocate process managers.
  vtksysProcess* renderServer = 0;
  vtksysProcess* server = 0;
  vtksysProcess* client = 0;
  if(this->TestRenderServer)
    {
    renderServer = vtksysProcess_New();
    if(!renderServer)
      {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the render server.\n";
      return 1;
      }
    }
  if(this->TestServer)
    {
    server = vtksysProcess_New();
    if(!server)
      {
      VTK_CLEAN_PROCESSES;
      cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the server.\n";
      return 1;
      }
    }
  client = vtksysProcess_New();
  if(!client)
    {
    VTK_CLEAN_PROCESSES;
    cerr << "vtkSMTestDriver: Cannot allocate vtksysProcess to run the client.\n";
    return 1;
    }

  vtkstd::vector<char> ClientStdOut;
  vtkstd::vector<char> ClientStdErr;
  vtkstd::vector<char> ServerStdOut;
  vtkstd::vector<char> ServerStdErr;
  vtkstd::vector<char> RenderServerStdOut;
  vtkstd::vector<char> RenderServerStdErr;

  // Construct the render server process command line
  vtksys_stl::vector<const char*> renderServerCommand;
  if(renderServer)
    {
    this->CreateCommandLine(renderServerCommand,
                            this->RenderServerExecutable.c_str(),
                            RENDER_SERVER,
                            this->MPIRenderServerNumProcessFlag.c_str());
    this->ReportCommand(&renderServerCommand[0], "renderserver");
    vtksysProcess_SetCommand(renderServer, &renderServerCommand[0]);
    vtksysProcess_SetWorkingDirectory(renderServer,
      this->GetDirectory(this->RenderServerExecutable).c_str());
    }

  vtksys_stl::vector<const char*> serverCommand;
  if(server)
    {
    const char* serverExe = this->ServerExecutable.c_str();
    vtkSMTestDriver::ProcessType serverType = SERVER;
    if(this->TestRenderServer)
      {
      serverExe = this->DataServerExecutable.c_str();
      serverType = DATA_SERVER;
      }


    this->CreateCommandLine(serverCommand,
                            serverExe,
                            serverType,
                            this->MPIServerNumProcessFlag.c_str());
    this->ReportCommand(&serverCommand[0], "server");
    vtksysProcess_SetCommand(server, &serverCommand[0]);
    vtksysProcess_SetWorkingDirectory(server, this->GetDirectory(serverExe).c_str());
    }

  // Construct the client process command line.
  vtksys_stl::vector<const char*> clientCommand;
  
  const char* pv = this->ClientExecutable.c_str();
  this->CreateCommandLine(clientCommand,
                          pv,
                          CLIENT,
                          "",
                          this->ArgStart, argc, argv);
  this->ReportCommand(&clientCommand[0], "client");
  vtksysProcess_SetCommand(client, &clientCommand[0]);
  vtksysProcess_SetWorkingDirectory(client, this->GetDirectory(pv).c_str());

  // Kill the processes if they are taking too long.
  if(this->ReverseConnection)
    {
    if(!this->StartServer(client, "client",
                          ClientStdOut, ClientStdErr))
      {
      cerr << "vtkSMTestDriver: Reverse connection client never started.\n";
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    // Now run the server
    if(!this->StartClient(server, "server"))
      {
      this->Stop(client, "client");
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    // Now run the render server
    if(!this->StartClient(renderServer, "renderserver"))
      {
      this->Stop(client, "client");
      this->Stop(server, "server");
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    }
  else
    {
    // Start the render server if there is one
    if(!this->StartServer(renderServer, "renderserver",
                          RenderServerStdOut, RenderServerStdErr))
      {
      cerr << "vtkSMTestDriver: Render server never started.\n";
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    // Start the data server if there is one
    if(!this->StartServer(server, "server",
                          ServerStdOut, ServerStdErr))
      {
      this->Stop(renderServer, "renderserver");
      cerr << "vtkSMTestDriver: Server never started.\n";
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    // Now run the client
    if(!this->StartClient(client, "client"))
      {
      this->Stop(server, "server");
      this->Stop(renderServer, "renderserver");
      VTK_CLEAN_PROCESSES;
      return -1;
      }
    }

  // Report the output of the processes.
  int clientPipe = 1;
  vtkstd::string output;
  int mpiError = 0;
  while(clientPipe)
    {
    clientPipe = this->WaitForAndPrintLine("client", client, output, 0.1,
                                           ClientStdOut, ClientStdErr, 0);
    if(!mpiError && this->OutputStringHasError("client", output))
      {
      mpiError = 1;
      }
    // If client has died, we wait for output from the server processess
    // for this->ServerExitTimeOut, then we'll kill the servers, if needed.
    double timeout = (clientPipe)? 0.1 : this->ServerExitTimeOut;
    output = "";
    this->WaitForAndPrintLine("server", server, output, timeout,
                                           ServerStdOut, ServerStdErr, 0);
    if(!mpiError && this->OutputStringHasError("server", output))
      {
      mpiError = 1;
      }
    output = "";
    this->WaitForAndPrintLine("renderserver", renderServer, output, timeout,
                                RenderServerStdOut, RenderServerStdErr, 0);
    if(!mpiError && this->OutputStringHasError("renderserver", output))
      {
      mpiError = 1;
      }
    output = "";
    }

  // Wait for the client and server to exit.
  vtksysProcess_WaitForExit(client, 0);

  // Once client is finished, the servers
  // must finish quickly. If not, is usually is a sign that
  // the client crashed/exited before it attempted to connect to 
  // the server.
  if(server)
    {
    vtksysProcess_WaitForExit(server, &this->ServerExitTimeOut);
    }
  if(renderServer)
    {
    vtksysProcess_WaitForExit(renderServer, &this->ServerExitTimeOut);
    }
#ifdef PV_TEST_CLEAN_COMMAND
  // If any executable did not exit properly, run a user-specified
  // command to cleanup leftover processes.  This is needed for tests
  // that fail when running in MPI mode.
  //
  // For example: "killall -9 rsh paraview"
  //
  if(vtksysProcess_GetState(client) != vtksysProcess_State_Exited ||
     vtksysProcess_GetState(server) != vtksysProcess_State_Exited ||
     vtksysProcess_GetState(renderServer) != vtksysProcess_State_Exited)
    {
    if(strlen(PV_TEST_CLEAN_COMMAND) > 0)
      {
      system(PV_TEST_CLEAN_COMMAND);
      }
    }
#endif
  // Get the results.
  int clientResult = this->ReportStatus(client, "client");
  int serverResult = 0;
  if(server)
    {
    this->ReportStatus(server, "server");
    vtksysProcess_Kill(server);
    }
  int renderServerResult = 0;
  if(renderServer)
    {
    renderServerResult = this->ReportStatus(renderServer, "renderserver");
    vtksysProcess_Kill(renderServer);
    }

  // Free process managers.
  VTK_CLEAN_PROCESSES;

  // Report the server return code if it is nonzero.  Otherwise report
  // the client return code.
  if(serverResult)
    {
    return serverResult;
    }
  // if renderServer return code is nonzero then return it
  if(renderServerResult)
    {
    return renderServerResult;
    }
  if(mpiError)
    {
    cerr << "vtkSMTestDriver: Error string found in ouput, vtkSMTestDriver returning "
         << mpiError << "\n";
    return mpiError;
    }
  // if both servers are fine return the client result
  return clientResult;
}

//----------------------------------------------------------------------------
void vtkSMTestDriver::ReportCommand(const char* const* command, const char* name)
{
  cerr << "vtkSMTestDriver: " << name << " command is:\n";
  for(const char* const * c = command; *c; ++c)
    {
    cerr << " \"" << *c << "\"";
    }
  cerr << "\n";
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::ReportStatus(vtksysProcess* process, const char* name)
{
  int result = 1;
  switch(vtksysProcess_GetState(process))
    {
    case vtksysProcess_State_Starting:
      {
      cerr << "vtkSMTestDriver: Never started " << name << " process.\n";
      } break;
    case vtksysProcess_State_Error:
      {
      cerr << "vtkSMTestDriver: Error executing " << name << " process: "
           << vtksysProcess_GetErrorString(process)
           << "\n";
      } break;
    case vtksysProcess_State_Exception:
      {
      cerr << "vtkSMTestDriver: " << name
                      << " process exited with an exception: ";
      switch(vtksysProcess_GetExitException(process))
        {
        case vtksysProcess_Exception_None:
          {
          cerr << "None";
          } break;
        case vtksysProcess_Exception_Fault:
          {
          cerr << "Segmentation fault";
          } break;
        case vtksysProcess_Exception_Illegal:
          {
          cerr << "Illegal instruction";
          } break;
        case vtksysProcess_Exception_Interrupt:
          {
          cerr << "Interrupted by user";
          } break;
        case vtksysProcess_Exception_Numerical:
          {
          cerr << "Numerical exception";
          } break;
        case vtksysProcess_Exception_Other:
          {
          cerr << "Unknown";
          } break;
        }
      cerr << "\n";
      } break;
    case vtksysProcess_State_Executing:
      {
      cerr << "vtkSMTestDriver: Never terminated " << name << " process.\n";
      } break;
    case vtksysProcess_State_Exited:
      {
      result = vtksysProcess_GetExitValue(process);
      cerr << "vtkSMTestDriver: " << name << " process exited with code "
                      << result << "\n";
      } break;
    case vtksysProcess_State_Expired:
      {
      cerr << "vtkSMTestDriver: killed " << name << " process due to timeout.\n";
      } break;
    case vtksysProcess_State_Killed:
      {
      cerr << "vtkSMTestDriver: killed " << name << " process.\n";
      } break;
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::WaitForLine(vtksysProcess* process, vtkstd::string& line,
                              double timeout,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err)
{
  line = "";
  vtkstd::vector<char>::iterator outiter = out.begin();
  vtkstd::vector<char>::iterator erriter = err.begin();
  while(1)
    {
    // Check for a newline in stdout.
    for(;outiter != out.end(); ++outiter)
      {
      if((*outiter == '\r') && ((outiter+1) == out.end()))
        {
        break;
        }
      else if(*outiter == '\n' || *outiter == '\0')
        {
        int length = outiter-out.begin();
        if(length > 1 && *(outiter-1) == '\r')
          {
          --length;
          }
        if(length > 0)
          {
          line.append(&out[0], length);
          }
        out.erase(out.begin(), outiter+1);
        return vtksysProcess_Pipe_STDOUT;
        }
      }

    // Check for a newline in stderr.
    for(;erriter != err.end(); ++erriter)
      {
      if((*erriter == '\r') && ((erriter+1) == err.end()))
        {
        break;
        }
      else if(*erriter == '\n' || *erriter == '\0')
        {
        int length = erriter-err.begin();
        if(length > 1 && *(erriter-1) == '\r')
          {
          --length;
          }
        if(length > 0)
          {
          line.append(&err[0], length);
          }
        err.erase(err.begin(), erriter+1);
        return vtksysProcess_Pipe_STDERR;
        }
      }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = vtksysProcess_WaitForData(process, &data, &length, &timeout);
    if(pipe == vtksysProcess_Pipe_Timeout)
      {
      // Timeout has been exceeded.
      return pipe;
      }
    else if(pipe == vtksysProcess_Pipe_STDOUT)
      {
      // Append to the stdout buffer.
      vtkstd::vector<char>::size_type size = out.size();
      out.insert(out.end(), data, data+length);
      outiter = out.begin()+size;
      }
    else if(pipe == vtksysProcess_Pipe_STDERR)
      {
      // Append to the stderr buffer.
      vtkstd::vector<char>::size_type size = err.size();
      err.insert(err.end(), data, data+length);
      erriter = err.begin()+size;
      }
    else if(pipe == vtksysProcess_Pipe_None)
      {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      if(!out.empty())
        {
        line.append(&out[0], outiter-out.begin());
        out.erase(out.begin(), out.end());
        return vtksysProcess_Pipe_STDOUT;
        }
      else if(!err.empty())
        {
        line.append(&err[0], erriter-err.begin());
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
  if(this->CurrentPrintLineName != pname)
    {
    cerr << "-------------- " << pname
         << " output --------------\n";
    // save the current pname
    this->CurrentPrintLineName = pname;
    }
  cerr << line << "\n";
  cerr.flush();
}

//----------------------------------------------------------------------------
int vtkSMTestDriver::WaitForAndPrintLine(const char* pname, vtksysProcess* process,
                                      vtkstd::string& line, double timeout,
                                      vtkstd::vector<char>& out,
                                      vtkstd::vector<char>& err,
                                      int* foundWaiting)
{
  int pipe = this->WaitForLine(process, line, timeout, out, err);
  if(pipe == vtksysProcess_Pipe_STDOUT || pipe == vtksysProcess_Pipe_STDERR)
    {
    this->PrintLine(pname, line.c_str());
    if(foundWaiting && (line.find("Waiting") != line.npos))
      {
      *foundWaiting = 1;
      }
    }
  return pipe;
}

//----------------------------------------------------------------------------
vtkstd::string vtkSMTestDriver::GetDirectory(vtkstd::string location)
{
  return vtksys::SystemTools::GetParentDirectory(location.c_str());
}
