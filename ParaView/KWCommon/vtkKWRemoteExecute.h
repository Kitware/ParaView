/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWRemoteExecute.h
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
// .NAME vtkKWRemoteExecute - execute process on remote computer
// .SECTION Description
// This class abstracts execution of programs on remote processes.

#ifndef __vtkKWRemoteExecute_h
#define __vtkKWRemoteExecute_h

#include "vtkObject.h"

class vtkMultiThreader;

class vtkKWRemoteExecuteInternal;

class VTK_EXPORT vtkKWRemoteExecute : public vtkObject
{
public:
  static vtkKWRemoteExecute* New();
  vtkTypeRevisionMacro(vtkKWRemoteExecute,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Run command remotely.
  int RunRemoteCommand(const char* args);

  // Description:
  // Run command.
  int RunCommand(const char* args);

  static void* RunCommandThread(void*);

  enum {
    NOT_RUN,
    RUNNING,
    SUCCESS,
    FAIL
  };
//ETX
  
  // Description:
  // Wait for remote command to finish.
  int WaitToFinish();

  // Description:
  // Detach process from the current console. Useful for when invoking new
  // process which should stay up even when the parent process exits. Returns
  // VTK_OK on success.
  static int Detach();
  
  // Description:
  // Set and get the remote host to run command.
  vtkSetStringMacro(RemoteHost);
  vtkGetStringMacro(RemoteHost);

  // Description:
  // Get the result. It can be NOT_RUN, RUNNING, SUCCESS, FAIL
  vtkGetMacro(Result, int);

  // Description:
  // Set SSH user
  vtkSetStringMacro(SSHUser);
  vtkGetStringMacro(SSHUser);

  // Description:
  // Set SSH command
  vtkSetStringMacro(SSHCommand);
  vtkGetStringMacro(SSHCommand);

  // Description:
  // Set SSH arguments
  vtkSetStringMacro(SSHArguments);
  vtkGetStringMacro(SSHArguments);

protected:
  vtkKWRemoteExecute();
  ~vtkKWRemoteExecute();

  vtkKWRemoteExecuteInternal* Internals;
  vtkMultiThreader* MultiThreader;

  char* SSHCommand;
  char* SSHArguments;
  char* SSHUser;
  char* RemoteHost;
  int ProcessRunning;
  int Result;

  int ProcessThreadId;

private:
  vtkKWRemoteExecute(const vtkKWRemoteExecute&); // Not implemented
  void operator=(const vtkKWRemoteExecute&); // Not implemented
};


#endif


