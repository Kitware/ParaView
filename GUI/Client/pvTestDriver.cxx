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
}

// now implement the pvTestDriver class

int pvTestDriver::WaitForAndPrintData(const char* pname, 
                                      kwsysProcess* process,
                                      double timeout,
                                      vtkstd::string* output)
{
  if(!process)
    {
    return 0;
    }
  char* data;
  int length;
  // Look for process output.
  int processPipe = kwsysProcess_WaitForData(process, &data, 
                                             &length, &timeout);
  if(processPipe == kwsysProcess_Pipe_STDOUT ||
     processPipe == kwsysProcess_Pipe_STDERR)
    {
    vtkstd::string str(data, data+length);
    if(output)
      {
      *output += str;
      }
    this->PrintData(pname, data, length);
    }
  return processPipe;
}

void pvTestDriver::PrintData(const char* pname, const char* data, int length)
{
  cerr << "\n-------------- " << pname
       << " output chunk begin --------------\n";
  cerr.write(data, length);
  cerr << "\n-------------- " << pname
       << " output chunk end   --------------\n\n";
  cerr.flush();
}

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

  // now find all the mpi information if mpi run is set
#ifdef VTK_USE_MPI
#ifdef VTK_MPIRUN_EXE
  this->MPIRun = VTK_MPIRUN_EXE;
#else
  cerr << "Error: VTK_MPIRUN_EXE must be set of VTK_USE_MPI is on.\n";
  return -1;
#endif
  int serverNumProc = 1;
  int renderNumProc = 1;

# ifdef VTK_MPI_MAX_NUMPROCS
  serverNumProc = VTK_MPI_MAX_NUMPROCS;
  renderNumProc = serverNumProc-1;
# endif
# ifdef VTK_MPI_NUMPROC_FLAG
  this->MPINumProcessFlag = VTK_MPI_NUMPROC_FLAG;
# else
  cerr << "Error VTK_MPI_NUMPROC_FLAG must be defined to run test if MPI is on.\n";
  return -1;
# endif
# ifdef VTK_MPI_PREFLAGS
  this->SeparateArguments(VTK_MPI_PREFLAGS, this->MPIFlags);
# endif
# ifdef VTK_MPI_POSTFLAGS
  SeparateArguments(VTK_MPI_POSTFLAGS, this->MPIPostFlags);
# endif  
  char buf[1024];
  sprintf(buf, "%d", serverNumProc);
  this->MPIServerNumProcessFlag = buf;
  sprintf(buf, "%d", renderNumProc);
  this->MPIRenderServerNumProcessFlag = buf;
  this->MPIClientNumProcessFlag = "1";
  
#endif // VTK_USE_MPI

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
      fprintf(stderr, "Test With one mpi process.\n");
      }
    }
  // check for the Other.pvs test
  // This test should allow error to be in the output of the test.
  for(int i =1; i < argc; ++i)
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
     for(unsigned int i = 0; i < this->MPIFlags.size(); ++i)
       {
       commandLine.push_back(this->MPIFlags[i].c_str());
       }
    }
  commandLine.push_back(this->ParaView.c_str());
  if(strlen(paraviewFlags) > 0)
    {
    commandLine.push_back(paraviewFlags);
    }
  
  for(int ii = argStart; ii < argCount; ++ii)
    {
    commandLine.push_back(argv[ii]);
    }
  
  for(unsigned int i = 0; i < this->MPIPostFlags.size(); ++i)
    {
    commandLine.push_back(MPIPostFlags[i].c_str());
    }
  commandLine.push_back(0);
}

int pvTestDriver::StartServer(kwsysProcess* server, const char* name)
{
  if(!server)
    {
    return 1;
    }
  cerr << "pvTestDirver: starting process " << name << "\n";
  kwsysProcess_SetTimeout(server, this->TimeOut);
  kwsysProcess_Execute(server);
  const char* waiting = "Waiting";
  const char* waiting_cur = waiting;
  const char* waiting_end = waiting_cur + strlen(waiting);
  while(waiting_cur != waiting_end)
    {
    // Search for the "Waiting" string in the server output.
    double timeout = 30.0;
    char* data;
    int length;
    int pipe = kwsysProcess_WaitForData(server, &data, &length, &timeout);
    if(pipe == kwsysProcess_Pipe_STDOUT)
      {
      // Search for the "Waiting" string starting from where the
      // previous search left off.  It is possible that the string
      // will be received in pieces.
      const char* data_end = data+length;
      for(const char* data_cur = data;
          data_cur != data_end && waiting_cur != waiting_end; ++data_cur)
        {
        if(*waiting_cur != *data_cur)
          {
          waiting_cur = waiting;
          }
        if(*waiting_cur == *data_cur)
          {
          ++waiting_cur;
          }
        }
      }
    if(pipe == kwsysProcess_Pipe_STDOUT || pipe == kwsysProcess_Pipe_STDERR)
      {
      this->PrintData(name, data, length);
      }
    else
      {
      cerr << "pvTestDirver: " << name << " never started.\n";
      kwsysProcess_Kill(server);
      return 0;
      }
    }
  cerr << "pvTestDirver: " << name << " sucessfully started.\n";
  return 1;
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
    0
  };
  
  if(this->AllowErrorInOutput)
    {
    return 0;
    }

  for(int i =0; possibleMPIErrors[i]; ++i)
    {
    if(output.find(possibleMPIErrors[i]) != output.npos)
      {
      cerr << "pvTestDriver: ***** Test will fail, because the string: \"" 
           << possibleMPIErrors[i] 
           << "\"\npvTestDriver: ***** was found in the following output from the " 
           << pname << ":\n\"" 
           << output.c_str() << "\"\n";
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int pvTestDriver::Main(int argc, char* argv[])
{
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

  // Construct the render server process command line
  kwsys_stl::vector<const char*> renderServerCommand;
  if(renderServer)
    {
    this->CreateCommandLine(renderServerCommand,
                            "--render-server",
                            this->MPIRenderServerNumProcessFlag.c_str());
    ReportCommand(&renderServerCommand[0], "renderserver");
    kwsysProcess_SetCommand(renderServer, &renderServerCommand[0]);
    }
  
  kwsys_stl::vector<const char*> serverCommand;
  if(server)
    {
    this->CreateCommandLine(serverCommand,
                            "--server",
                            this->MPIServerNumProcessFlag.c_str());
    ReportCommand(&serverCommand[0], "server");
    kwsysProcess_SetCommand(server, &serverCommand[0]);
    }

  // Construct the client process command line.
  kwsys_stl::vector<const char*> clientCommand;
  vtkstd::string clientFlag;
  if(renderServer)
    {
    clientFlag = "--client-render-server";
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
                          clientFlag.c_str(),
                          this->MPIClientNumProcessFlag.c_str(),
                          this->ArgStart, argc, argv);
  ReportCommand(&clientCommand[0], "client");
  kwsysProcess_SetCommand(client, &clientCommand[0]);

  // Kill the processes if they are taking too long.
  kwsysProcess_SetTimeout(client, this->TimeOut);
  if(server)
    {
    kwsysProcess_SetTimeout(server, this->TimeOut);
    }
  // Start the render server if there is one
  if(!this->StartServer(renderServer, "Render Server"))
    {
    cerr << "pvTestDriver: Render server never started.\n";
    return -1;
    }
  // Start the data server if there is one
  if(!this->StartServer(server, "Server"))
    {
    cerr << "pvTestDriver: Server never started.\n";
    return -1;
    }
  // Now run the client
  kwsysProcess_Execute(client);

  // Report the output of the processes.
  int clientPipe = 1;
  int serverPipe = 1;
  int renderServerPipe = 1;
  vtkstd::string output;
  int mpiError = 0;
  while(clientPipe || serverPipe || renderServerPipe)
    {
    clientPipe = WaitForAndPrintData("client", client, 0.1, &output);
    if(!mpiError && this->OutputStringHasError("client", output))
      {
      mpiError = 1;
      }
    output = "";
    serverPipe = WaitForAndPrintData("server", server, 0.1, &output);
    if(!mpiError && this->OutputStringHasError("server", output))
      {
      mpiError = 1;
      }
    output = "";
    renderServerPipe = WaitForAndPrintData("renderServer", renderServer, 0.1, &output);
    if(!mpiError && this->OutputStringHasError("renderServer", output))
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
   
  // Get the results.
  int clientResult = this->ReportStatus(client, "client");
  int serverResult = 0;
  if(server)
    {
    ReportStatus(server, "server");
    }
  int renderServerResult = 0;
  if(renderServer)
    {
    renderServerResult = ReportStatus(renderServer, "renderserver");
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
