/*=========================================================================

  Program:   ParaView
  Module:    TestClientServer.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "TestClientServer.h"

#include "vtkSystemIncludes.h"

#if defined(_WIN32) && !defined(__CYGWIN__)
# include <windows.h>
#else
# include <unistd.h>
# include <sys/wait.h>
#endif

#include <kwsys/Process.h>
#include <kwsys/stl/string>
#include <kwsys/stl/vector>
#include <vtkstd/string>

void ReportCommand(const char* const* command, const char* name);
int ReportStatus(kwsysProcess* process, const char* name);


int WaitForAndPrintData(kwsysProcess* process, double timeout, int* foundWaiting )
{
  if(!process)
    {
    return 0;
    }
  char* data;
  int length;
  // Look for process output.
  int processPipe = kwsysProcess_WaitForData(process, &data, &length, &timeout);
  if(processPipe == kwsysProcess_Pipe_STDOUT)
    {
    if(foundWaiting)
      {
      vtkstd::string str(data, data+length);
      if(str.find("Waiting") != str.npos)
        {
        *foundWaiting = 1;
        }
      }
    cerr.write(data, length);
    cerr.flush();
    }
  else if(processPipe == kwsysProcess_Pipe_STDERR)
    {
    cerr.write(data, length);
    cerr.flush();
    }
  
  return processPipe;
}

inline void SeparateArguments(const char* str, vtkstd::vector<vtkstd::string>& flags)
{
  vtkstd::string arg = str;
  vtkstd::string::size_type pos1 = 0;
  vtkstd::string::size_type pos2 = arg.find(" ");
  if(pos2 == arg.npos)
    {
    flags.push_back(str);
    return;
    }
  while(pos2 != arg.npos)
    {
    flags.push_back(arg.substr(pos1, pos2-pos1));
    pos1 = pos2+1;
    pos2 = arg.find(" ", pos1+1);
    } 
  flags.push_back(arg.substr(pos1, pos2-pos1));
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int argStart = 1;
  int testRenderServer = 0;
  // try to make sure that this timesout before dart so it can kill all the processes
  double timeOut = DART_TESTING_TIMEOUT - 10.0;
  if(timeOut < 0)
    {
    timeOut = 1500;
    }
  
  if(argc > 1)
    {
    int index = 1;
    if(strcmp(argv[index], "--test-render-server") == 0)
      {
      argStart = index+1;
      testRenderServer = 1;
      fprintf(stderr, "Test Render Server\n");
      }
    }
  // mpi code
  vtkstd::string mpiRun;
  
  vtkstd::vector<vtkstd::string> mpiFlags;
  vtkstd::vector<vtkstd::string> mpiPostFlags;
  vtkstd::string mpiNumProcFlag;
  int serverNumProc = 1;
  int renderNumProc = 1;
#ifdef VTK_MPIRUN_EXE
  mpiRun = VTK_MPIRUN_EXE;

#ifdef VTK_MPI_MAX_NUMPROCS
  serverNumProc = VTK_MPI_MAX_NUMPROCS;
  renderNumProc = serverNumProc-1;
#endif
# ifdef VTK_MPI_NUMPROC_FLAG
  mpiNumProcFlag = VTK_MPI_NUMPROC_FLAG;
# else
  cerr << "Error VTK_MPI_NUMPROC_FLAG must be defined to run test\n";
  return -1;
# endif
# ifdef VTK_MPI_PREFLAGS
  SeparateArguments(VTK_MPI_PREFLAGS, mpiFlags);
# endif
# ifdef VTK_MPI_POSTFLAGS
  SeparateArguments(VTK_MPI_POSTFLAGS, mpiPostFlags);
# endif  
  char buf[1024];
  sprintf(buf, "%d", serverNumProc);
  vtkstd::string serverNumProcess = buf;
   sprintf(buf, "%d", renderNumProc);
  vtkstd::string renderServerNumProcess = buf;
#endif
  // Allocate process managers.
  kwsysProcess* renderServer = 0;
  if(testRenderServer)
    {
    renderServer = kwsysProcess_New();
    if(!renderServer)
      {
      cerr << "Cannot allocate kwsysProcess to run the render server.\n";
      return 1;
      }
    }
  kwsysProcess* server = kwsysProcess_New();
  if(!server)
    {
    cerr << "Cannot allocate kwsysProcess to run the server.\n";
    return 1;
    }
  kwsysProcess* client = kwsysProcess_New();
  if(!client)
    {
    kwsysProcess_Delete(server);
    cerr << "Cannot allocate kwsysProcess to run the client.\n";
    return 1;
    }

  // Location of the paraview executable.
  kwsys_stl::string paraview = PARAVIEW_BINARY_DIR;
#ifdef  CMAKE_INTDIR
  paraview += "/" CMAKE_INTDIR;
#endif
  paraview += "/paraview";
  unsigned int i;
  // Construct the render server process command line
  if(renderServer)
    {
    // Construct the server process command line.
    kwsys_stl::vector<const char*> renderServerCommand;
    if(mpiRun.size())
      {
      renderServerCommand.push_back(mpiRun.c_str());
      renderServerCommand.push_back(mpiNumProcFlag.c_str());
      renderServerCommand.push_back(renderServerNumProcess.c_str());
      for(i = 0; i < mpiFlags.size(); ++i)
        {
        cout << mpiFlags[i].c_str() << "\n";
        renderServerCommand.push_back(mpiFlags[i].c_str());
        }
      }
    renderServerCommand.push_back(paraview.c_str());
    renderServerCommand.push_back("--render-server");
    for(i = 0; i < mpiPostFlags.size(); ++i)
      {
      renderServerCommand.push_back(mpiPostFlags[i].c_str());
      }
    renderServerCommand.push_back(0);
    ReportCommand(&renderServerCommand[0], "renderserver");
    kwsysProcess_SetCommand(renderServer, &renderServerCommand[0]);
    }
  
  // Construct the server process command line.
  kwsys_stl::vector<const char*> serverCommand;
  if(mpiRun.size())
    {
    serverCommand.push_back(mpiRun.c_str());
    serverCommand.push_back(mpiNumProcFlag.c_str());
    serverCommand.push_back(serverNumProcess.c_str());
    for(i = 0; i < mpiFlags.size(); ++i)
      {
      serverCommand.push_back(mpiFlags[i].c_str());
      }
    }
  serverCommand.push_back(paraview.c_str()); 
  serverCommand.push_back("--server");
  for(i = 0; i < mpiPostFlags.size(); ++i)
    {
    serverCommand.push_back(mpiPostFlags[i].c_str());
    }
    
  serverCommand.push_back(0);
  ReportCommand(&serverCommand[0], "server");
  kwsysProcess_SetCommand(server, &serverCommand[0]);

  // Construct the client process command line.
  kwsys_stl::vector<const char*> clientCommand;
  clientCommand.push_back(paraview.c_str());
  if(renderServer)
    {
    clientCommand.push_back("--client-render-server"); 
    }
  else
    {
    clientCommand.push_back("--client");
    }
  
  for(int ii=argStart; ii < argc; ++ii)
    {
    clientCommand.push_back(argv[ii]);
    }
  clientCommand.push_back(0);
  ReportCommand(&clientCommand[0], "client");
  kwsysProcess_SetCommand(client, &clientCommand[0]);

  // Kill the processes if they are taking too long.
  kwsysProcess_SetTimeout(server, timeOut);
  kwsysProcess_SetTimeout(client, timeOut);
  int foundWaiting = 0;
  if(renderServer)
    {
    cerr << "start render server\n";
    kwsysProcess_SetTimeout(renderServer, timeOut);
    kwsysProcess_Execute(renderServer);
    while(!foundWaiting)
      {
      if(!WaitForAndPrintData(renderServer, 30.0, &foundWaiting))
        {
        cerr << "render server never started\n";
        return -1;
        }
      }
    cerr << "Started Render Server\n";
    }
  
  // Execute the server and then the client.
  kwsysProcess_Execute(server);
  foundWaiting = 0;
  while(!foundWaiting)
    {
    if(!WaitForAndPrintData(server, 30.0, &foundWaiting))
      {
      cerr << "server never started\n";
      kwsysProcess_Kill(renderServer);
      return -1;
      }
    }
  cerr << "Started data Server\n";
  kwsysProcess_Execute(client);

  // Report the output of the processes.
  int clientPipe = 1;
  int serverPipe = 1;
  int renderServerPipe = 1;
  while(clientPipe || serverPipe || renderServerPipe)
    {
    clientPipe = WaitForAndPrintData(client, 0.1, 0);
    serverPipe = WaitForAndPrintData(server, 0.1, 0);
    renderServerPipe = WaitForAndPrintData(renderServer, 0.1, 0);
    }

  // Wait for the client and server to exit.
  kwsysProcess_WaitForExit(client, 0);
  kwsysProcess_WaitForExit(server, 0);
  if(renderServer)
    {
    kwsysProcess_WaitForExit(renderServer, 0); 
    }
   
  // Get the results.
  int serverResult = ReportStatus(server, "server");
  int clientResult = ReportStatus(client, "client");
  int renderServerResult = 0;
  if(renderServer)
    {
    renderServerResult = ReportStatus(renderServer, "renderserver");
    }

  // Free process managers.
  kwsysProcess_Delete(client);
  kwsysProcess_Delete(server);
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
  // if both servers are fine return the client result
  return clientResult;
}

//----------------------------------------------------------------------------
void ReportCommand(const char* const* command, const char* name)
{
  cerr << "The " << name << " command is:\n";
  for(const char* const * c = command; *c; ++c)
    {
    cerr << " \"" << *c << "\"";
    }
  cerr << "\n";
}

//----------------------------------------------------------------------------
int ReportStatus(kwsysProcess* process, const char* name)
{
  int result = 1;
  switch(kwsysProcess_GetState(process))
    {
    case kwsysProcess_State_Starting:
      {
      cerr << "Never started " << name << " process.\n";
      } break;
    case kwsysProcess_State_Error:
      {
      cerr << "Error executing " << name << " process: "
           << kwsysProcess_GetErrorString(process)
           << "\n";
      } break;
    case kwsysProcess_State_Exception:
      {
      cerr << "The " << name
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
      cerr << "Never terminated " << name << " process.\n";
      } break;
    case kwsysProcess_State_Exited:
      {
      result = kwsysProcess_GetExitValue(process);
      cerr << "The " << name << " process exited with code "
                      << result << "\n";
      } break;
    case kwsysProcess_State_Expired:
      {
      cerr << "Killed " << name << " process due to timeout.\n";
      } break;
    case kwsysProcess_State_Killed:
      {
      cerr << "Killed " << name << " process.\n";
      } break;
    }
  return result;
}
