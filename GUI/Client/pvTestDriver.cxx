/*=========================================================================

  Program:   ParaView
  Module:    pvTestDriver.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSystemIncludes.h"

#include "pvTestDriver.h"
#include "pvTestDriverConfig.h"

#include <kwsys/SystemTools.hxx>

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h>
# include <sys/wait.h>
#endif

// The main function as this class should only be used by this program
int main(int argc, char* argv[])
{
  pvTestDriver d;
  return d.Main(argc, argv);
}

pvTestDriver::pvTestDriver()
{
  this->AllowErrorInOutput = 0;
  this->RenderServerNumProcesses = 0;
  this->TimeOut = 300;
  this->TestRenderServer = 0;
  this->TestServer = 0;
  this->ReverseConnection = 0;
}

pvTestDriver::~pvTestDriver()
{
}

// now implement the pvTestDriver class

void pvTestDriver::SeparateArguments(const char* str, 
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


void pvTestDriver::CollectConfiguredOptions()
{
  // try to make sure that this timesout before dart so it can kill all the processes
  this->TimeOut = DART_TESTING_TIMEOUT - 10.0;
  if(this->TimeOut < 0)
    {
    this->TimeOut = 1500;
    }
  // Find the location of paraview
  this->ParaView = PARAVIEW_BINARY_DIR;
#ifdef  CMAKE_INTDIR
  this->ParaView  += "/" CMAKE_INTDIR;
#endif
  this->ParaView += "/paraview";
  // Find the location of the paraview render server
  this->ParaViewRenderServer = this->ParaView;
  // Find the location of the paraview server
  this->ParaViewServer = PARAVIEW_BINARY_DIR;
#ifdef  CMAKE_INTDIR
    this->ParaViewServer  += "/" CMAKE_INTDIR;
#endif
    // To test pvserver, change the following to pvserver
    this->ParaViewServer += "/pvserver";
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
  this->MPIClientNumProcessFlag = "1";
  
#endif // VTK_USE_MPI

# ifdef VTK_MPI_CLIENT_PREFLAGS
  this->SeparateArguments(VTK_MPI_CLIENT_PREFLAGS, this->MPIClientPreFlags);
# endif
# ifdef VTK_MPI_CLIENT_POSTFLAGS
  this->SeparateArguments(VTK_MPI_CLIENT_POSTFLAGS, this->MPIClientPostFlags);
# endif  
# ifdef VTK_MPI_SERVER_PREFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_PREFLAGS, this->MPIServerPreFlags);
# endif
# ifdef VTK_MPI_SERVER_POSTFLAGS
  this->SeparateArguments(VTK_MPI_SERVER_POSTFLAGS, this->MPIServerPostFlags);
# endif  

// For remote testing (via ssh)
# ifdef PV_SSH_FLAGS
  this->SeparateArguments(PV_SSH_FLAGS, this->PVSSHFlags);
# endif //PV_SSH_FLAGS

# ifdef PV_SETUP_SCRIPT
  this->PVSetupScript = PV_SETUP_SCRIPT;
# endif //PV_SETUP_SCRIPT
}

int pvTestDriver::ProcessCommandLine(int argc, char* argv[])
{
  this->ArgStart = 1;
  if(argc > 1)
    {
    int index = 1;
    if(strcmp(argv[index], "--test-render-server") == 0)
      {
      this->ArgStart = index+1;
      this->TestRenderServer = 1;
      this->TestServer = 1;
      fprintf(stderr, "Test Render Server.\n");
      }
    if(strcmp(argv[index], "--test-r2d") == 0)
      {
      this->ArgStart = index+1;
      this->TestRenderServer = 2;
      this->TestServer = 1;
      fprintf(stderr, "Test Render Server.\n");
      }
    if(strcmp(argv[index], "--test-server") == 0)
      {
      this->ArgStart = index+1;
      this->TestServer = 1;
      fprintf(stderr, "Test Server.\n");
      }
    if(strcmp(argv[index], "--one-mpi-np") == 0)
      {
      this->MPIClientNumProcessFlag = this->MPIServerNumProcessFlag = "1";
      this->ArgStart = index+1;
      fprintf(stderr, "Test with one mpi process.\n");
      }
    }
  int i;
  for(i =1; i < argc - 1; ++i)
    {
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
    if(strncmp(argv[i], "--client-preflags",17) == 0)
      {
      this->SeparateArguments(argv[i+1], this->MPIClientPreFlags);
      this->ArgStart = i+2;
      fprintf(stderr, "Extras client preflags were specified: %s\n", argv[i+1]);
      }
    if(strncmp(argv[i], "--client-postflags",18) == 0)
      {
      this->SeparateArguments(argv[i+1], this->MPIClientPostFlags);
      this->ArgStart = i+2;
      fprintf(stderr, "Extras client postflags were specified: %s\n", argv[i+1]);
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
    }
  
  // check for the Other.pvs test
  // This test should allow error to be in the output of the test.
  for(i =1; i < argc; ++i)
    {
    int len = strlen(argv[i]) - 9;
    if(len > 0 && strncmp(argv[i]+len, "Other.pvs", 9) == 0)
      {
      this->AllowErrorInOutput = 1;
      }
    }
  
  return 1;
}

void 
pvTestDriver::CreateCommandLine(kwsys_stl::vector<const char*>& commandLine,
                                const char* paraView,
                                const char* paraviewFlags, 
                                const char* numProc,
                                int argStart,
                                int argCount,
                                char* argv[])
{
  if(this->MPIRun.size())
    {
    commandLine.push_back(this->MPIRun.c_str());
    commandLine.push_back(this->MPINumProcessFlag.c_str());
    commandLine.push_back(numProc);
    for(unsigned int i = 0; i < this->MPIPreFlags.size(); ++i)
      {
      commandLine.push_back(this->MPIPreFlags[i].c_str());
      }
    // If there is specific flags for the client to pass to mpirun, add them
    if( strcmp( paraviewFlags, "--client" ) == 0)
      {
      for(unsigned int i = 0; i < this->MPIClientPreFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIClientPreFlags[i].c_str());
        }
      }
    // If there is specific flags for the server to pass to mpirun, add them
    if( strcmp( paraviewFlags, "--server" ) == 0)
      {
      for(unsigned int i = 0; i < this->MPIServerPreFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIServerPreFlags[i].c_str());
        }
      }
    }

  if(this->PVSSHFlags.size() && (strcmp( paraviewFlags, "--server" ) == 0))
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
    if(strlen(paraviewFlags) > 0 && strcmp(paraviewFlags, "--server") != 0)
      {
      commandLine.push_back(paraviewFlags);
      }
    if(this->ReverseConnection)
      {
      commandLine.push_back("-rc");
      }
    
    for(unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
      {
      commandLine.push_back(MPIPostFlags[i].c_str());
      }
    // If there is specific flags for the server to pass to paraview, add them
    if( strcmp( paraviewFlags, "--client" ) == 0)
      {
      for(unsigned int i = 0; i < this->MPIClientPostFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIClientPostFlags[i].c_str());
        }
      }
    // If there is specific flags for the server to pass to mpirun, add them
    if( strcmp( paraviewFlags, "--server" ) == 0)
      {
      for(unsigned int i = 0; i < this->MPIServerPostFlags.size(); ++i)
        {
        commandLine.push_back(this->MPIServerPostFlags[i].c_str());
        }
      }
    // remaining flags for the test 
    for(int ii = argStart; ii < argCount; ++ii)
      {
      commandLine.push_back(argv[ii]);
      }
    }
  commandLine.push_back(0);
}

int pvTestDriver::StartServer(kwsysProcess* server, const char* name,
                              vtkstd::vector<char>& out,
                              vtkstd::vector<char>& err)
{
  if(!server)
    {
    return 1;
    }
  cerr << "pvTestDriver: starting process " << name << "\n";
  kwsysProcess_SetTimeout(server, this->TimeOut);
  kwsysProcess_Execute(server);
  int foundWaiting = 0;
  vtkstd::string output;
  while(!foundWaiting)
    {
    int pipe = this->WaitForAndPrintLine(name, server, output, 100.0, out, err,
                                         &foundWaiting);
    if(pipe == kwsysProcess_Pipe_None ||
       pipe == kwsysProcess_Pipe_Timeout)
      {
      break;
      }
    }
  if(foundWaiting)
    {
    cerr << "pvTestDriver: " << name << " sucessfully started.\n";
    return 1;
    }
  else
    {
    cerr << "pvTestDriver: " << name << " never started.\n";
    kwsysProcess_Kill(server);
    return 0;
    }
}

int pvTestDriver::OutputStringHasError(const char* pname, vtkstd::string& output)
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
    "Memcheck, a memory error detector for x86-linux",  //valgrind
    "error in locking authority file",  //Ice-T
    "WARNING: Far depth failed sanity check, resetting.", //Ice-T
    0
  };
  
  if(this->AllowErrorInOutput)
    {
    return 0;
    }

  vtkstd::vector<vtkstd::string> lines;
  vtkstd::vector<vtkstd::string>::iterator it;
  kwsys::SystemTools::Split(output.c_str(), lines);

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
          cerr << "pvTestDriver: ***** Test will fail, because the string: \"" 
            << possibleMPIErrors[i] 
            << "\"\npvTestDriver: ***** was found in the following output from the " 
            << pname << ":\n\"" 
            << it->c_str() << "\"\n";
          return 1;
          }
        }
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int pvTestDriver::Main(int argc, char* argv[])
{
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
  kwsysProcess* renderServer = 0;
  if(this->TestRenderServer)
    {
    renderServer = kwsysProcess_New();
    if(!renderServer)
      {
      cerr << "pvTestDriver: Cannot allocate kwsysProcess to run the render server.\n";
      return 1;
      }
    }
  kwsysProcess* server = 0;
  if(this->TestServer)
    {
    server = kwsysProcess_New();
    if(!server)
      {
      cerr << "pvTestDriver: Cannot allocate kwsysProcess to run the server.\n";
      return 1;
      }
    }
  kwsysProcess* client = kwsysProcess_New();
  if(!client)
    {
    kwsysProcess_Delete(server);
    cerr << "pvTestDriver: Cannot allocate kwsysProcess to run the client.\n";
    return 1;
    }

  vtkstd::vector<char> ClientStdOut;
  vtkstd::vector<char> ClientStdErr;
  vtkstd::vector<char> ServerStdOut;
  vtkstd::vector<char> ServerStdErr;
  vtkstd::vector<char> RenderServerStdOut;
  vtkstd::vector<char> RenderServerStdErr;

  // Construct the render server process command line
  kwsys_stl::vector<const char*> renderServerCommand;
  if(renderServer)
    {
    this->CreateCommandLine(renderServerCommand,
                            this->ParaView.c_str(),
                            "--render-server",
                            this->MPIRenderServerNumProcessFlag.c_str());
    this->ReportCommand(&renderServerCommand[0], "renderserver");
    kwsysProcess_SetCommand(renderServer, &renderServerCommand[0]);
    }
  
  kwsys_stl::vector<const char*> serverCommand;
  if(server)
    {
    this->CreateCommandLine(serverCommand,
                            this->ParaViewServer.c_str(),
                            "--server",
                            this->MPIServerNumProcessFlag.c_str());
    this->ReportCommand(&serverCommand[0], "server");
    kwsysProcess_SetCommand(server, &serverCommand[0]);
    }

  // Construct the client process command line.
  kwsys_stl::vector<const char*> clientCommand;
  vtkstd::string clientFlag;
  if(renderServer)
    {
    if(this->TestRenderServer == 2)
      {
      clientFlag = "-r2d";
      }
    else
      {
      clientFlag = "--client-render-server";
      }
    }
  else
    {
    if(server)
      {
      clientFlag = "--client";
      }
    else
      {
      // use the server number of processes flag if not in server mode
      this->MPIClientNumProcessFlag = this->MPIServerNumProcessFlag;
      }
    }
  this->CreateCommandLine(clientCommand,
                          this->ParaView.c_str(),
                          clientFlag.c_str(),
                          this->MPIClientNumProcessFlag.c_str(),
                          this->ArgStart, argc, argv);
  this->ReportCommand(&clientCommand[0], "client");
  kwsysProcess_SetCommand(client, &clientCommand[0]);

  // Kill the processes if they are taking too long.
  kwsysProcess_SetTimeout(client, this->TimeOut);
  if(server)
    {
    kwsysProcess_SetTimeout(server, this->TimeOut);
    }
  if(renderServer)
    {
    kwsysProcess_SetTimeout(renderServer, this->TimeOut);
    }
  if(this->ReverseConnection)
    {
    if(!this->StartServer(client, "client",
                          ClientStdOut, ClientStdErr))
      {
      cerr << "pvTestDriver: Reverse connection client never started.\n";
      return -1;
      }
    // Now run the server
    if(server)
      {
      kwsysProcess_Execute(server);
      }
    // Now run the render server
    if(renderServer)
      {
      kwsysProcess_Execute(renderServer);
      }
    }
  else
    {
    // Start the render server if there is one
    if(!this->StartServer(renderServer, "renderserver",
                          RenderServerStdOut, RenderServerStdErr))
      {
      cerr << "pvTestDriver: Render server never started.\n";
      return -1;
      }
    // Start the data server if there is one
    if(!this->StartServer(server, "server",
                          ServerStdOut, ServerStdErr))
      {
      cerr << "pvTestDriver: Server never started.\n";
      return -1;
      }
    // Now run the client
    kwsysProcess_Execute(client);
    }
  
  // Report the output of the processes.
  int clientPipe = 1;
  int serverPipe = 1;
  int renderServerPipe = 1;
  vtkstd::string output;
  int mpiError = 0;
  while(clientPipe || serverPipe || renderServerPipe)
    {
    clientPipe = this->WaitForAndPrintLine("client", client, output, 0.1,
                                           ClientStdOut, ClientStdErr, 0);
    if(!mpiError && this->OutputStringHasError("client", output))
      {
      mpiError = 1;
      }
    output = "";
    serverPipe = this->WaitForAndPrintLine("server", server, output, 0.1,
                                           ServerStdOut, ServerStdErr, 0);
    if(!mpiError && this->OutputStringHasError("server", output))
      {
      mpiError = 1;
      }
    output = "";
    renderServerPipe =
      this->WaitForAndPrintLine("renderserver", renderServer, output, 0.1,
                                RenderServerStdOut, RenderServerStdErr, 0);
    if(!mpiError && this->OutputStringHasError("renderserver", output))
      {
      mpiError = 1;
      }
    output = "";
    }

  // Wait for the client and server to exit.
  kwsysProcess_WaitForExit(client, 0);
  if(server)
    {
    kwsysProcess_WaitForExit(server, 0);
    }
  if(renderServer)
    {
    kwsysProcess_WaitForExit(renderServer, 0); 
    }
#ifdef PV_TEST_CLEAN_COMMAND
  // If any executable did not exit properly, run a user-specified
  // command to cleanup leftover processes.  This is needed for tests
  // that fail when running in MPI mode.
  //
  // For example: "killall -9 rsh paraview"
  //
  if(kwsysProcess_GetState(client) != kwsysProcess_State_Exited ||
     kwsysProcess_GetState(server) != kwsysProcess_State_Exited ||
     kwsysProcess_GetState(renderServer) != kwsysProcess_State_Exited)
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
    }
  int renderServerResult = 0;
  if(renderServer)
    {
    renderServerResult = this->ReportStatus(renderServer, "renderserver");
    }

  // Free process managers.
  kwsysProcess_Delete(client);
  if(server)
    {
    kwsysProcess_Delete(server);
    }
  if(renderServer)
    {
    kwsysProcess_Delete(renderServer);
    }
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
    cerr << "pvTestDriver: Error string found in ouput, pvTestDriver returning " 
         << mpiError << "\n";
    return mpiError;
    }
  // if both servers are fine return the client result
  return clientResult;
}

//----------------------------------------------------------------------------
void pvTestDriver::ReportCommand(const char* const* command, const char* name)
{
  cerr << "pvTestDriver: " << name << " command is:\n";
  for(const char* const * c = command; *c; ++c)
    {
    cerr << " \"" << *c << "\"";
    }
  cerr << "\n";
}

//----------------------------------------------------------------------------
int pvTestDriver::ReportStatus(kwsysProcess* process, const char* name)
{
  int result = 1;
  switch(kwsysProcess_GetState(process))
    {
    case kwsysProcess_State_Starting:
      {
      cerr << "pvTestDriver: Never started " << name << " process.\n";
      } break;
    case kwsysProcess_State_Error:
      {
      cerr << "pvTestDriver: Error executing " << name << " process: "
           << kwsysProcess_GetErrorString(process)
           << "\n";
      } break;
    case kwsysProcess_State_Exception:
      {
      cerr << "pvTestDriver: " << name
                      << " process exited with an exception: ";
      switch(kwsysProcess_GetExitException(process))
        {
        case kwsysProcess_Exception_None:
          {
          cerr << "None";
          } break;
        case kwsysProcess_Exception_Fault:
          {
          cerr << "Segmentation fault";
          } break;
        case kwsysProcess_Exception_Illegal:
          {
          cerr << "Illegal instruction";
          } break;
        case kwsysProcess_Exception_Interrupt:
          {
          cerr << "Interrupted by user";
          } break;
        case kwsysProcess_Exception_Numerical:
          {
          cerr << "Numerical exception";
          } break;
        case kwsysProcess_Exception_Other:
          {
          cerr << "Unknown";
          } break;
        }
      cerr << "\n";
      } break;
    case kwsysProcess_State_Executing:
      {
      cerr << "pvTestDriver: Never terminated " << name << " process.\n";
      } break;
    case kwsysProcess_State_Exited:
      {
      result = kwsysProcess_GetExitValue(process);
      cerr << "pvTestDriver: " << name << " process exited with code "
                      << result << "\n";
      } break;
    case kwsysProcess_State_Expired:
      {
      cerr << "pvTestDriver: killed " << name << " process due to timeout.\n";
      } break;
    case kwsysProcess_State_Killed:
      {
      cerr << "pvTestDriver: killed " << name << " process.\n";
      } break;
    }
  return result;
}

//----------------------------------------------------------------------------
int pvTestDriver::WaitForLine(kwsysProcess* process, vtkstd::string& line,
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
        return kwsysProcess_Pipe_STDOUT;
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
        return kwsysProcess_Pipe_STDERR;
        }
      }

    // No newlines found.  Wait for more data from the process.
    int length;
    char* data;
    int pipe = kwsysProcess_WaitForData(process, &data, &length, &timeout);
    if(pipe == kwsysProcess_Pipe_Timeout)
      {
      // Timeout has been exceeded.
      return pipe;
      }
    else if(pipe == kwsysProcess_Pipe_STDOUT)
      {
      // Append to the stdout buffer.
      vtkstd::vector<char>::size_type size = out.size();
      out.insert(out.end(), data, data+length);
      outiter = out.begin()+size;
      }
    else if(pipe == kwsysProcess_Pipe_STDERR)
      {
      // Append to the stderr buffer.
      vtkstd::vector<char>::size_type size = err.size();
      err.insert(err.end(), data, data+length);
      erriter = err.begin()+size;
      }
    else if(pipe == kwsysProcess_Pipe_None)
      {
      // Both stdout and stderr pipes have broken.  Return leftover data.
      if(!out.empty())
        {
        line.append(&out[0], outiter-out.begin());
        out.erase(out.begin(), out.end());
        return kwsysProcess_Pipe_STDOUT;
        }
      else if(!err.empty())
        {
        line.append(&err[0], erriter-err.begin());
        err.erase(err.begin(), err.end());
        return kwsysProcess_Pipe_STDERR;
        }
      else
        {
        return kwsysProcess_Pipe_None;
        }
      }
    }
}

//----------------------------------------------------------------------------
void pvTestDriver::PrintLine(const char* pname, const char* line)
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
int pvTestDriver::WaitForAndPrintLine(const char* pname, kwsysProcess* process,
                                      vtkstd::string& line, double timeout,
                                      vtkstd::vector<char>& out,
                                      vtkstd::vector<char>& err,
                                      int* foundWaiting)
{
  int pipe = this->WaitForLine(process, line, timeout, out, err);
  if(pipe == kwsysProcess_Pipe_STDOUT || pipe == kwsysProcess_Pipe_STDERR)
    {
    this->PrintLine(pname, line.c_str());
    if(foundWaiting && (line.find("Waiting") != line.npos))
      {
      *foundWaiting = 1;
      }
    }
  return pipe;
}
