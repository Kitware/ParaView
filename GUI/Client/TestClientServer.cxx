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

void ReportCommand(const char* const* command, const char* name);
int ReportStatus(kwsysProcess* process, const char* name);


inline void PauseForServerStart()
{
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
}

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  int argStart = 1;
  int testRenderServer = 0;
  if(argc > 1)
    {
    if(strcmp(argv[1], "--test-render-server") == 0)
      {
      argStart = 2;
      testRenderServer = 1;
      fprintf(stderr, "Test Render Server\n");
      }
    }
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

  // Construct the render server process command line
  if(renderServer)
    {
    // Construct the server process command line.
    kwsys_stl::vector<const char*> renderServerCommand;
    renderServerCommand.push_back(paraview.c_str());
    renderServerCommand.push_back("--render-server");
    renderServerCommand.push_back(0);
    ReportCommand(&renderServerCommand[0], "renderserver");
    kwsysProcess_SetCommand(renderServer, &renderServerCommand[0]);
    }
  
  // Construct the server process command line.
  kwsys_stl::vector<const char*> serverCommand;
  serverCommand.push_back(paraview.c_str());
  serverCommand.push_back("--server");
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
  
  for(int i=argStart; i < argc; ++i)
    {
    clientCommand.push_back(argv[i]);
    }
  clientCommand.push_back(0);
  ReportCommand(&clientCommand[0], "client");
  kwsysProcess_SetCommand(client, &clientCommand[0]);

  // Kill the processes if they are taking too long.
  kwsysProcess_SetTimeout(server, 1400);
  kwsysProcess_SetTimeout(client, 1400);
  if(renderServer)
    {
    fprintf(stderr, "start render server\n");
    kwsysProcess_SetTimeout(renderServer, 1400);
    kwsysProcess_Execute(renderServer);
    PauseForServerStart();
    }
  
  // Execute the server and then the client.
  kwsysProcess_Execute(server);
  PauseForServerStart();
  kwsysProcess_Execute(client);

  // Report the output of the processes.
  int clientPipe = 1;
  int serverPipe = 1;
  int renderServerPipe = 0;
  if(renderServer)
    {
    renderServerPipe = 1;
    }
  while(clientPipe || serverPipe || renderServerPipe)
    {
    char* data;
    int length;
    double timeout;

    // Look for client output.
    timeout = 0.1;
    clientPipe = kwsysProcess_WaitForData(client, &data, &length, &timeout);
    if(clientPipe == kwsysProcess_Pipe_STDOUT)
      {
      cout.write(data, length);
      cout.flush();
      }
    else if(clientPipe == kwsysProcess_Pipe_STDERR)
      {
      cerr.write(data, length);
      cout.flush();
      }

    // Look for server output.
    timeout = 0.1;
    serverPipe = kwsysProcess_WaitForData(server, &data, &length, &timeout);
    if(serverPipe == kwsysProcess_Pipe_STDOUT)
      {
      cout.write(data, length);
      cout.flush();
      }
    else if(serverPipe == kwsysProcess_Pipe_STDERR)
      {
      cerr.write(data, length);
      cout.flush();
      }
    if(renderServer)
      {
      renderServerPipe = kwsysProcess_WaitForData(server, &data, &length, &timeout);
      if(renderServerPipe == kwsysProcess_Pipe_STDOUT)
        {
        cout.write(data, length);
        cout.flush();
        }
      else if(renderServerPipe == kwsysProcess_Pipe_STDERR)
        {
        cerr.write(data, length);
        cout.flush();
        }
      }
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
  cout << "The " << name << " command is:\n";
  for(const char* const * c = command; *c; ++c)
    {
    cout << " \"" << *c << "\"";
    }
  cout << "\n";
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
      cout << "The " << name << " process exited with code "
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
