/*=========================================================================

  Module:    vtkKWRemoteExecute.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
  vtkTypeMacro(vtkKWRemoteExecute,vtkObject);
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





