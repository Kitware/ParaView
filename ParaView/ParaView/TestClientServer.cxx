/*=========================================================================

  Program:   ParaView
  Module:    TestClientServer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include <kwsys/std/string>
#include <kwsys/std/vector>

void ReportCommand(const char* const* command, const char* name);
int ReportStatus(kwsysProcess* process, const char* name);

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // Allocate process managers.
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
  kwsys_std::string paraview = PARAVIEW_BINARY_DIR;
  paraview += "/bin";
#ifdef  CMAKE_INTDIR
  paraview += "/" CMAKE_INTDIR;
#endif
  paraview += "/paraview";

  // Construct the server process command line.
  kwsys_std::vector<const char*> serverCommand;
  serverCommand.push_back(paraview.c_str());
  serverCommand.push_back("--server");
  serverCommand.push_back(0);
  ReportCommand(&serverCommand[0], "server");
  kwsysProcess_SetCommand(server, &serverCommand[0]);

  // Construct the client process command line.
  kwsys_std::vector<const char*> clientCommand;
  clientCommand.push_back(paraview.c_str());
  clientCommand.push_back("--client");
  for(int i=1; i < argc; ++i)
    {
    clientCommand.push_back(argv[i]);
    }
  clientCommand.push_back(0);
  ReportCommand(&clientCommand[0], "client");
  kwsysProcess_SetCommand(client, &clientCommand[0]);

  // Kill the processes if they are taking too long.
  kwsysProcess_SetTimeout(server, 1400);
  kwsysProcess_SetTimeout(client, 1400);

  // Execute the server and then the client.
  kwsysProcess_Execute(server);
#ifdef _WIN32
  Sleep(1000);
#else
  sleep(1);
#endif
  kwsysProcess_Execute(client);

  // Report the output of the processes.
  int clientPipe = 1;
  int serverPipe = 1;
  while(clientPipe || serverPipe)
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
    serverPipe = kwsysProcess_WaitForData(client, &data, &length, &timeout);
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
    }

  // Wait for the client and server to exit.
  kwsysProcess_WaitForExit(client, 0);
  kwsysProcess_WaitForExit(server, 0);

  // Get the results.
  int serverResult = ReportStatus(server, "server");
  int clientResult = ReportStatus(client, "client");

  // Free process managers.
  kwsysProcess_Delete(client);
  kwsysProcess_Delete(server);

  // Report the server return code if it is nonzero.  Otherwise report
  // the client return code.
  if(serverResult)
    {
    return serverResult;
    }
  else
    {
    return clientResult;
    }
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
